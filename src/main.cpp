#include <iostream>
#include <format>
#include <string>

int main(int argc, char* argv[]) {
   if(argc != 2 || !std::string(argv[1]).ends_with(".or")) {
      std::cerr << std::format("Incorrect usage!\nCorrect usage: {} <file.or>\n", argv[0]);
      return 1;
   }

   std::cout << std::format("Compiling '{}'...\n", argv[1]);
   return 0;
}
