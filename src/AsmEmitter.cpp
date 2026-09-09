#include <pch/Precompiled.h>
#include "AsmEmitter.h"

namespace
{
   bool isImmediate(std::string_view value) {
      return std::isdigit(static_cast<unsigned char>(value[0])) || value == "TRUE" || value == "FALSE";
   }

   std::string_view lowerBoolean(std::string_view value) {
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
   err::SourceLocation location{ .row = static_cast<std::uint32_t>(line) };
   if(category == err::Category::INTERNAL)
      location.file = "AsmEmitter.cpp";

   g_errors.report(err::Phase::EMITTING_ASSEMBLY, category, location, message, isFatal);
}

void AsmEmitter::write(std::string_view cmd, std::optional<std::string_view> comment) {
   if(comment)
      m_output += std::format("\t{} ; {}\n", cmd, *comment);
   else
      m_output += std::format("\t{}\n", cmd);
}

void AsmEmitter::pushValue(std::string_view value, std::optional<std::string_view> comment) {
   if(isImmediate(value)) {
      value = lowerBoolean(value);
      if(comment)
         m_stack.push(std::format("{} ; {}", value, *comment));
      else
         m_stack.push(value);

   } else {
      if(auto symbol = m_stack.find(value)) {
         if(comment)
            m_stack.push(std::format("QWORD [rbp - {}] ; '{}', {}", symbol->offset, value, *comment));
         else
            m_stack.push(std::format("QWORD [rbp - {}] ; '{}'", symbol->offset, value));

      } else {
         error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", value), true);
         return;
      }
   }
}

void AsmEmitter::movFoldedValue(std::string_view dest, std::string_view value, std::optional<std::string_view> comment) {
   if(isImmediate(value)) {
      write(std::format("mov {}, {}", dest, lowerBoolean(value)), comment);

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
   const std::string& left = *instr.operandLeft;
   const std::string& right = *instr.operandRight;
   std::string opcode = to_string(instr.opcode);

   if(left == ir::TOS && right == ir::TOS) {
      error(err::Category::INTERNAL, __LINE__, "Both operands of binary expression are TOS!", true);
      return;
   }

   if(right == ir::TOS) {
      m_stack.pop(std::format("rbx ; rhs for opcode '{}'", opcode));

      if(left == ir::SOS)
         m_stack.pop(std::format("rax ; lhs for opcode '{}'", opcode));
      else
         movFoldedValue("rax", left, "lhs for opcode " + opcode);

   } else if(left == ir::TOS) {
      m_stack.pop(std::format("rax ; lhs for opcode '{}'", opcode));
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

void AsmEmitter::handle(const ir::Instruction& instr) {
   switch(instr.opcode) {
      case OpCode::PUSH_INT:
      case OpCode::PUSH_BOOL:
      case OpCode::PUSH_VAR:
         pushValue(*instr.operandLeft);
         break;

      case OpCode::DEF_VAR_MUT:
      case OpCode::DEF_VAR_CONST: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;
         bool isMutable = instr.opcode == OpCode::DEF_VAR_MUT;

         if(value != ir::TOS) {
            pushValue(value,
               std::format("Declaration of {} '{}'", isMutable ? "mutable" : "const", varName));
         }

         m_stack.setTop(varName, isMutable);
         break;
      }

      case OpCode::ALLOC_VAR:
         m_stack.push(std::nullopt, true, *instr.operandLeft);
         break;

      case OpCode::STORE_VAR: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;

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
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("inc QWORD [rbp - {}]", symbol->offset), std::format("{}++", symbol->name));
         else
            error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", *instr.operandLeft), true);

         break;
      }

      case OpCode::DECR: {
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("dec QWORD [rbp - {}]", symbol->offset), std::format("{}--", symbol->name));
         else
            error(err::Category::NAME_RESOLUTION, __LINE__, std::format("Use of undeclared identifier '{}'!", *instr.operandLeft), true);

         break;
      }

      case OpCode::ADD: handleBinary(instr, "add"); break;
      case OpCode::SUB: handleBinary(instr, "sub"); break;
      case OpCode::MUL: handleBinary(instr, "imul"); break;

      case OpCode::DIV: handleDivMod(instr, false); break;
      case OpCode::MOD: handleDivMod(instr, true); break;

      case OpCode::NEG: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop("rax");
         else
            movFoldedValue("rax", *instr.operandLeft);

         write("neg rax");
         m_stack.push("rax");
         break;
      }

      case OpCode::EXIT: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop("rdi");
         else
            movFoldedValue("rdi", *instr.operandLeft);

         m_output += "\n";
         write("mov rax, 1 | 0x2000000", "exit syscall number for macOS");
         write("syscall");
         break;
      }

      case OpCode::SCOPE_START: m_stack.startScope(); break;
      case OpCode::SCOPE_END: m_stack.endScope(); break;

      default:
         error(err::Category::INTERNAL, __LINE__, std::format("Unhandled opcode: '{}'!", to_string(instr.opcode)), true);
         break;
   }
}
