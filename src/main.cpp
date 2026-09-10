#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"
#include "AsmEmitter.h"
#include "FileHandler.h"
#include "Errors.h"

int main(int argc, char* argv[]) {
   if(argc < 2 || argc > 3) {
      std::println("Incorrect usage!\nCorrect usage: {} /path/to/file.aura [--no-run]", argv[0]);
      return EXIT_FAILURE;
   }

   std::string aurumFilePath = argv[1];
   // Test runners compile first and run the produced executable themselves.
   const bool noRun = argc == 3 && std::string_view(argv[2]) == "--no-run";
   if(!aurumFilePath.ends_with(".aura") || (argc == 3 && !noRun)) {
      std::println("Incorrect usage!\nCorrect usage: {} /path/to/file.aura [--no-run]", argv[0]);
      return EXIT_FAILURE;
   }

   FileHandler fileHandler(aurumFilePath);
   std::println("Compiling aurum file '{}'...", aurumFilePath);

   // parse & generate assembly
   Tokenizer tokenizer(fileHandler.getSourceCode());
   std::vector<Token> tokens = tokenizer.tokenize();
   if(g_errors.count() > 0)
      g_errors.throwAll(err::Phase::TOKENIZING);

   Parser parser(std::move(tokens));
   ast::Program program = parser.parse();
   if(g_errors.count() > 0)
      g_errors.throwAll(err::Phase::PARSING);

   Generator generator(std::move(program));
   std::vector<ir::Instruction> instructions = generator.generate();
   fileHandler.outputIR(generator.getIR());
   if(g_errors.count() > 0)
      g_errors.throwAll(err::Phase::GENERATING);

   AsmEmitter emitter(std::move(instructions));
   std::string assembly = emitter.emitAssembly();
   fileHandler.outputAssembly(assembly);
   if(g_errors.count() > 0)
      g_errors.throwAll(err::Phase::EMITTING_ASSEMBLY);

   fileHandler.assemble(emitter.getRequiredLibs());
   if(!noRun)
      fileHandler.runExecutable();

   return EXIT_SUCCESS;
}
