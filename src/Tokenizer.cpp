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

void Tokenizer::emplaceKeyword(std::vector<Token>& tokens, std::string& buffer) {
   if(buffer == "mint")
      tokens.emplace_back(TokenType::MINT);
   else if(buffer == "bar")
      tokens.emplace_back(TokenType::BAR);
   else if(buffer == "exit")
      tokens.emplace_back(TokenType::EXIT);
   else
      tokens.emplace_back(TokenType::IDENTIFIER, buffer);

   buffer.clear();
}

void Tokenizer::emplaceNumber(std::vector<Token>& tokens, std::string& buffer) {
   /// @todo float vs int type

   tokens.emplace_back(TokenType::INTEGER_LITERAL, buffer);
   buffer.clear();
}

std::expected<std::vector<Token>, std::string>Tokenizer::tokenize() {
   /// @todo use a better, lighter data structure than vector
   std::vector<Token> tokens{};
   std::string buffer;

   while(peek()) {
      if(std::isalpha(*peek()) || *peek() == '_') {
         do buffer.push_back(consume());
         while(peek() && (std::isalnum(*peek()) || *peek() == '_' ));

         emplaceKeyword(tokens, buffer);

      } else if(std::isdigit(*peek())) {
         /// @todo allow float type
         do buffer.push_back(consume());
         while(peek() && std::isdigit(*peek()));

         emplaceNumber(tokens, buffer);

      } else if(*peek() == '$') { // comments. ignored in compilation completely
         consume();
         switch(*peek()) {
            case '$':
               do consume();
               while(peek() && *peek() != '\n');

               consume(); // consume newline
               break;

            case '~':
               do consume();
               while(peek() && peek() != '~');

               if(!peek(1) || *peek(1) != '$')
                  return std::unexpected("Comment unclosed at end of file!");
               consume(2); // consume ~$

            default:
               continue; // the while loop's else case will handle unknown character error msg
         }

      } else if(std::isspace(static_cast<unsigned char>(*peek()))) {
         do consume();
         while(peek() && std::isspace(static_cast<unsigned char>(*peek())));

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
               tokens.emplace_back(TokenType::FSLASH);
               break;

            case '%':
               tokens.emplace_back(TokenType::PERCENT);
               break;

            case '^':
               tokens.emplace_back(TokenType::CARET);
               break;

            case '(':
               tokens.emplace_back(TokenType::OPEN_PAREN);
               break;

            case ')':
               tokens.emplace_back(TokenType::CLOSE_PAREN);
               break;

            case '{':
               tokens.emplace_back(TokenType::OPEN_CURLY);
               break;

            case '}':
               tokens.emplace_back(TokenType::CLOSE_CURLY);
               break;

            default:
               return std::unexpected(std::format("Unexpected character '{}'!", next));
         }
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE);
   return tokens;
}
