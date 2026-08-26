#pragma once
#include "ast.h"
#include "Stack.h"

class Generator {
public:
   explicit Generator(ast::Program program): m_program(std::move(program))
      { m_output.reserve(4096); } // to avoid constant reallocation as it grows

   std::expected<std::string, std::string> generate();
   std::vector<std::string> getRequiredLibs() const;

private:
   const ast::Program m_program;

   std::string m_output;
   Stack m_stack;

   std::set<std::string> m_requiredExterns; // required pre-built asm functions our program actually uses

private:
   /// @note std::format might throw exception, so these 3 functions cannot be noexcept

   /// writes comment on its own line
   void comment(std::string_view comment);

   /// writes command into m_output with proper formatting
   /// @param comment WITH preceeding ;
   void write(std::string_view cmd);

private:

   /// @return nullopt if everything went well. string if error (containing error info)
   template<ast::AstNode T>
   [[nodiscard]] std::optional<std::string> generate(const T*);

   // --- statements ---

   template<> std::optional<std::string> generate(const ast::Declaration* declaration);
   template<> std::optional<std::string> generate(const ast::Exit* exit);
   template<> std::optional<std::string> generate(const ast::Increment* increment);
   template<> std::optional<std::string> generate(const ast::Decrement* decrement);

   // --- expression ---

   template<> std::optional<std::string> generate(const ast::IntegerLiteral* integerLiteral);
   template<> std::optional<std::string> generate(const ast::Identifier* identifier);
   template<> std::optional<std::string> generate(const ast::Negative* negative);
   template<> std::optional<std::string> generate(const ast::BinaryExpr* binaryExpr);

   // --- variant's overload ---

   /// @return nullopt if everything went well. string if error (containing error info)
   template<ast::VariantNode V>
   [[nodiscard]] std::optional<std::string> generate(const V* varNode) {
      return std::visit([this](auto&& arg) -> std::optional<std::string> {
         using PtrT = std::decay_t<decltype(arg)>;
         if constexpr(!std::is_same_v<PtrT, std::monostate>) {
            using T = std::remove_pointer_t<PtrT>;
            return generate<T>(arg);
         }

         return "Tried to call generate on monstate!";
      }, *varNode);
   }
};
