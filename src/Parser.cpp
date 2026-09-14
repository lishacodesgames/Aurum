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
            g_errors.report(
               err::Phase::PARSING, err::Category::INTERNAL, token.location, "Unhandled datatype token: " + to_string(token.type));
            return std::nullopt;
      }
   }
}

ast::Program Parser::parse() {
   ast::Program program;
   while(peek() != TokenType::END_OF_FILE) {
      ast::Statement statement = parseStatement();

      if(std::holds_alternative<std::monostate>(statement)) {
         recover();
         continue;
      }

      program.push_back(statement); // only push back valid statement.
   }

   return program;
}

#pragma region Helpers

void Parser::error(err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) {
   g_errors.report(err::Phase::PARSING, category, location, message, isFatal);
}

void Parser::recover() {
   int depth = 0;

   while(true) {
      switch(peek().type) {
         case TokenType::END_OF_FILE: return;

         case TokenType::OPEN_CURLY:
            depth++;
            consume();
            continue;

         case TokenType::CLOSE_CURLY:
            if(depth == 0)
               return; // brace isn't ours to eat, so we return without consume

            // brace was opened by us
            depth--;
            consume();
            continue;

         case TokenType::SEMICOLON:
            if(depth == 0) {
               // end of statement we had to recover from
               consume();
               return;
            }

            // end of a statement nested inside { ... } opened by us
            consume();
            continue;

         default:
            consume();
            continue;
      }
   }
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

#pragma endregion

// PARSE OVERLOADS

#pragma region Statements

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
               error(
                  err::Category::SYNTAX, peek(1).location,
                  std::format("Unexpected token '{}' after identifier '{}'!", to_string(peek(1).type), *peek().value));
               return std::monostate{};
            }
         }
      }
      
      case TokenType::OPEN_CURLY: {
         ast::Block* block = parse<ast::Block>();
         VALIDATE_PTR_RETURN_MONO(block);

         return ast::Statement(std::in_place_type<ast::Block*>, block);
      }

      case TokenType::IF: {
         ast::If* ifStmt = parse<ast::If>();
         VALIDATE_PTR_RETURN_MONO(ifStmt);

         return ast::Statement(std::in_place_type<ast::If*>, ifStmt);
      }

      default: {
         error(
            err::Category::SYNTAX, peek().location,
            "Unexpected token, unable to parse statement beginning with: " + to_string(peek().type));
         return std::monostate{};
      }
   }
}

template<> ast::Declaration* Parser::parse<ast::Declaration>() {
   bool valueMutable = consume().type == TokenType::BAR;
   bool typeMutable = valueMutable;
   std::optional<Type> type = std::nullopt;

   if(tryConsume(TokenType::LESS_THAN)) {
      if(!valueMutable) {
         error(err::Category::SYNTAX, peek(-1).location, "Can only use type locking syntax on 'bar'! Use type hints for immutables.");
         return nullptr;
      }

      typeMutable = false;
      type = getType(consume());
      VALIDATE_PTR_RETURN_NULL(type);

      VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::GREATER_THAN,
         Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `>`!" }));

   } else if(tryConsume(TokenType::COLON)) {
      type = getType(consume());
      VALIDATE_PTR_RETURN_NULL(type);
   }

   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   std::optional<ast::Expression> expression = std::nullopt; // in case it's a Declaration without Definition

   // bar x = None; is valid. but in that case, don't try to parse expr
   if(tryConsume(TokenType::EQUALS) && !tryConsume(TokenType::NONE)) {
      ast::Expression expr = parseExpression();
      VALIDATE_VARIANT_RETURN_NULL(expr);

      expression = expr;
   }

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`!" }));

   return m_arena.create<ast::Declaration>(identifier, expression, valueMutable, typeMutable, type);
}

template<>
ast::Assignment* Parser::parse<ast::Assignment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume =

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`!" }));

   return m_arena.create<ast::Assignment>(identifier, expression);
}

template<>
ast::Exit* Parser::parse<ast::Exit>() {
   consume(); // consume exit keyword

   ast::Expression expression = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`!" }));

   return m_arena.create<ast::Exit>(expression);
}

template<>
ast::Increment* Parser::parse<ast::Increment>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume ++
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`!" }));

   return m_arena.create<ast::Increment>(identifier);
}

template<>
ast::Decrement* Parser::parse<ast::Decrement>() {
   ast::Identifier* identifier = parse<ast::Identifier>();
   VALIDATE_PTR_RETURN_NULL(identifier);

   consume(); // consume --
   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::SEMICOLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `;`!" }));

   return m_arena.create<ast::Decrement>(identifier);
}

template<>
ast::Block* Parser::parse<ast::Block>() {
   std::vector<ast::Statement> stmts;
   consume(); // consume {

   bool error = false;
   while(!tryConsume(TokenType::CLOSE_CURLY)) {
      ast::Statement statement = parseStatement();
      if(std::holds_alternative<std::monostate>(statement)) {
         error = true;
         recover();
         continue;
      }

      stmts.push_back(statement);
   }

   if(error)
      return nullptr;
   else
      return m_arena.create<ast::Block>(std::move(stmts));
}

template<>
ast::If* Parser::parse<ast::If>() {
   consume(); // consume if keyword

   // parentheses are optional, but if present, expression will handle them
   ast::Expression condition = parseExpression();
   VALIDATE_VARIANT_RETURN_NULL(condition);

   VALIDATE_PTR_RETURN_NULL(tryConsume(TokenType::COLON,
      Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Expected `:`!" }));

   err::SourceLocation location = peek().location;
   ast::Statement thenBranch = parseStatement();
   if(std::holds_alternative<std::monostate>(thenBranch)) {
      error(err::Category::SYNTAX, location, "Expected a statement after if condition!");
      recover();
      return nullptr;
   } else if(std::holds_alternative<ast::Declaration*>(thenBranch)) {
      error(err::Category::SCOPING, location, "Cannot declare a variable in an un-scoped control statement!");
      recover();
      return nullptr;
   }

   return m_arena.create<ast::If>(condition, thenBranch);
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

      case TokenType::LOGICAL_NOT:
      case TokenType::MINUS: {
         ast::UnaryExpr* negation = parse<ast::UnaryExpr>();
         VALIDATE_PTR_RETURN_MONO(negation);

         return ast::Expression(std::in_place_type<ast::UnaryExpr*>, negation);
      }

      case TokenType::OPEN_PAREN: {
         consume();
         ast::Expression expression = parseExpression();

         /// @todo store and then pass to macro, i dont like this long ugly string
         VALIDATE_PTR_RETURN_MONO(tryConsume(TokenType::CLOSE_PAREN,
            Error{ .category = err::Category::SYNTAX, .location = peek().location, .message = "Unclosed parentheses!" }));
         return expression;
      }

      default:
         error(
            err::Category::SYNTAX, peek().location,
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
   
         binaryExpr->left = expression;
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
         error(err::Category::SYNTAX, peek().location, "Expected a literal! Got: " + to_string(peek().type));
         return nullptr;
   }
}

template<>
ast::Identifier* Parser::parse<ast::Identifier>() {
   std::optional<Token> identToken = tryConsume(TokenType::IDENTIFIER,
      Error{.category = err::Category::SYNTAX, .location = peek().location,
            .message = "Expected an identifier! Got: " + to_string(peek().type) });

   VALIDATE_PTR_RETURN_NULL(identToken);
   return m_arena.create<ast::Identifier>(*identToken);
}

template<>
ast::UnaryExpr* Parser::parse<ast::UnaryExpr>() {
   Token op = consume();
   if(!isUnaryOperator(op.type)) {
      error(err::Category::SYNTAX, op.location, "Invalid unary operator: " + to_string(op.type));
      return nullptr;
   }

   ast::Expression expression = parseTerm();
   VALIDATE_VARIANT_RETURN_NULL(expression);

   return m_arena.create<ast::UnaryExpr>(op, expression);
}

template<>
ast::BinaryExpr* Parser::parse<ast::BinaryExpr>() {
   Token op = consume();
   Type type = getReturnType(op.type);
   if(type == Type::NONE) {
      error(err::Category::SYNTAX, op.location, "Invalid binary operator: " + to_string(op.type));
      return nullptr;
   }

   int precedence = getPrecedence(op.type);
   int nextMinPrec = isLeftAssociative(op.type) ? precedence + 1 : precedence;

   ast::Expression rhs = parseExpression(nextMinPrec);
   VALIDATE_VARIANT_RETURN_NULL(rhs);

   return m_arena.create<ast::BinaryExpr>(std::monostate{}, op, rhs, type);
}

#pragma endregion
