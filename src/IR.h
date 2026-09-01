#pragma once

#define OP_CODES \
   /* pushing / declaring / popping */ \
   X(PUSH_INT) X(PUSH_VAR) \
   X(DEF_VAR) X(ALLOC_VAR) X(STORE_VAR) \
\
   /* in-place, no push/pop involved */ \
   X(INCR) X(DECR) \
\
   /* operators */ \
   X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(NEG) \
\
   /* control flow */ \
   X(EXIT) \
   X(LABEL) X(JUMP) X(JUMP_FALSE) \
\
   /* scope */ \
   X(SCOPE_START) X(SCOPE_END)

namespace ir
{
   inline const std::string TOS = "$tos"; /// an operand that says: the value that is currently on the stack, before the running of this operation
   enum class OpCode {
      #define X(name) name,
         OP_CODES
      #undef X
   };

   /// @return 0, 1 or 2
   uint8_t operands(OpCode opcode); /// how many operands does this opcode require
   std::string to_string(OpCode opcode);

   struct Instruction {
      OpCode opcode;
      std::optional<std::string> operandLeft = std::nullopt; // nullopt = whatever's on the stack
      std::optional<std::string> operandRight = std::nullopt; // nullopt = whatever's on the stack

      explicit Instruction(OpCode opcode) : opcode(opcode) {}

      explicit Instruction(OpCode opcode, std::string_view operandLeft)
         : opcode(opcode), operandLeft(operandLeft) {}

      // convenience overload: takes views, owns copies internally.
      explicit Instruction(OpCode opcode, std::string_view operandLeft, std::string_view operandRight)
         : opcode(opcode), operandLeft(operandLeft), operandRight(operandRight) {}
   };
}
