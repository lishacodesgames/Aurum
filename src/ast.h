#pragma once
#include "Token.h"
#include "Errors.h"

/// Abstract Syntax Tree
namespace ast
{
   // --- EXPRESSIONS ---

   struct IntegerLiteral {
      int value;

      /// @param value type = string because Token stores value as a string
      explicit IntegerLiteral(const std::string& value) {
         try {
            this->value = std::stoi(value);
         } catch(const std::invalid_argument& e) {
            FATAL_ERROR("Tried to convert {} to an integer literal!", value);
         } catch(const std::out_of_range& e) {
            FATAL_ERROR("'{}' is too large for an integer literal!", value);
         }
      }

      std::string to_string() const { return std::to_string(value); }
   };

   struct Identifier {
      std::string name;

      explicit Identifier(std::string_view value) : name(value) {}
   };

   // expressions that contain an expression
   struct Negative;
   struct BinaryExpr;

   using Expression = std::variant<std::monostate, IntegerLiteral*, Identifier*, Negative*, BinaryExpr*>;

   struct Negative {
      Expression* operand;

      explicit Negative(Expression* operand) : operand(operand) {}
   };

   struct BinaryExpr {
      Expression* left, *right;
      TokenType op;

      explicit BinaryExpr(Expression* left, TokenType op, Expression* right)
         : left(left), right(right), op(op) {}
   };

   // --- STATEMENTS ---

   /// @todo type constraints: bar vs bar<>
   struct Declaration {
      Identifier* identifier;
      std::optional<Expression*> expression;
      bool isMutable; /// TRUE = bar, FALSE = mint.

      explicit Declaration(Identifier* identifier, bool isMutable) : identifier(identifier), isMutable(isMutable) {}

      explicit Declaration(Identifier* identifier, Expression* expression, bool isMutable)
         : identifier(identifier), expression(expression), isMutable(isMutable) {}
   };

   struct Exit {
      Expression* expression;

      explicit Exit(Expression* expression) : expression(expression) {}
   };

   struct Increment {
      /// Storing the entire identifier, because later on it might contain more information
      Identifier* identifier;

      explicit Increment(Identifier* identifier) : identifier(identifier) {}
   };

   struct Decrement {
      Identifier* identifier;

      explicit Decrement(Identifier* identifier) : identifier(identifier) {}
   };

   using Statement = std::variant<std::monostate, Declaration*, Exit*, Increment*, Decrement*>;

   // --- PROGRAM ---

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
