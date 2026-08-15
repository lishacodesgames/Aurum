#pragma once
#include "ast.h"

class Generator {
public:
   explicit Generator(ast::Program program): m_program(std::move(program))
      { m_output.reserve(4096); } // to avoid constant reallocation as it grows

   std::expected<std::string, std::string> generate();

private:
   const ast::Program m_program;

   std::string m_output;
   uint32_t m_stackSize = 0;

   /// Key Type: string, name of the variable
   /// Value Type: uint32_t, offset from rbp
   std::unordered_map<std::string, uint32_t> m_symbolTable;

private:
   /// Increases stack size by 1 and pushes value to the top
   void push(std::string_view value, std::optional<std::string> comment = std::nullopt);

   /// COPIES value to reg
   void mov(std::string reg, std::string value, std::optional<std::string> comment = std::nullopt);

   /// REMOVES the value from the top of the stack
   void pop(std::string_view reg, std::optional<std::string> comment = std::nullopt);

private:
   /// @return nullopt if everything went well. string if error (containing error info)
   template<AstNode T>
   [[nodiscard]] std::optional<std::string> generate(const T&);

   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   template<> std::optional<std::string> generate<ast::Declaration>(const ast::Declaration& declaration);
   template<> std::optional<std::string> generate<ast::Exit>(const ast::Exit& exit);

   // expression

   /// @brief pushes the literal on top of the stack
   /// @param integerLiteral must be popped off the stack and used
   template<> std::optional<std::string> generate<ast::IntegerLiteral>(const ast::IntegerLiteral& integerLiteral);

   /// @brief assigns m_symbolTable[name] <- m_stackSize (latest pushed value)
   /// @throws runtime_error if symbol already exists in m_symbolTable
   /// @note MUST call expression first
   template<> std::optional<std::string> generate<ast::Identifier>(const ast::Identifier& identifier);

   // Variant overload
   template<VariantNode V>
   [[nodiscard]] std::optional<std::string> generate(const V& variant) {
      return std::visit([this](auto&& arg) -> std::optional<std::string> {
         using T = std::decay_t<decltype(arg)>; // decay_t removes && rvalue reference, const, etc. Only returns pure type
         if constexpr(!std::is_same_v<T, std::monostate>)
            return generate<T>(arg);

         return "Tried to call generate on monostate!";
      }, variant);
   }
};
