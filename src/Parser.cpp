#include <pch/Precompiled.h>
#include "Parser.h"

#define VALIDATE_VARIANT_RETURN_MONO(var) if(std::holds_alternative<std::monostate>((var))) return std::monostate{};
#define VALIDATE_VARIANT_RETURN_NULL(var) if(std::holds_alternative<std::monostate>((var))) return nullptr;

#define VALIDATE_PTR_RETURN_MONO(ptr) if(!(ptr)) return std::monostate{};
#define VALIDATE_PTR_RETURN_NULL(ptr) if(!(ptr)) return nullptr;

ast::Program Parser::parse() {
   ast::Program program;
   while(peek() != TokenType::END_OF_FILE) {
      /// @todo error save system that
      /// 1. confirms ; at the end of each statement (don't check inside parseStatement, do it here)
      /// 2. if ANY error occurs ANYWHERE, don't return, but LOG it or save the string error wtv
      /// 3. consume till the next ; and then begin parsing the next statement

      ast::Statement statement = parseStatement();
      if(std::holds_alternative<std::monostate>(statement)) {
         while(peek() != TokenType::SEMICOLON)
            consume();

         consume(); // consume semicolon
         continue;
      }

      program.push_back(std::move(statement));
   }

   return program;
}

Token Parser::peek(int offset) const noexcept {
   if(m_pos + offset >= m_tokens.size())
      m_reporter.report(Phase::PARSING, Category::INTERNAL, m_tokens.at(m_pos).location, "Parser tried to access outside tokens!");

   return m_tokens[m_pos + offset];
}

Token Parser::consume(std::uint32_t count) noexcept {
   Token current = m_tokens.at(m_pos);
   m_pos += count;

   return current;
}

/// @todo fix both tryConsumes my brain is not working
Token Parser::tryConsume(TokenType type, std::optional<std::string_view> errMsg, Category errCategory, bool hasValue) {
   if(auto token = tryConsume(type, hasValue))
      return *token;

   /// @todo fatal vs non fatal distinction
   if(errMsg)
      m_reporter.report(Phase::PARSING, errCategory, peek().location, *errMsg, true);
   else
      m_reporter.report(Phase::PARSING, errCategory, peek().location, std::format("Expected `{}`!", getCharsOf(type)), true);
}

std::optional<Token> Parser::tryConsume(TokenType type, bool hasValue) {
   if(peek().type != type || (hasValue && !peek().value))
      return std::nullopt;

   return consume();
}

// PARSE OVERLOADS

#pragma region Statements

/// @tod
ast::Statement Parser::parseStatement() {
   switch(peek().type) {
      case TokenType::BAR:
      case TokenType::MINT: {
         ast::Declaration* declaration = parse<ast::Declaration>();
         VALIDATE_PTR_RETURN_MONO(declaration);

         // must explicitly construct Declaration in Statement bcz 1 implicit conversion to expected<> already happening
         return ast::Statement(std::in_place_type<ast::Declaration*>, declaration);
      }

      case TokenType::EXIT: {
         ast::Exit* exit = parse<ast::Exit>();
         VALIDATE_PTR_RETURN_MONO(exit);

         return ast::Statement(std::in_place_type<ast::Exit*>, exit);
      }

      case TokenType::IDENTIFIER: {
         switch(peek(1).type) {
            case TokenType::INCREMENT: {
               ast::Increment* increment = parse<ast::Increment>();
               VALIDATE_PTR_RETURN_MONO(increment);

               return ast::Statement(std::in_place_type<ast::Increment*>, increment);
            }

            case TokenType::DECREMENT: {
               ast::Decrement* decrement = parse<ast::Decrement>();
               VALIDATE_PTR_RETURN_MONO(decrement)

               return ast::Statement(std::in_place_type<ast::Decrement*>, decrement);
            }

            case TokenType::EQUALS: {
               ast::Assignment* assignment = parse<ast::Assignment>();
               VALIDATE_PTR_RETURN_MONO(assignment);

               return ast::Statement(std::in_place_type<ast::Assignment*>, assignment);
            }

            default: {
               m_reporter.report(Phase::PARSING, Category::SYNTAX, consume().location,
                  "Expected a unary postfix operator! Got: " + getCharsOf(peek(1).type));
               return std::monostate{};
            }
         }
      }
      
      case TokenType::OPEN_CURLY: {
         ast::Block* block = parse<ast::Block>();
         VALIDATE_PTR_RETURN_MONO(block);

         return ast::Statement(std::in_place_type<ast::Block*>, block);
      }

      default: {
         m_reporter.report(Phase::PARSING, Category::SYNTAX, consume().location,
            "Unexpected token, unable to parse statement beginning with: " + to_string(consume().type));
         return std::monostate{};
      }
   }
}

template<> ast::Declaration* Parser::parse() {
   bool isMutable = consume().type == TokenType::BAR;

   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   ast::Expression* expression = nullptr; // in case it's a Declaration without Definition

   if(auto next = tryConsume(TokenType::EQUALS)) {
      ast::Expression expr = parseExpression();
      VALIDATE_VARIANT_RETURN_NULL(expr);

      expression = m_arena.create<ast::Expression>(std::move(expr));
   }

   tryConsume(TokenType::SEMICOLON, std::nullopt, Category::SYNTAX);

   if(expression)
      return m_arena.create<ast::Declaration>(identifier, expression, isMutable);
   else
      return m_arena.create<ast::Declaration>(identifier, isMutable);
}

template<>
ast::Assignment* Parser::parse<ast::Assignment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   tryConsume(TokenType::EQUALS, std::nullopt, Category::SYNTAX);

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   tryConsume(TokenType::SEMICOLON, std::nullopt, Category::SYNTAX);
   return m_arena.create<ast::Assignment>(identifier, m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Exit* Parser::parse<ast::Exit>() {
   tryConsume(TokenType::EXIT, std::nullopt, Category::SYNTAX);

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   tryConsume(TokenType::SEMICOLON, std::nullopt, Category::SYNTAX);
   return m_arena.create<ast::Exit>(m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Increment* Parser::parse<ast::Increment>() {
   auto identifier = parse<ast::Identifier>();
   if(!identifier) return nullptr;

   tryConsume(TokenType::INCREMENT, std::nullopt, Category::SYNTAX);
   tryConsume(TokenType::SEMICOLON, std::nullopt, Category::SYNTAX);
   return m_arena.create<ast::Increment>(identifier);
}

template<>
ast::Decrement* Parser::parse<ast::Decrement>() {
   auto identifier = parse<ast::Identifier>();
   if(!identifier) return nullptr;

   tryConsume(TokenType::DECREMENT, std::nullopt, Category::SYNTAX);
   tryConsume(TokenType::SEMICOLON, std::nullopt, Category::SYNTAX);
   return m_arena.create<ast::Decrement>(identifier);
}

template<>
ast::Block* Parser::parse<ast::Block>() {
   std::vector<ast::Statement> stmts;
   tryConsume(TokenType::OPEN_CURLY, std::nullopt, Category::SYNTAX);

   while(tryConsume(TokenType::CLOSE_CURLY) == std::nullopt) {
      ast::Statement statement = parseStatement();
      if(std::holds_alternative<std::monostate>(statement)) return nullptr;

      stmts.push_back(std::move(statement));
   }

   return m_arena.create<ast::Block>(std::move(stmts));
}

#pragma endregion

#pragma region Expressions

ast::Expression Parser::parseTerm() {
   switch(peek().type) {
      case TokenType::INTEGER_LITERAL: {
         auto integerLiteral = parse<ast::IntegerLiteral>();
         if(!integerLiteral) return std::monostate{};

         return ast::Expression(std::in_place_type<ast::IntegerLiteral*>, integerLiteral);
      }

      case TokenType::IDENTIFIER: {
         auto identifier = parse<ast::Identifier>();
         if(!identifier) return std::monostate{};

         return ast::Expression(std::in_place_type<ast::Identifier*>, identifier);
      }

      case TokenType::MINUS: {
         auto negation = parse<ast::Negative>();
         if(!negation) return std::monostate{};

         return ast::Expression(std::in_place_type<ast::Negative*>, negation);
      }

      case TokenType::PLUS: {
         consume();
         return parseTerm();
      }

      case TokenType::OPEN_PAREN: {
         consume();
         ast::Expression expression = parseExpression();

         tryConsume(TokenType::CLOSE_PAREN, std::nullopt, Category::SYNTAX);
         return expression; // same return type so we don't need to unwrap and rewrap
      }

      default:
         m_reporter.report(Phase::PARSING, Category::SYNTAX, consume().location,
            "Unexpected token, unable to parse term beginning with: " + to_string(consume().type));
         return std::monostate{};
   }
}

ast::Expression Parser::parseExpression(int minPrec) {
   ast::Expression expression = parseTerm();

   if(!std::holds_alternative<std::monostate>(expression)) {
      while(isBinaryOperator(peek().type) && getPrecedence(peek().type) >= minPrec) {
         ast::BinaryExpr* binaryExpr = parse<ast::BinaryExpr>();
         if(!binaryExpr) return std::monostate{};
   
         binaryExpr->left = m_arena.create<ast::Expression>(std::move(expression));
         expression = ast::Expression(std::in_place_type<ast::BinaryExpr*>, binaryExpr);
      }
   }

   return expression; // also returns monostate if parseTerm failed
}

template<>
ast::IntegerLiteral* Parser::parse<ast::IntegerLiteral>() {
   Token integerLiteral = tryConsume(TokenType::INTEGER_LITERAL, "Expected an integer literal!", Category::SYNTAX, true);
   return m_arena.create<ast::IntegerLiteral>(*integerLiteral.value);
}

template<>
ast::Identifier* Parser::parse<ast::Identifier>() {
   Token identifier = tryConsume(TokenType::IDENTIFIER, "Expected an identifier!", Category::SYNTAX, true);
   return m_arena.create<ast::Identifier>(*identifier.value);
}

template<>
ast::Negative* Parser::parse<ast::Negative>() {
   tryConsume(TokenType::MINUS, std::nullopt, Category::SYNTAX);

   ast::Expression expression = parseTerm(); // recursion
   if(std::holds_alternative<std::monostate>(expression))
      return nullptr;

   return m_arena.create<ast::Negative>(m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::BinaryExpr* Parser::parse<ast::BinaryExpr>() {
   TokenType op = consume().type;
   int precedence = getPrecedence(op);
   int nextMinPrec = isLeftAssociative(op) ? precedence + 1 : precedence;

   ast::Expression rhs = parseExpression(nextMinPrec);
   if(std::holds_alternative<std::monostate>(rhs)) return nullptr;

   return m_arena.create<ast::BinaryExpr>(nullptr, op, m_arena.create<ast::Expression>(std::move(rhs)));
}

#pragma endregion
