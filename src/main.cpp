#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Generator.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string aurumFile = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !aurumFile.ends_with(".aura") || aurumFile.find('/') != std::string::npos)
      FATAL_ERROR("Incorrect usage!\nCorrect usage: {} <file.aura>, without any slashes", argv[0]);

   // set up output paths
   if(!std::filesystem::exists("scripts"))
      FATAL_ERROR("Please run from the root of the project, where the 'scripts' folder is located.");

   std::string fileBaseName = aurumFile.substr(0, aurumFile.find_last_of('.'));
   std::filesystem::path outputDir("out");
   std::filesystem::create_directories(outputDir); // does nothing if it already exists
   std::filesystem::path assemblyFile = outputDir / (fileBaseName + ".asm");

   // get Aurum source code
   std::println("Compiling aurum file '{}'...", aurumFile);
   std::string contents;
   {
      std::ifstream srcFile(aurumFile);
      if(!srcFile) {
         std::println(stderr, "Could not open file '{}'!", aurumFile);
         return EXIT_FAILURE;
      }

      std::ostringstream src; // osstream because we only want to store, not read
      src << srcFile.rdbuf(); // read the entire file into the string stream
      contents = src.str();

      if(contents.empty())
         FATAL_ERROR("Empty Aurum file!");
      // end of scope closes file
   }

   // parse & generate assembly
   Tokenizer tokenizer(contents);
   auto tokens = tokenizer.tokenize();
   if(!tokens)
      FATAL_ERROR(tokens.error());

   Parser parser(std::move(*tokens));
   auto program = parser.parse();
   if(!program)
      FATAL_ERROR(program.error());

   Generator generator(std::move(*program));
   auto assembly = generator.generate();
   if(!assembly)
      FATAL_ERROR(assembly.error());

   // output assembly to assemblyFile
   {
      std::ofstream outAssembly(assemblyFile);
      outAssembly << *assembly;
      // end of scope closes file
   }

   std::println("Successfully compiled to assembly file '{}'", assemblyFile.string());

   // compile assembly into executable
   std::string compileAssemblyCommand = "./scripts/compile_nasm_mac_x64.sh " + assemblyFile.string(); // check args of the script
   std::string executablePath = (outputDir / fileBaseName).string();

   std::println("Compiling assembly file '{}' to executable...", executablePath);
   system(compileAssemblyCommand.c_str()); // compile the assembly file to an executable

   // disclaimers for the user
   std::println("\n(Ignore the ld warning)");
   std::println("Successfully compiled to executable '{}'", executablePath);
   std::println("\nCurrently, our cute little Aurum file does nothing except exit with an integer value.");
   std::println("To confirm this, run the executable ('./{}') and check the exit code with 'echo $?'", executablePath);

   return EXIT_SUCCESS;
}
