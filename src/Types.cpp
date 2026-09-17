#include <pch/Precompiled.h>
#include "Token.h"

#include "Errors.h"
#include "Types.h"

namespace
{
   std::string getCharsOf(TokenType type) {
      #define X(name) \
         if(type == TokenType::name) { \
            return #name; \
         }

         TOKEN_TYPES
      #undef X

      // should never run
      return "bye world";
   }
}

bool isBinaryOperator(TokenType type) {
   switch(type) {
      case TokenType::PLUS:
      case TokenType::MINUS:
      case TokenType::STAR:
      case TokenType::FSLASH:
      case TokenType::PERCENT:
      case TokenType::CARET:

      case TokenType::EQUALITY:
      case TokenType::INEQUALITY:
      case TokenType::LESS_EQUALS:
      case TokenType::LESS_THAN:
      case TokenType::GREATER_EQUALS:
      case TokenType::GREATER_THAN:
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
      case TokenType::MINUS:
         return true;

      default:
         return false;
   }
}

/**
 * eventually want to get to this
 * 
 * General Order of Operations
 * 6: Exponent (^)
 * 5: Multiplicative (*, /, %): Multiplication, division, and remainder
 * 4: Additive (+, -): Addition and subtraction
 * 3: Relational (<, >, <=, >=): Comparisons
 * 2: Equality (==, !=): Checking if items match
 * 1: Logical AND (&&)
 * 0: Logical OR (||)
 */
int getPrecedence(TokenType type) {
   switch(type) {
      case TokenType::LOGICAL_OR:      return 0;

      case TokenType::LOGICAL_AND:     return 1;

      case TokenType::EQUALITY:
      case TokenType::INEQUALITY:      return 2;

      case TokenType::LESS_THAN:
      case TokenType::GREATER_THAN:
      case TokenType::LESS_EQUALS:
      case TokenType::GREATER_EQUALS:  return 3;

      case TokenType::PLUS:
      case TokenType::MINUS:           return 4;

      case TokenType::STAR:
      case TokenType::FSLASH:
      case TokenType::PERCENT:         return 5;

      case TokenType::CARET:           return 6;

      default:
         assert(false && "Can't find precedence");
         return 0;
   }
}

bool isLeftAssociative(TokenType type) {
   if(type == TokenType::CARET)
      return false;

   return true;
}

std::string to_string(TokenType type) {
   switch(type) {
      case TokenType::EQUALS:          return "=";
      case TokenType::COLON:           return ":";
      case TokenType::SEMICOLON:       return ";";

      case TokenType::OPEN_PAREN:      return "(";
      case TokenType::CLOSE_PAREN:     return ")";
      case TokenType::OPEN_BRACKET:    return "[";
      case TokenType::CLOSE_BRACKET:   return "]";
      case TokenType::OPEN_CURLY:      return "{";
      case TokenType::CLOSE_CURLY:     return "}";

      case TokenType::LESS_THAN:       return "<";
      case TokenType::GREATER_THAN:    return ">";
      case TokenType::PLUS:            return "+";
      case TokenType::MINUS:           return "-";
      case TokenType::STAR:            return "*";
      case TokenType::FSLASH:          return "/";
      case TokenType::PERCENT:         return "%";
      case TokenType::CARET:           return "^";

      case TokenType::EQUALITY:        return "==";
      case TokenType::INEQUALITY:      return "!=";
      case TokenType::LESS_EQUALS:     return "<=";
      case TokenType::GREATER_EQUALS:  return ">=";

      case TokenType::LOGICAL_AND:     return "&&";
      case TokenType::LOGICAL_OR:      return "||";
      case TokenType::LOGICAL_NOT:     return "!";

      case TokenType::INCREMENT:       return "++";
      case TokenType::DECREMENT:       return "--";

      case TokenType::PLUS_EQUALS:     return "+=";
      case TokenType::MINUS_EQUALS:    return "-=";
      case TokenType::STAR_EQUALS:     return "*=";
      case TokenType::SLASH_EQUALS:    return "/=";
      case TokenType::PERCENT_EQUALS:  return "%=";

      case TokenType::BSLASH:          return "\\";

      default:                         return getCharsOf(type);
   }
}

std::string to_string(DataType type) {
   switch(type) {
      case DataType::NONE:  return "NONE";
      case DataType::INT:   return "INT";
      case DataType::BOOL:  return "BOOL";
      default: assert(false && "Unhandled datatype");
   }
}

DataType getReturnType(TokenType op) {
   switch(op) {
      case TokenType::PLUS:
      case TokenType::MINUS:
      case TokenType::STAR:
      case TokenType::FSLASH:
      case TokenType::PERCENT:
      case TokenType::CARET:
         return DataType::INT; /// @todo change when introducing other numeric types

      case TokenType::EQUALITY:
      case TokenType::INEQUALITY:
      case TokenType::LESS_EQUALS:
      case TokenType::LESS_THAN:
      case TokenType::GREATER_EQUALS:
      case TokenType::GREATER_THAN:
      case TokenType::LOGICAL_AND:
      case TokenType::LOGICAL_OR:
         return DataType::BOOL;

      default:
         return DataType::NONE;
   }
}
