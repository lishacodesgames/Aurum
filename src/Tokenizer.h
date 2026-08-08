#pragma once
#include "Token.h"

/// @todo taking negative integer literals
class Tokenizer {
public:
   explicit Tokenizer(std::string_view src) : m_src(src) {
      tokenize();
   }

   /// @warning DO NOT use tokens after releasing them. Stored vector is emptied.
   /// @todo unnecessary, I think. Change this.
   std::vector<Token> releaseTokens() const { return std::move(m_tokens); }

private:
   std::string_view m_src;
   size_t m_pos = 0;

   /// @todo use a better, lighter data structure than vector
   std::vector<Token> m_tokens{};

private:
   std::optional<char> peek(int offset = 0);

   /** 
    * @brief increments m_pos but returns current character
    * @param count by how much to increment m_pos
    * @returns CURRENT char
    * @throws runtime_error if next char doesn't exist, so check with peek() before calling
    */ 
   char consume(uint32_t count = 1);

   Token next(); // needed?

   /// @todo make public, change class as needed
   void tokenize();
};
