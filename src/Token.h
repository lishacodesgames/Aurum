#pragma once
#include "Errors.h"
#include "Types.h"

struct Token {
   TokenType type;
   err::SourceLocation location;
   std::optional<std::string> value = std::nullopt;

   Token(TokenType type) : type(type) {} // for implicit conversion
   Token(TokenType type, err::SourceLocation location) : type(type), location(location) {}
   // might need to change value(value) to value{value} bcz param is string VIEW
   Token(TokenType type, std::string_view value, err::SourceLocation location) : type(type), location(location), value(value) {}

   std::string to_string() const {
      return std::format("{{ type: {}, value: {} }}", ::to_string(type), value ? value.value() : "nullopt");
   }

   bool operator==(TokenType type) const noexcept { return type == this->type; }
};
