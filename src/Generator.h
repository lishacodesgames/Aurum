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
   explicit Generator(ast::Program program)
      : m_program(std::move(program)) {}

   std::vector<ir::Instruction> generate();
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
   void generate(const T*);

   // -- statements --
   template<> void generate(const ast::Declaration* declaration);
   template<> void generate(const ast::Assignment* assignment);
   template<> void generate(const ast::Exit* exit);
   template<> void generate(const ast::Increment* increment);
   template<> void generate(const ast::Decrement* decrement);
   template<> void generate(const ast::Block* block);

   // -- expressions --
   template<> void generate(const ast::IntegerLiteral* integerLiteral);
   template<> void generate(const ast::Identifier* identifier);
   template<> void generate(const ast::Negative* negative);
   template<> void generate(const ast::BinaryExpr* binaryExpr);

   // -- variant's overload
   template<ast::VariantNode V>
   void generate(const V* varNode) {
      return std::visit([this](auto&& arg) -> void {
         using PtrT = std::decay_t<decltype(arg)>;
         if constexpr(!std::is_same_v<PtrT, std::monostate>) {
            using T = std::remove_pointer_t<PtrT>;
            generate<T>(arg);
         } else {
            g_errors.report(Phase::GENERATING, Category::INTERNAL,
               { "Generator.h" }, "Tried to call generate on monostate!", true);
         }
      }, *varNode);
   }
};
