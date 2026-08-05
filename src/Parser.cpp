#include <pch/Precompiled.h>
#include "Parser.h"

std::expected<ast::Node, std::string> Parser::parse() {
   // while(peek().has_value()) { // idk if this is needed
      if(peek()->type == TokenType::EXIT) {
         consume();
         return parseExit();
      } else {
         return std::unexpected("Unknown root node!");
      }
   // }
}

std::optional<Token> Parser::peek(int ahead) {
   if(m_pos + ahead >= m_tokens.size())
      return std::nullopt;
   else
      return m_tokens.at(m_pos + ahead);
}

Token Parser::consume() {
   if(!peek())
      throw std::runtime_error("Tried to consume out-of-bounds Token!");

   return m_tokens.at(m_pos++);
}

std::expected<ast::Exit, std::string> Parser::parseExit() {
   auto expression = parseExpression(); 
   if(expression.has_value()) {
      return ast::Exit{ .expression = *expression };
   } else {
      return std::unexpected("Failed to parse Exit node!");
   }
}

std::expected<ast::Expression, std::string> Parser::parseExpression() {
   if(peek().has_value() && peek()->type == TokenType::INTEGER_LITERAL) {
      return ast::Expression{ .integerLiteral = consume() };
   } else {
      return std::unexpected("Failed to parse Expression node!");
   }
}
