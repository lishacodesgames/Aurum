#pragma once
#include "IR.h"
#include "Stack.h"

class Assembler {
public:
   explicit Assembler(std::vector<ir::Instruction> instructions) : m_instructions(std::move(instructions))
      { m_output.reserve(4096); }

   std::expected<std::string, std::string> assemble();
   std::vector<std::string> getRequiredLibs() const; /// @todo noexcept?

private:
   const std::vector<ir::Instruction> m_instructions;

   std::string m_output;
   Stack m_stack;
   std::set<std::string> m_requiredExterns{};

private:
   void comment(std::string_view comment) { m_output += std::format("\t; {}\n", comment); }
   void write(std::string_view cmd) { m_output += std::format("\t{}\n", cmd); }

   /// @param value an integer literal or an identifier
   [[nodiscard]] std::optional<std::string> pushValue(std::string_view value);

   /// @param dest a register or the stack location of a variable
   /// @param value an integer literal or an identifier
   [[nodiscard]] std::optional<std::string> movFoldedValue(std::string_view dest, std::string_view value);

   /// @param varName name of destination variable
   /// @param value integer literal, identifier, or a register depending on valueIsReg
   /// @param valueIsReg if true, value won't be looked for in the symbol table
   [[nodiscard]] std::optional<std::string> movToVar(std::string_view varName, std::string_view value, bool valueIsReg);

   /// resolves a two-operand instruction so that after this call: rax = left, rbx = right
   [[nodiscard]] std::optional<std::string> resolveBinaryOperands(const ir::Instruction& insr);

   /// @param asmMnemonic the assembly instruction mnemonic for this binary opcode
   [[nodiscard]] std::optional<std::string> handleBinary(const ir::Instruction& instr, std::string_view asmMnemonic);
   [[nodiscard]] std::optional<std::string> handleDivMod(const ir::Instruction& instr, bool wantRemainder);
   [[nodiscard]] std::optional<std::string> handle(const ir::Instruction& instr);
};
