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
   X(PLUS) X(MINUS) X(STAR) X(FSLASH) X(PERCENT) X(CARET) /* (+ - * / % ^) */ \
   X(EQUALITY) X(INEQUALITY) X(LESS_EQUALS) X(GREATER_EQUALS) /* (== != <= >=) */ \
   X(LOGICAL_AND) X(LOGICAL_OR) X(LOGICAL_NOT) /* (&& || !) */ \
   X(INCREMENT) X(DECREMENT) /* (++ --) */ \
\
   /* Compound assignment (+= -= *= /= %=) */ \
   X(PLUS_EQUALS) X(MINUS_EQUALS) X(STAR_EQUALS) X(SLASH_EQUALS) X(PERCENT_EQUALS) \
\
   /* Not sure abt but do exist */ \
   X(BSLASH) \
\
   /* Special */ \
   X(END_OF_FILE)

enum class TokenType {
   #define X(name) name,
      TOKEN_TYPES
   #undef X
};

std::string to_string(TokenType type);
std::string getCharsOf(TokenType type); /// eg. returns ( for OPEN_PAREN

bool isBinaryOperator(TokenType type);
bool isUnaryOperator(TokenType type);

int getPrecedence(TokenType type);
bool isLeftAssociative(TokenType type);

struct Token {
   TokenType type;
   std::optional<std::string> value = std::nullopt; /// @todo change type. Template it maybe

   Token(TokenType type) : type(type) {} // for implicit conversion
   Token(TokenType type, std::string value) : type(type), value(value) {}

   std::string to_string() const;

   bool operator==(TokenType type) const noexcept { return type == this->type; }
};
