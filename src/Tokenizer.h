#pragma once
#include "Token.h"

class Tokenizer {
public:
   explicit Tokenizer(std::string_view src) : m_src(src) {}

   std::vector<Token> tokenize();

private:
   std::string m_src;
   std::size_t m_pos = 0;
   SourceLocation m_location;

private:
   [[nodiscard]] std::optional<char> peek(int offset = 0) const noexcept;

   /** 
    * @brief increments m_pos but returns current character
    * @param count by how much to increment m_pos
    * @returns CURRENT char
    * @throws runtime_error if next char doesn't exist, so check with peek() before calling
    */
   char consume(std::uint32_t count = 1U) noexcept;

private:
   // --- HELPERS ---
   void emplaceKeyword(std::vector<Token>& tokens, std::string& buffer);
   void emplaceNumber(std::vector<Token>& tokens, std::string& buffer);
   void emplaceChar(std::vector<Token>& tokens, char current);
};
