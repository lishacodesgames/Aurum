#include <pch/Precompiled.h>
#include "Tokenizer.h"

std::optional<char> Tokenizer::peek(int ahead) {
   if(m_pos + ahead >= m_src.size())
      return std::nullopt;
   else
      return m_src.at(m_pos + ahead);
}

char Tokenizer::consume() {
   if(!peek())
      throw std::runtime_error("Tried to consume end of file character!");

   return m_src.at(m_pos++);
}

Token Tokenizer::next() {
   std::string buffer;

   while(peek() && std::isspace(*peek()))
      consume();

   if(!peek()) {
      return Token(TokenType::END_OF_FILE);
   } else if(std::isalpha(*peek())) {
      // assign buffer
      buffer.push_back(consume());

      while(peek() && std::isalnum(*peek()) || *peek() == '_') {
         buffer.push_back(consume());
      }

      // check buffer
      if(buffer == "exit")
         return TokenType::EXIT;
      else // for now
         throw std::runtime_error("Unkown token!");
   } else if(std::isdigit(*peek())) {
      // assign buffer
      buffer.push_back(consume());

      while(peek().has_value() && std::isdigit(*peek())) {
         buffer.push_back(consume());
      }
      /// @todo check buffer  
      return { TokenType::INTEGER_LITERAL, buffer };
   } else if(*peek() == ';') {
      consume();
      return TokenType::SEMICOLON;
   }

   throw std::runtime_error("Unknown character!");
}

void Tokenizer::tokenize() {
   std::string buffer;

   while(peek()) {
      if(std::isalpha(*peek())) {
         // assign buffer
         buffer.push_back(consume());
         while(peek() && (std::isalnum(*peek()) || *peek() == '_' )) {
            buffer.push_back(consume());
         }

         // check buffer
         if(buffer == "exit") {
            m_tokens.emplace_back(TokenType::EXIT);
            buffer.clear();
            continue;
         } else {
            throw std::runtime_error("Unkown token!");
         }
      } else if(std::isdigit(*peek())) {
         // assign buffer
         buffer.push_back(consume());
         while(peek() && std::isdigit(*peek())) {
            buffer.push_back(consume());
         }

         /// @todo check buffer
         m_tokens.emplace_back(TokenType::INTEGER_LITERAL, buffer);
         buffer.clear();
         continue;
      } else if(*peek() == ';') {
         consume();
         m_tokens.emplace_back(TokenType::SEMICOLON);
         continue;
      } else if(std::isspace(*peek())) {
         consume();
         continue;
      } else {
         throw std::runtime_error("Unknown character!");
      }

      // this part should not be reached. Each control flow statement should end in continue
      throw std::runtime_error("Reached end of tokenizer while loop!");
   }
}
