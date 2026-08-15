#pragma once

#define TOKEN_TYPES \
   /* Keywords */ \
   X(MINT) \
   X(BAR) \
   X(EXIT) \
\
   /* Literals & identifiers */ \
   X(INTEGER_LITERAL) \
   X(IDENTIFIER) \
\
   /* Punctuation */ \
   X(EQUALS) \
   X(COLON) \
   X(SEMICOLON) \
\
   /* Brackets */ \
   X(OPEN_PAREN)   X(CLOSE_PAREN)   /* () */ \
   X(OPEN_BRACKET) X(CLOSE_BRACKET) /* [] */ \
   X(OPEN_CURLY)   X(CLOSE_CURLY)   /* {} */ \
   X(LESS_THAN)    X(GREATER_THAN)  /* <> */ \
\
   /* Operators */ \
   X(PLUS) X(MINUS) X(STAR) X(SLASH) X(PERCENT) /* (+ - * / %) */ \
   X(EQUALITY) X(INEQUALITY) X(LESS_EQUALS) X(GREATER_EQUALS) /* (== != <= >=) */ \
   X(LOGICAL_AND) X(LOGICAL_OR) X(LOGICAL_NOT) /* (&& || !) */ \
\
   /* Compound assignment (+= -= *= /= %=) */ \
   X(PLUS_EQUALS) X(MINUS_EQUALS) X(STAR_EQUALS) X(SLASH_EQUALS) X(PERCENT_EQUALS) \
\
   /* Not sure abt but do exist */ \
   X(BITWISE_AND) X(BITWISE_OR) X(BITWISE_NOT) X(BITWISE_XOR) \
   X(LEFT_SHIFT) X(RIGHT_SHIFT) \
\
   /* Special */ \
   X(END_OF_FILE) /* @todo implement end of file */

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
