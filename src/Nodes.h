#pragma once
#include "Token.h"

/// Abstract Syntax Tree
namespace ast {
   struct Node {
      /// @todo maybe a nested NodeKind enum
      virtual ~Node() = default;
   };

   struct Expression : public Node {
      virtual ~Expression() = default;
   };

   struct IntegerLiteral : public Expression {
      Token value = TokenType::INTEGER_LITERAL;

      explicit IntegerLiteral(std::string value) { this->value.value = value; }
   };

   struct Identifier : public Expression {
      Token name = TokenType::IDENTIFIER;

      explicit Identifier(std::string name) { this->name.value = name; }
   };

   struct Exit : public Node {
      /// @typedef unique_ptr so we can safely downcast to Expression's children
      std::unique_ptr<Expression> expression;

      explicit Exit(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
   };
}
