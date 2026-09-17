#include <pch/Precompiled.h>
#include "Lexer.h"

#include "Errors.h"

namespace
{
   TokenType getKeyword(const std::string& buffer) {
      if(buffer == "mint")
         return TokenType::MINT;
      else if(buffer == "bar")
         return TokenType::BAR;
      else if(buffer == "if")
         return TokenType::IF;
      else if(buffer == "elif")
         return TokenType::ELIF;
      else if(buffer == "else")
         return TokenType::ELSE;
      else if(buffer == "exit")
         return TokenType::EXIT;

      else if(buffer == "True")
         return TokenType::TRUE;
      else if(buffer == "False")
         return TokenType::FALSE;

      else if(buffer == "None")
         return TokenType::NONE;
      else if(buffer == "int")
         return TokenType::INT;
      else if(buffer == "bool")
         return TokenType::BOOL;

      else
         return TokenType::IDENTIFIER;
   }

   TokenType getNumericType(const std::string& buffer) {
      /// @todo float vs int type
      return TokenType::INTEGER_LITERAL;
   }
}

std::optional<char> Lexer::peek(int offset) const noexcept {
   int targetPos = static_cast<int>(m_pos) + offset;

   if(targetPos < 0 || targetPos >= static_cast<int>(m_src.size())) // \0 char shouldn't be counted
      return std::nullopt;

   return m_src[targetPos]; // use [] when we've checked bounds ourselves to avoid the bounds-checking overhead in .at()
}

char Lexer::consume(std::uint32_t count) noexcept {
   assert(peek(count - 1));

   char current = m_src[m_pos];
   while(count != 0) {
      if(m_src.at(m_pos) == '\n') {
         m_location.column = 1;
         m_location.row++;
      } else {
         m_location.column++;
      }

      m_pos++;
      count--;
   }

   return current;
}

std::vector<Token> Lexer::tokenize() {
   std::vector<Token> tokens{};
   tokens.reserve(m_src.size() / 3); // rough heuristic of 3 chars per token
   std::string buffer;

   while(peek()) {
      err::SourceLocation location = m_location; // save location of beginning

      if(std::isalpha(*peek()) || *peek() == '_') {
         do buffer.push_back(consume());
         while(peek() && (std::isalnum(*peek()) || *peek() == '_' ));

         tokens.emplace_back(getKeyword(buffer), buffer, location);
         buffer.clear();

      } else if(std::isdigit(*peek())) {
         /// @todo allow float type
         do buffer.push_back(consume());
         while(peek() && std::isdigit(*peek()));

         tokens.emplace_back(getNumericType(buffer), buffer, location);
         buffer.clear();

      } else if(std::isspace(static_cast<unsigned char>(*peek()))) {
         do consume();
         while(peek() && std::isspace(static_cast<unsigned char>(*peek())));

      } else if(*peek() == '$') {
         consume();
         switch(*peek()) {
            case '$':
               do consume();
               while(peek() && *peek() != '\n');

               if(peek())
                  consume(); // consume newline
               break;

            case '~':
               do consume();
               while(peek() && *peek() != '~');

               if(!peek() || *peek() != '~' || !peek(1) || *peek(1) != '$')
                  g_errors.report(err::Phase::LEXING, err::Category::SYNTAX, location, "Multi-line comment unclosed!", true);

               consume(2); // consume '~$'
               break;

            default:
               continue; // while loop will handle unknown character error
         }

      } else {
         switch(consume()) {
            case '=':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::EQUALITY, location);
               } else {
                  tokens.emplace_back(TokenType::EQUALS, location);
               }
               break;

            case '<':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::LESS_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::LESS_THAN, location);
               }
               break;

            case '>':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::GREATER_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::GREATER_THAN, location);
               }
               break;

            case '+':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::PLUS_EQUALS, location);
               } else if(peek() && *peek() == '+') {
                  consume();
                  tokens.emplace_back(TokenType::INCREMENT, location);
               } else {
                  tokens.emplace_back(TokenType::PLUS, location);
               }
               break;

            case '-':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::MINUS_EQUALS, location);
               } else if(peek() && *peek() == '-') {
                  consume();
                  tokens.emplace_back(TokenType::DECREMENT, location);
               } else {
                  tokens.emplace_back(TokenType::MINUS, location);
               }
               break;

            case '*':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::STAR_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::STAR, location);
               }
               break;

            case '/':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::SLASH_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::FSLASH, location);
               }
               break;

            case '%':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::PERCENT_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::PERCENT, location);
               }
               break;

            case '^':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::CARET_EQUALS, location);
               } else {
                  tokens.emplace_back(TokenType::CARET, location);
               }
               break;

            case '!':
               if(peek() && *peek() == '=') {
                  consume();
                  tokens.emplace_back(TokenType::INEQUALITY, location);
               } else {
                  tokens.emplace_back(TokenType::LOGICAL_NOT, location);
               }
               break;

            case '&':
               if(peek() && *peek() == '&') {
                  consume();
                  tokens.emplace_back(TokenType::LOGICAL_AND, location);
               } else {
                  g_errors.report(
                     err::Phase::LEXING, err::Category::SYNTAX, { "Lexer.cpp", __LINE__ },
                     std::format("Unexpected character '{}' after '&'!", *peek()));
               }
               break;

            case '|':
               if(peek() && *peek() == '|') {
                  consume();
                  tokens.emplace_back(TokenType::LOGICAL_OR, location);
               } else {
                  g_errors.report(
                     err::Phase::LEXING, err::Category::SYNTAX, { "Lexer.cpp", __LINE__ },
                     std::format("Unexpected character '{}' after '|'!", *peek()));
               }
               break;

            case ':': tokens.emplace_back(TokenType::COLON, location);         break;
            case ';': tokens.emplace_back(TokenType::SEMICOLON, location);     break;
            case '(': tokens.emplace_back(TokenType::OPEN_PAREN, location);    break;
            case ')': tokens.emplace_back(TokenType::CLOSE_PAREN, location);   break;
            case '[': tokens.emplace_back(TokenType::OPEN_BRACKET, location);  break;
            case ']': tokens.emplace_back(TokenType::CLOSE_BRACKET, location); break;
            case '{': tokens.emplace_back(TokenType::OPEN_CURLY, location);    break;
            case '}': tokens.emplace_back(TokenType::CLOSE_CURLY, location);   break;

            case '\\': tokens.emplace_back(TokenType::BSLASH, location);       break;

            default:
               g_errors.report(
                  err::Phase::LEXING, err::Category::SYNTAX, location,
                  std::format("Unexpected character '{}'!", *peek(-1)));
               break;
         }
      }
   }

   tokens.emplace_back(TokenType::END_OF_FILE, m_location);
   return tokens;
}
