#include <pch/Precompiled.h>
#include "Generator.h"

std::expected<std::vector<ir::Instruction>, std::string> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements) {
      if(auto error = generate<ast::Statement>(&stmt))
         return std::unexpected(*error);
   }

   emit(ir::OpCode::EXIT, "0"); // in case user hasn't exited
   return m_instructions;
}

void Generator::emit(ir::OpCode op, std::optional<std::string_view> operand1, std::optional<std::string_view> operand2) {
   uint8_t requiredOperands = ir::operands(op);

   // verifying if they're correct
   if(requiredOperands == 0 && !operand1 && !operand2)
      m_instructions.emplace_back(op);
   else if(requiredOperands == 1 && operand1 && !operand2)
      m_instructions.emplace_back(op, *operand1);
   else if(requiredOperands == 2 && operand1 && operand2)
      m_instructions.emplace_back(op, *operand1, *operand2);
   else
      throw std::runtime_error(std::format("Expected {} operands for opcode '{}'!", requiredOperands, ir::to_string(op)));
}

bool Generator::isDeclared(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(it->contains(name))
         return true;
   }

   return false;
}

std::optional<bool> Generator::findMutability(const std::string& name) const {
   for(auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if(auto found = it->find(name); found != it->end())
         return found->second;
   }

   return std::nullopt;
}

std::optional<std::string> Generator::tryFold(const ast::Expression* expr) const {
   return std::visit([](auto&& arg) -> std::optional<std::string> {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::IntegerLiteral*>)
         return arg->to_string();
      else if constexpr(std::is_same_v<PtrT, ast::Identifier*>)
         return arg->name;

      return std::nullopt;
   }, *expr);
}

#pragma region Statements

template <>
inline std::optional<std::string> Generator::generate(const ast::Declaration* declaration) {
   const std::string& varName = declaration->identifier->name;

   if(isDeclared(varName))
      return std::format("Redeclaration of identifier '{}'!", varName);

   if(declaration->expression) {
      if(auto folded = tryFold(*declaration->expression)) {
         emit(ir::OpCode::DEF_VAR, varName, *folded);
      } else {
         if(auto exprError = generate<ast::Expression>(*declaration->expression)) return exprError;
         emit(ir::OpCode::DEF_VAR, varName, ir::TOS);
      }
   } else {
      emit(ir::OpCode::ALLOC_VAR, varName);
   }

   m_scopes.back()[varName] = declaration->isMutable;
   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Assignment* assignment) {
   const std::string& varName = assignment->identifier->name;
   auto mutability = findMutability(varName);

   if(!mutability.has_value())
      return std::format("Use of undeclared identifier '{}'!", varName);
   else if(!*mutability)
      return std::format("Tried to modify immutable variable '{}'!", varName);

   if(auto folded = tryFold(assignment->expression)) {
      emit(ir::OpCode::STORE_VAR, varName, *folded);
   } else {
      if(auto exprError = generate<ast::Expression>(assignment->expression)) return exprError;
      emit(ir::OpCode::STORE_VAR, varName, ir::TOS);
   }

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Exit* exit) {
   if(auto folded = tryFold(exit->expression))
      emit(ir::OpCode::EXIT, *folded);
   else {
      if(auto exprError = generate<ast::Expression>(exit->expression))
         return exprError;

      emit(ir::OpCode::EXIT, ir::TOS);
   }

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Increment* increment) {
   const std::string& varName = increment->identifier->name;
   auto mutability = findMutability(varName);

   if(!mutability.has_value())
      return std::format("Use of undeclared identifier '{}'!", varName);
   else if(!*mutability)
      return std::format("Tried to modify immutable variable '{}'!", varName);

   emit(ir::OpCode::INCR, varName);
   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Decrement* decrement) {
   const std::string& varName = decrement->identifier->name;
   auto mutability = findMutability(varName);

   if(!mutability.has_value())
      return std::format("Use of undeclared identifier '{}'!", varName);
   else if(!*mutability)
      return std::format("Tried to modify immutable variable '{}'!", varName);

   emit(ir::OpCode::DECR, varName);
   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Block* block) {
   pushScope();

   for(const ast::Statement& stmt : block->statements) {
      if(auto stmtError = generate<ast::Statement>(&stmt))
         return stmtError;
   }

   popScope();
   return std::nullopt;
}

#pragma endregion

#pragma region Expressions

template <>
std::optional<std::string> Generator::generate(const ast::IntegerLiteral* integerLiteral) {
   emit(ir::OpCode::PUSH_INT, integerLiteral->to_string());
   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Identifier* identifier) {
   const std::string& varName = identifier->name;
   if(!isDeclared(varName))
      return std::format("Use of undeclared '{}!", varName);

   emit(ir::OpCode::PUSH_VAR, varName);
   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Negative* negative) {
   if(auto folded = tryFold(negative->operand)) {
      emit(ir::OpCode::NEG, *folded);
   } else {
      if(auto exprError = generate<ast::Expression>(negative->operand))
         return exprError;

      emit(ir::OpCode::NEG, ir::TOS);
   }

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::BinaryExpr* binaryExpr) {
   std::string left, right;

   if(auto folded = tryFold(binaryExpr->left)) {
      left = *folded;
   } else {
      if(auto exprError = generate<ast::Expression>(binaryExpr->left))
         return exprError;

      left = ir::TOS;
   }

   if(auto folded = tryFold(binaryExpr->right)) {
      right = *folded;
   } else {
      if(auto exprError = generate<ast::Expression>(binaryExpr->right))
         return exprError;

      right = ir::TOS;
   }

   ir::OpCode opcode;
   switch(binaryExpr->op) {
      case TokenType::PLUS:
         opcode = ir::OpCode::ADD;
         break;

      case TokenType::STAR:
         opcode = ir::OpCode::MUL;
         break;

      case TokenType::MINUS:
         opcode = ir::OpCode::SUB;
         break;

      case TokenType::FSLASH:
         opcode = ir::OpCode::DIV;
         break;

      case TokenType::PERCENT:
         opcode = ir::OpCode::MOD;
         break;

      case TokenType::CARET:
         /// @todo calling exponentiation

      default:
         return std::format("Unsupported binary operator: '{}'!", getCharsOf(binaryExpr->op));
   }

   emit(opcode, left, right);
   return std::nullopt;
}

#pragma endregion
