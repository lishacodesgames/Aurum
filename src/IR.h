#pragma once

#define OP_CODES \
   /* pushing / declaring / popping */ \
   X(PUSH_INT) X(PUSH_BOOL) X(PUSH_VAR) \
   X(DEF_VAR) X(ALLOC_VAR) X(STORE_VAR) \
\
   /* in-place, no push/pop involved */ \
   X(INCR) X(DECR) \
\
   /* operators */ \
   X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(NEG) X(NOT) \
   X(EQ) X(NEQ) X(LT) X(GT) X(LTE) X(GTE) /* EQUALS, NOT EQUALS, LESS THAN, GREATER THAN, LESS THAN EQUALS, GREATER THAN EQUALS */ \
\
   /* control flow */ \
   X(EXIT) \
   X(LABEL) X(FUNC) \
   X(JUMP) X(JUMP_IF) X(JUMP_IF_NOT) \
\
   /* scope */ \
   X(SCOPE_START) X(SCOPE_END)

enum class OpCode {
   #define X(name) name,
      OP_CODES
   #undef X
};

/// @return 1 or 2
/// @note everytime you add a new opcode, update this function
uint8_t operands(OpCode opcode); /// how many operands does this opcode require

std::string to_string(OpCode opcode);
bool isIndented(OpCode opcode); /// when outputting IR

namespace ir
{
   /// an operand that says: the value that is currently on the stack, before the running of this operation
   inline const std::string TOS = "$tos";
   /// an operand that says: the value that is currently SECOND on the stack, before running of this operation
   inline const std::string SOS = "$sos";

   struct Instruction {
      OpCode opcode;
      std::string operand1; // each opcode has atleast 1 operand
      std::optional<std::string> operand2 = std::nullopt;

      explicit Instruction(OpCode opcode, std::string_view operand1)
         : opcode(opcode), operand1(operand1) {}

      // convenience overload: takes views, owns copies internally.
      explicit Instruction(OpCode opcode, std::string_view operand1, std::string_view operand2)
         : opcode(opcode), operand1(operand1), operand2(operand2) {}
   };
}
