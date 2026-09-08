#include <pch/Precompiled.h>
#include "Parser.h"

#define VALIDATE_VARIANT_RETURN_MONO(var) if(std::holds_alternative<std::monostate>((var))) return std::monostate{}
#define VALIDATE_VARIANT_RETURN_NULL(var) if(std::holds_alternative<std::monostate>((var))) return nullptr

#define VALIDATE_PTR_RETURN_MONO(ptr) if(!(ptr)) return std::monostate{}
#define VALIDATE_PTR_RETURN_NULL(ptr) if(!(ptr)) return nullptr

ast::Program Parser::parse() {
   ast::Program program;
   while(peek() != TokenType::END_OF_FILE) {
      ast::Statement statement = parseStatement();

      if(std::holds_alternative<std::monostate>(statement)) {
         while(peek() != TokenType::SEMICOLON && peek() != TokenType::END_OF_FILE)
            consume();

         if(peek() == TokenType::SEMICOLON)
            consume(); // consume semicolon, not eof
         continue;
      }

      program.push_back(std::move(statement));
   }

   return program;
}

void Parser::error(Category category, SourceLocation location, std::string_view message, bool isFatal) {
   g_errors.report(Phase::PARSING, category, location, message, isFatal);
}

Token Parser::peek(int offset) const noexcept {
   if(m_pos + offset < m_tokens.size() - 1)
      return m_tokens.at(m_pos + offset);
   else
      return TokenType::END_OF_FILE;
}

Token Parser::consume(std::uint32_t count) noexcept {
   Token current = m_tokens.at(m_pos);
   m_pos += count;

   return current;
}

std::optional<Token> Parser::tryConsume(TokenType type, std::optional<Error> error, bool hasValue) {
   if(peek().type != type || (hasValue && !peek().value)) {
      if(error)
         this->error(error->category, error->location, error->message, error->isFatal);

      return std::nullopt;
   }

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
               VALIDATE_PTR_RETURN_MONO(decrement);

               return ast::Statement(std::in_place_type<ast::Decrement*>, decrement);
            }

            case TokenType::EQUALS: {
               ast::Assignment* assignment = parse<ast::Assignment>();
               VALIDATE_PTR_RETURN_MONO(assignment);

               return ast::Statement(std::in_place_type<ast::Assignment*>, assignment);
            }

            default: {
               error(Category::SYNTAX, peek(1).location, std::format("Unexpected token {} after identifier {}", getCharsOf(peek(1).type), *peek(1).value), false);
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
         error(Category::SYNTAX, peek().location, "Unexpected token, unable to parse statement beginning with: " + to_string(peek().type), true);
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

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

   if(expression)
      return m_arena.create<ast::Declaration>(identifier, expression, isMutable);
   else
      return m_arena.create<ast::Declaration>(identifier, isMutable);
}

template<>
ast::Assignment* Parser::parse<ast::Assignment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume =

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));
   return m_arena.create<ast::Assignment>(identifier, m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Exit* Parser::parse<ast::Exit>() {
   consume(); // consume exit keyword

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));
   return m_arena.create<ast::Exit>(m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Increment* Parser::parse<ast::Increment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume ++
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));
   return m_arena.create<ast::Increment>(identifier);
}

template<>
ast::Decrement* Parser::parse<ast::Decrement>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume --
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));
   return m_arena.create<ast::Decrement>(identifier);
}

template<>
ast::Block* Parser::parse<ast::Block>() {
   std::vector<ast::Statement> stmts;
   consume(); // consume {

   while(!tryConsume(TokenType::CLOSE_CURLY)) {
      ast::Statement statement = parseStatement();
      VALIDATE_VARIANT_RETURN_NULL(statement);

      stmts.push_back(std::move(statement));
   }

   return m_arena.create<ast::Block>(std::move(stmts));
}

#pragma endregion

#pragma region Expressions

ast::Expression Parser::parseTerm() {
   switch(peek().type) {
      case TokenType::INTEGER_LITERAL: {
         ast::IntegerLiteral* integerLiteral = parse<ast::IntegerLiteral>();
         VALIDATE_PTR_RETURN_MONO(integerLiteral);

         return ast::Expression(std::in_place_type<ast::IntegerLiteral*>, integerLiteral);
      }

      case TokenType::IDENTIFIER: {
         ast::Identifier* identifier = parse<ast::Identifier>();
         VALIDATE_PTR_RETURN_MONO(identifier);

         return ast::Expression(std::in_place_type<ast::Identifier*>, identifier);
      }

      case TokenType::MINUS: {
         ast::Negative* negation = parse<ast::Negative>();
         VALIDATE_PTR_RETURN_MONO(negation);

         return ast::Expression(std::in_place_type<ast::Negative*>, negation);
      }

      case TokenType::PLUS: {
         consume();
         return parseTerm();
      }

      case TokenType::OPEN_PAREN: {
         consume();
         ast::Expression expression = parseExpression();

         /// @todo store and then pass to macro, i dont like this long ugly string
         VALIDATE_PTR_RETURN_MONO(tryConsume(TokenType::CLOSE_PAREN, Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Unclosed parentheses!" }));
         return expression;
      }

      default:
         error(Category::SYNTAX, peek().location, "Unexpected token, unable to parse term beginning with: " + to_string(peek().type), false);
         return std::monostate{};
   }
}

ast::Expression Parser::parseExpression(int minPrec) {
   ast::Expression expression = parseTerm();

   if(!std::holds_alternative<std::monostate>(expression)) {
      while(isBinaryOperator(peek().type) && getPrecedence(peek().type) >= minPrec) {
         ast::BinaryExpr* binaryExpr = parse<ast::BinaryExpr>();
         VALIDATE_PTR_RETURN_MONO(binaryExpr);
   
         binaryExpr->left = m_arena.create<ast::Expression>(std::move(expression));
         expression = ast::Expression(std::in_place_type<ast::BinaryExpr*>, binaryExpr);
      }
   }

   return expression;
}

template<>
ast::IntegerLiteral* Parser::parse<ast::IntegerLiteral>() {
   std::optional<Token> integerLiteral = tryConsume(TokenType::INTEGER_LITERAL,
      Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected an integer literal!" }, true);
   VALIDATE_PTR_RETURN_NULL(integerLiteral);

   return m_arena.create<ast::IntegerLiteral>(*integerLiteral);
}

template<>
ast::Identifier* Parser::parse<ast::Identifier>() {
   std::optional<Token> identifier = tryConsume(TokenType::IDENTIFIER,
      Error{ .category = Category::SYNTAX, .location = peek().location, .message = "Expected an identifier!" }, true);
   VALIDATE_PTR_RETURN_NULL(identifier);

   return m_arena.create<ast::Identifier>(*identifier);
}

template<>
ast::Negative* Parser::parse<ast::Negative>() {
   consume(); // consume -

   ast::Expression expression = parseTerm(); // recursion
   VALIDATE_VARIANT_RETURN_NULL(expression);

   return m_arena.create<ast::Negative>(m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::BinaryExpr* Parser::parse<ast::BinaryExpr>() {
   Token op = consume();
   int precedence = getPrecedence(op.type);
   int nextMinPrec = isLeftAssociative(op.type) ? precedence + 1 : precedence;

   ast::Expression rhs = parseExpression(nextMinPrec);
   VALIDATE_VARIANT_RETURN_NULL(rhs);

   return m_arena.create<ast::BinaryExpr>(nullptr, op, m_arena.create<ast::Expression>(std::move(rhs)));
}

#pragma endregion
