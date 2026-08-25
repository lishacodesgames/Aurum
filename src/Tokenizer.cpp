#include <pch/Precompiled.h>
#include "Tokenizer.h"

#include "Errors.h"

std::optional<char> Tokenizer::peek(int offset) const noexcept {
   int targetPos = static_cast<int>(m_pos) + offset;

   if(targetPos < 0 || targetPos >= static_cast<int>(m_src.size())) // \0 char shouldn't be counted
      return std::nullopt;

   return m_src[targetPos]; // use [] when we've checked bounds ourselves to avoid the bounds-checking overhead in .at()
}

char Tokenizer::consume(std::uint32_t count) noexcept {
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
         else if(buffer == "bar")
            tokens.emplace_back(TokenType::BAR);
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
         if(*peek() == '$') { // inline comment "$$ .."
            consume();
            while(peek() && *peek() != '\n') {
               consume();
            }

         } else if(*peek() == '~') { // multi-line comment "$~ ... ~$"
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

      } else if(std::isspace(static_cast<unsigned char>(*peek()))) {
         consume();

      } else {
         char next = consume();

         switch(next) {
            case ';':
               tokens.emplace_back(TokenType::SEMICOLON);
               break;

            case '=':
               tokens.emplace_back(TokenType::EQUALS);
               break;

            case '-':
               if(peek() == '-') {
                  consume();
                  tokens.emplace_back(TokenType::DECREMENT);
               } else {
                  tokens.emplace_back(TokenType::MINUS);
               }

               break;

            case '+':
               if(peek() == '+') {
                  consume();
                  tokens.emplace_back(TokenType::INCREMENT);
               } else {
                  tokens.emplace_back(TokenType::PLUS);
               }

               break;

            case '*':
               tokens.emplace_back(TokenType::STAR);
               break;

            case '/':
               tokens.emplace_back(TokenType::SLASH);
               break;

            case '%':
               tokens.emplace_back(TokenType::PERCENT);
               break;

            case '(':
               tokens.emplace_back(TokenType::OPEN_PAREN);
               break;

            case ')':
               tokens.emplace_back(TokenType::CLOSE_PAREN);
               break;

            case '^':
               tokens.emplace_back(TokenType::CARET);
               break;

            default:
               return std::unexpected(std::format("Unknown character '{}'!", next));
         }
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE);
   return tokens;
}
