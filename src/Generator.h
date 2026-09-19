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
   /// Stores both mutabilities and the datatype. Purely for semantic validity, not layout
   struct SymbolInfo {
      bool valueMutable = false;
      bool typeMutable = false;
      DataType type = DataType::NONE;
   };

   struct LoopLabels {
      std::string breakLabel;
      std::string continueLabel;
   };

private:
   const ast::Program m_program;
   std::vector<ir::Instruction> m_instructions;
   std::string m_ir;

   std::vector<std::unordered_map<std::string, SymbolInfo>> m_scopes{};
   std::size_t m_labelCount = 0;
   std::vector<LoopLabels> m_loopStack{}; // for control keywords in nested loops

private:
   void comment(std::string_view comment, bool newLine = true);
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

   void error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal = false) const;

private:
   // --- helpers ---

   /// @retval folded string: ONLY for leaf expressions (literal/identifier)
   /// @retval nullopt: for compound expressions (negative/binary)
   std::optional<std::string> tryFold(const ast::Expression& expr);
   std::optional<DataType> inferType(const ast::Expression& expr) const;

   /// @return NOT string view because that'll leave a dangling pointer if folded
   std::string resolveOperand(const ast::Expression& expr);
   std::optional<DataType> resolveDeclaredType(DataType exprType, const ast::Declaration* declaration);

   /// @param stmt "if"/"while"/"do-while". It's printed in the error msg. It's const char because it's only called with string literals
   /// @return whether expr's inferred type is BOOL
   bool isValidCondition(const ast::Expression& expr, const char* stmt);
   bool isBinaryExprValid(const ast::BinaryExpr* binaryExpr) const;

private:
   template<ast::AstNode T>
   void generate(const T*);

   // -- statements --
   template<> void generate(const ast::Declaration* declaration);
   template<> void generate(const ast::Exit* exit);
   template<> void generate(const ast::Break* brk);
   template<> void generate(const ast::Continue* cnt);
   template<> void generate(const ast::Block* block);
   template<> void generate(const ast::If* ifStmt);
   template<> void generate(const ast::While* whileStmt);
   template<> void generate(const ast::DoWhile* doWhileStmt);

   // -- both --
   template<> void generate(const ast::Assignment* assignment);
   template<> void generate(const ast::Increment* increment);
   template<> void generate(const ast::Decrement* decrement);

   // -- expressions --
   template<> void generate(const ast::Literal* literal);
   template<> void generate(const ast::Identifier* identifier);
   template<> void generate(const ast::UnaryExpr* unaryExpr);
   template<> void generate(const ast::BinaryExpr* binaryExpr);

   // -- variant's overload
   template<ast::VariantNode V>
   void generate(const V& varNode) {
      return std::visit([this](auto&& arg) -> void {
         using T = std::remove_pointer_t<std::decay_t<decltype(arg)>>;
         if constexpr(std::is_same_v<T, std::monostate>)
            assert(false && "Tried to call generate on monostate!");
         else
            generate<T>(arg);
      }, varNode);
   }
};
