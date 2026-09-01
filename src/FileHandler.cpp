#include <pch/Precompiled.h>
#include "FileHandler.h"

#include "Errors.h"

FileHandler::FileHandler(std::string_view aurumFilePath) : aurumFilePath(aurumFilePath) {
   if(!std::filesystem::exists("scripts"))
      FATAL_ERROR("Please run from the root of the project, where the 'scripts' folder is located.");

   std::filesystem::path outDir("out");
   std::filesystem::create_directories(outDir); // does nothing if it already exists

   std::string name = std::filesystem::path(this->aurumFilePath).stem().string();
   irFilePath = outDir / (name + ".ir");
   assemblyFilePath = outDir / (name + ".asm");
   executableFilePath = outDir / name;
   assembleCommand = "./scripts/assemble_nasm_mac_x64.sh " + assemblyFilePath;
}

std::string FileHandler::getSourceCode() const {
   std::ifstream srcFile(aurumFilePath);
   if(!srcFile)
      FATAL_ERROR("Could not open file '{}'!", aurumFilePath);

   std::ostringstream contents;
   contents << srcFile.rdbuf();
   if(contents.view().empty()) // check empty with 0 allocations
      FATAL_ERROR("Empty Aurum file: {}!", aurumFilePath);

   return contents.str();
}

void FileHandler::outputIR(const std::string_view IR) const {
   std::ofstream irFile(irFilePath);
   irFile << IR;

   std::println("Successfully made an Intermediate Representation at '{}'!", irFilePath);
}

void FileHandler::outputAssembly(std::string_view assembly) const {
   std::ofstream asmFile(assemblyFilePath);
   asmFile << assembly;

   std::println("Successfully compiled to assembly file '{}'!\n", assemblyFilePath);
}

void FileHandler::assemble(const std::vector<std::string>& args) const {
   std::println("Compiling assembly file '{}' to executable...", executableFilePath);

   std::string command = assembleCommand;
   for(const std::string& file : args)
      command += " " + file;

   int assembleResult = std::system(command.c_str());
   if(!assembleResult)
      std::println("Successfully assembled to executable '{}'!", executableFilePath);
   else
      FATAL_ERROR("Assembling failed! Exit code: {}", assembleResult);
}

void FileHandler::runExecutable() const {
   std::string runExecutableCommand = "./" + executableFilePath;

   std::print("\nRunning compiled executable '{}'...\n\n\033[38:5:80m", runExecutableCommand); // prints any output of executable in teal
   int result = std::system(runExecutableCommand.c_str());
   std::println("\033[0m");

   // On macOS, std::system does not return the program's raw exit code directly. Instead, it returns a 16-bit wait status integer encoded by the operating system
   // to get the real exit code, we must divide by 256
   std::println("Successfully ran executable! Exited with exit code: \033[4m{}\033[0m", result / 256); // prints exit code underlined
}
