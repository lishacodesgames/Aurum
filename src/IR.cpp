#include <pch/Precompiled.h>
#include "IR.h"

namespace ir
{
   uint8_t operands(OpCode opcode) {
      switch(opcode) {
         case OpCode::SCOPE_START:
         case OpCode::SCOPE_END:
            return 0;


         case OpCode::PUSH_INT:
         case OpCode::PUSH_VAR:
         case OpCode::ALLOC_VAR:

         case OpCode::INCR:
         case OpCode::DECR:
         case OpCode::NEG:

         case OpCode::EXIT:
         case OpCode::LABEL:
         case OpCode::JUMP:
            return 1;


         case OpCode::DEF_VAR:
         case OpCode::STORE_VAR:

         case OpCode::ADD:
         case OpCode::SUB:
         case OpCode::MUL:
         case OpCode::DIV:
         case OpCode::MOD:

         case OpCode::JUMP_FALSE:
            return 2;

         default:
            throw std::runtime_error(std::format("How many operands does this opcode have: '{}?!", to_string(opcode)));
      }
   }

   std::string to_string(OpCode opcode) {
      #define X(name) \
         if(opcode == OpCode::name) { \
            return #name; \
         }

         OP_CODES
      #undef X

      return "silver"; // should never run
   }
}  // namespace ir
