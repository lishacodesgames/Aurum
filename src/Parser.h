#pragma once
#include "Token.h"
#include "Nodes.h"

class Parser {
public:
   explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

   std::expected<ast::Node, std::string> parse();

private:
   std::vector<Token> m_tokens{};
   size_t m_pos = 0;

private:
   std::optional<Token> peek(int offset = 0);

   /** 
    * @brief increments m_pos but returns current Token
    * @param count by how much to increment m_pos
    * @returns CURRENT Token
    * @throws runtime_error if next Token doesn't exist, so check with peek() before calling
    */ 
   Token consume(uint32_t count = 1);

   /// @todo move to Node classes as static methods
   std::expected<ast::Exit, std::string> parseExit();
   std::expected<ast::Expression, std::string> parseExpression();
};
