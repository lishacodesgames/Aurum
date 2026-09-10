#include <pch/Precompiled.h>
#include "Parser.h"

#define VALIDATE_VARIANT_RETURN_MONO(var) if(std::holds_alternative<std::monostate>((var))) return std::monostate{}
#define VALIDATE_VARIANT_RETURN_NULL(var) if(std::holds_alternative<std::monostate>((var))) return nullptr

#define VALIDATE_PTR_RETURN_MONO(ptr) if(!(ptr)) return std::monostate{}
#define VALIDATE_PTR_RETURN_NULL(ptr) if(!(ptr)) return nullptr

namespace
{
   /// @return never returns NONE, only actual data types
   std::optional<Type> getType(Token token) {
      switch(token.type) {
         case TokenType::INT:    return Type::INT;
         case TokenType::BOOL:   return Type::BOOL;

         default:
            g_errors.report(err::Phase::PARSING, err::Category::INTERNAL, token.location, "Unhandled datatype token: " + to_string(token.type));
            return std::nullopt;
      }
   }
}

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

void Parser::error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) {
   g_errors.report(err::Phase::PARSING, category, location, message, isFatal);
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
               error(err::Category::SYNTAX, peek(1).location, std::format("Unexpected token {} after identifier {}", getCharsOf(peek(1).type), *peek(1).value));
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
         error(err::Category::SYNTAX, peek().location, "Unexpected token, unable to parse statement beginning with: " + to_string(peek().type));
         return std::monostate{};
      }
   }
}

template<> ast::Declaration* Parser::parse<ast::Declaration>() {
   bool valueMutable = consume().type == TokenType::BAR;
   bool typeMutable = false;
   Type type = Type::NONE;

   bool typeAnnotations = true;
   if(tryConsume(TokenType::LESS_THAN)) {
      if(!valueMutable) {
         error(err::Category::SYNTAX, peek(-1).location, "Can only use type locking syntax on 'bar'! Use type hints for immutables.");
         return nullptr;
      }

      std::optional<Type> declType = getType(consume());
      VALIDATE_PTR_RETURN_NULL(declType);

      type = *declType;

      VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::GREATER_THAN,
         Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `>`!" }));

   } else if(tryConsume(TokenType::COLON)) {
      std::optional<Type> declType = getType(consume());
      VALIDATE_PTR_RETURN_NULL(declType);

      type = *declType;
      typeMutable = valueMutable;
   } else {
      typeAnnotations = false;
   }

   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   ast::Expression* expression = nullptr; // in case it's a Declaration without Definition
   if(tryConsume(TokenType::EQUALS)) {
      ast::Expression expr = parseExpression();
      VALIDATE_VARIANT_RETURN_NULL(expr);

      expression = m_arena.create<ast::Expression>(std::move(expr));
   }

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

   if(typeAnnotations)
      return m_arena.create<ast::Declaration>(identifier, expression, valueMutable, type, typeMutable);
   else
      return m_arena.create<ast::Declaration>(identifier, expression, valueMutable);
}

template<>
ast::Assignment* Parser::parse<ast::Assignment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume =

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

   return m_arena.create<ast::Assignment>(identifier, m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Exit* Parser::parse<ast::Exit>() {
   consume(); // consume exit keyword

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

   return m_arena.create<ast::Exit>(m_arena.create<ast::Expression>(std::move(expression)));
}

template<>
ast::Increment* Parser::parse<ast::Increment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume ++
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

   return m_arena.create<ast::Increment>(identifier);
}

template<>
ast::Decrement* Parser::parse<ast::Decrement>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume --
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`" }));

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
      case TokenType::INTEGER_LITERAL:
      case TokenType::TRUE:
      case TokenType::FALSE: {
         ast::Literal* literal = parse<ast::Literal>();
         VALIDATE_PTR_RETURN_MONO(literal);

         return ast::Expression(std::in_place_type<ast::Literal*>, literal);
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
         VALIDATE_PTR_RETURN_MONO(tryConsume(TokenType::CLOSE_PAREN, Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Unclosed parentheses!" }));
         return expression;
      }

      default:
         error(err::Category::SYNTAX, peek().location,
            "Unexpected token, unable to parse term beginning with: " + to_string(peek().type));
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
ast::Literal* Parser::parse<ast::Literal>() {
   switch(peek().type) {
      case TokenType::INTEGER_LITERAL:
         return m_arena.create<ast::Literal>(Type::INT, consume());

      case TokenType::TRUE:
      case TokenType::FALSE:
         return m_arena.create<ast::Literal>(Type::BOOL, consume());

      default:
         error(err::Category::SYNTAX, peek().location, "Expected a literal!");
         return nullptr;
   }
}

template<>
ast::Identifier* Parser::parse<ast::Identifier>() {
   std::optional<Token> identToken = tryConsume(TokenType::IDENTIFIER,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected an identifier!" });

   VALIDATE_PTR_RETURN_NULL(identToken);
   return m_arena.create<ast::Identifier>(*identToken);
}

template<>
ast::Negative* Parser::parse<ast::Negative>() {
   consume(); // consume minus

   ast::Expression expression = parseTerm();
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
