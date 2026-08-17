#include <pch/Precompiled.h>
#include "Tokenizer.h"

#include "Errors.h"

std::optional<char> Tokenizer::peek(int offset) const noexcept {
   int targetPos = static_cast<int>(m_pos) + offset;

   if(targetPos < 0 || targetPos >= static_cast<int>(m_src.size())) // \0 char shouldn't be counted
      return std::nullopt;

   return m_src[targetPos]; // use [] when we've checked bounds ourselves to avoid the bounds-checking overhead in .at()
}

char Tokenizer::consume(uint32_t count) {
   if(!peek(count - 1))
      FATAL_ERROR("Tried to consume end of file character!");

   char current = m_src[m_pos];
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

      } else if(*peek() == '$') { // comments. ignored in compilation completely
         consume();

         if(peek() && *peek() == '$') { // inline comment "$$ .."
            consume();
            while(peek() && *peek() != '\n') {
               consume();
            }

         } else if(peek() && *peek() == '~') { // multi-line comment "$~ ... ~$"
            consume();
            while(*peek() != '~') {
               consume();

               if(!peek()) // unexpected end of file
                  return std::unexpected("Unexpected end of file!\nExpected a `~`!");
            }

            if(!peek(1) || *peek(1) != '$')
               return std::unexpected("Expected a `$`!");
            consume(2);

         } else {
            return std::unexpected("Expected a `$` or `~`!");
         }

      } else if(*peek() == ';') {
         consume();
         tokens.emplace_back(TokenType::SEMICOLON);

      } else if(*peek() == '=') {
         consume();
         tokens.emplace_back(TokenType::EQUALS);

      /// @todo increment & decrement operators
      } else if(*peek() == '-') {
         consume();
         tokens.emplace_back(TokenType::MINUS);

      } else if(*peek() == '+') {
         consume();
         tokens.emplace_back(TokenType::PLUS);

      } else if(std::isspace(*peek())) {
         consume();

      } else {
         return std::unexpected(std::format("Unknown character '{}'!", consume()));
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE);
   return tokens;
}
