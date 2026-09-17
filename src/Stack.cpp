#include <pch/Precompiled.h>
#include "Stack.h"

#include "Errors.h"

void Stack::push(std::optional<std::string_view> value, std::string_view name) {
   m_stack.emplace_back(name, (m_stack.size() + 1) * 8);

   if(value)
      m_emitterOutput += std::format("\tpush {}\n", *value);
   else
      m_emitterOutput += "\tsub rsp, 8\n";
}

void Stack::pop(std::string_view reg) {
   m_stack.pop_back();
   m_emitterOutput += std::format("\tpop {}\n", reg);
}

std::optional<Symbol> Stack::find(std::string_view name) const {
   if(auto it = get(name))
      return *it.value();

   return std::nullopt;
}

std::uint32_t Stack::offset(std::string_view name) const {
   if(auto it = get(name))
      return it.value()->offset;
   
   assert(false && "This variable isn't on the stack");
   return 0;
}

void Stack::startScope() {
   m_emitterOutput += std::format("\n\t; SCOPE {} START\n", m_scopeMarks.size());
   m_scopeMarks.push_back(m_stack.size());
}

void Stack::endScope() {
   assert(!m_scopeMarks.empty() && "Tried to end a non-existent scope!");

   std::size_t mark = m_scopeMarks.back();
   m_scopeMarks.pop_back();

   assert(mark <= m_stack.size());
   std::size_t count = m_stack.size() - mark; // how many new variables were in the scope
   if(count == 0) 
      return; // no new memory, nothing to cleanup

   m_emitterOutput += std::format("\n\tadd rsp, {} ; reclaiming scope memory of {} variable(s)\n", count * 8, count);
   m_emitterOutput += std::format("\t; SCOPE {} END\n\n", m_scopeMarks.size());
   m_stack.erase(m_stack.begin() + mark, m_stack.end());
}

// non-const
std::optional<std::vector<Symbol>::iterator> Stack::get(std::string_view name) {
   auto it = std::find_if(m_stack.begin(), m_stack.end(), [name](const Symbol& symbol) { return symbol.name == name; });
   if(it != m_stack.end())
      return it;

   return std::nullopt;
}

// const
std::optional<std::vector<Symbol>::const_iterator> Stack::get(std::string_view name) const {
   const auto it = std::find_if(m_stack.begin(), m_stack.end(), [name](const Symbol& symbol) { return symbol.name == name; });
   if(it != m_stack.end())
      return it;
   
   return std::nullopt;
}
