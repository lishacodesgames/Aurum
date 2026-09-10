#pragma once
#include "Token.h"
#include "Errors.h"

enum class Type {
   NONE, INT, BOOL
};

inline std::string to_string(Type type) {
   switch(type) {
      case Type::NONE:  return "NONE";
      case Type::INT:   return "INT";
      case Type::BOOL:  return "BOOL";

      default:
         g_errors.report(err::Phase::GENERATING, err::Category::INTERNAL, { "ast.h", __LINE__ },
            "Unhandled Type in to_string(Type)!", true);
         return "";
   }
}

/// Abstract Syntax Tree
namespace ast
{
#pragma region Expressions

   struct Literal {
      Type type;
      Token token;

      explicit Literal(Type type, Token token) : type(type), token(token) {}
   };

   struct Identifier {
      Token token = TokenType::IDENTIFIER;

      explicit Identifier(Token token) : token(token) {}
   };

   // expressions that contain an expression
   struct Negative;
   struct BinaryExpr;

   using Expression = std::variant<std::monostate, Literal*, Identifier*, Negative*, BinaryExpr*>;

   struct Negative {
      Expression* operand;

      explicit Negative(Expression* operand) : operand(operand) {}
   };

   struct BinaryExpr {
      Expression* left, *right;
      Token op;

      explicit BinaryExpr(Expression* left, Token op, Expression* right)
         : left(left), right(right), op(op) {}
   };

#pragma endregion

#pragma region Statements

   /// @todo change hintType and lockedType to just type and typeMutable
   struct Declaration {
      Identifier* identifier;
      Expression* expression; /// nullptr = declaration without definition
      bool valueMutable; /// TRUE = bar, FALSE = mint.
      std::optional<Type> hintType = std::nullopt; /// can never be NONE
      std::optional<Type> lockedType = std::nullopt; /// can never be NONE

      /// type (if expression defined) must be inferred
      explicit Declaration(Identifier* identifier, Expression* expression, bool valueMutable)
         : identifier(identifier), expression(expression), valueMutable(valueMutable) {}

      /// For when some type annotation is given
      explicit Declaration(Identifier* identifier, Expression* expression, bool valueMutable, Type type, bool typeMutable)
         : identifier(identifier), expression(expression), valueMutable(valueMutable)
      {
         if(typeMutable)
            hintType = type;
         else
            lockedType = type;
      }
   };

   struct Assignment {
      Identifier* identifier;
      Expression* expression;

      explicit Assignment(Identifier* identifier, Expression* expression) : identifier(identifier), expression(expression) {}
   };

   struct Exit {
      Expression* expression;

      explicit Exit(Expression* expression) : expression(expression) {}
   };

   struct Increment {
      Identifier* identifier;

      explicit Increment(Identifier* identifier) : identifier(identifier) {}
   };

   struct Decrement {
      Identifier* identifier;

      explicit Decrement(Identifier* identifier) : identifier(identifier) {}
   };

   // statements that contain statements
   struct Block;

   using Statement = std::variant<std::monostate, Declaration*, Assignment*, Exit*, Increment*, Decrement*, Block*>;

   struct Block {
      std::vector<Statement> statements;

      explicit Block(std::vector<Statement> statements) : statements(statements) {}
   };

#pragma endregion

   struct Program {
      std::vector<Statement> statements;

      // for convenience
      bool empty() const noexcept { return statements.empty(); }
      void push_back(Statement stmt) { statements.push_back(stmt); } // not noexcept bcz push_back might throw bad_alloc()
   };

   // --- TRAIT MACHINERY ---
   namespace detail // not meant to be used anywhere else (header implementation name convention)
   {
      // 1. Primary template — intentionally undefined; only specialization below is used
      template<typename T, typename V>
      struct is_variant_alternative;

      // 2. Specialization — triggers when Variant is shaped like std::variant<Ts...>,
      template<typename T, typename... Ts>
      struct is_variant_alternative<T, std::variant<Ts...>>
         : std::disjunction<std::is_same<T, Ts>...> {}; // check if T is part of the variant types, ::value will be true if it is.

      template<typename T, typename Variant>
      inline constexpr bool is_variant_alternative_v = detail::is_variant_alternative<T, Variant>::value;
   }

   template<typename T>
   concept AstNode = detail::is_variant_alternative_v<T*, Expression> || detail::is_variant_alternative_v<T*, Statement>;

   template<typename T>
   concept VariantNode = std::is_same_v<T, Expression> || std::is_same_v<T, Statement>;
}
