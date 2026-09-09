#include <pch/Precompiled.h>
#include "IR.h"

#include "Errors.h"

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


      case OpCode::DEF_VAR_CONST:
      case OpCode::DEF_VAR_MUT:
      case OpCode::STORE_VAR:

      case OpCode::ADD:
      case OpCode::SUB:
      case OpCode::MUL:
      case OpCode::DIV:
      case OpCode::MOD:

      case OpCode::JUMP_FALSE:
         return 2;


      default:
         g_errors.report(err::Phase::GENERATING, err::Category::INTERNAL, { "IR.cpp", __LINE__ },
            std::format("How many operands does this opcode have: '{}?!", to_string(opcode)), true);
         return 8;
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
   g_errors.report(err::Phase::GENERATING, err::Category::INTERNAL, { "IR.cpp", __LINE__ }, "idk", true);
   return "";
}
