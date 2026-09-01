#include <pch/Precompiled.h>
#include "AsmEmitter.h"

std::expected<std::string, std::string> AsmEmitter::emitAssembly() {
   for(const ir::Instruction& instr : m_instructions) {
      if(auto instrError = handle(instr))
         return std::unexpected(std::move(*instrError));
   }

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

std::optional<std::string> AsmEmitter::pushValue(std::string_view value) {
   bool isNumeric = std::isdigit(static_cast<unsigned char>(value[0]));

   if(isNumeric) {
      m_stack.push(m_output, value);
   } else {
      if(auto symbol = m_stack.find(value))
         m_stack.push(m_output, std::format("QWORD [rbp - {}] ; '{}'", symbol->offset, value));
      else
         return std::format("Use of undeclared identifier '{}'!", value);
   }

   return std::nullopt;
}

std::optional<std::string> AsmEmitter::movFoldedValue(std::string_view dest, std::string_view value) {
   bool isNumeric = std::isdigit(static_cast<unsigned char>(value[0]));

   if(isNumeric) {
      write(std::format("mov {}, {}", dest, value));
   } else {
      if(auto symbol = m_stack.find(value))
         write(std::format("mov {}, QWORD [rbp - {}] ; '{}", dest, symbol->offset, value));
      else
         return std::format("Use of undeclared identifier '{}'!", value);
   }

   return std::nullopt;
}

std::optional<std::string> AsmEmitter::movToVar(std::string_view varName, std::string_view value, bool valueIsReg) {
   auto symbol = m_stack.find(varName);
   if(!symbol)
      return std::format("Use of undeclared identifier '{}'!", varName);

   if(!valueIsReg) {
      // also validates value's existence if it's an identifier
      if(auto movError = movFoldedValue(std::format("QWORD [rbp - {}]", symbol->offset), value)) return movError;
   } else {
      write(std::format("mov QWORD [rbp - {}], {}", symbol->offset, value));
   }

   return std::nullopt;
}

std::optional<std::string> AsmEmitter::resolveBinaryOperands(const ir::Instruction& instr) {
   const std::string& left = *instr.operandLeft;
   const std::string& right = *instr.operandRight;

   bool leftIsTOS = left == ir::TOS;
   bool rightIsTOS = right == ir::TOS;

   if(leftIsTOS && rightIsTOS) {
      // both already on stack, in the correct order by grace of Generator
      m_stack.pop(m_output, std::format("rbx ; rhs for opcode '{}'", ir::to_string(instr.opcode)));
      m_stack.pop(m_output, std::format("rax ; lhs for opcode '{}'", ir::to_string(instr.opcode)));
      return std::nullopt;
   }

   if(rightIsTOS) {
      if(auto movError = movFoldedValue("rax", left)) return movError;
      m_stack.pop(m_output, "rbx");

      return std::nullopt;
   } 

   if(leftIsTOS) {
      m_stack.pop(m_output, "rax");
      if(auto movError = movFoldedValue("rbx", right)) return movError;

      return std::nullopt;
   }

   // both folded
   if(auto movError = movFoldedValue("rax", left)) return movError;
   if(auto movError = movFoldedValue("rbx", right)) return movError;
   return std::nullopt;
}

std::optional<std::string> AsmEmitter::handleBinary(const ir::Instruction& instr, std::string_view asmMnemonic) {
   if(auto resolveError = resolveBinaryOperands(instr)) return resolveError;

   write(std::format("{} rax, rbx", asmMnemonic));
   m_stack.push(m_output, "rax");
   return std::nullopt;
}

std::optional<std::string> AsmEmitter::handleDivMod(const ir::Instruction& instr, bool wantRemainder) {
   if(auto resolveError = resolveBinaryOperands(instr)) return resolveError;

   write("cqo ; prep rdx:rax for division");
   write("idiv rbx");

   if(wantRemainder)
      m_stack.push(m_output, "rdx");
   else
      m_stack.push(m_output, "rax");

   return std::nullopt;
}

std::optional<std::string> AsmEmitter::handle(const ir::Instruction& instr) {
   switch(instr.opcode) {
      case ir::OpCode::PUSH_INT: {
         if(auto pushError = pushValue(*instr.operandLeft)) return pushError;
         return std::nullopt;
      }

      case ir::OpCode::PUSH_VAR: {
         if(auto pushError = pushValue(*instr.operandLeft)) return pushError;
         return std::nullopt;
      }

      case ir::OpCode::DEF_VAR: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;

         if(value != ir::TOS)
            if(auto pushError = pushValue(value)) return pushError;

         /// @todo mutability
         m_stack.setTop(varName, true);
         return std::nullopt;
      }

      case ir::OpCode::ALLOC_VAR: {
         m_stack.push(m_output, std::nullopt, true, *instr.operandLeft);
         return std::nullopt;
      }

      case ir::OpCode::STORE_VAR: {
         const std::string& varName = *instr.operandLeft;
         const std::string& value = *instr.operandRight;

         if(value == ir::TOS) {
            // popping to rax then moving is generally faster than popping directly to location
            m_stack.pop(m_output, "rax");
            if(auto movError = movToVar(varName, "rax", true)) return movError;
         } else {
            if(auto movError = movToVar(varName, value, false)) return movError;
         }

         return std::nullopt;
      }

      case ir::OpCode::INCR: {
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("inc QWORD [rbp - {}] ; {}++", symbol->offset, symbol->name));
         else
            return std::format("Use of undeclared identifier '{}'!", *instr.operandLeft);

         return std::nullopt;
      }

      case ir::OpCode::DECR: {
         if(auto symbol = m_stack.find(*instr.operandLeft))
            write(std::format("dec QWORD [rbp - {}] ; {}--", symbol->offset, symbol->name));
         else
            return std::format("Use of undeclared identifier '{}'!", *instr.operandLeft);

         return std::nullopt;
      }

      case ir::OpCode::ADD: return handleBinary(instr, "add");
      case ir::OpCode::SUB: return handleBinary(instr, "sub");
      case ir::OpCode::MUL: return handleBinary(instr, "imul"); // signed multiplication

      case ir::OpCode::DIV: return handleDivMod(instr, false);
      case ir::OpCode::MOD: return handleDivMod(instr, true);

      case ir::OpCode::NEG: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop(m_output, "rax");
         else
            if(auto movError = movFoldedValue("rax", *instr.operandLeft)) return movError;

         write("neg rax");
         m_stack.push(m_output, "rax");
         return std::nullopt;
      }

      case ir::OpCode::EXIT: {
         if(*instr.operandLeft == ir::TOS)
            m_stack.pop(m_output, "rdi");
         else
            if(auto movError = movFoldedValue("rdi", *instr.operandLeft)) return movError;

         m_output += "\n";
         write("mov rax, 1 | 0x2000000 ; exit syscall number for macOS");
         write("syscall");
         return std::nullopt;
      }

      case ir::OpCode::SCOPE_START:
         m_stack.startScope(m_output);
         return std::nullopt;

      case ir::OpCode::SCOPE_END:
         m_stack.endScope(m_output);
         return std::nullopt;

      default:
         return std::format("Unhandled opcode: '{}'!", ir::to_string(instr.opcode));
   }
}
