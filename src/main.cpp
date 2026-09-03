#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"
#include "AsmEmitter.h"
#include "FileHandler.h"
#include "Errors.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string aurumFilePath = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !aurumFilePath.ends_with(".aura")) {
      std::println("Incorrect usage!\nCorrect usage: {} /path/to/file.aura", argv[0]);
      return EXIT_FAILURE;
   }

   FileHandler fileHandler(aurumFilePath);
   std::println("Compiling aurum file '{}'...", aurumFilePath);

   // parse & generate assembly
   Tokenizer tokenizer(fileHandler.getSourceCode());
   std::vector<Token> tokens = tokenizer.tokenize();
   if(!g_errors.empty()) {
      g_errors.printAll();
      return EXIT_FAILURE;
   }

   Parser parser(std::move(tokens));
   ast::Program program = parser.parse();
   if(!g_errors.empty()) {
      g_errors.printAll();
      return EXIT_FAILURE;
   }

   Generator generator(std::move(program));
   std::vector<ir::Instruction> instructions = generator.generate();
   if(!g_errors.empty()) {
      g_errors.printAll();
      return EXIT_FAILURE;
   }

   fileHandler.outputIR(generator.getIR());

   AsmEmitter emitter(std::move(instructions));
   std::string assembly = emitter.emitAssembly();

   fileHandler.outputAssembly(assembly);
   fileHandler.assemble(emitter.getRequiredLibs());
   fileHandler.runExecutable();

   return EXIT_SUCCESS;
}
