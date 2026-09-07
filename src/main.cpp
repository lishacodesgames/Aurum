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
   if(std::size_t count = g_errors.count(); count > 0) {
      g_errors.printAll();
      throw std::runtime_error(std::format("{} error{} generated during tokenizing.", count, count > 1 ? "s" : ""));
   }

   Parser parser(std::move(tokens));
   ast::Program program = parser.parse();
   if(std::size_t count = g_errors.count(); count > 0) {
      g_errors.printAll();
      throw std::runtime_error(std::format("{} error{} generated during parsing.", count, count > 1 ? "s" : ""));
   }

   Generator generator(std::move(program));
   std::vector<ir::Instruction> instructions = generator.generate();
   fileHandler.outputIR(generator.getIR());
   if(std::size_t count = g_errors.count(); count > 0) {
      g_errors.printAll();
      throw std::runtime_error(std::format("{} error{} generated during generating.", count, count > 1 ? "s" : ""));
   }

   AsmEmitter emitter(std::move(instructions));
   std::string assembly = emitter.emitAssembly();
   fileHandler.outputAssembly(assembly);
   if(std::size_t count = g_errors.count(); count > 0) {
      g_errors.printAll();
      throw std::runtime_error(std::format("{} error{} generated during generating.", count, count > 1 ? "s" : ""));
   }

   fileHandler.assemble(emitter.getRequiredLibs());
   fileHandler.runExecutable();

   return EXIT_SUCCESS;
}
