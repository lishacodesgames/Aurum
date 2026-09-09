#include <pch/Precompiled.h>
#include "Generator.h"

namespace
{
   err::SourceLocation getLocation(const ast::Expression* expr) {
      return std::visit([](auto&& arg) -> err::SourceLocation {
         using PtrT = std::decay_t<decltype(arg)>;
         if constexpr(std::is_same_v<PtrT, ast::BinaryExpr*>)
            return arg->op.location;
         else if constexpr(std::is_same_v<PtrT, ast::Literal*> || std::is_same_v<PtrT, ast::Identifier*>)
            return arg->token.location;
         else if constexpr(std::is_same_v<PtrT, ast::Negative*>)
            return getLocation(arg->operand);
         else // monostate
            return {};
      }, *expr);
   }
}

std::vector<ir::Instruction> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements)
      generate<ast::Statement>(&stmt);

   emit(OpCode::EXIT, "0"); // in case user hasn't exited
   return m_instructions; // NOT to be moved bcz it needs to be accessed later
}

std::string Generator::getIR() const {
   std::string IR;

   IR += "; Intermediate Representation for Aurum\n\n";
   IR += "_main:\n"; /// @todo function definition opcodes

   for(const ir::Instruction& instr : m_instructions) {
      IR += "\t" + to_string(instr.opcode);

      if(instr.operandLeft) {
         IR += " " + *instr.operandLeft;
         if(instr.operandRight)
            IR += ", " + *instr.operandRight;
      }

      IR += "\n";
   }

   return IR;
}

void Generator::emit(OpCode op, std::optional<std::string_view> operand1, std::optional<std::string_view> operand2) {
   uint8_t requiredOperands = operands(op);

   // verifying if they're correct
   if(requiredOperands == 0 && !operand1 && !operand2)
      m_instructions.emplace_back(op);
   else if(requiredOperands == 1 && operand1 && !operand2)
      m_instructions.emplace_back(op, *operand1);
   else if(requiredOperands == 2 && operand1 && operand2)
      m_instructions.emplace_back(op, *operand1, *operand2);
   else
      error(err::Category::INTERNAL, { "Generator.cpp", __LINE__ },
         std::format("Expected {} operands for opcode '{}'!", requiredOperands, to_string(op)), true);
}

bool Generator::isDeclared(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(it->contains(name))
         return true;
   }

   return false;
}

std::optional<Generator::SymbolInfo> Generator::findSymbol(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(auto found = it->find(name); found != it->end())
         return found->second;
   }

   return std::nullopt;
}

std::optional<std::string> Generator::tryFold(const ast::Expression* expr) const {
   return std::visit([](auto&& arg) -> std::optional<std::string> {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::Identifier*>)
         return arg->token.value.value();
      else if constexpr(std::is_same_v<PtrT, ast::Literal*>) {
         if(arg->type == Type::INT)
            return arg->token.value.value();
         if(arg->token.type == TokenType::TRUE)
            return "TRUE";
         if(arg->token.type == TokenType::FALSE)
            return "FALSE";
      }

      return std::nullopt;
   }, *expr);
}

Type Generator::inferType(const ast::Expression* expr) const {
   return std::visit([this](auto&& arg) -> Type {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::Literal*>) {
         return arg->type;

      } else if constexpr(std::is_same_v<PtrT, ast::Identifier*>) {
         const std::string& varName = arg->token.value.value();
         std::optional<SymbolInfo> symbol = findSymbol(varName);

         if(!symbol) {
            error(err::Category::NAME_RESOLUTION, arg->token.location,
               std::format("Use of undeclared identifier '{}'!", varName));
            return Type::NONE;

         } else if(symbol->type == Type::NONE) {
            error(err::Category::TYPE_MISMATCH, arg->token.location,
               std::format("Use of uninitialized identifier '{}'!", varName));
         }

         return symbol->type;

      } else if constexpr(std::is_same_v<PtrT, ast::Negative*>) {
         if(inferType(arg->operand) != Type::INT) {
            error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), "Unary '-' requires an int operand!");
            return Type::NONE;
         }

         return Type::INT;

      } else if constexpr(std::is_same_v<PtrT, ast::BinaryExpr*>) {
         if(inferType(arg->left) != Type::INT || inferType(arg->right) != Type::INT) {
            error(err::Category::TYPE_MISMATCH, arg->op.location,
               std::format("Operator '{}' requires int operands!", getCharsOf(arg->op.type)));
            return Type::NONE;
         }

         return Type::INT;
      }

      return Type::NONE;
   }, *expr);
}

void Generator::error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) const {
   g_errors.report(err::Phase::GENERATING, category, location, message, isFatal);
}

#pragma region Statements

template <>
void Generator::generate(const ast::Declaration* declaration) {
   const std::string& varName = declaration->identifier->token.value.value();
   SymbolInfo symbol{
      .valueMutable = declaration->valueMutable,
      .typeMutable = declaration->lockedType == Type::NONE,
   };

   if(isDeclared(varName)) {
      error(err::Category::NAME_RESOLUTION, declaration->identifier->token.location,
         std::format("Redeclaration of identifier '{}'!", varName));
      return;
   }

   OpCode op = declaration->valueMutable ? OpCode::DEF_VAR_MUT : OpCode::DEF_VAR_CONST;

   if(declaration->expression) {
      symbol.type = inferType(*declaration->expression);
      if(symbol.type == Type::NONE)
         return;

      if(auto folded = tryFold(*declaration->expression)) {
         emit(op, varName, *folded);
      } else {
         generate<ast::Expression>(*declaration->expression);
         emit(op, varName, ir::TOS);
      }
   } else {
      if(!declaration->valueMutable) {
         error(err::Category::MUTABILITY, declaration->identifier->token.location,
            std::format("Cannot declare immutable variable '{}' without initializing it!", varName));
         return;
      }

      emit(OpCode::ALLOC_VAR, varName);
   }

   m_scopes.back()[varName] = symbol;
}

template <>
void Generator::generate(const ast::Assignment* assignment) {
   const std::string& varName = assignment->identifier->token.value.value();
   std::optional<SymbolInfo> symbol = findSymbol(varName);

   if(!symbol) {
      error(err::Category::NAME_RESOLUTION, assignment->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;
   } else if(!symbol->valueMutable) {
      error(err::Category::MUTABILITY, assignment->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;
   }

   Type expressionType = inferType(assignment->expression);
   if(expressionType == Type::NONE)
      return;

   std::string value;
   if(auto folded = tryFold(assignment->expression)) {
      value = *folded;
   } else {
      generate<ast::Expression>(assignment->expression);
      value = ir::TOS;
   }

   emit(OpCode::STORE_VAR, varName, value);

   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(auto found = it->find(varName); found != it->end()) {
         found->second.type = expressionType;
         break;
      }
   }
}

template <>
void Generator::generate(const ast::Exit* exit) {
   Type expressionType = inferType(exit->expression);
   if(expressionType == Type::NONE)
      return;

   std::string code;
   if(auto folded = tryFold(exit->expression)) {
      emit(OpCode::EXIT, *folded);
   } else {
      generate<ast::Expression>(exit->expression);
      emit(OpCode::EXIT, ir::TOS);
   }
}

template <>
void Generator::generate(const ast::Increment* increment) {
   const std::string& varName = increment->identifier->token.value.value();
   std::optional<SymbolInfo> symbol = findSymbol(varName);

   if(!symbol) {
      error(err::Category::NAME_RESOLUTION, increment->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;

   } else if(!symbol->valueMutable) {
      error(err::Category::MUTABILITY, increment->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;

   } else if(symbol->type != Type::INT) {
      error(err::Category::TYPE_MISMATCH, increment->identifier->token.location,
         std::format("Cannot increment non-int identifier '{}' of type {}!", varName, to_string(symbol->type)));
      return;
   }

   emit(OpCode::INCR, varName);
}

template <>
void Generator::generate(const ast::Decrement* decrement) {
   const std::string& varName = decrement->identifier->token.value.value();
   std::optional<SymbolInfo> symbol = findSymbol(varName);

   if(!symbol) {
      error(err::Category::NAME_RESOLUTION, decrement->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;

   } else if(!symbol->valueMutable) {
      error(err::Category::MUTABILITY, decrement->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;

   } else if(symbol->type != Type::INT) {
      error(err::Category::TYPE_MISMATCH, decrement->identifier->token.location,
         std::format("Cannot decrement non-int identifier '{}' of type {}!", varName, to_string(symbol->type)));
      return;
   }

   emit(OpCode::DECR, varName);
}

template <>
void Generator::generate(const ast::Block* block) {
   pushScope();

   for(const ast::Statement& stmt : block->statements)
      generate<ast::Statement>(&stmt);

   popScope();
}

#pragma endregion

#pragma region Expressions

template <>
void Generator::generate(const ast::Literal* literal) {
   if(literal->type == Type::INT)
      emit(OpCode::PUSH_INT, literal->token.value.value());
   else if(literal->token.type == TokenType::TRUE)
      emit(OpCode::PUSH_BOOL, "TRUE");
   else if(literal->token.type == TokenType::FALSE)
      emit(OpCode::PUSH_BOOL, "FALSE");
}

template <>
void Generator::generate(const ast::Identifier* identifier) {
   const std::string& varName = identifier->token.value.value();
   if(!isDeclared(varName)) {
      error(err::Category::NAME_RESOLUTION, identifier->token.location, std::format("Use of undeclared identifier '{}'!", varName));
      return;
   }

   emit(OpCode::PUSH_VAR, varName);
}

template <>
void Generator::generate(const ast::Negative* negative) {
   std::string operand;
   if(auto folded = tryFold(negative->operand)) {
      operand = *folded;
   } else {
      generate<ast::Expression>(negative->operand);
      operand = ir::TOS;
   }

   emit(OpCode::NEG, operand);
}

template <>
void Generator::generate(const ast::BinaryExpr* binaryExpr) {
   std::string left, right;

   if(auto folded = tryFold(binaryExpr->left)) {
      left = *folded;
   } else {
      generate<ast::Expression>(binaryExpr->left);
      left = ir::TOS;
   }

   if(auto folded = tryFold(binaryExpr->right)) {
      right = *folded;
   } else {
      generate<ast::Expression>(binaryExpr->right);
      right = ir::TOS;
   }

   if(right == ir::TOS && left == ir::TOS)
      left = ir::SOS; // left was pushed first so it's SECOND ON STACK

   OpCode opcode;
   switch(binaryExpr->op.type) {
      case TokenType::PLUS:
         opcode = OpCode::ADD;
         break;

      case TokenType::STAR:
         opcode = OpCode::MUL;
         break;

      case TokenType::MINUS:
         opcode = OpCode::SUB;
         break;

      case TokenType::FSLASH:
         opcode = OpCode::DIV;
         break;

      case TokenType::PERCENT:
         opcode = OpCode::MOD;
         break;

      case TokenType::CARET:
         /// @todo calling exponentiation

      default:
         error(err::Category::INTERNAL, { "Generator.cpp", __LINE__ }, std::format("Unsupported binary operator: '{}'!", getCharsOf(binaryExpr->op.type)));
         return;
   }

   emit(opcode, left, right);
}

#pragma endregion
