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

void Tokenizer::tokenize() {
   std::string buffer;

   while(peek().has_value()) {
      if(std::isalpha(peek().value())) {
         // assign buffer
         buffer.push_back(consume());

         while(peek().has_value() && std::isalnum(peek().value()) || peek().value() == '_') {
            buffer.push_back(consume());
         }

         // check buffer
         if(buffer == "exit") {
            m_tokens.emplace_back(TokenType::EXIT);
            buffer.clear();
            continue;
         } else { // for now
            // throw std::runtime_error("Unknown token!");
         }
      } else if(std::isdigit(peek().value())) {
         // assign buffer
         buffer.push_back(consume());

         while(peek().has_value() && std::isdigit(peek().value())) {
            buffer.push_back(consume());
         }

         /// @todo check buffer
         m_tokens.emplace_back(TokenType::INTEGER_LITERAL, buffer);
         buffer.clear();
         continue;
      } else if(peek().value() == ';') {
         m_tokens.emplace_back(TokenType::SEMICOLON);
         consume();
         continue;
      } else if(std::isspace(peek().value())) {
         consume();
         continue;
      }

      // this part should not be reached. Each control flow statement should end in continue
      throw std::runtime_error("Reached enf of tokenizer while loop!");
   }
}
