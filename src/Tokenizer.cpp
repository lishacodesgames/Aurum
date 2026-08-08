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
         throw std::runtime_error(std::format("Unkown token '{}'!", buffer));
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

   throw std::runtime_error(std::format("Unknown character '{}'!", consume()));
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
            throw std::runtime_error(std::format("Unkown token '{}'!", buffer));
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
      }

      throw std::runtime_error(std::format("Unknown character '{}'!", consume()));
   }
}
