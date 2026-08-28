#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Generator.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string aurumFilePath = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !aurumFilePath.ends_with(".aura"))
      FATAL_ERROR("Incorrect usage!\nCorrect usage: {} <file.aura>", argv[0]);

   // set up output paths
   if(!std::filesystem::exists("scripts"))
      FATAL_ERROR("Please run from the root of the project, where the 'scripts' folder is located.");

   std::filesystem::path aurumFile(aurumFilePath);
   std::string fileBaseName = aurumFile.stem().string();

   std::filesystem::path outputDir("out");
   std::filesystem::create_directories(outputDir); // does nothing if it already exists
   std::filesystem::path assemblyFile = outputDir / (fileBaseName + ".asm");
   std::filesystem::path executablePath = outputDir / fileBaseName;

   // get Aurum source code
   std::println("Compiling aurum file '{}'...", aurumFile.string());
   std::string contents;
   {
      std::ifstream srcFile(aurumFile);
      if(!srcFile)
         FATAL_ERROR("Could not open file '{}'!", aurumFile.string());

      std::ostringstream src; // osstream because we only want to store, not read
      src << srcFile.rdbuf(); // read the entire file into the string stream
      contents = src.str();

      if(contents.empty())
         FATAL_ERROR("Empty Aurum file: {}!", aurumFile.string());
      // end of scope closes file
   }

   // parse & generate assembly
   Tokenizer tokenizer(contents);
   auto tokens = tokenizer.tokenize();
   if(!tokens)
      FATAL_ERROR("{}", tokens.error());

   Parser parser(std::move(*tokens));
   auto program = parser.parse();
   if(!program)
      FATAL_ERROR("{}", program.error());

   Generator generator(std::move(*program));
   auto assembly = generator.generate();
   if(!assembly)
      FATAL_ERROR("{}", assembly.error());

   // output assembly to assemblyFile
   {
      std::ofstream outAssembly(assemblyFile);
      outAssembly << *assembly;
      // end of scope closes file
   }

   std::println("Successfully compiled to assembly file '{}'!\n", assemblyFile.string());

   // compile assembly into executable
   std::vector<std::string> libArgs = generator.getRequiredLibs();
   std::string compileAssemblyCommand = std::format("./scripts/compile_nasm_mac_x64.sh {}", assemblyFile.string()); // check args of the script
   for(const std::string& file : libArgs)
      compileAssemblyCommand += std::format(" {}", file);

   std::println("Compiling assembly file '{}' to executable...", executablePath.string());
   
   int assembleResult = std::system(compileAssemblyCommand.c_str()); // compile the assembly file to an executable
   if(!assembleResult) {
      std::println("Successfully assembled to executable '{}'!", executablePath.string());
   } else {
      std::println("Assembling failed! Exit code: {}", assembleResult);
      return EXIT_FAILURE;
   }

   std::string runExecutableCommand = std::format("./{}", executablePath.string());
   std::print("\nRunning compiled executable '{}'...\n\n\033[38:5:80m", runExecutableCommand); // prints any output of executable in teal
   int result = std::system(runExecutableCommand.c_str());
   std::println("\033[0m");

   // On Linux and macOS, std::system does not return the program's raw exit code directly. Instead, it returns a 16-bit wait status integer encoded by the operating system
   // to get the real exit code, we must divide by 256
   std::println("Successfully ran executable! Exited with exit code: {}", result / 256);

   return EXIT_SUCCESS;
}
