#pragma once

#define TOKEN_TYPES \
   X(EXIT) \
   X(INTEGER_LITERAL) \
   X(SEMICOLON)

enum class TokenType {
   #define X(name) name,
      TOKEN_TYPES
   #undef X
};

struct Token {
   TokenType type;
   std::optional<std::string> value = std::nullopt; /// @todo change type. Template it maybe
   
   std::string to_string() const;
};

std::string to_string(TokenType type);
std::vector<Token> tokenize(std::string_view src);
