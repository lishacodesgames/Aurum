# IR Tags used by Aurum

|    OpCode   | Operand 1 | Operand 2 | Effect |
| ----------- | --------- | --------- | ------ |
| `PUSH_INT` | integer literal | - | Push a literal onto the stack
| `PUSH_VAR` | *name* | - | Push variable's current value onto the stack
| `DEF_VAR` | *name* | *value* | Push *value* onto the stack and name it
| `ALLOC_VAR` | *name* | - | Reserve an uninitalised slot and name it
| `STORE_VAR` | *name* (destination) | *value* | Write *value* into *name*'s existing slot
| `INCR` | *name* | - | *name*++
| `DECR` | *name* | - | *name*--
| `ADD`/`SUB`/`MUL`/`DIV`/`MOD` | left *value* | right *value* | Perform binary operation and push result on top of stack
| `NEG` | *value* | - | push (-*value*) on top of the stack |
| `EXIT` | *value* | - | exit syscall with *value* as exitcode
| `LABEL` | label name | - | marks a position, no runtime effect |
| `JUMP` | label name | - | unconditional goto
| `JUMP_FALSE` | label name | condition (@todo shape) | conditional jump
| `SCOPE_START` | - | - | marks beginning of new scope |
| `SCOPE_END` | - | - | marks end of latest scope |

### Legend
- *value* = folded or `$tos` (top of stack)
- *name* = variable name
