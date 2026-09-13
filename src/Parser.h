#pragma once
#include "Token.h"
#include "ast.h"
#include "ArenaAllocator.h"

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
   ArenaAllocator m_arena;

private:
   void error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal = false);
   void recover();

   Token peek(int offset = 0) const noexcept; // exit(1) doesn't count as an exception

   /** 
    * @brief increments m_pos but returns current Token
    * @param count by how much to increment m_pos
    * @returns CURRENT Token
    */ 
   Token consume(std::uint32_t count = 1) noexcept;

   /**
    * @brief confirms whether the next token is 'valid'. If yes, then it consumes it. Otherwise logs error (if given)
    * @note next token is 'valid' if it is type and has value if hasValue is set
    * @param error if nullopt, error isn't reported. Else, it should contain `category, location, msg, isFatal`. Rest all will be overridden by error() helper method
    * @param hasValue whether not having value contributes to next token being 'valid'
    * @return if next token is valid, returns the token, else nullopt
    */
   [[nodiscard]] std::optional<Token> tryConsume(TokenType type, std::optional<Error> error = std::nullopt, bool hasValue = false);

private:
   template<ast::AstNode T>
   T* parse();

   // by value because there's no circular dependencies here and we don't want them in the arena

   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   ast::Statement parseStatement();

   template<> ast::Declaration* parse<ast::Declaration>();
   template<> ast::Assignment* parse<ast::Assignment>();
   template<> ast::Exit* parse<ast::Exit>();
   template<> ast::Increment* parse<ast::Increment>();
   template<> ast::Decrement* parse<ast::Decrement>();
   template<> ast::Block* parse<ast::Block>();

   // expressions
   /// @param minPrec the minimum precedence that has to be parsed from the expression
   ast::Expression parseExpression(int minPrec = 0);

   template<> ast::Literal* parse<ast::Literal>();
   template<> ast::Identifier* parse<ast::Identifier>();
   template<> ast::Negative* parse<ast::Negative>();

   /// @return BinaryExpr containing op and rhs, left MUST be assigned by caller
   template<> ast::BinaryExpr* parse<ast::BinaryExpr>();

   // helpers
   ast::Expression parseTerm();
};
