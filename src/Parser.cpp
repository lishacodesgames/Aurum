#include <pch/Precompiled.h>
#include "Parser.h"

std::expected<ast::Program, std::string> Parser::parse() {
   ast::Program program;
   while(peek() != TokenType::END_OF_FILE) {
      auto statement = parseStatement();
      if(!statement)
         return std::unexpected(statement.error());

      /// @todo error save system that
      /// 1. confirms ; at the end of each statement (don't check inside parseStatement, do it here)
      /// 2. if ANY error occurs ANYWHERE, don't return, but LOG it or save the string error wtv
      /// 3. consume till the next ; and then begin parsing the next statement

      program.push_back(std::move(*statement));
   }

   if(program.empty())
      return std::unexpected("Program parsed to be empty!");

   return program;
}

Token Parser::peek(int offset) const noexcept {
   if(m_pos + offset >= m_tokens.size())
      LOG_ERROR("Parser tried to access outside tokens!");

   return m_tokens.at(m_pos + offset);
}

Token Parser::consume(std::uint32_t count) noexcept {
   Token current = m_tokens.at(m_pos);
   m_pos += count;

   return current;
}

// PARSE OVERLOADS

#pragma region Statements

std::expected<ast::Statement, std::string> Parser::parseStatement() {
   switch(peek().type) {
      case TokenType::BAR:
      case TokenType::MINT: {
         auto declaration = parse<ast::Declaration>();
         if(!declaration)
            return std::unexpected(declaration.error());

         // must explicitly construct Declaration in Statement bcz 1 implicit conversion to expected<> already happening
         return ast::Statement(std::in_place_type<ast::Declaration*>, declaration.value());
      }

      case TokenType::EXIT: {
         auto exit = parse<ast::Exit>();
         if(!exit)
            return std::unexpected(exit.error());

         return ast::Statement(std::in_place_type<ast::Exit*>, exit.value());
      }

      case TokenType::IDENTIFIER: {
         switch(peek(1).type) {
            case TokenType::INCREMENT: {
               auto increment = parse<ast::Increment>();
               if(!increment)
                  return std::unexpected(increment.error());

               return ast::Statement(std::in_place_type<ast::Increment*>, increment.value());
            }

            case TokenType::DECREMENT: {
               auto decrement = parse<ast::Decrement>();
               if(!decrement)
                  return std::unexpected(decrement.error());

               return ast::Statement(std::in_place_type<ast::Decrement*>, decrement.value());
            }

            case TokenType::EQUALS: {
               auto assignment = parse<ast::Assignment>();
               if(!assignment)
                  return std::unexpected(assignment.error());

               return ast::Statement(std::in_place_type<ast::Assignment*>, assignment.value());
            }

            default:
               return std::unexpected("Expected a unary postfix operator! Got: " + to_string(peek(1).type));
         }
      }
      
      case TokenType::OPEN_CURLY: {
         auto block = parse<ast::Block>();
         if(!block)
            return std::unexpected(block.error());

         return ast::Statement(std::in_place_type<ast::Block*>, block.value());
      }

      default:
         return std::unexpected("Unexpected token, unable to parse statement beginning with: " + to_string(consume().type));
   }
}

template<>
std::expected<ast::Declaration*, std::string> Parser::parse<ast::Declaration>() {
   bool isMutable = consume().type == TokenType::BAR;

   auto identifier = parse<ast::Identifier>();
   if(!identifier)
      return std::unexpected(identifier.error());

   ast::Expression* expression = nullptr; // in case it's a Declaration without Definition

   if(peek() == TokenType::EQUALS) {
      consume();

      auto expr = parseExpression();
      if(!expr)
         return std::unexpected(expr.error());

      expression = m_arena.create<ast::Expression>(std::move(expr.value()));
   }
   
   if(peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   consume();

   if(expression)
      return m_arena.create<ast::Declaration>(*identifier, expression, isMutable);
   else
      return m_arena.create<ast::Declaration>(*identifier, isMutable);
}

template<>
std::expected<ast::Assignment*, std::string> Parser::parse<ast::Assignment>() {
   auto identifier = parse<ast::Identifier>();
   if(!identifier)
      return std::unexpected(identifier.error());

   consume(); // consume equals

   auto expression = parseExpression();
   if(!expression)
      return std::unexpected(expression.error());

   /// @todo extract
   if(peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`");

   consume();

   return m_arena.create<ast::Assignment>(*identifier, m_arena.create<ast::Expression>(std::move(expression.value())));
}

template<>
std::expected<ast::Exit*, std::string> Parser::parse<ast::Exit>() {
   consume(); // consume exit

   auto expression = parseExpression();
   if(!expression)
      return std::unexpected(expression.error());

   if(peek() != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   consume();

   return m_arena.create<ast::Exit>(m_arena.create<ast::Expression>(std::move(expression.value())));
}

template<>
std::expected<ast::Increment*, std::string> Parser::parse<ast::Increment>() {
   auto identifier = parse<ast::Identifier>();
   if(!identifier)
      std::unexpected(identifier.error());

   if(peek(1) != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   consume(2); // consume ++;
   return m_arena.create<ast::Increment>(std::move(identifier.value()));
}

template<>
std::expected<ast::Decrement*, std::string> Parser::parse<ast::Decrement>() {
   auto identifier = parse<ast::Identifier>();
   if(!identifier)
      return std::unexpected(identifier.error());

   if(peek(1) != TokenType::SEMICOLON)
      return std::unexpected("Expected `;`!");

   consume(2); // consume --;
   
   /// @todo do I need to std::move a pointer?
   return m_arena.create<ast::Decrement>(std::move(identifier.value()));
}

template<>
std::expected<ast::Block*, std::string> Parser::parse<ast::Block>() {
   std::vector<ast::Statement> stmts;
   consume(); // consume open_curly

   while(peek() != TokenType::CLOSE_CURLY) {
      /// @todo check for unexpected end of file?

      auto statement = parseStatement();
      if(!statement)
         return std::unexpected(statement.error());

      stmts.push_back(*statement);
   }

   consume(); // consume close_curly
   return m_arena.create<ast::Block>(std::move(stmts));
}

#pragma endregion

#pragma region Expressions

std::expected<ast::Expression, std::string> Parser::parseTerm() {
   switch(peek().type) {
      case TokenType::INTEGER_LITERAL: {
         auto integerLiteral = parse<ast::IntegerLiteral>();
         if(!integerLiteral)
            return std::unexpected(integerLiteral.error());

         return ast::Expression(std::in_place_type<ast::IntegerLiteral*>, integerLiteral.value());
      }

      case TokenType::IDENTIFIER: {
         auto identifier = parse<ast::Identifier>();
         if(!identifier)
            return std::unexpected(identifier.error());

         return ast::Expression(std::in_place_type<ast::Identifier*>, identifier.value());
      }

      case TokenType::MINUS: {
         auto negation = parse<ast::Negative>(); // recursion
         if(!negation)
            return std::unexpected(negation.error());

         return ast::Expression(std::in_place_type<ast::Negative*>, negation.value());
      }

      case TokenType::PLUS: {
         consume();
         return parseTerm();
      }

      case TokenType::OPEN_PAREN: {
         consume();
         auto expression = parseExpression();

         if(peek() != TokenType::CLOSE_PAREN)
            return std::unexpected("Expected `)`!");

         consume();
         return expression; // same return type so we don't need to unwrap and rewrap
      }

      default:
         return std::unexpected(std::format("Expected an Term. Got: '{}'!", to_string(consume().type)));
   }
}

std::expected<ast::Expression, std::string> Parser::parseExpression(int minPrec) {
   auto expression = parseTerm();
   if(!expression)
      return expression;

   while(isBinaryOperator(peek().type) && getPrecedence(peek().type) >= minPrec) {
      auto binaryExpr = parse<ast::BinaryExpr>();
      if(!binaryExpr)
         return std::unexpected(binaryExpr.error());

      (*binaryExpr)->left = m_arena.create<ast::Expression>(*expression);
      expression = ast::Expression(std::in_place_type<ast::BinaryExpr*>, binaryExpr.value());
   }

   return expression;
}

template<>
std::expected<ast::IntegerLiteral*, std::string> Parser::parse<ast::IntegerLiteral>() {
   if(peek() != TokenType::INTEGER_LITERAL || !peek().value)
      return std::unexpected("Expected an integer literal!");

   return m_arena.create<ast::IntegerLiteral>(*consume().value);
}

template<>
std::expected<ast::Identifier*, std::string> Parser::parse<ast::Identifier>() {
   if(peek() != TokenType::IDENTIFIER || !peek().value)
      return std::unexpected("Expected an identifier!");

   return m_arena.create<ast::Identifier>(*consume().value);
}

template<>
std::expected<ast::Negative*, std::string> Parser::parse<ast::Negative>() {
   consume(); // consume minus

   auto expression = parseTerm(); // recursion
   if(!expression)
      return std::unexpected(expression.error());

   return m_arena.create<ast::Negative>(m_arena.create<ast::Expression>(std::move(expression.value())));
}

template<>
std::expected<ast::BinaryExpr*, std::string> Parser::parse<ast::BinaryExpr>() {
   TokenType op = consume().type;
   int precedence = getPrecedence(op);
   int nextMinPrec = isLeftAssociative(op) ? precedence + 1 : precedence;

   auto rhs = parseExpression(nextMinPrec);
   if(!rhs)
      return std::unexpected(rhs.error());

   return m_arena.create<ast::BinaryExpr>(nullptr, op, m_arena.create<ast::Expression>(rhs.value()));
}

#pragma endregion
