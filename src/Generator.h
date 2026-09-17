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
   explicit Generator(ast::Program program);

   /// @return MOVES ir out of generator. MUST NOT use generator after this
   std::string getIR();
   std::vector<ir::Instruction> generate();

private:
   /// Purely for semantic validity, not layout
   struct SymbolInfo {
      bool valueMutable = false;
      bool typeMutable = false;
      DataType type = DataType::NONE;
   };

private:
   const ast::Program m_program;
   std::vector<ir::Instruction> m_instructions;
   std::string m_ir;

   std::vector<std::unordered_map<std::string, SymbolInfo>> m_scopes{};
   std::size_t m_labelCount = 0;

private:
   void emit(OpCode op, std::string_view operand1, std::optional<std::string_view> operand2 = std::nullopt);

   // these three functions count scopes/labels in index order (starting from 0)

   /// @param prefix label name
   /// @return new label as ".prefix_labelCount" (to avoid duplicate label names)
   std::string newLabel(std::string_view prefix);
   void pushScope();
   void popScope();

   /// check each scope starting from latest for name
   bool isDeclared(const std::string& name) const;

   /// @return ptr to symbol info or nullptr if it doesn't exist
   SymbolInfo* findSymbol(const std::string& name);
   const SymbolInfo* findSymbol(const std::string& name) const;

   /// @retval folded string: ONLY for leaf expressions (literal/identifier)
   /// @retval nullopt: for compound expressions (negative/binary)
   std::optional<std::string> tryFold(const ast::Expression& expr) const;
   std::optional<DataType> inferType(const ast::Expression& expr) const;
   bool isBinaryExprValid(const ast::BinaryExpr* binaryExpr) const;

   void error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal = false) const;

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
   template<> void generate(const ast::If* ifStmt);

   // -- expressions --
   template<> void generate(const ast::Literal* literal);
   template<> void generate(const ast::Identifier* identifier);
   template<> void generate(const ast::UnaryExpr* unaryExpr);
   template<> void generate(const ast::BinaryExpr* binaryExpr);

   // -- variant's overload
   template<ast::VariantNode V>
   void generate(const V& varNode) {
      return std::visit([this](auto&& arg) -> void {
         using PtrT = std::decay_t<decltype(arg)>;
         if constexpr(!std::is_same_v<PtrT, std::monostate>) {
            using T = std::remove_pointer_t<PtrT>;
            generate<T>(arg);
            return;
         }

         error(err::Category::INTERNAL, { "Generator.h", __LINE__ }, "Tried to call generate on monostate!", true);
      }, varNode);
   }
};
