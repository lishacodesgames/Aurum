#include <pch/Precompiled.h>
#include "Token.h"

std::string to_string(TokenType type) {
   #define X(name) \
      if(type == TokenType::name) { \
         return #name; \
      }

      TOKEN_TYPES
   #undef X

   return "to_string(TokenType) messed up!"; // should never run
}

std::string Token::to_string() const {
   return std::format("{{type: {}, value: {}}}", ::to_string(type), value ? value.value() : "nullopt");
}
