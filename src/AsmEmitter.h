#pragma once
#include "IR.h"
#include "Stack.h"
#include "Errors.h"

class AsmEmitter {
public:
   explicit AsmEmitter(std::vector<ir::Instruction> instructions, ErrorReporter& reporter)
      : m_instructions(std::move(instructions)), m_stack(m_output, reporter), m_reporter(reporter)
   { m_output.reserve(4096); }

   std::expected<std::string, std::string> emitAssembly();
   std::vector<std::string> getRequiredLibs() const;

private:
   const std::vector<ir::Instruction> m_instructions;

   std::string m_output;
   Stack m_stack;
   std::set<std::string> m_requiredExterns{};

   ErrorReporter& m_reporter;

private:
   void comment(std::string_view comment) { m_output += std::format("\t; {}\n", comment); }
   void write(std::string_view cmd) { m_output += std::format("\t{}\n", cmd); }

   /// @param value an integer literal or an identifier
   void pushValue(std::string_view value);

   /// @param dest a register or the stack location of a variable
   /// @param value an integer literal or an identifier
   void movFoldedValue(std::string_view dest, std::string_view value);

   /// @param varName name of destination variable
   /// @param value integer literal, identifier, or a register depending on valueIsReg
   /// @param valueIsReg if true, value won't be looked for in the symbol table
   void movToVar(std::string_view varName, std::string_view value, bool valueIsReg);

   /// resolves a two-operand instruction so that after this call: rax = left, rbx = right
   void resolveBinaryOperands(const ir::Instruction& insr);

   /// @param asmMnemonic the assembly instruction mnemonic for this binary opcode
   void handleBinary(const ir::Instruction& instr, std::string_view asmMnemonic);
   void handleDivMod(const ir::Instruction& instr, bool wantRemainder);
   void handle(const ir::Instruction& instr);
};
