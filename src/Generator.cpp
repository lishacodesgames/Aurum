#include <pch/Precompiled.h>
#include "Generator.h"

std::string Generator::generate() {
   m_output += "; macOS x86_64, NASM syntax\n\n";
   m_output += "global _main\n";
   m_output += "_main:\n";

   for(const ast::Statement& stmt : m_program.statements)
      generate<ast::Statement>(stmt);

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
inline void Generator::generate<ast::Declaration>(const ast::Declaration& declaration) {
   if(m_symbolTable.contains(declaration.identifier.name))
      throw std::runtime_error(std::format("Redeclaration of identifier '{}'!", declaration.identifier.name));

   generate<ast::Expression>(declaration.expression);
   generate<ast::Identifier>(declaration.identifier);
}

/// @todo Expression
template<>
void Generator::generate<ast::Exit>(const ast::Exit& exit) {
   generate<ast::Expression>(exit.expression);

   pop("rdi"); // store return value
   mov("rax", "1 | 0x2000000"); // exit syscall number
   m_output += "\tsyscall\n";
}

// expressions

template<>
void Generator::generate<ast::IntegerLiteral>(const ast::IntegerLiteral& integerLiteral) {
   push(integerLiteral.value);
}

template<>
void Generator::generate<ast::Identifier>(const ast::Identifier& identifier) {
   m_symbolTable[identifier.name] = m_stackSize;
}
