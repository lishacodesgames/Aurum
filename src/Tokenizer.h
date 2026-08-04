#pragma once
#include "Token.h"

class Tokenizer {
public:
   explicit Tokenizer(std::string_view src) : m_src(src) {
      tokenize();
   }

   std::vector<Token> getTokens() const { return m_tokens; } /// Return all tokens

private:
   std::string_view m_src;
   size_t m_pos = 0;

   /// @todo use a better, lighter data structure than vector
   std::vector<Token> m_tokens{};

private:
   std::optional<char> peek(int ahead = 0);

   /// @brief increments m_pos
   /// @returns CURRENT char
   /// @throws runtime_error if next char doesn't exist, so check with peek() before calling
   char consume();

   /// Tokenize the source code into m_tokens
   void tokenize();
};
