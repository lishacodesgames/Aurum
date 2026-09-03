#include <pch/Precompiled.h>
#include "Generator.h"

std::vector<ir::Instruction> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements)
      generate<ast::Statement>(&stmt);

   emit(ir::OpCode::EXIT, "0"); // in case user hasn't exited
   return m_instructions; // NOT to be moved bcz it needs to be accessed later
}

std::string Generator::getIR() const {
   std::string IR;

   IR += "; Intermediate Representation for Aurum\n\n";
   IR += "_main:\n"; /// @todo function definition opcodes

   for(const ir::Instruction& instr : m_instructions) {
      IR += "\t" + ir::to_string(instr.opcode);

      if(instr.operandLeft) {
         IR += " " + *instr.operandLeft;
         if(instr.operandRight)
            IR += ", " + *instr.operandRight;
      }

      IR += "\n";
   }

   return IR;
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

/// @todo if hit error anywhere, return and parse the next statement
#pragma region Statements

template <>
void Generator::generate(const ast::Declaration* declaration) {
   const std::string& varName = declaration->identifier->name;

   if(isDeclared(varName)) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Redeclaration of identifier '{}'!", varName), true);
      return;
   }

   if(declaration->expression) {
      if(auto folded = tryFold(*declaration->expression)) {
         emit(ir::OpCode::DEF_VAR, varName, *folded);
      } else {
         generate<ast::Expression>(*declaration->expression);
         emit(ir::OpCode::DEF_VAR, varName, ir::TOS);
      }
   } else {
      emit(ir::OpCode::ALLOC_VAR, varName);
   }

   m_scopes.back()[varName] = declaration->isMutable;
}

template <>
void Generator::generate(const ast::Assignment* assignment) {
   const std::string& varName = assignment->identifier->name;
   std::optional<bool> mutability = findMutability(varName);

   if(!mutability.has_value()) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
            /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", varName), true);
      return;
   } else if(!*mutability) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
            /* @todo */ {}, std::format("Tried to modify immutable variable '{}'!", varName), true);
      return;
   }

   std::string arg2;
   if(auto folded = tryFold(assignment->expression)) {
      arg2 = *folded;
   } else {
      generate<ast::Expression>(assignment->expression);
      arg2 = ir::TOS;
   }

   emit(ir::OpCode::STORE_VAR, varName, arg2);
}

template <>
void Generator::generate(const ast::Exit* exit) {
   std::string arg;
   if(auto folded = tryFold(exit->expression))
      arg = *folded;
   else {
      generate<ast::Expression>(exit->expression);
      arg = ir::TOS;
   }

   emit(ir::OpCode::EXIT, arg);
}

template <>
void Generator::generate(const ast::Increment* increment) {
   const std::string& varName = increment->identifier->name;
   std::optional<bool> mutability = findMutability(varName);

   if(!mutability.has_value()) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", varName), true);
      return;
   } else if(!*mutability) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Tried to modify immutable variable '{}'!", varName), true);
      return;
   }

   emit(ir::OpCode::INCR, varName);
}

template <>
void Generator::generate(const ast::Decrement* decrement) {
   const std::string& varName = decrement->identifier->name;
   std::optional<bool> mutability = findMutability(varName);

   if(!mutability.has_value()) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", varName), true);
      return;
   } else if(!*mutability) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Tried to modify immutable variable '{}'!", varName), true);
      return;
   }

   emit(ir::OpCode::DECR, varName);
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
void Generator::generate(const ast::IntegerLiteral* integerLiteral) {
   emit(ir::OpCode::PUSH_INT, integerLiteral->to_string());
}

template <>
void Generator::generate(const ast::Identifier* identifier) {
   const std::string& varName = identifier->name;
   if(!isDeclared(varName)) {
      g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
         /* todo */ {}, std::format("Use of undeclared '{}!", varName), true);
      return;
   }

   emit(ir::OpCode::PUSH_VAR, varName);
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

   emit(ir::OpCode::NEG, operand);
}

/// @todo fix: both should not be tos. maybe add sos (second on stack as a value)
/// sub tos, tos should result in 0 basically (tos - tos = 0)
/// but instead it assumes left tos to be below right tos which is not good
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
         g_errors.report(Phase::GENERATING, Category::NAME_RESOLUTION,
            /* todo */ {}, std::format("Unsupported binary operator: '{}'!", getCharsOf(binaryExpr->op)), true);
         return;
   }

   emit(opcode, left, right);
}

#pragma endregion
