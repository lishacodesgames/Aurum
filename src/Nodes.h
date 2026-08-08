#pragma once
#include "Token.h"

/// Abstract Syntax Tree
namespace ast {
   // struct Node {
   //    /// @todo
   // };

   struct Expression /* : public Node */ {
      Token integerLiteral = TokenType::INTEGER_LITERAL;
   };

   struct Exit /* : public Node */ {
      Expression expression;
   };

   using Node = Exit;
}
