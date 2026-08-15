#pragma once
#include "ast.h"

class Generator {
public:
   explicit Generator(ast::Program program) : m_program(std::move(program))
      { m_output.reserve(4096); } // to avoid constant reallocation as it grows

   std::string generate();

private:
   const ast::Program m_program;

   std::string m_output;
   uint32_t m_stackSize = 0;

   /// Key Type: string, name of the variable
   /// Value Type: uint32_t, offset from rbp
   std::unordered_map<std::string, uint32_t> m_symbolTable;

private:
   /// Increases stack size by 1 and pushes value to the top
   void push(std::string_view value);

   /// COPIES value to reg
   void mov(std::string reg, std::string value);

   /// REMOVES the value from the top of the stack
   void pop(std::string_view reg);

private:
   template<AstNode T>
   void generate(const T&);

   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   template<> void generate<ast::Declaration>(const ast::Declaration& declaration);
   template<> void generate<ast::Exit>(const ast::Exit& exit);

   // expression

   /// @brief pushes the literal on top of the stack
   /// @param integerLiteral must be popped off the stack and used
   template<> void generate<ast::IntegerLiteral>(const ast::IntegerLiteral& integerLiteral);

   /// @brief assigns m_symbolTable[name] <- m_stackSize (latest pushed value)
   /// @throws runtime_error if symbol already exists in m_symbolTable
   /// @note MUST call expression first
   template<> void generate<ast::Identifier>(const ast::Identifier& identifier);

   // Variant overload
   template<VariantNode V>
   void generate(const V& variant) {
      std::visit([this](auto&& arg) {
         using T = std::decay_t<decltype(arg)>; // decay_t removes && rvalue reference, const, etc. Only returns pure type
         if constexpr(!std::is_same_v<T, std::monostate>)
            generate<T>(arg);
      }, variant);
   }
};
