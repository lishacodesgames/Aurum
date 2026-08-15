#include <pch/Precompiled.h>
#include "Parser.h"

std::expected<ast::Program, std::string> Parser::parse() {
   if(peek() == TokenType::END_OF_FILE)
      return std::unexpected("Unexpected end of file!");

   ast::Program program;
   while(peek() != TokenType::END_OF_FILE) {
      auto statement = parse<ast::Statement>();
      if(!statement)
         return std::unexpected(statement.error());

      program.push_back(std::move(*statement));
   }

   if(program.empty())
      return std::unexpected("Program parsed to be empty!");

   return program;
}

Token Parser::peek(int offset) {
   if(m_pos + offset >= m_tokens.size())
      FATAL_ERROR("Parser tried to access outside tokens!");

   return m_tokens.at(m_pos + offset);
}

Token Parser::consume(uint32_t count) {
   Token current = m_tokens.at(m_pos);
   m_pos += count;

   return current;
}

// PARSE OVERLOADS

// statements

template<>
std::expected<ast::Statement, std::string> Parser::parse<ast::Statement>() {
   switch(peek().type) {
      case TokenType::MINT: {
         auto declaration = parse<ast::Declaration>();
         if(!declaration)
            return std::unexpected(declaration.error());

         // must explicitly construct Declaration in Statement bcz 1 implicit conversion to expected<> already happening
         return ast::Statement(std::in_place_type<ast::Declaration*>, *declaration);
      }

      case TokenType::EXIT: {
         auto exit = parse<ast::Exit>();
         if(!exit)
            return std::unexpected(exit.error());

         return ast::Statement(std::in_place_type<ast::Exit*>, *exit);
      }

      default:
         return std::unexpected(std::format("Expected statement! Got: {}", to_string(consume().type)));
   }
}

template<>
std::expected<ast::Declaration*, std::string> Parser::parse<ast::Declaration>() {
   consume(); // consume keyword

   auto identifier = parse<ast::Identifier>();
   if(!identifier)
      return std::unexpected(identifier.error());

   std::optional<ast::Expression*> expression; // in case it's a Declaration without Definition

   if(peek() == TokenType::EQUALS) {
      consume();

      auto expr = parse<ast::Expression>();
      if(!expr)
         return std::unexpected(expr.error());

      expression = m_arena.create<ast::Expression>(std::move(*expr));
   }
   
   if(peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   consume();

   if(expression)
      return m_arena.create<ast::Declaration>(*identifier, *expression);
   else
      return m_arena.create<ast::Declaration>(*identifier);
}

template<>
std::expected<ast::Exit*, std::string> Parser::parse<ast::Exit>() {
   consume(); // consume exit

   auto expression = parse<ast::Expression>();
   if(!expression)
      return std::unexpected(expression.error());

   if(peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");
   else
      consume();

   return m_arena.create<ast::Exit>(m_arena.create<ast::Expression>(std::move(*expression)));
}

// expressions

template<>
std::expected<ast::Expression, std::string> Parser::parse<ast::Expression>() {
   switch(peek().type) {
      case TokenType::INTEGER_LITERAL: {
         auto integerLiteral = parse<ast::IntegerLiteral>();
         if(!integerLiteral)
            return std::unexpected(integerLiteral.error());

         return ast::Expression(std::in_place_type<ast::IntegerLiteral*>, *integerLiteral);
      }

      case TokenType::IDENTIFIER: {
         auto identifier = parse<ast::Identifier>();
         if(!identifier)
            return std::unexpected(identifier.error());

         return ast::Expression(std::in_place_type<ast::Identifier*>, *identifier);
      }

      case TokenType::MINUS: {
         auto negation = parse<ast::Negative>(); // recursion
         if(!negation)
            return std::unexpected(negation.error());

         return ast::Expression(std::in_place_type<ast::Negative*>, *negation);
      }

      default:
         return std::unexpected(std::format("Expected expression! Got: '{}'!", to_string(consume().type)));
   }
}

template<>
std::expected<ast::IntegerLiteral*, std::string> Parser::parse<ast::IntegerLiteral>() {
   if(peek() != TokenType::INTEGER_LITERAL || !peek().value)
      return std::unexpected("Expected an integer literal!");

   return m_arena.create<ast::IntegerLiteral>(*consume().value);
}

template<>
std::expected<ast::Identifier*, std::string> Parser::parse<ast::Identifier>() {
   /// @todo remember identifiers thru some sort of map maybe
   if(peek() != TokenType::IDENTIFIER || !peek().value)
      return std::unexpected("Expected an identifier!");

   return m_arena.create<ast::Identifier>(*consume().value);
}

template<>
std::expected<ast::Negative*, std::string> Parser::parse<ast::Negative>() {
   consume(); // consume minus

   auto expression = parse<ast::Expression>(); // recursion
   if(!expression)
      return std::unexpected(expression.error());

   return m_arena.create<ast::Negative>(m_arena.create<ast::Expression>(std::move(*expression)));
}
