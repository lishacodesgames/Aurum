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

   using Expression = std::variant<std::monostate, IntegerLiteral*, Identifier*>;

   // --- STATEMENTS ---

   /// @todo mutability & type constraints: mint vs bar vs bar<>
   struct Declaration {
      ast::Identifier* identifier;
      std::optional<ast::Expression*> expression;

      explicit Declaration(ast::Identifier* identifier) : identifier(identifier) {}

      Declaration(ast::Identifier* identifier, ast::Expression* expression)
         : identifier(identifier), expression(expression) {}
   };

   struct Exit {
      ast::Expression* expression;

      explicit Exit(ast::Expression* expression) : expression(expression) {}
   };

   using Statement = std::variant<std::monostate, Declaration*, Exit*>;

   // --- PROGRAM ---

   struct Program {
      std::vector<ast::Statement> statements;

      // for convenience
      [[nodiscard]] bool empty() const noexcept { return statements.empty(); }
      void push_back(ast::Statement stmt) { statements.push_back(stmt); } // not noexcept bcz push_back might throw bad_alloc()
   };
}

// --- TRAIT MACHINERY ---
namespace detail // not meant to be used other than to define AstNode (header implementation name convention)
{
   // 1. Primary template — intentionally undefined; only specialization below is used
   template<typename T, typename V>
   struct is_variant_alternative;

   // 2. Specialization — triggers when Variant is shaped like std::variant<Ts...>,
   template<typename T, typename... Ts>
   struct is_variant_alternative<T, std::variant<Ts...>>
      : std::disjunction<std::is_same<T, Ts>...> {}; // check if T is part of the variant types, ::value will be true if it is.

   // 3. Shorthand, avoids writing ::value everywhere
   template<typename T, typename Variant>
   inline constexpr bool is_variant_alternative_v = is_variant_alternative<T, Variant>::value;
}

template<typename T>
concept AstNode = detail::is_variant_alternative_v<T*, ast::Expression> || detail::is_variant_alternative_v<T*, ast::Statement>;

template<typename T>
concept VariantNode = std::is_same_v<T, ast::Expression> || std::is_same_v<T, ast::Statement>;
