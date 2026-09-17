#include <pch/Precompiled.h>
#include "FileHandler.h"

#include "Errors.h"

FileHandler::FileHandler(std::string_view aurumFilePath) : m_aurumFilePath(aurumFilePath) {
   assert(std::filesystem::exists("scripts") && "Please run from the root of the project, where the 'scripts' folder is located.");

   std::filesystem::path outDir("out");
   std::filesystem::create_directories(outDir); // does nothing if it already exists

   name = std::filesystem::path(aurumFilePath).stem().string();;
   m_irFilePath = outDir / (name + ".ir");
   m_assemblyFilePath = outDir / (name + ".asm");
   m_executableFilePath = outDir / name;
   m_assembleCommand = "./scripts/assemble_nasm_mac_x64.sh " + m_assemblyFilePath;
}

std::string FileHandler::getSourceCode() const {
   std::ifstream srcFile(m_aurumFilePath);
   std::println("{}", m_aurumFilePath);
   assert(srcFile);

   std::ostringstream contents;
   contents << srcFile.rdbuf();
   assert(!contents.view().empty() && "Empty Aurum file"); // check empty with 0 allocations

   return contents.str();
}

void FileHandler::outputIR(const std::string_view IR) const {
   std::ofstream irFile(m_irFilePath);
   irFile << IR;

   std::println("Successfully made an Intermediate Representation at '{}'!", m_irFilePath);
}

void FileHandler::outputAssembly(std::string_view assembly) const {
   std::ofstream asmFile(m_assemblyFilePath);
   asmFile << assembly;

   std::println("Successfully compiled to assembly file '{}'!\n", m_assemblyFilePath);
}

void FileHandler::assemble(const std::vector<std::string>& args) const {
   std::println("Assembling assembly file '{}' into executable...", m_assemblyFilePath);

   std::string command = m_assembleCommand;
   for(const std::string& file : args)
      command += " " + file;

   int assembleResult = std::system(command.c_str());
   assert(!assembleResult);

   std::println("Successfully assembled to executable '{}'!", m_executableFilePath);
}

void FileHandler::runExecutable() const {
   std::string runExecutableCommand = "./" + m_executableFilePath;

   std::print("\nRunning compiled executable '{}'...\n\n\033[38:5:80m", runExecutableCommand); // prints any output of executable in teal
   int result = std::system(runExecutableCommand.c_str());
   std::println("\033[0m");

   // On macOS, std::system does not return the program's raw exit code directly. Instead, it returns a 16-bit wait status integer encoded by the operating system
   // to get the real exit code, we must divide by 256
   std::println("Successfully ran executable! Exited with exit code: \033[4m{}\033[0m", result / 256); // prints exit code underlined
}
