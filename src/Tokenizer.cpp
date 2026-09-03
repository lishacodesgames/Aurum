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
      m_reporter.report(Phase::TOKENIZING, Category::INTERNAL, m_location, "Tried to consume end of file character!", true);

   char current = m_src[m_pos];
   m_pos += count;

   while(count != 0) {
      if(m_src.at(m_pos) == '\n') {
         m_location.row = 0;
         m_location.column++;
      } else {
         m_location.row++;
      }

      m_pos++;
      count--;
   }

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

void Tokenizer::emplaceChar(std::vector<Token>& tokens, char current) {
   switch(current) {
      case '$':
         switch(*peek()) {
            case '$':
               do consume();
               while(peek() && *peek() != '\n');

               consume(); // consume newline
               break;

            case '~':
               do consume();
               while(peek() && peek() != '~');

               consume(); // consume '~'
               if(!peek() || *peek() != '$')
                  m_reporter.report(Phase::TOKENIZING, Category::SYNTAX, m_location, "Multi-line comment unclosed!", true);

               consume(); // consume '$'
               break;

            default:
               return; // the while loop's else case will handle unknown character error msg
         }

      case '-':
         if(peek() == '-') {
            consume();
            tokens.emplace_back(TokenType::DECREMENT, m_location);
         } else {
            tokens.emplace_back(TokenType::MINUS, m_location);
         }
         break;

      case '+':
         if(peek() == '+') {
            consume();
            tokens.emplace_back(TokenType::INCREMENT, m_location);
         } else {
            tokens.emplace_back(TokenType::PLUS, m_location);
         }
         break;

      case ';':
         tokens.emplace_back(TokenType::SEMICOLON, m_location);
         break;

      case '=':
         tokens.emplace_back(TokenType::EQUALS, m_location);
         break;

      case '*':
         tokens.emplace_back(TokenType::STAR, m_location);
         break;

      case '/':
         tokens.emplace_back(TokenType::FSLASH, m_location);
         break;

      case '%':
         tokens.emplace_back(TokenType::PERCENT, m_location);
         break;

      case '^':
         tokens.emplace_back(TokenType::CARET, m_location);
         break;

      case '(':
         tokens.emplace_back(TokenType::OPEN_PAREN, m_location);
         break;

      case ')':
         tokens.emplace_back(TokenType::CLOSE_PAREN, m_location);
         break;

      case '{':
         tokens.emplace_back(TokenType::OPEN_CURLY, m_location);
         break;

      case '}':
         tokens.emplace_back(TokenType::CLOSE_CURLY, m_location);
         break;

      default:
         m_reporter.report(Phase::TOKENIZING, Category::SYNTAX, m_location, std::format("Unexpected character '{}'!", current), true);
   }
}

std::vector<Token> Tokenizer::tokenize() {
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

      } else if(std::isspace(static_cast<unsigned char>(*peek()))) {
         do consume();
         while(peek() && std::isspace(static_cast<unsigned char>(*peek())));

      } else {
         emplaceChar(tokens, consume());
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE);
   return tokens;
}
