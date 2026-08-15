#include <pch/Precompiled.h>
#include "Generator.h"

std::expected<std::string, std::string> Generator::generate() {
   m_output += "; macOS x86_64, NASM syntax\n\n";
   m_output += "global _main\n";
   m_output += "_main:\n";
   m_output += "\tpush rbp     ; save the caller's base pointer\n";
   m_output += "\tmov rbp, rsp ; set our base pointer to the current stack pointer\n";

   for(const ast::Statement& stmt : m_program.statements) {
      auto returnValue = generate<ast::Statement>(stmt);
      if(returnValue)
         return std::unexpected(*returnValue);
   }

   return m_output;
}

void Generator::push(std::string_view value) {
   // does: rsp -= 8 and mov's value to [rsp] (top of stack)
   m_output += std::format("\tpush {}\n", value);
   m_stackSize++;
}

void Generator::mov(std::string reg, std::string value) {
   m_output += std::format("\tmov {}, {}\n", reg, value);
}

void Generator::pop(std::string_view reg) {
   // does: reg = value; rsp += 8
   m_output += std::format("\tpop {}\n", reg);
   m_stackSize--;
}

// GENERATE OVERLOADS

// statements
template <>
inline std::optional<std::string> Generator::generate<ast::Declaration>(const ast::Declaration& declaration) {
   if(m_symbolTable.contains(declaration.identifier.name))
      return std::format("Redeclaration of identifier '{}'!", declaration.identifier.name);

   if(declaration.expression) {
      auto returnValue = generate<ast::Expression>(*declaration.expression);
      if(returnValue)
         return std::move(returnValue);
   } else { // declaration without definition: identifier points to garbage value
      m_output += "\tsub rsp, 8\n";
      m_stackSize++;
   }

   m_symbolTable[declaration.identifier.name] = m_stackSize;

   return std::nullopt;
}

/// @todo Expression
template<>
std::optional<std::string> Generator::generate<ast::Exit>(const ast::Exit& exit) {
   auto returnValue = generate<ast::Expression>(exit.expression);
   if(returnValue)
      return std::move(returnValue);

   pop("rdi"); // store return value
   mov("rax", "1 | 0x2000000"); // exit syscall number
   m_output += "\tsyscall\n";

   return std::nullopt;
}

// expressions

template<>
std::optional<std::string> Generator::generate<ast::IntegerLiteral>(const ast::IntegerLiteral& integerLiteral) {
   push(integerLiteral.to_string());

   return std::nullopt;
}

template<>
std::optional<std::string> Generator::generate<ast::Identifier>(const ast::Identifier& identifier) {
   if(!m_symbolTable.contains(identifier.name))
      return std::format("Use of undeclared identifier '{}'!", identifier.name);

   push(std::format("qword [rbp - {}]", m_symbolTable.at(identifier.name) * 8));

   return std::nullopt;
}
