#include <pch/Precompiled.h>

#include "Tokenizer.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string orumFile = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !orumFile.ends_with(".or") || orumFile.find('/') != std::string::npos) {
      std::println("Incorrect usage!");
      std::println("Correct usage: {} <file.or>, without any slashes", argv[0]);
      return EXIT_FAILURE;
   }
   std::string fileBaseName = orumFile.substr(0, orumFile.find_last_of('.'));
   std::filesystem::path outputDir("out");
   std::filesystem::create_directories(outputDir); // does nothing if it already exists
   
   std::println("Compiling Orum file '{}'...", orumFile);
   std::string orum_src;
   {
      std::ifstream file(orumFile);
      if(!file) {
         throw std::runtime_error(std::format("Could not open file '{}'", orumFile));
      }

      std::ostringstream contents; // osstream because we only want to store, not read
      contents << file.rdbuf(); // read the entire file into the string stream
      orum_src = contents.str();
      // end of scope closes file
   }

   Tokenizer tokenizer(orum_src);

   std::filesystem::path assemblyFile = outputDir / (fileBaseName + ".asm");
   toAssembly(tokenizer.getTokens(), assemblyFile);
   std::println("Successfully compiled to assembly file '{}'", assemblyFile.string());

   if(!std::filesystem::exists("scripts")) {
      std::println("Please run from the root of the project, where the 'scripts' folder is located.");
      return EXIT_FAILURE;
   }

   std::string compileAssemblyCommand = std::string("./scripts/compile_assembly_mac_x64.sh ") + assemblyFile.string() + " ./out"; // check args of the script
   std::string executablePath = (outputDir / fileBaseName).string();

   std::println("Compiling assembly file '{}' to executable...", executablePath);
   system(compileAssemblyCommand.c_str()); // compile the assembly file to an executable
   
   std::println("\n(Ignore the ld warning)");
   std::println("Successfully compiled to executable '{}'", executablePath);
   std::println("\nCurrently, our cute little Orum file does nothing except exit with code 40.");
   std::println("To confirm this, run the executable ('./{}') and check the exit code with 'echo $?'", executablePath);

   return EXIT_SUCCESS; // fanciness
}
