#pragma once
#include "Token.h"
#include "ast.h"
#include "mem.h"

// Terms are either numbers or parenthesized expressions.
// Expressions consist of terms connected by binary operators.
// Note how these two terms are mutually dependent
// For precedence climbing I used: https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

class Parser {
public:
   explicit Parser(std::vector<Token> tokens)
      : m_tokens(std::move(tokens)), m_arena(4 * 1024 * 1024) {} // 4MB stack frame (for now)

   ~Parser() { m_arena.reset(); }

   ast::Program parse();

private:
   std::vector<Token> m_tokens{};
   std::size_t m_pos = 0;
   mem::ArenaAllocator m_arena;

private:
   Token peek(int offset = 0) const noexcept; // exit(1) doesn't count as an exception

   /** 
    * @brief increments m_pos but returns current Token
    * @param count by how much to increment m_pos
    * @returns CURRENT Token
    */ 
   Token consume(std::uint32_t count = 1) noexcept;

   /**
    * @brief tries to consume type. If not, the throws errMsg.
    * @param errMsg if nullopt, throws "Expected `TYPE`!"
    * @param hasValue whether not having value should also throw
    * @return consumed token
    */
   Token tryConsume(TokenType type, std::optional<std::string_view> errMsg, Category errCategory, bool hasValue = false);

   /**
    * @brief confirms whether the next token is type. If yes, then it consumes it. Otherwise does nothing
    * @param hasValue whether not having value contributes to next token being valid
    * @return if next token is valid (is type and/or hasValue), returns true. Else false
    */
   std::optional<Token> tryConsume(TokenType type, bool hasValue = false);

private:
   template<ast::AstNode T>
   T* parse();

   // by value because there's no circular dependencies here and we don't want them in the arena

   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   ast::Statement parseStatement();

   template<> ast::Declaration* parse();
   template<> ast::Assignment* parse();
   template<> ast::Exit* parse();
   template<> ast::Increment* parse();
   template<> ast::Decrement* parse();
   template<> ast::Block* parse();

   // expressions
   /// @param minPrec the minimum precedence that has to be parsed from the expression
   ast::Expression parseExpression(int minPrec = 0);

   template<> ast::IntegerLiteral* parse();
   template<> ast::Identifier* parse();
   template<> ast::Negative* parse();

   /// @return BinaryExpr containing op and rhs, left will be assigned by caller
   template<> ast::BinaryExpr* parse();

   // helpers
   ast::Expression parseTerm();
};
