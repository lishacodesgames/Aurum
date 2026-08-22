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

void Generator::write(std::string_view cmd, std::optional<std::string_view> comment) {
   m_output += std::format("\t{} {}\n", cmd, comment ? *comment : "");
}

void Generator::push(std::string_view value, std::optional<std::string_view> comment) {
   // does: rsp -= 8 and mov's value to [rsp] (top of stack)
   write(std::format("push {}", value), comment);
   m_stackSize++;
}

void Generator::pop(std::string_view reg, std::optional<std::string_view> comment) {
   // does: reg = value; rsp += 8
   write(std::format("pop {}", reg), comment);
   m_stackSize--;
}

// GENERATE OVERLOADS

// statements
template <>
inline std::optional<std::string> Generator::generate(const ast::Declaration* declaration) {
   if(m_symbolTable.contains(declaration->identifier->name))
      return std::format("Redeclaration of identifier '{}'!", declaration->identifier->name);

   comment(std::format("declaration of {}", declaration->identifier->name));
   if(declaration->expression) {
      auto returnValue = generate<ast::Expression>(*declaration->expression);
      if(returnValue)
         return std::move(returnValue);
   } else { // declaration without definition: identifier points to garbage value
      write("sub rsp, 8");
      m_stackSize++;
   }
   comment("end of declaration\n");

   m_symbolTable[declaration->identifier->name] = m_stackSize;

   return std::nullopt;
}

/// @todo Expression
template<>
std::optional<std::string> Generator::generate(const ast::Exit* exit) {
   m_output += "\n";
   comment("Exiting...");

   auto returnValue = generate<ast::Expression>(exit->expression);
   if(returnValue)
      return std::move(returnValue);

   write("mov rax, 1 | 0x2000000", "; exit syscall number");
   pop("rdi", "; store return value");
   write("syscall");

   return std::nullopt;
}

// expressions

template<>
std::optional<std::string> Generator::generate(const ast::IntegerLiteral* integerLiteral) {
   push(integerLiteral->to_string());

   return std::nullopt;
}

template<>
std::optional<std::string> Generator::generate(const ast::Identifier* identifier) {
   if(!m_symbolTable.contains(identifier->name))
      return std::format("Use of undeclared identifier '{}'!", identifier->name);

   push(std::format("qword [rbp - {}]", m_symbolTable.at(identifier->name) * 8), std::format("; '{}'", identifier->name));

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::Negative* negative) {
   auto returnValue = generate<ast::Expression>(negative->operand);
   if(returnValue)
      return std::move(returnValue);

   pop("rax");
   write("neg rax");
   push("rax");

   return std::nullopt;
}

template <>
std::optional<std::string> Generator::generate(const ast::BinaryExpr* binaryExpr) {
   auto leftReturnValue = generate<ast::Expression>(binaryExpr->left);
   if(leftReturnValue)
      return std::move(leftReturnValue);

   auto rightReturnValue = generate<ast::Expression>(binaryExpr->right);
   if(rightReturnValue)
      return std::move(rightReturnValue);

   pop("rbx", "; rhs");
   pop("rax", "; lhs");

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
         write("cqo", "; prep rdx:rax for division");
         write("idiv rbx");
         comment("rax now holds the quotient");
         break;

      case TokenType::PERCENT:
         write("cqo", "; prep rdx:rax for division");
         write("idiv rbx");
         write("mov rax, rdx", "; store remainder");
         break;

      case TokenType::CARET:
         m_requiredExterns.insert("exponentiate");
         write("call exponentiate");
         write("call print_int"); // temp
         m_requiredExterns.insert("print_int");
         break;

      default:
         return std::format("Unsupported binary operator: '{}'!", to_string(binaryExpr->op));
   }

   push("rax", std::format("; result of binary operation '{}'", to_string(binaryExpr->op)));

   return std::nullopt;
}
