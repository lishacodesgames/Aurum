#include <pch/Precompiled.h>
#include "Token.h"

#include "Errors.h"

std::string Token::to_string() const {
   return std::format("{{ type: {}, value: {} }}", ::to_string(type), value ? value.value() : "nullopt");
}

bool isBinaryOperator(TokenType type) {
   switch(type) {
      case TokenType::PLUS:
      case TokenType::MINUS:
      case TokenType::STAR:
      case TokenType::FSLASH:
      case TokenType::PERCENT:
      case TokenType::CARET:

      case TokenType::PLUS_EQUALS:
      case TokenType::MINUS_EQUALS:
      case TokenType::STAR_EQUALS:
      case TokenType::SLASH_EQUALS:
      case TokenType::PERCENT_EQUALS:

      case TokenType::LOGICAL_AND:
      case TokenType::LOGICAL_OR:
         return true;

      default:
         return false;
   }
}

bool isUnaryOperator(TokenType type) {
   switch(type) {
      case TokenType::LOGICAL_NOT:
      case TokenType::INCREMENT:
      case TokenType::DECREMENT:
      case TokenType::MINUS:
      case TokenType::PLUS:
         return true;

      default:
         return false;
   }
}

/**
 * eventually want to get to this
 * 
 * General Order of Operations
 * 8: Unary/Postfix (++, --, !): Increments, decrements, and logical NOT
 * 7: Exponent (^)
 * 6: Multiplicative (*, /, %): Multiplication, division, and remainder
 * 5: Additive (+, -): Addition and subtraction
 * 4: Relational (<, >, <=, >=): Comparisons
 * 3: Equality (==, !=): Checking if items match
 * 2: Logical AND (&&)
 * 1: Logical OR (||)
 * 0: Assignment (=): Saving final values last
 */
int getPrecedence(TokenType type) {
   switch(type) {
      case TokenType::PLUS:
      case TokenType::MINUS:
         return 0;

      case TokenType::STAR:
      case TokenType::FSLASH:
      case TokenType::PERCENT:
         return 1;

      case TokenType::CARET:
         return 2;

      default:
         LOG_ERROR("Unknown token '{}'. Can't find precedence!", to_string(type));
         return -1;
   }
}

bool isLeftAssociative(TokenType type) {
   switch(type) {
      case TokenType::CARET:
         return false;

      default:
         return true;
   }
}

std::string to_string(TokenType type) {
   #define X(name) \
      if(type == TokenType::name) { \
         return #name; \
      }

      TOKEN_TYPES
   #undef X

   return "bye world"; // should never run
}

std::string getCharsOf(TokenType type) {
   switch(type) {
      case TokenType::EQUALS:
         return "=";
      case TokenType::COLON:
         return ":";
      case TokenType::SEMICOLON:
         return ";";

      case TokenType::OPEN_PAREN:
         return "(";
      case TokenType::CLOSE_PAREN:
         return ")";
      case TokenType::OPEN_BRACKET:
         return "[";
      case TokenType::CLOSE_BRACKET:
         return "]";
      case TokenType::OPEN_CURLY:
         return "{";
      case TokenType::CLOSE_CURLY:
         return "}";

      case TokenType::LESS_THAN:
         return "<";
      case TokenType::GREATER_THAN:
         return ">";
      case TokenType::PLUS:
         return "+";
      case TokenType::MINUS:
         return "-";
      case TokenType::STAR:
         return "*";
      case TokenType::FSLASH:
         return "/";
      case TokenType::PERCENT:
         return "%";
      case TokenType::CARET:
         return "^";

      case TokenType::EQUALITY:
         return "==";
      case TokenType::INEQUALITY:
         return "!=";
      case TokenType::LESS_EQUALS:
         return "<=";
      case TokenType::GREATER_EQUALS:
         return ">=";

      case TokenType::LOGICAL_AND:
         return "&&";
      case TokenType::LOGICAL_OR:
         return "||";
      case TokenType::LOGICAL_NOT:
         return "!";

      case TokenType::INCREMENT:
         return "++";
      case TokenType::DECREMENT:
         return "--";

      case TokenType::PLUS_EQUALS:
         return "+=";
      case TokenType::MINUS_EQUALS:
         return "-=";
      case TokenType::STAR_EQUALS:
         return "*=";
      case TokenType::SLASH_EQUALS:
         return "/=";
      case TokenType::PERCENT_EQUALS:
         return "%=";

      case TokenType::BSLASH:
         return "\\";
      
      default:
         return to_string(type);
   }
}
