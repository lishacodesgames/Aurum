#include <pch/Precompiled.h>
#include "Generator.h"

namespace
{
   err::SourceLocation getLocation(const ast::Expression& expr) {
      return std::visit([](auto&& arg) -> err::SourceLocation {
         using PtrT = std::decay_t<decltype(arg)>;

         if constexpr(std::is_same_v<PtrT, ast::BinaryExpr*>)
            return arg->opToken.location;
         else if constexpr(std::is_same_v<PtrT, ast::Literal*> || std::is_same_v<PtrT, ast::Identifier*>)
            return arg->token.location;
         else if constexpr(std::is_same_v<PtrT, ast::UnaryExpr*>)
            return getLocation(arg->operand);
         else // monostate
            return {};
      }, expr);
   }

   /// @return whether dest <- value is a valid DataType conversion
   bool isAssignable(DataType dest, DataType value) {
      /// @todo type conversion system, for now only exact matches are allowed
      if(dest == DataType::NONE)
         return true; // None can always be overridden

      if(value == DataType::NONE)
         return false; // can't assign uninitialized value to a variable

      return dest == value;
   }
}

Generator::Generator(ast::Program program) : m_program(program) {
   m_ir += "; Intermediate Representation for Aurum\n\n";
   m_ir += "_main:\n"; /// @todo function definitions will take over this
   pushScope(); // push scope 0
   m_ir += "\n";
}

std::string Generator::getIR() {
   return std::move(m_ir);
}

std::vector<ir::Instruction> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements)
      generate<ast::Statement>(stmt);

   m_ir += "\n";
   popScope();
   emit(OpCode::EXIT, "0"); // in case user hasn't exited
   return m_instructions; // NOT to be moved bcz it needs to be accessed later
}

#pragma region Helpers

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

   // format: opcode operand1, operand2
   m_ir += std::format("{}{} {}", isIndented(op) ? "\t" : "", to_string(op), operand1);
   if(operand2)
      m_ir += std::format(", {}", *operand2);

   m_ir += "\n";
}

std::string Generator::newLabel(std::string_view prefix) {
   return std::format(".{}_{}", prefix, m_labelCount++); // post++ bcz indexing starts from 0
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

std::optional<std::string> Generator::tryFold(const ast::Expression& expr) const {
   return std::visit([](auto&& arg) -> std::optional<std::string> {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::Identifier*>)
         return arg->token.value.value();
      else if constexpr(std::is_same_v<PtrT, ast::Literal*>) {
         if(arg->type == DataType::INT)
            return arg->token.value.value();
         if(arg->token.type == TokenType::TRUE)
            return "TRUE";
         if(arg->token.type == TokenType::FALSE)
            return "FALSE";
      }

      return std::nullopt;
   }, expr);
}

std::optional<DataType> Generator::inferType(const ast::Expression& expr) const {
   return std::visit([this](auto&& arg) -> std::optional<DataType> {
      using PtrT = std::decay_t<decltype(arg)>;

      if constexpr(std::is_same_v<PtrT, ast::Literal*>) {
         return arg->type;

      } else if constexpr(std::is_same_v<PtrT, ast::Identifier*>) {
         const std::string& varName = arg->token.value.value();
         const SymbolInfo* symbol = findSymbol(varName);

         if(!symbol) {
            error(err::Category::NAME_RESOLUTION, arg->token.location, std::format("Use of undeclared identifier '{}'!", varName));
            return std::nullopt;

         } else if(symbol->type == DataType::NONE) {
            error(err::Category::TYPE_MISMATCH, arg->token.location, std::format("Use of uninitialized identifier '{}'!", varName));
            return std::nullopt;
         }

         return symbol->type;

      } else if constexpr(std::is_same_v<PtrT, ast::UnaryExpr*>) {
         std::optional<DataType> operandType = inferType(arg->operand);
         if(!operandType) {
            error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), std::format("Invalid expression for unary operator '{}'!", to_string(arg->opToken.type)));
            return std::nullopt;
         }

         switch(arg->opToken.type) {
            case TokenType::LOGICAL_NOT:
               if(*operandType != DataType::BOOL) {
                  error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), "Unary operator '!' requires type BOOL, but got " + to_string(*operandType));
                  return std::nullopt;
               }

               return DataType::BOOL;

            case TokenType::MINUS:
               if(*operandType != DataType::INT) {
                  error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), "Unary operator '-' requires type INT, but got " + to_string(*operandType));
                  return std::nullopt;
               }

               return DataType::INT;

            default:
               error(err::Category::TYPE_MISMATCH, getLocation(arg->operand), "Invalid unary operator: " + to_string(arg->opToken.type));
               return std::nullopt;
         }

      } else if constexpr(std::is_same_v<PtrT, ast::BinaryExpr*>) {
         if(isBinaryExprValid(arg))
            return arg->type;

         return std::nullopt;
      }

      error(err::Category::INTERNAL, { "Generator.cpp", __LINE__ }, "Cannot infer type!", true);
      return std::nullopt;
   }, expr);
}

bool Generator::isBinaryExprValid(const ast::BinaryExpr* binaryExpr) const {
   std::optional<DataType> leftType = inferType(binaryExpr->left);
   std::optional<DataType> rightType = inferType(binaryExpr->right);
   TokenType op = binaryExpr->opToken.type;

   if(!leftType || !rightType) {
      error(
         err::Category::TYPE_MISMATCH, binaryExpr->opToken.location,
         std::format("Operator '{}' requires {} operands!", to_string(op), to_string(binaryExpr->type)));
      return false;
   }

   DataType opType = getReturnType(op);
   if(opType == DataType::BOOL && *leftType != *rightType) {
      error(err::Category::TYPE_MISMATCH, binaryExpr->opToken.location, std::format(
         "Operator '{}' must have same types on both sides but has {} and {}!",
         to_string(op), to_string(*leftType), to_string(*rightType)));
      return false;
   } else if(opType == DataType::INT && (*leftType != DataType::INT || *rightType != DataType::INT)) {
      error(err::Category::TYPE_MISMATCH, binaryExpr->opToken.location, std::format(
         "Operator '{}' requires INT operands, but has {} and {}",
         to_string(op), to_string(*leftType), to_string(*rightType))
      );
   }

   return true;
}

void Generator::error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) const {
   g_errors.report(err::Phase::GENERATING, category, location, message, isFatal);
}

#pragma endregion

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
      std::optional<DataType> exprType = inferType(*declaration->expression);
      if(!exprType)
         return; // error msg is handled by inferType()

      symbol.type = *exprType;
      if(declaration->type.has_value() && *declaration->type != *exprType) {
         // if declaration should have a specific type and it doesn't match expression's: it should be reassigned
         symbol.type = *declaration->type;

         if(!isAssignable(*declaration->type, *exprType)) {
            // if type is not assignable from expression's then there's an error

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

      if(auto folded = tryFold(*declaration->expression)) {
         emit(OpCode::DEF_VAR, varName, *folded);
      } else {
         generate<ast::Expression>(*declaration->expression);
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

   std::optional<DataType> exprType = inferType(assignment->expression);
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
   std::optional<DataType> exprType = inferType(exit->expression);
   if(!exprType)
      return;
   if(!isAssignable(DataType::INT, *exprType)) {
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

   } else if(symbol->type != DataType::INT) {
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

   } else if(symbol->type != DataType::INT) {
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
      generate<ast::Statement>(stmt);

   popScope();
}

template <>
void Generator::generate(const ast::If* ifStmt) {
   std::optional<DataType> condType = inferType(ifStmt->condition);
   if(!condType)
      return;
   if(*condType != DataType::BOOL) {
      /// @todo add support for implicit conversion to bool
      error(
         err::Category::TYPE_MISMATCH, getLocation(ifStmt->condition),
         "If condition must be of type BOOL, but is " + to_string(*condType));
      return;
   }

   std::string endLabel = newLabel("if_end");
   if(auto folded = tryFold(ifStmt->condition)) {
      emit(OpCode::JUMP_IF_NOT, *folded, endLabel);
   } else {
      generate<ast::Expression>(ifStmt->condition);
      emit(OpCode::JUMP_IF_NOT, ir::TOS, endLabel);
   }

   generate<ast::Statement>(ifStmt->thenBranch);
   emit(OpCode::LABEL, endLabel);
}

#pragma endregion

#pragma region Expressions

template <>
void Generator::generate(const ast::Literal* literal) {
   if(literal->type == DataType::INT)
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
void Generator::generate(const ast::UnaryExpr* unaryExpr) {
   OpCode opcode;
   switch(unaryExpr->opToken.type) {
      case TokenType::LOGICAL_NOT:  opcode = OpCode::NOT; break;
      case TokenType::MINUS:        opcode = OpCode::NEG; break;
      default: error(err::Category::SYNTAX, unaryExpr->opToken.location, "Invalid unary operator: " + to_string(unaryExpr->opToken.type));
   }

   if(auto folded = tryFold(unaryExpr->operand)) {
      emit(opcode, *folded);
   } else {
      generate<ast::Expression>(unaryExpr->operand);
      emit(opcode, ir::TOS);
   }
}

template <>
void Generator::generate(const ast::BinaryExpr* binaryExpr) {
   if(!isBinaryExprValid(binaryExpr))
      return;

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
   switch(binaryExpr->opToken.type) {
      case TokenType::PLUS:            opcode = OpCode::ADD; break;
      case TokenType::STAR:            opcode = OpCode::MUL; break;
      case TokenType::MINUS:           opcode = OpCode::SUB; break;
      case TokenType::FSLASH:          opcode = OpCode::DIV; break;
      case TokenType::PERCENT:         opcode = OpCode::MOD; break;

      case TokenType::LOGICAL_AND:     opcode = OpCode::AND; break;
      case TokenType::LOGICAL_OR:      opcode = OpCode::OR;  break;

      case TokenType::EQUALITY:        opcode = OpCode::EQ;  break;
      case TokenType::INEQUALITY:      opcode = OpCode::NEQ; break;
      case TokenType::LESS_THAN:       opcode = OpCode::LT;  break;
      case TokenType::GREATER_THAN:    opcode = OpCode::GT;  break;
      case TokenType::LESS_EQUALS:     opcode = OpCode::LTE; break;
      case TokenType::GREATER_EQUALS:  opcode = OpCode::GTE; break;

      case TokenType::CARET:
         /// @todo calling exponentiation

      default:
         error(
            err::Category::INTERNAL, { "Generator.cpp", __LINE__ },
            std::format("Unsupported binary operator: '{}'!", to_string(binaryExpr->opToken.type)));
         return;
   }

   emit(opcode, left, right);
}

#pragma endregion
