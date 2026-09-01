#include <pch/Precompiled.h>

#include "Tokenizer.h"
#include "Parser.h"
#include "Generator.h"
#include "AsmEmitter.h"
#include "FileHandler.h"

int main(int argc, char* argv[]) {
   // make sure the correct number of arguments is provided
   std::string aurumFilePath = argv[1]; // is not accessed if it is not provided, so this is safe
   if(argc != 2 || !aurumFilePath.ends_with(".aura"))
      FATAL_ERROR("Incorrect usage!\nCorrect usage: {} /path/to/file.aura", argv[0]);

   FileHandler fileHandler(aurumFilePath);
   std::println("Compiling aurum file '{}'...", aurumFilePath);

   // parse & generate assembly
   Tokenizer tokenizer(fileHandler.getSourceCode());
   auto tokens = tokenizer.tokenize();
   if(!tokens)
      FATAL_ERROR("{}", tokens.error());

   Parser parser(std::move(*tokens));
   auto program = parser.parse();
   if(!program)
      FATAL_ERROR("{}", program.error());

   /// @todo output IR instructions into a file
   Generator generator(std::move(*program));
   auto instructions = generator.generate();
   if(!instructions)
      FATAL_ERROR("{}", instructions.error());

   fileHandler.outputIR(*instructions);

   AsmEmitter emitter(std::move(*instructions));
   auto assembly = emitter.emitAssembly();
   if(!assembly)
      FATAL_ERROR("{}", assembly.error());

   fileHandler.outputAssembly(*assembly);
   fileHandler.assemble(emitter.getRequiredLibs());
   fileHandler.runExecutable();

   return EXIT_SUCCESS;
}
