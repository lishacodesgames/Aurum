#include <pch/Precompiled.h>
#include "Token.h"

std::string Token::to_string() const {
   return std::format("{{{0}, {1}}}", ::to_string(type), value ? value.value() : "nullopt");
}

std::string to_string(TokenType type) {
   #define X(name) \
      if(type == TokenType::name) { \
         return #name; \
      }

      TOKEN_TYPES
   #undef X

   return "to_string(TokenType) messed up!";
}

std::vector<Token> tokenize(std::string_view src) {
   std::vector<Token> tokens;
   std::string buffer;

   size_t i = 0; // size_t cuz we're comparing with .size()
   while(i < src.size()) { // not a forloop bcz we will handle incrementing i inside, and that'll be confusing
      /// @note each while loop in the if statements uses ++i and leaves i one character ahead of the current one

      if(std::isalpha(src.at(i))) { // is alphabet
         // assign buffer
         buffer.push_back(src.at(i));
         while(std::isalnum(src.at(++i)) || src.at(i) == '_') { // isalpha() || isdigit(), cuz var names start with letters but can contain both
            buffer.push_back(src.at(i)); // `i` has been incremented already
         }

         // check buffer
         if(buffer == "exit") {
            tokens.emplace_back(TokenType::EXIT);
            buffer.clear();
            continue;
         } else { // for now
            throw std::runtime_error("Unknown token!\n");
         }
      } else if(std::isdigit(src.at(i))) {
         // assign buffer
         buffer.push_back(src.at(i));
         while(std::isdigit(src.at(++i))) {
            buffer.push_back(src.at(i));
         }

         /// @todo check buffer
         tokens.emplace_back(TokenType::INTEGER_LITERAL, buffer);
         buffer.clear();
         continue;
      } else if(src.at(i) == ';') {
         tokens.emplace_back(TokenType::SEMICOLON);
         ++i; // increment i to move past the semicolon
         continue;
      } else if(std::isspace(src.at(i))) {
         ++i; // just skip whitespace
         continue;
      }

      // this part should not be reached. Each control flow previously has a continue;
      throw std::runtime_error( "Reached end of tokenizer while loop!\n");
   }

   return tokens;
}

void toAssembly(const std::vector<Token>& tokens, const std::filesystem::path& assemblyOutputFile) {
   std::string assembly = R"asm(; macOS x86_64, NASM syntax
)asm";
   
   // for now, this part is always in the beginning
   assembly.append(R"asm(
global _main
_main:)asm");

   size_t i = 0;
   while(i < tokens.size()) {
      if(tokens.at(i).type == TokenType::EXIT) {
         if(tokens.at(++i) != TokenType::INTEGER_LITERAL) {
            throw std::runtime_error("Exit code not given!");
         }

         assembly.append(
            std::format(R"asm(
   mov eax, {}
   ret
)asm", tokens.at(i).value.value())); // good practice to end files with 1 newline

         ++i; // move past the integer literal
         continue;
      } else if(tokens.at(i).type == TokenType::SEMICOLON) {
         ++i; // just skip semicolons
         continue;
      } else {
         throw std::runtime_error("Unknown token!");
      }
   }

   {
      std::ofstream outfile(assemblyOutputFile);
      outfile << assembly;
   }
}
