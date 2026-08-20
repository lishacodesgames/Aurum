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

int getPrecedence(TokenType type) {
   switch(type) {
      case TokenType::PLUS:
      case TokenType::MINUS:
         return 0;

      case TokenType::STAR:
      case TokenType::SLASH:
         return 1;

      default:
         LOG_ERROR("Unknown token '{}'. Can't find precedence!", to_string(type));
         return -1;
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
