#include <pch/Precompiled.h>
#include "AsmEmitter.h"

namespace
{
   bool isLiteral(std::string_view value) {
      return std::isdigit(static_cast<unsigned char>(value[0])) || value == "TRUE" || value == "FALSE";
   }

   std::string_view getPushable(std::string_view value) {
      if(value == "TRUE")
         return "1";
      if(value == "FALSE")
         return "0";
      return value;
   }
}

std::string AsmEmitter::emitAssembly() {
   for(const ir::Instruction& instr : m_instructions)
      handle(instr);

   std::string externs;
   for(const std::string& libFunc : m_requiredExterns)
      externs += "extern " + libFunc + "\n";

   /// @todo make _main: output part of the opcodes
   std::string header = std::format(
R"delim(; macOS x86_64, NASM syntax

{}
global _main
_main:
	push rbp       ; save the caller's base pointer
	mov rbp, rsp   ; mov our stack pointer to the current base pointer

)delim", externs);

   return header + m_output;
}

std::vector<std::string> AsmEmitter::getRequiredLibs() const {
   std::vector<std::string> files{};

   for(const std::string& libFunc : m_requiredExterns)
      files.push_back(std::format("vault/{}.asm", libFunc));

   return files;
}

void AsmEmitter::error(err::Category category, int line, std::string_view message, bool isFatal) const {
   g_errors.report(err::Phase::EMITTING_ASSEMBLY, category,
      { "AsmEmitter.cpp", static_cast<std::uint32_t>(line) }, message, isFatal);
}

void AsmEmitter::writeLabel(std::string_view label) {
   m_output += std::format("{}:\n", label);
}

void AsmEmitter::write(std::string_view cmd, std::optional<std::string_view> comment) {
   if(comment)
      m_output += std::format("\t{} ; {}\n", cmd, *comment);
   else
      m_output += std::format("\t{}\n", cmd);
}

void AsmEmitter::pushValue(std::string_view value, std::optional<std::string_view> comment) {
   if(isLiteral(value)) {
      if(comment)
         m_stack.push(std::format("{} ; {}", getPushable(value), *comment));
      else
         m_stack.push(getPushable(value));

   } else {
      if(auto symbol = m_stack.find(value)) {
         if(comment)
            m_stack.push(std::format("QWORD [rbp - {}] ; '{}', {}", symbol->offset, value, *comment));
         else
            m_stack.push(std::format("QWORD [rbp - {}] ; '{}'", symbol->offset, value));

      } else {
         error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", value));
         return;
      }
   }
}

void AsmEmitter::movFoldedValue(std::string_view dest, std::string_view value, std::optional<std::string_view> comment) {
   if(isLiteral(value)) {
      write(std::format("mov {}, {}", dest, getPushable(value)), comment);

   } else {
      if(auto symbol = m_stack.find(value)) {
         if(comment)
            write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}', {}", value, *comment));
         else
            write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}'", value));

      } else {
         error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", value), true);
         return;
      }
   }
}

void AsmEmitter::movToVar(std::string_view varName, std::string_view value, bool valueIsReg, std::optional<std::string_view> comment) {
   auto symbol = m_stack.find(varName);
   if(!symbol) {
      error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", value), true);
      return;
   }

   if(!valueIsReg)
      movFoldedValue(std::format("QWORD [rbp - {}]", symbol->offset), value, comment);
   else
      write(std::format("mov QWORD [rbp - {}], {}", symbol->offset, value), comment);
}

void AsmEmitter::resolveBinaryOperands(const ir::Instruction& instr) {
   const std::string& left = instr.operand1;
   const std::string& right = *instr.operand2;
   std::string opcode = to_string(instr.opcode);

   if(left == ir::TOS && right == ir::TOS) {
      error(err::Category::INTERNAL, __LINE__, "Both operands of binary expression are TOS!", true);
      return;
   }

   if(right == ir::TOS) {
      m_stack.pop("rbx ; rhs for opcode " + opcode);

      if(left == ir::SOS)
         m_stack.pop("rax ; lhs for opcode " + opcode);
      else
         movFoldedValue("rax", left, "lhs for opcode " + opcode);

   } else if(left == ir::TOS) {
      m_stack.pop("rax ; lhs for opcode " + opcode);
      movFoldedValue("rbx", right, "rhs for opcode " + opcode);

   } else {
      movFoldedValue("rax", left, "lhs for opcode " + opcode);
      movFoldedValue("rbx", right, "rhs for opcode " + opcode);
   }
}

void AsmEmitter::handleBinary(const ir::Instruction& instr, std::string_view asmMnemonic) {
   resolveBinaryOperands(instr);

   write(std::format("{} rax, rbx", asmMnemonic));
   m_stack.push("rax");
}

void AsmEmitter::handleDivMod(const ir::Instruction& instr, bool wantRemainder) {
   resolveBinaryOperands(instr);

   write("cqo", "prep rdx:rax for division");
   write("idiv rbx");

   if(wantRemainder)
      m_stack.push("rdx ; store remainder");
   else
      m_stack.push("rax ; store quotient");
}

void AsmEmitter::handleJump(const ir::Instruction& instr, bool conditional, std::optional<bool> jumpCondition) {
   if(conditional && !jumpCondition)
      error(err::Category::INTERNAL, __LINE__, "No condition given for conditional jump!");

   // conditional
   if(conditional) {
      const std::string& cond = instr.operand1;
      const std::string& label = *instr.operand2;

      if(cond == ir::TOS)
         m_stack.pop("rax");
      else
         movFoldedValue("rax", cond);

      write("test rax, rax");
      if(*jumpCondition)
         write("jnz " + label); // jump on true
      else
         write("jz " + label); // jump on false

      return;
   }

   /// @todo non-conditional
   error(err::Category::INTERNAL, __LINE__, "Unconditional jump not yet implemented!");
}

void AsmEmitter::handle(const ir::Instruction& instr) {
   switch(instr.opcode) {
      case OpCode::PUSH_INT:
      case OpCode::PUSH_BOOL:
      case OpCode::PUSH_VAR:
         pushValue(instr.operand1);
         break;

      case OpCode::DEF_VAR: {
         const std::string& varName = instr.operand1;
         const std::string& value = *instr.operand2;

         if(value != ir::TOS)
            pushValue(value, "Declaration of {}" + varName);

         m_stack.nameTop(varName);
         break;
      }

      case OpCode::ALLOC_VAR:
         m_stack.push(std::nullopt, instr.operand1);
         break;

      case OpCode::STORE_VAR: {
         const std::string& varName = instr.operand1;
         const std::string& value = *instr.operand2;

         if(value == ir::TOS) {
            // popping to rax then moving is generally faster than popping directly to location
            m_stack.pop("rax");
            movToVar(varName, "rax", true, std::format("{} = {}", varName, value));
         } else {
            movToVar(varName, value, false, std::format("{} = {}", varName, value));
         }
         break;
      }

      case OpCode::INCR: {
         if(auto symbol = m_stack.find(instr.operand1))
            write(std::format("inc QWORD [rbp - {}]", symbol->offset), std::format("{}++", symbol->name));
         else
            error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", instr.operand1));

         break;
      }

      case OpCode::DECR: {
         if(auto symbol = m_stack.find(instr.operand1))
            write(std::format("dec QWORD [rbp - {}]", symbol->offset), std::format("{}--", symbol->name));
         else
            error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", instr.operand1));

         break;
      }

      case OpCode::ADD: handleBinary(instr, "add"); break;
      case OpCode::SUB: handleBinary(instr, "sub"); break;
      case OpCode::MUL: handleBinary(instr, "imul"); break;

      case OpCode::DIV: handleDivMod(instr, false); break;
      case OpCode::MOD: handleDivMod(instr, true); break;

      case OpCode::NEG: {
         if(instr.operand1 == ir::TOS)
            m_stack.pop("rax");
         else
            movFoldedValue("rax", instr.operand1);

         write("neg rax");
         m_stack.push("rax");
         break;
      }

      case OpCode::EXIT: {
         if(instr.operand1 == ir::TOS)
            m_stack.pop("rdi");
         else
            movFoldedValue("rdi", instr.operand1);

         write("mov rax, 1 | 0x2000000", "exit syscall number for macOS");
         write("syscall");
         break;
      }

      case OpCode::LABEL:
         writeLabel(instr.operand1);
         break;

      /// @todo handle FUNC

      case OpCode::JUMP:         handleJump(instr); break;
      case OpCode::JUMP_IF:      handleJump(instr, true, true); break;
      case OpCode::JUMP_IF_NOT:  handleJump(instr, true, false); break;

      case OpCode::SCOPE_START: m_stack.startScope(); break;
      case OpCode::SCOPE_END: m_stack.endScope(); break;

      default:
         error(err::Category::INTERNAL, __LINE__, std::format("Unhandled opcode: '{}'!", to_string(instr.opcode)), true);
         break;
   }
}
