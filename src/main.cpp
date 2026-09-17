#include <pch/Precompiled.h>

#include "Lexer.h"
#include "Parser.h"
#include "Generator.h"
#include "AsmEmitter.h"
#include "FileHandler.h"
#include "Errors.h"

void throwIfError(err::Phase phase) {
   if(g_errors.count() > 0)
      g_errors.throwAll(phase);
}

FileHandler g_fileHandler("ueishdfsdfgyueashdsbafudsifhda"); // will be overridden in main, which will run before anyone outside accesses fileHandler

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

   FileHandler mine(aurumFilePath);
   g_fileHandler = mine;
   std::println("Compiling aurum file '{}'...", aurumFilePath);

   // parse & generate assembly
   Lexer lexer(g_fileHandler.getSourceCode());
   std::vector<Token> tokens = lexer.tokenize();
   throwIfError(err::Phase::LEXING);

   Parser parser(std::move(tokens));
   ast::Program program = parser.parse();
   throwIfError(err::Phase::PARSING);

   Generator generator(std::move(program));
   std::vector<ir::Instruction> instructions = generator.generate();
   g_fileHandler.outputIR(generator.getIR());
   throwIfError(err::Phase::GENERATING);

   AsmEmitter emitter(std::move(instructions));
   std::string assembly = emitter.emitAssembly();
   g_fileHandler.outputAssembly(assembly);
   throwIfError(err::Phase::EMITTING_ASSEMBLY);

   g_fileHandler.assemble(emitter.getRequiredLibs());
   if(!noRun)
      g_fileHandler.runExecutable();
   throwIfError(err::Phase::RUNNING);

   return EXIT_SUCCESS;
}
