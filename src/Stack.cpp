#include <pch/Precompiled.h>
#include "Stack.h"

#include "Errors.h"

void Stack::push(std::optional<std::string_view> value, bool isMutable, std::string_view name) {
   m_stack.emplace_back(name, (m_stack.size() + 1) * 8, isMutable);

   if(value)
      m_output += std::format("\tpush {}\n", *value);
   else
      m_output += "\tsub rsp, 8\n";
}

void Stack::pop(std::string_view reg) {
   m_stack.pop_back();
   m_output += std::format("\tpop {}\n", reg);
}

std::optional<Symbol> Stack::find(std::string_view name) const {
   auto it = get(name);
   if(it)
      return *it.value();

   return std::nullopt;
}

std::uint32_t Stack::offset(std::string_view name) const {
   auto it = get(name);
   if(it)
      return it.value()->offset;
   
   m_reporter.report(Phase::EMITTING_ASSEMBLY, Category::INTERNAL, { "Stack.cpp" }, std::format("Tried to get offset of a variable that doesn't exist: '{}'", name), true);
   return 0;   
}

void Stack::startScope() {
   m_scopeMarks.push_back(m_stack.size());
   m_output += std::format("\n\t; Entering scope {}...\n", m_scopeMarks.size());
}

void Stack::endScope() {
   if(m_scopeMarks.empty())
      m_reporter.report(Phase::EMITTING_ASSEMBLY, Category::INTERNAL, { "Stack.cpp"}, "Tried to end a non-existent scope!", true);

   m_output += std::format("\t; Leaving scope {}...\n", m_scopeMarks.size());
   std::size_t mark = m_scopeMarks.back();
   m_scopeMarks.pop_back();

   std::size_t count = m_stack.size() - mark; // how many new variables were in the scope
   if(count == 0) 
      return; // no new memory, nothing to cleanup

   m_output += std::format("\tadd rsp, {} ; reclaiming scope memory of {} variable(s)\n", count * 8, count);
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
