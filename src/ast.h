#pragma once
#include "Token.h"
#include "Errors.h"

/// Abstract Syntax Tree
/// @note Do not use pointers to Expression and Statement, since they are variants of pointer types anyways
/// @note No need to std::move Expression and Statement, since they are variants to trivially copyable types (pointers, monostate)
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
   struct UnaryExpr;
   struct BinaryExpr;

   using Expression = std::variant<std::monostate, Literal*, Identifier*, UnaryExpr*, BinaryExpr*>;

   struct UnaryExpr {
      Token op; /// '!' requires BOOL, '-' requires INT
      Expression operand;

      explicit UnaryExpr(Token op, Expression operand) : op(op), operand(operand) {}
   };

   struct BinaryExpr {
      Expression left, right;
      Token op;
      Type type; /// can never be None

      explicit BinaryExpr(Expression left, Token op, Expression right, Type type)
         : left(left), right(right), op(op), type(type) {}
   };

#pragma endregion

#pragma region Statements

   /// @todo change hintType and lockedType to just type and typeMutable
   struct Declaration {
      Identifier* identifier;
      std::optional<Expression> expression; /// nullopt = declaration without definition
      bool valueMutable; /// TRUE = bar, FALSE = mint.
      bool typeMutable;
      std::optional<Type> type = std::nullopt; // if specified by hint or locking. Can be NONE by "bar: None x;" but it's redundant

      /// @param type if nullopt, generator must infer identifier's type by expression (if defined. Else, null)
      explicit Declaration(
         Identifier* identifier, std::optional<Expression> expression, bool valueMutable,
         bool typeMutable, std::optional<Type> type = std::nullopt)
         : identifier(identifier), expression(expression), valueMutable(valueMutable), typeMutable(typeMutable), type(type) {}
   };

   struct Assignment {
      Identifier* identifier;
      Expression expression;

      explicit Assignment(Identifier* identifier, Expression expression) : identifier(identifier), expression(expression) {}
   };

   struct Exit {
      Expression expression;

      explicit Exit(Expression expression) : expression(expression) {}
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
   struct If;

   using Statement = std::variant<std::monostate, Declaration*, Assignment*, Exit*, Increment*, Decrement*, If*, Block*>;

   struct Block {
      std::vector<Statement> statements;

      explicit Block(std::vector<Statement> statements) : statements(statements) {}
   };

   struct If {
      Expression condition;
      Statement thenBranch;

      explicit If(Expression condition, Statement thenBranch) : condition(condition), thenBranch(thenBranch) {}
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
