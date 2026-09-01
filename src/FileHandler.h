#pragma once
#include "IR.h"

class FileHandler {
public:
   FileHandler(std::string_view aurumFilePath);

   /// @return aurum source code
   std::string getSourceCode() const;

   void outputIR(const std::string_view IR) const;
   void outputAssembly(std::string_view assembly) const;

   /// runs the assemble script
   void assemble(const std::vector<std::string>& args) const;
   void runExecutable() const;

private:
   std::string aurumFilePath;
   std::string irFilePath;
   std::string assemblyFilePath;
   std::string executableFilePath;
   std::string assembleCommand;
};
