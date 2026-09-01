#pragma once
#include "IR.h"
#include "ast.h"

/**
 * Lowers AST -> flat IR. Purely structural: no stack offsets, no asm text.
 * Still needs a scope-aware symbol table, but ONLY for validity checks
 * (redeclaration / undeclared / mutability) -- NOT for memory layout.
 * Layout is the AsmEmitter's job now, since it's a sequential concern that
 * only makes sense once the tree has been flattened.
 */
class Generator {
public:
   explicit Generator(ast::Program program) : m_program(std::move(program)) {}

   std::expected<std::vector<ir::Instruction>, std::string> generate();
   std::string getIR() const;

private:
   const ast::Program m_program;
   std::vector<ir::Instruction> m_instructions;

   /// name -> isMutable, per scope. Purely for semantic validity, not layout
   /// start with 1 empty global scope
   std::vector<std::unordered_map<std::string, bool>> m_scopes{{}};

private:
   void emit(ir::OpCode op, std::optional<std::string_view> operand1 = std::nullopt, std::optional<std::string_view> operand2 = std::nullopt);
   
   /// add an empty map to m_scopes
   void pushScope() {
      m_scopes.emplace_back();
      emit(ir::OpCode::SCOPE_START);
   }

   /// pop latest scope
   void popScope() {
      m_scopes.pop_back();
      emit(ir::OpCode::SCOPE_END);
   }

   bool isDeclared(const std::string& name) const; /// check each scope starting from latest for identifier

   /// @retval TRUE: if found and mutable
   /// @retval FALSE: if found but not mutable
   /// @retval NULLOPT: if name not found
   std::optional<bool> findMutability(const std::string& name) const;

   /// @retval folded string: ONLY for leaf expressions (literal/identifier)
   /// @retval nullopt: for compound expressions (negative/binary)
   std::optional<std::string> tryFold(const ast::Expression* expr) const;

private:
   /// @retval error striing if falied
   /// @retval nullopt if everything went well
   template<ast::AstNode T>
   [[nodiscard]] std::optional<std::string> generate(const T*);

   // -- statements --
   template<> std::optional<std::string> generate(const ast::Declaration* declaration);
   template<> std::optional<std::string> generate(const ast::Assignment* assignment);
   template<> std::optional<std::string> generate(const ast::Exit* exit);
   template<> std::optional<std::string> generate(const ast::Increment* increment);
   template<> std::optional<std::string> generate(const ast::Decrement* decrement);
   template<> std::optional<std::string> generate(const ast::Block* block);

   // -- expressions --
   template<> std::optional<std::string> generate(const ast::IntegerLiteral* integerLiteral);
   template<> std::optional<std::string> generate(const ast::Identifier* identifier);
   template<> std::optional<std::string> generate(const ast::Negative* negative);
   template<> std::optional<std::string> generate(const ast::BinaryExpr* binaryExpr);

   // -- variant's overload
   template<ast::VariantNode V>
   [[nodiscard]] std::optional<std::string> generate(const V* varNode) {
      return std::visit([this](auto&& arg) -> std::optional<std::string> {
         using PtrT = std::decay_t<decltype(arg)>;
         if constexpr(!std::is_same_v<PtrT, std::monostate>) {
            using T = std::remove_pointer_t<PtrT>;
            return generate<T>(arg);
         }

         return "Tried to call generate on monostate!";
      }, *varNode);
   }
};
