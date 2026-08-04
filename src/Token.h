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

   bool operator==(TokenType type) const { return type == this->type; }
};

std::string to_string(TokenType type);

void toAssembly(const std::vector<Token>& tokens, const std::string& assemblyOutputFile);
