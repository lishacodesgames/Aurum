#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Generator.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string aurumFile = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !aurumFile.ends_with(".arum") || aurumFile.find('/') != std::string::npos) {
      std::println("Incorrect usage!");
      std::println("Correct usage: {} <file.arum>, without any slashes", argv[0]);
      return EXIT_FAILURE;
   }

   // set up output paths
   if(!std::filesystem::exists("scripts")) {
      std::println("Please run from the root of the project, where the 'scripts' folder is located.");
      return EXIT_FAILURE;
   }

   std::string fileBaseName = aurumFile.substr(0, aurumFile.find_last_of('.'));
   std::filesystem::path outputDir("out");
   std::filesystem::create_directories(outputDir); // does nothing if it already exists
   std::filesystem::path assemblyFile = outputDir / (fileBaseName + ".asm");
   
   // get Aurum source code
   std::println("Compiling aurum file '{}'...", aurumFile);
   std::string aurum_src;
   {
      std::ifstream srcFile(aurumFile);
      if(!srcFile) {
         throw std::runtime_error(std::format("Could not open file '{}'", aurumFile));
      }

      std::ostringstream contents; // osstream because we only want to store, not read
      contents << srcFile.rdbuf(); // read the entire file into the string stream
      aurum_src = contents.str();
      // end of scope closes file
   }

   // parse & generate assembly
   Tokenizer tokenizer(aurum_src);
   Parser parser(tokenizer.releaseTokens());
   auto rootNode = parser.parse();
   if(!rootNode)
      throw std::runtime_error(rootNode.error());

   Generator generator(std::move(*rootNode));
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
   std::println("\nCurrently, our cute little Aurum file does nothing except exit with an integer value.");
   std::println("To confirm this, run the executable ('./{}') and check the exit code with 'echo $?'", executablePath);

   return EXIT_SUCCESS; // fanciness
}
