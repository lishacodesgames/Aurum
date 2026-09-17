#pragma once
#include "IR.h"

class FileHandler {
public:
   std::string name;
   FileHandler(std::string_view aurumFilePath);

   std::string getSourceCode() const;

   void outputIR(const std::string_view IR) const;
   void outputAssembly(std::string_view assembly) const;

   /// runs the assemble script
   /// @throws runtime_error if assembling failed
   void assemble(const std::vector<std::string>& args) const;
   void runExecutable() const;

private:
   std::string m_aurumFilePath;
   std::string m_irFilePath;
   std::string m_assemblyFilePath;
   std::string m_executableFilePath;
   std::string m_assembleCommand;
};

extern FileHandler g_fileHandler;
