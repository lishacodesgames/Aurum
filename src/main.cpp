#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Generator.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string orumFile = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !orumFile.ends_with(".or") || orumFile.find('/') != std::string::npos) {
      std::println("Incorrect usage!");
      std::println("Correct usage: {} <file.or>, without any slashes", argv[0]);
      return EXIT_FAILURE;
   }

   // set up output paths
   if(!std::filesystem::exists("scripts")) {
      std::println("Please run from the root of the project, where the 'scripts' folder is located.");
      return EXIT_FAILURE;
   }

   std::string fileBaseName = orumFile.substr(0, orumFile.find_last_of('.'));
   std::filesystem::path outputDir("out");
   std::filesystem::create_directories(outputDir); // does nothing if it already exists
   std::filesystem::path assemblyFile = outputDir / (fileBaseName + ".asm");
   
   // get orum source code
   std::println("Compiling Orum file '{}'...", orumFile);
   std::string orum_src;
   {
      std::ifstream srcFile(orumFile);
      if(!srcFile) {
         throw std::runtime_error(std::format("Could not open file '{}'", orumFile));
      }

      std::ostringstream contents; // osstream because we only want to store, not read
      contents << srcFile.rdbuf(); // read the entire file into the string stream
      orum_src = contents.str();
      // end of scope closes file
   }

   // parse & generate assembly
   Tokenizer tokenizer(orum_src);
   Parser parser(tokenizer.releaseTokens());
   auto rootNode = parser.parse();
   if(!rootNode)
      throw std::runtime_error(rootNode.error());

   Generator generator(*rootNode);
   std::string assembly = generator.generate();

   // output assembly to assemblyFile
   {
      std::ofstream outAssembly(assemblyFile);
      outAssembly << assembly;
      // end of scope closes file
   }

   std::println("Successfully compiled to assembly file '{}'", assemblyFile.string());

   // compile assembly into executable
   std::string compileAssemblyCommand = std::format("./scripts/compile_assembly_mac_x64.sh {} ./out", assemblyFile.string()); // check args of the script
   std::string executablePath = (outputDir / fileBaseName).string();

   std::println("Compiling assembly file '{}' to executable...", executablePath);
   system(compileAssemblyCommand.c_str()); // compile the assembly file to an executable
   
   // disclaimers for the user
   std::println("\n(Ignore the ld warning)");
   std::println("Successfully compiled to executable '{}'", executablePath);
   std::println("\nCurrently, our cute little Orum file does nothing except exit with code 40.");
   std::println("To confirm this, run the executable ('./{}') and check the exit code with 'echo $?'", executablePath);

   return EXIT_SUCCESS; // fanciness
}
