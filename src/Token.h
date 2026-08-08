#pragma once

#define TOKEN_TYPES \
   X(EXIT) \
   X(SEMICOLON) \
   X(END_OF_FILE) \
\
   X(INTEGER_LITERAL) \
\
   /* variable name */ \
   X(IDENTIFIER) \
\
   /* [] */ \
   X(OPEN_BRACKET) X(CLOSE_BRACKET) \
   /* () */ \
   X(OPEN_PAREN) X(CLOSE_PAREN) \
   /* {} */ \
   X(OPEN_CURLY) X(CLOSE_CURLY) \

enum class TokenType {
   #define X(name) name,
      TOKEN_TYPES
   #undef X
};

struct Token {
   TokenType type;
   std::optional<std::string> value = std::nullopt; /// @todo change type. Template it maybe

   Token(TokenType type) : type(type) {} // for implicit conversion
   Token(TokenType type, std::string value) : type(type), value(value) {}

   std::string to_string() const;

   bool operator==(TokenType type) const { return type == this->type; }
};

std::string to_string(TokenType type);
