#include <pch/Precompiled.h>
#include "Generator.h"

std::expected<std::string, std::string> Generator::generate() {
   for(const ast::Statement& stmt : m_program.statements) {
      auto returnValue = generate<ast::Statement>(&stmt);
      if(returnValue)
         return std::unexpected(*returnValue);
   }

   // won't get executed if user exits explicitly, just as a safety net
   m_output += "\n";
   comment("default exit statement in case user hasn't exited explicitly");
   write("mov rax, 1 | 0x2000000");
   write("mov rdi, 0");
   write("syscall");

   std::string header;
   header += "; macOS x86_64, NASM syntax\n\n";
   for(const std::string& func : m_requiredExterns)
      header += std::format("extern {}\n", func);
   header += "\nglobal _main\n";
   header += "_main:\n";
   header += "\tpush rbp      ; save the caller's base pointer\n";
   header += "\tmov rbp, rsp  ; mov our stack pointer to the current base pointer\n";
   header += "\t; rbp doesn't move for this entire function\n\n";

   return header + m_output;
}

std::vector<std::string> Generator::getRequiredLibs() const {
   std::vector<std::string> files{};

   for(const std::string& lib : m_requiredExterns)
      files.push_back(std::format("vault/{}.asm", lib));

   return files;
}

void Generator::comment(std::string_view comment) {
   m_output += std::format("\t; {}\n", comment);
}

void Generator::write(std::string_view cmd) {
   m_output += std::format("\t{}\n", cmd);
}

// GENERATE OVERLOADS

#pragma region Statements

template<>
inline std::optional<std::string> Generator::generate(const ast::Declaration* declaration) {
   std::string_view varName = declaration->identifier->name;

   if(m_stack.contains(varName))
      return std::format("Redeclaration of identifier '{}'!", varName);

   comment(std::format("declaration of {}", varName));
   if(declaration->expression) {
      auto exprError = generate<ast::Expression>(*declaration->expression);
      if(exprError)
         return exprError;

      m_stack.changeTop(varName, declaration->isMutable);
   } else {
      // declaration without definition: identifier points to garbage value
      m_stack.push(m_output, std::nullopt, declaration->isMutable, varName);
   }
   comment("end of declaration\n");

   return std::nullopt;
}

template<>
std::optional<std::string> Generator::generate(const ast::Exit* exit) {
   m_output += "\n";
   comment("Exiting (by user)...");

   auto exprError = generate<ast::Expression>(exit->expression);
   if(exprError)
      return exprError;

   write("mov rax, 1 | 0x2000000 ; exit syscall number");
   m_stack.pop(m_output, "rdi");
   write("syscall");

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Increment* increment) {
   const std::string& varName = increment->identifier->name;
   const auto symbol = m_stack.find(varName);

   if(!symbol)
      return std::format("Use of undeclared identifier '{}'!", varName);
   else if(!symbol->isMutable)
      return std::format("Tried to modify immutable variable '{}'!", varName);
   else 
      write(std::format("inc QWORD [rbp - {}] ; {}++", symbol->offset, varName));

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Decrement* decrement) {
   const std::string& varName = decrement->identifier->name;
   const auto symbol = m_stack.find(varName);

   if(!symbol)
      return std::format("Use of undeclared identifier '{}'!", varName);
   else if(!symbol->isMutable)
      return std::format("Tried to modify immutable variable '{}'!", varName);
   else 
      write(std::format("dec QWORD [rbp - {}] ; {}--", symbol->offset, varName));

   return std::nullopt;
}

#pragma endregion

#pragma region Expressions

template<>
std::optional<std::string> Generator::generate(const ast::IntegerLiteral* integerLiteral) {
   m_stack.push(m_output, integerLiteral->to_string(), false);

   return std::nullopt;
}

template<>
std::optional<std::string> Generator::generate(const ast::Identifier* identifier) {
   const std::string& varName = identifier->name;
   const auto symbol = m_stack.find(varName);

   if(!symbol)
      return std::format("Use of undeclared identifier '{}'!", varName);
   else
      m_stack.push(m_output, std::format("QWORD [rbp - {}] ; '{}'", symbol->offset, varName));

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Negative* negative) {
   auto operand = generate<ast::Expression>(negative->operand);
   if(operand)
      return operand;

   m_stack.pop(m_output, "rax");
   write("neg rax");
   m_stack.push(m_output, "rax", false);

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::BinaryExpr* binaryExpr) {
   auto left = generate<ast::Expression>(binaryExpr->left);
   if(left)
      return left;

   auto right = generate<ast::Expression>(binaryExpr->right);
   if(right)
      return right;

   m_stack.pop(m_output, std::format("rbx ; rhs for op '{}'", to_string(binaryExpr->op)));
   m_stack.pop(m_output, std::format("rax ; lhs for op '{}'", to_string(binaryExpr->op)));

   switch(binaryExpr->op) {
      case TokenType::PLUS:
         write("add rax, rbx");
         break;

      case TokenType::STAR:
         write("imul rax, rbx");
         break;

      case TokenType::MINUS:
         write("sub rax, rbx");
         break;

      case TokenType::SLASH:
         write("cqo ; prep rdx:rax for division");
         write("idiv rbx");
         comment("rax now holds the quotient");
         break;

      case TokenType::PERCENT:
         write("cqo ; prep rdx:rax for division");
         write("idiv rbx");
         write("mov rax, rdx ; store remainder");
         break;

      case TokenType::CARET:
         m_requiredExterns.insert("exponentiate");
         write("call exponentiate");
         break;

      default:
         return std::format("Unsupported binary operator: '{}'!", to_string(binaryExpr->op));
   }

   m_stack.push(m_output, std::format("rax ; result of binary operation '{}'", to_string(binaryExpr->op)), false);

   return std::nullopt;
}

#pragma endregion
