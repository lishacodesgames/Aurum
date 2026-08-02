#include <pch/Precompiled.h>

#include "Token.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   if(argc != 2 || !std::string(argv[1]).ends_with(".or")) {
      std::cerr << std::format("Incorrect usage!\nCorrect usage: {} <file.or>\n", argv[0]);
      return EXIT_FAILURE; // fanciness
   }

   std::cout << std::format("Compiling Orum file '{}'...\n\n", argv[1]);

   std::string orum_src;
   {
      std::ifstream orumFile(argv[1]);
      if(!orumFile) {
         std::cerr << std::format("Error: Could not open file '{}'\n", argv[1]);
         return EXIT_FAILURE;
      }

      std::ostringstream contents; // o because we only want to store, not read
      contents << orumFile.rdbuf(); // read the entire file into the string stream
      orum_src = contents.str();
      // end of scope closes file
   }   

   for(const auto& token: tokenize(orum_src)) {
      std::cout << token.to_string() << '\n';
   }

   return EXIT_SUCCESS;
}
