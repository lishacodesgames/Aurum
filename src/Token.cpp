#include <pch/Precompiled.h>
#include "Token.h"

std::string Token::to_string() const {
   return std::format("{{type: {}, value: {}}}", ::to_string(type), value ? value.value() : "nullopt");
}

bool isBinaryOperator(TokenType type) {
   switch(type) {
      case TokenType::PLUS:
      case TokenType::MINUS:
      case TokenType::STAR:
      case TokenType::SLASH:
      case TokenType::PERCENT:
      case TokenType::PLUS_EQUALS:
      case TokenType::MINUS_EQUALS:
      case TokenType::STAR_EQUALS:
      case TokenType::SLASH_EQUALS:
      case TokenType::PERCENT_EQUALS:
         return true;

      default:
         return false;
   }
}

bool isUnaryOperator(TokenType type) {
   switch(type) {
      case TokenType::INCREMENT:
      case TokenType::DECREMENT:
      case TokenType::MINUS:
      case TokenType::PLUS:
         return true;

      default:
         return false;
   }
}

std::string to_string(TokenType type) {
   #define X(name) \
      if(type == TokenType::name) { \
         return #name; \
      }

      TOKEN_TYPES
   #undef X

   return "to_string(TokenType) messed up!"; // should never run
}
