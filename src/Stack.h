#pragma once
#include "Errors.h"

/// @todo type info, once you have more than one type
/// @todo SymbolKind kind; if functions/variables need distinguishing (func vs var)
struct Symbol { // for now, can only be a variable
   std::string name;
   std::uint32_t offset = 0; // offset from rbp, in BYTES

   explicit Symbol(std::string_view name, std::uint32_t offset)
      : name(name), offset(offset) {}
};

class Stack {
public:
   Stack(std::string& emitterOutput) : m_emitterOutput(emitterOutput) {}

   /**
    * @param value the value / register being pushed. nullopt if variable has only been declared and not defined. can also be followed by a comment 
    * @param name name of variable being pushed
    * @return the instruction text to emit for this push
    */
   void push(std::optional<std::string_view> value, std::string_view name = "");

   /// @param reg can be followed by a comment
   void pop(std::string_view reg);

   /// name the variable at the very top of the stack
   void nameTop(std::string_view name) { m_stack.back().name = name; }
   bool contains(std::string_view name) const { return get(name) != std::nullopt; }
   std::optional<Symbol> find(std::string_view name) const;

   /// @return offset from rbp in BYTES
   std::uint32_t offset(std::string_view name) const;

   /// @return size of stack in 8-byte elements
   std::size_t size() const noexcept { return m_stack.size(); }

   void startScope();
   void endScope();

private:
   std::vector<Symbol> m_stack{};
   std::vector<std::size_t> m_scopeMarks{}; /// how many vars existed when each scope was added

   std::string& m_emitterOutput;

private:
   /// private helper function for the 3 public ones: contains, find, offset
   std::optional<std::vector<Symbol>::iterator> get(std::string_view name);
   std::optional<std::vector<Symbol>::const_iterator> get(std::string_view name) const;
};
