#include <pch/Precompiled.h>
#include "AsmEmitter.h"

namespace
{
   bool isLiteral(std::string_view value) {
      assert(!value.empty()); // assert and not error because isLiteral shouldn't ever be called with an empty value, so this is just a sanity check
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
   /// @todo make handle function instruction do the save caller's base pointer and make new stack frame
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

#pragma region Writers

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
      auto symbol = m_stack.find(value);
      assert(symbol);

      if(comment)
         m_stack.push(std::format("QWORD [rbp - {}] ; '{}', {}", symbol->offset, value, *comment));
      else
         m_stack.push(std::format("QWORD [rbp - {}] ; '{}'", symbol->offset, value));
   }
}

void AsmEmitter::movFoldedValue(std::string_view dest, std::string_view value, std::optional<std::string_view> comment) {
   if(isLiteral(value)) {
      write(std::format("mov {}, {}", dest, getPushable(value)), comment);

   } else {
      auto symbol = m_stack.find(value);
      assert(symbol);

      if(comment)
         write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}', {}", value, *comment));
      else
         write(std::format("mov {}, QWORD [rbp - {}]", dest, symbol->offset), std::format("'{}'", value));
   }
}

void AsmEmitter::movToVar(std::string_view varName, std::string_view value, bool valueIsReg, std::optional<std::string_view> comment) {
   auto symbol = m_stack.find(varName);
   assert(symbol);

   if(!valueIsReg)
      movFoldedValue(std::format("QWORD [rbp - {}]", symbol->offset), value, comment);
   else
      write(std::format("mov QWORD [rbp - {}], {}", symbol->offset, value), comment);
}

#pragma endregion

#pragma region Handlers

void AsmEmitter::resolveBinaryOperands(const ir::Instruction& instr) {
   const std::string& left = instr.operand1;
   const std::string& right = *instr.operand2;
   std::string opcode = to_string(instr.opcode);

   assert(left != ir::TOS || right != ir::TOS); // both shouldn't be TOS

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

void AsmEmitter::handleBinary(const ir::Instruction& instr) {
   resolveBinaryOperands(instr);

   switch(instr.opcode) {
      case OpCode::ADD: write("add rax, rbx");  break;
      case OpCode::SUB: write("sub rax, rbx");  break;
      case OpCode::MUL: write("imul rax, rbx"); break; // signed multiplication

      case OpCode::DIV:
         write("cqo", "prep rdx:rax for division");
         write("idiv rbx");
         break;

      case OpCode::MOD:
         write("cqo", "prep rdx:rax for division");
         write("idiv rbx");
         m_stack.push("rdx ; result of MOD");
         return;

      case OpCode::AND: write("and rax, rbx"); break;
      case OpCode::OR:  write("or rax, rbx");  break;

      default: assert(false && "Invalid binary instruction");
   }

   m_stack.push("rax ; result of " + to_string(instr.opcode));
}

void AsmEmitter::handleCondJump(const ir::Instruction& instr) {
   const std::string& cond = instr.operand1;
   const std::string& label = *instr.operand2;

   if(cond == ir::TOS)
      m_stack.pop("rax");
   else
      movFoldedValue("rax", cond);

   write("test rax, rax");   // cond is guaranteed to be a boolean (0/1) literal
   if(instr.opcode == OpCode::JUMP_IF) // condition must be true
      write("jnz " + label); // jump on true
   else                                // condition must false
      write("jz " + label);  // jump on false
}

void AsmEmitter::handleComparison(const ir::Instruction& instr) {
   resolveBinaryOperands(instr); // lhs = rax, rhs = rbx

   write("cmp rax, rbx"); // cmmp sets all the comparison flags, and we can read any of them

   // set the LOWEST 8 BITS (1byte) of RCX based on the needed comparison flag
   switch(instr.opcode) {
      case OpCode::EQ:  write("sete cl", "rax == rbx?");  break;
      case OpCode::NEQ: write("setne cl", "rax != rbx?"); break;

      /// @note these 4 operators check value compared with sign
      case OpCode::LT:  write("setl cl", "rax < rbx?");   break;
      case OpCode::GT:  write("setg cl", "rax > rbx?");    break;
      case OpCode::LTE: write("setle cl", "rax <= rbx?"); break;
      case OpCode::GTE: write("setge cl", "rax >= rbx?"); break;
      default: assert(false && "Invalid comparison instruction");
   }

   write("movzx rax, cl", "mov set flag to rax and zero out its higher 7 bytes");
   m_stack.push("rax");
}

void AsmEmitter::handle(const ir::Instruction& instr) {
   // entire function body is just a switch statement
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
            pushValue(value, "Declaration of " + varName);

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
            break;
         }

         movToVar(varName, value, false, std::format("{} = {}", varName, value));
         break;
      }

      case OpCode::INCR: {
         auto symbol = m_stack.find(instr.operand1);
         assert(symbol);

         write(std::format("inc QWORD [rbp - {}]", symbol->offset), std::format("{}++", symbol->name));
         break;
      }

      case OpCode::DECR: {
         auto symbol = m_stack.find(instr.operand1);
         assert(symbol);

         write(std::format("dec QWORD [rbp - {}]", symbol->offset), std::format("{}--", symbol->name));
         break;
      }

      case OpCode::ADD: case OpCode::SUB:
      case OpCode::MUL: case OpCode::DIV:
      case OpCode::MOD:
      case OpCode::AND: case OpCode::OR:
         handleBinary(instr);
         break;

      case OpCode::NEG: {
         if(instr.operand1 == ir::TOS)
            m_stack.pop("rax");
         else
            movFoldedValue("rax", instr.operand1);

         write("neg rax");
         m_stack.push("rax");
         break;
      }

      case OpCode::NOT: {
         if(instr.operand1 == ir::TOS)
            m_stack.pop("rax");
         else
            movFoldedValue("rax", instr.operand1);

         // xor gives 1 if different, 0 if same
         write("xor rax, 1", "!rax"); // condition will always be 0 or 1 since it's bool
         m_stack.push("rax");
         break;
      }

      case OpCode::EQ:  case OpCode::NEQ:
      case OpCode::LT:  case OpCode::GT:
      case OpCode::LTE: case OpCode::GTE:
         handleComparison(instr);
         break;

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

      case OpCode::JUMP:
         write("jmp " + instr.operand1);
         break;

      case OpCode::JUMP_IF:
      case OpCode::JUMP_IF_NOT:
         handleCondJump(instr);
         break;

      case OpCode::SCOPE_START: m_stack.startScope(); break;
      case OpCode::SCOPE_END:   m_stack.endScope();   break;

      default: assert(false && "Unhandled opcode");
   }
}

#pragma endregion
