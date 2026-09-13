#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"
#include "AsmEmitter.h"
#include "FileHandler.h"
#include "Errors.h"

void throwIfError(err::Phase phase) {
   if(g_errors.count() > 0)
      g_errors.throwAll(phase);
}

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
   throwIfError(err::Phase::TOKENIZING);

   Parser parser(std::move(tokens));
   ast::Program program = parser.parse();
   throwIfError(err::Phase::PARSING);

   Generator generator(std::move(program));
   std::vector<ir::Instruction> instructions = generator.generate();
   fileHandler.outputIR(generator.getIR());
   throwIfError(err::Phase::GENERATING);

   AsmEmitter emitter(std::move(instructions));
   std::string assembly = emitter.emitAssembly();
   fileHandler.outputAssembly(assembly);
   throwIfError(err::Phase::EMITTING_ASSEMBLY);

   fileHandler.assemble(emitter.getRequiredLibs());
   if(!noRun)
      fileHandler.runExecutable();
   throwIfError(err::Phase::RUNNING);

   return EXIT_SUCCESS;
}
