#include <pch/Precompiled.h>
#include "Tokenizer.h"

std::optional<char> Tokenizer::peek(int offset) {
   if(m_pos + offset >= m_src.size())
      return std::nullopt;
   else
      return m_src.at(m_pos + offset);
}

char Tokenizer::consume(uint32_t count) {
   if(!peek())
      throw std::runtime_error("Tried to consume end of file character!");

   char current = m_src.at(m_pos);
   m_pos += count;

   return current;
}

std::expected<std::vector<Token>, std::string>Tokenizer::tokenize() {
   /// @todo use a better, lighter data structure than vector
   std::vector<Token> tokens{};
   std::string buffer;

   while(peek()) {
      if(std::isalpha(*peek())) {
         // assign buffer
         buffer.push_back(consume()); // first character can only be a letter, so we consume that before allowing other characters
         while(peek() && (std::isalnum(*peek()) || *peek() == '_' ))
            buffer.push_back(consume());

         // check buffer
         if(buffer == "exit")
            tokens.emplace_back(TokenType::EXIT);
         else if(buffer == "mint")
            tokens.emplace_back(TokenType::MINT);
         else
            tokens.emplace_back(TokenType::IDENTIFIER, buffer);

         buffer.clear();

      } else if(std::isdigit(*peek())) {
         // assign buffer
         buffer.push_back(consume());
         while(peek() && std::isdigit(*peek()))
            buffer.push_back(consume());

         /// @todo check buffer
         tokens.emplace_back(TokenType::INTEGER_LITERAL, buffer);
         buffer.clear();

      } else if(*peek() == ';') {
         consume();
         tokens.emplace_back(TokenType::SEMICOLON);

      } else if(*peek() == '=') {
         consume();
         tokens.emplace_back(TokenType::EQUALS);

      } else if(std::isspace(*peek())) {
         consume();

      } else {
         return std::unexpected(std::format("Unknown character '{}'!", consume()));
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE);
   return tokens;
}
