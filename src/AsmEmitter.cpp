#include <pch/Precompiled.h>
#include "AsmEmitter.h"

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

void AsmEmitter::write(std::string_view cmd, std::optional<std::string_view> comment) {
   if(comment)
      m_output += std::format("\t{} ; {}\n", cmd, *comment);
   else
      m_output += std::format("\t{}\n", cmd);
}

void AsmEmitter::pushValue(std::string_view value, std::optional<std::string_view> comment) {
   if(std::isdigit(static_cast<unsigned char>(value[0]))) {
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
         g_errors.report(Phase::EMITTING_ASSEMBLY, Category::NAME_RESOLUTION,
            /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", value), true);
         return;
      }
   }
}

void AsmEmitter::movFoldedValue(std::string_view dest, std::string_view value, std::optional<std::string_view> comment) {
   if(std::isdigit(static_cast<unsigned char>(value[0]))) {
      write(std::format("mov {}, {}", dest, value), comment);

   } else {
      if(auto symbol = m_stack.find(value)) {
         if(comment)
            write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}', {}", value, *comment));
         else
            write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}'", value));

      } else {
         g_errors.report(Phase::EMITTING_ASSEMBLY, Category::NAME_RESOLUTION,
            /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", value), true);
         return;
      }
   }
}

void AsmEmitter::movToVar(std::string_view varName, std::string_view value, bool valueIsReg, std::optional<std::string_view> comment) {
   auto symbol = m_stack.find(varName);
   if(!symbol) {
      g_errors.report(Phase::EMITTING_ASSEMBLY, Category::NAME_RESOLUTION,
         /* @todo */ {}, std::format("Use of undeclared identifier '{}'!", value), true);
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
   std::string opcode = ir::to_string(instr.opcode);

   if(left == ir::TOS && right == ir::TOS) {
      g_errors.report(Phase::EMITTING_ASSEMBLY, Category::INTERNAL, { "AsmEmitter.cpp" }, "Both operands of binary expression are TOS!", true);
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
      case ir::OpCode::PUSH_INT:
         pushValue(*instr.operandLeft);

      case ir::OpCode::PUSH_VAR:
         pushValue(*instr.operandLeft);

      case ir::OpCode::DEF_VAR: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;

         if(value != ir::TOS)
            pushValue(value, std::format("'{}'", varName));

         /// @todo mutability
         m_stack.setTop(varName, true);
      }

      case ir::OpCode::ALLOC_VAR:
         m_stack.push(std::nullopt, true, *instr.operandLeft);

      case ir::OpCode::STORE_VAR: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;

         if(value == ir::TOS) {
            // popping to rax then moving is generally faster than popping directly to location
            m_stack.pop("rax");
            movToVar(varName, "rax", true, std::format("{} = {}", varName, value));
         } else {
            movToVar(varName, value, false, std::format("{} = {}", varName, value));
         }
      }

      case ir::OpCode::INCR: {
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("inc QWORD [rbp - {}]", symbol->offset), std::format("{}++", symbol->name));
         else
            g_errors.report(Phase::EMITTING_ASSEMBLY, Category::NAME_RESOLUTION,
               /* todo */ {}, std::format("Use of undeclared identifier '{}'!", *instr.operandLeft), true);
      }

      case ir::OpCode::DECR: {
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("dec QWORD [rbp - {}]", symbol->offset), std::format("{}--", symbol->name));
         else
            g_errors.report(Phase::EMITTING_ASSEMBLY, Category::NAME_RESOLUTION,
               /* todo */ {}, std::format("Use of undeclared identifier '{}'!", *instr.operandLeft), true);
      }

      case ir::OpCode::ADD: return handleBinary(instr, "add");
      case ir::OpCode::SUB: return handleBinary(instr, "sub");
      case ir::OpCode::MUL: return handleBinary(instr, "imul"); // signed multiplication

      case ir::OpCode::DIV: return handleDivMod(instr, false);
      case ir::OpCode::MOD: return handleDivMod(instr, true);

      case ir::OpCode::NEG: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop("rax");
         else
            movFoldedValue("rax", *instr.operandLeft);

         write("neg rax");
         m_stack.push("rax");
      }

      case ir::OpCode::EXIT: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop("rdi");
         else
            movFoldedValue("rdi", *instr.operandLeft);

         m_output += "\n";
         write("mov rax, 1 | 0x2000000", "exit syscall number for macOS");
         write("syscall");
      }

      case ir::OpCode::SCOPE_START:
         m_stack.startScope();

      case ir::OpCode::SCOPE_END:
         m_stack.endScope();

      default:
         g_errors.report(Phase::EMITTING_ASSEMBLY, Category::INTERNAL,
            { "AsmEmitter.cpp" }, std::format("Unhandled opcode: '{}'!", ir::to_string(instr.opcode)), true);
   }
}
