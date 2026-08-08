#pragma once
#include "Token.h"

/// Abstract Syntax Tree
namespace ast {
   // struct Node {
   //    /// @todo
   // };

   struct IntegerLiteral {
      Token integer = TokenType::INTEGER_LITERAL;
   };

   struct Identifier {
      Token name;
   };

   /// @todo do we need monostate?
   using Expression = std::variant<std::monostate, IntegerLiteral, Identifier>;

   struct Exit /* : public Node */ {
      Expression expression;
   };

   using Node = Exit; // for now
}
