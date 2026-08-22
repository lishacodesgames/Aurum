#pragma once
#include "ast.h"

class Generator {
public:
   explicit Generator(ast::Program program): m_program(std::move(program))
      { m_output.reserve(4096); } // to avoid constant reallocation as it grows

   std::expected<std::string, std::string> generate();
   std::vector<std::string> getRequiredLibs() const;

private:
   const ast::Program m_program;

   std::string m_output;
   uint32_t m_stackSize = 0;

   /// Key Type: string, name of the variable
   /// Value Type: uint32_t, offset from stack
   std::unordered_map<std::string, uint32_t> m_symbolTable;
   std::set<std::string> m_requiredExterns; // required pre-built asm functions our program actually uses

private:
   /// @note std::format might throw exception, so these 3 functions cannot be noexcept

   /// writes comment on its own line
   void comment(std::string_view comment);

   /// writes command into m_output with proper formatting
   /// @param comment WITH preceeding ;
   void write(std::string_view cmd, std::optional<std::string_view> comment = std::nullopt);

   /// Increases stack size by 1 and pushes value to the top
   /// @param comment WITH preceeding ;
   void push(std::string_view value, std::optional<std::string_view> comment = std::nullopt);

   /// REMOVES the value from the top of the stack and decreases stack size by 1
   /// @param comment WITH preceeding ;
   void pop(std::string_view reg, std::optional<std::string_view> comment = std::nullopt);

private:
   /// @return nullopt if everything went well. string if error (containing error info)
   template<AstNode T>
   [[nodiscard]] std::optional<std::string> generate(const T*);

   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   template<> std::optional<std::string> generate(const ast::Declaration* declaration);
   template<> std::optional<std::string> generate(const ast::Exit* exit);

   // expression

   /// @brief pushes the literal on top of the stack
   /// @param integerLiteral must be popped off the stack and used
   template<> std::optional<std::string> generate(const ast::IntegerLiteral* integerLiteral);

   /// @brief assigns m_symbolTable[name] <- m_stackSize (latest pushed value)
   /// @throws runtime_error if symbol already exists in m_symbolTable
   /// @note MUST call expression first
   template<> std::optional<std::string> generate(const ast::Identifier* identifier);

   template<> std::optional<std::string> generate(const ast::Negative* negative);
   template<> std::optional<std::string> generate(const ast::BinaryExpr* binaryExpr);

private:
   // Variant overload
   template<VariantNode V>
   [[nodiscard]] std::optional<std::string> generate(const V* variant) {
      return std::visit([this](auto&& arg) -> std::optional<std::string> {
         using PtrT = std::decay_t<decltype(arg)>;              // e.g. Declaration*
         if constexpr(!std::is_same_v<PtrT, std::monostate>) {
            using T = std::remove_pointer_t<PtrT>;               // Declaration
            return generate<T>(arg);
         }

         return "Tried to call generate on monostate!";
      }, *variant);
   }
};
