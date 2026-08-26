#pragma once

/// @todo type info, once you have more than one type
/// @todo SymbolKind kind; if functions/variables need distinguishing (func vs var)
struct Symbol { // for now, can only be a variable
   std::string name;
   std::uint32_t offset = 0; // offset from rbp, in BYTES
   bool isMutable; // TRUE = bar, FALSE = mint.

   explicit Symbol(std::string_view name, std::uint32_t offset, bool isMutable)
      : name(name), offset(offset), isMutable(isMutable) {}
};

class Stack {
public:
   /**
    * @param name name of variable being pushed
    * @param isMutable if the value can be changed after this.
    * @param value the value / register being pushed. Can be empty if variable has only been declared and not defined
    * @param comment WITH PRECEEDING SEMICOLON
    * @return the instruction text to emit for this push
    */
   void push(std::string& output, std::optional<std::string_view> value, bool isMutable = false, std::string_view name = "");
   void pop(std::string& output, std::string_view reg);

   /// name the variable at the very top of the stack
   void nameTop(std::string_view name) { m_stack.back().name = name; }
   void changeTop(std::string_view name, bool isMutable) {
      nameTop(name);
      m_stack.back().isMutable = isMutable;
   }

   bool contains(std::string_view name) const { return get(name) != std::nullopt; }
   std::optional<Symbol> find(std::string_view name) const;

   /// @return offset from rbp in BYTES
   std::uint32_t offset(std::string_view name) const;

   /// @return size of stack IN BYTES
   std::size_t size() const noexcept { return m_size; }

private:
   std::vector<Symbol> m_stack{};
   std::uint32_t m_size = 0; /// in BYTES

private:
   /// private helper function for the 3 public ones: contains, find, offset
   std::optional<std::vector<Symbol>::iterator> get(std::string_view name);
   std::optional<std::vector<Symbol>::const_iterator> get(std::string_view name) const;
};
