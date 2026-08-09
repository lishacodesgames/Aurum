#include <pch/Precompiled.h>
#include "Parser.h"

std::expected<std::unique_ptr<ast::Node>, std::string> Parser::parse() {
   // while(peek().has_value()) { // idk if this is needed
      if(peek()->type == TokenType::EXIT) {
         consume();
         return parseExit();
      } else {
         return std::unexpected("Unknown root node!");
      }
   // }
}

std::optional<Token> Parser::peek(int offset) {
   if(m_pos + offset >= m_tokens.size())
      return std::nullopt;
   else
      return m_tokens.at(m_pos + offset);
}

Token Parser::consume(uint32_t count) {
   if(!peek())
      throw std::runtime_error("Tried to consume out-of-bounds Token!");

   Token current = m_tokens.at(m_pos);
   m_pos += count;

   return current;
}

std::expected<std::unique_ptr<ast::Exit>, std::string> Parser::parseExit() {
   auto expression = parseExpression(); 
   if(!expression)
      return std::unexpected(expression.error());
   if(!peek() || *peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   return std::make_unique<ast::Exit>(std::move(*expression));
}

std::expected<std::unique_ptr<ast::Expression>, std::string> Parser::parseExpression() {
   if(!peek())
      return std::unexpected("Expected an expression!");

   if(peek()->type == TokenType::INTEGER_LITERAL)
      return std::make_unique<ast::IntegerLiteral>(consume().type);
   else if(peek()->type == TokenType::IDENTIFIER)
      return std::make_unique<ast::Identifier>(consume().type);
   else
      return std::unexpected(std::format("Unknown expression type: '{}'!", consume().to_string()));
}
