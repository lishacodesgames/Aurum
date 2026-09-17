#include <pch/Precompiled.h>
#include "IR.h"

#include "Errors.h"

uint8_t operands(OpCode opcode) {
   switch(opcode) {
      case OpCode::PUSH_INT:
      case OpCode::PUSH_BOOL:
      case OpCode::PUSH_VAR:
      case OpCode::ALLOC_VAR:

      case OpCode::INCR:
      case OpCode::DECR:

      case OpCode::NEG:
      case OpCode::NOT:

      case OpCode::EXIT:
      case OpCode::LABEL:
      case OpCode::FUNC: /// @todo might have 3 where 3rd is an array of params
      case OpCode::JUMP:

      case OpCode::SCOPE_START:
      case OpCode::SCOPE_END:
         return 1;


      case OpCode::DEF_VAR:
      case OpCode::STORE_VAR:

      case OpCode::ADD:
      case OpCode::SUB:
      case OpCode::MUL:
      case OpCode::DIV:
      case OpCode::MOD:

      case OpCode::AND:
      case OpCode::OR:

      case OpCode::EQ:
      case OpCode::NEQ:
      case OpCode::LT:
      case OpCode::GT:
      case OpCode::LTE:
      case OpCode::GTE:

      case OpCode::JUMP_IF:
      case OpCode::JUMP_IF_NOT:
         return 2;


      default: assert(false && "How many operands does this opcode have?!");
   }
}

std::string to_string(OpCode opcode) {
   #define X(name) \
      if(opcode == OpCode::name) { \
         return #name; \
      }

      OP_CODES
   #undef X

   // should never run
   return "idk";
}

bool isIndented(OpCode opcode) {
   switch(opcode) {
      case OpCode::FUNC:
      case OpCode::LABEL:
         return false;

      default: return true;
   }
}
