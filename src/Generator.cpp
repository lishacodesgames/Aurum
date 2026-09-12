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

   /// @return whether dest <- value is a valid Type conversion
   bool isAssignable(Type dest, Type value) {
      /// @todo type conversion system, for now only exact matches are allowed
      if(dest == Type::NONE)
         return true;

      if(value == Type::NONE)
         return false; // can't assign uninitialized value to a variable

      return dest == value;
   }
}

std::vector<ir::Instruction> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements)
      generate<ast::Statement>(&stmt);

   emit(OpCode::EXIT, "0"); // in case user hasn't exited
   return m_instructions; // NOT to be moved bcz it needs to be accessed later
}

void Generator::emit(OpCode op, std::string_view operand1, std::optional<std::string_view> operand2) {
   uint8_t requiredOperands = operands(op);

   // verifying if they're correct
   if(requiredOperands == 1 && !operand2)
      m_instructions.emplace_back(op, operand1);
   else if(requiredOperands == 2 && operand2)
      m_instructions.emplace_back(op, operand1, *operand2);
   else
      error(
         err::Category::INTERNAL, { "Generator.cpp", __LINE__ },
         std::format("Expected {} operands for opcode '{}'!", requiredOperands, to_string(op)), true);

   m_ir += std::format("\t{} {}", to_string(op), operand1);
   if(operand2)
      m_ir += std::format(", {}", *operand2);
   m_ir += "\n";
}

void Generator::pushScope() {
   emit(OpCode::SCOPE_START, std::to_string(m_scopes.size()));
   m_scopes.emplace_back();
}

void Generator::popScope() {
   m_scopes.pop_back();
   emit(OpCode::SCOPE_END, std::to_string(m_scopes.size()));
}

bool Generator::isDeclared(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(it->contains(name))
         return true;
   }

   return false;
}

Generator::SymbolInfo* Generator::findSymbol(const std::string& name) {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(auto found = it->find(name); found != it->end())
         return &(found->second);
   }

   return nullptr;
}

const Generator::SymbolInfo* Generator::findSymbol(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(auto found = it->find(name); found != it->end())
         return &(found->second);
   }

   return nullptr;
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

std::optional<Type> Generator::inferType(const ast::Expression* expr) const {
   return std::visit([this](auto&& arg) -> std::optional<Type> {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::Literal*>) {
         return arg->type;

      } else if constexpr(std::is_same_v<PtrT, ast::Identifier*>) {
         const std::string& varName = arg->token.value.value();
         const SymbolInfo* symbol = findSymbol(varName);

         if(!symbol) {
            error(err::Category::NAME_RESOLUTION, arg->token.location, std::format("Use of undeclared identifier '{}'!", varName));
            return std::nullopt;

         } else if(symbol->type == Type::NONE) {
            error(err::Category::TYPE_MISMATCH, arg->token.location, std::format("Use of uninitialized identifier '{}'!", varName));
            return std::nullopt;
         }

         return symbol->type;

      } else if constexpr(std::is_same_v<PtrT, ast::Negative*>) {
         std::optional<Type> operandType = inferType(arg->operand);
         if(!operandType || *operandType != Type::INT) {
            error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), "Unary operator '-' requires an int operand!");
            return std::nullopt;
         }

         return Type::INT;

      } else if constexpr(std::is_same_v<PtrT, ast::BinaryExpr*>) {
         std::optional<Type> leftType = inferType(arg->left);
         std::optional<Type> rightType = inferType(arg->right);

         if(!leftType || !rightType || *leftType != Type::INT || *rightType != Type::INT) {
            error(
               err::Category::TYPE_MISMATCH, arg->op.location,
               std::format("Operator '{}' requires int operands!", getCharsOf(arg->op.type)));
            return std::nullopt;
         }

         return Type::INT;
      }

      error(err::Category::INTERNAL, { "Generator.cpp", __LINE__ }, "Cannot infer type!", true);
      return std::nullopt;
   }, *expr);
}

void Generator::error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) const {
   g_errors.report(err::Phase::GENERATING, category, location, message, isFatal);
}

#pragma region Statements

template <>
void Generator::generate(const ast::Declaration* declaration) {
   const std::string& varName = declaration->identifier->token.value.value();
   SymbolInfo symbol{ declaration->valueMutable, declaration->typeMutable };

   if(isDeclared(varName)) {
      error(
         err::Category::NAME_RESOLUTION, declaration->identifier->token.location,
         std::format("Redeclaration of identifier '{}'!", varName));
      return;
   }

   if(declaration->expression) {
      std::optional<Type> exprType = inferType(declaration->expression);
      if(!exprType)
         return; // error msg is handled by inferType()

      symbol.type = *exprType;
      if(declaration->type.has_value() && *declaration->type != *exprType) {
         // if declaration should have a specific type and it doesn't match expression's: it should be reassigned
         symbol.type = *declaration->type;

         if(!isAssignable(*declaration->type, *exprType)) {
            // if type is not assignable from expression's then smth's wrong

            if(declaration->typeMutable) { // it's a type hint
               error(err::Category::TYPE_MISMATCH, declaration->identifier->token.location, std::format(
                  "Type hint {} is incorrect, cannot convert {} to it! (for declaration of identifier '{}')",
                  to_string(*declaration->type), to_string(*exprType), declaration->identifier->token.value.value()));
               return;
            } else {
               error(err::Category::TYPE_MISMATCH, declaration->identifier->token.location, std::format(
                  "Expected expression of type {} but got {}! (for declaration of identifier '{}')",
                  to_string(*declaration->type), to_string(*exprType), declaration->identifier->token.value.value()));
               return;
            }
         }
      }

      if(auto folded = tryFold(declaration->expression)) {
         emit(OpCode::DEF_VAR, varName, *folded);
      } else {
         generate<ast::Expression>(declaration->expression);
         emit(OpCode::DEF_VAR, varName, ir::TOS);
      }

   } else {
      if(!declaration->valueMutable) {
         error(
            err::Category::MUTABILITY, declaration->identifier->token.location,
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
   SymbolInfo* symbol = findSymbol(varName);

   /// @todo extract this part
   if(!symbol) {
      error(
         err::Category::NAME_RESOLUTION, assignment->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;
   } else if(!symbol->valueMutable) {
      error(
         err::Category::MUTABILITY, assignment->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;
   }

   std::optional<Type> exprType = inferType(assignment->expression);
   if(!exprType)
      return;

   if(symbol->type != *exprType) {
      if(!symbol->typeMutable && !isAssignable(symbol->type, *exprType)) {
         error(
            err::Category::TYPE_MISMATCH, assignment->identifier->token.location,
            std::format("Tried to change type of locked variable '{}'!", varName));
         return;
      }

      if(symbol->typeMutable)
         symbol->type = *exprType;
   }

   std::string value;
   if(auto folded = tryFold(assignment->expression)) {
      value = *folded;
   } else {
      generate<ast::Expression>(assignment->expression);
      value = ir::TOS;
   }

   emit(OpCode::STORE_VAR, varName, value);
}

template <>
void Generator::generate(const ast::Exit* exit) {
   std::optional<Type> exprType = inferType(exit->expression);
   if(!exprType)
      return;
   if(!isAssignable(Type::INT, *exprType)) {
      error(
         err::Category::INTERNAL, getLocation(exit->expression),
         "Exit code must be of type INT, but is " + to_string(*exprType));
      return;
   }

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
   const SymbolInfo* symbol = findSymbol(varName);

   if(!symbol) {
      error(
         err::Category::NAME_RESOLUTION, increment->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;

   } else if(!symbol->valueMutable) {
      error(
         err::Category::MUTABILITY, increment->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;

   } else if(symbol->type != Type::INT) {
      error(
         err::Category::TYPE_MISMATCH, increment->identifier->token.location,
         std::format("Cannot increment non-int identifier '{}' of type {}!", varName, to_string(symbol->type)));
      return;
   }

   emit(OpCode::INCR, varName);
}

template <>
void Generator::generate(const ast::Decrement* decrement) {
   const std::string& varName = decrement->identifier->token.value.value();
   const SymbolInfo* symbol = findSymbol(varName);

   if(!symbol) {
      error(err::Category::NAME_RESOLUTION, decrement->identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;

   } else if(!symbol->valueMutable) {
      error(err::Category::MUTABILITY, decrement->identifier->token.location,
         std::format("Tried to modify immutable variable '{}'!", varName));
      return;

   } else if(symbol->type != Type::INT) {
      error(
         err::Category::TYPE_MISMATCH, decrement->identifier->token.location,
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
      error(
         err::Category::NAME_RESOLUTION, identifier->token.location,
         std::format("Use of undeclared identifier '{}'!", varName));
      return;
   }

   emit(OpCode::PUSH_VAR, varName);
}

template <>
void Generator::generate(const ast::Negative* negative) {
   if(auto folded = tryFold(negative->operand)) {
      emit(OpCode::NEG, *folded);
   } else {
      generate<ast::Expression>(negative->operand);
      emit(OpCode::NEG, ir::TOS);
   }
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
         error(
            err::Category::INTERNAL, { "Generator.cpp", __LINE__ },
            std::format("Unsupported binary operator: '{}'!", getCharsOf(binaryExpr->op.type)));
         return;
   }

   emit(opcode, left, right);
}

#pragma endregion
