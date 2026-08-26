#include <pch/Precompiled.h>
#include "Stack.h"

#include "Errors.h"

std::string Stack::push(std::optional<std::string_view> value, bool isMutable, std::string_view name) {
   m_size += 8;
   m_stack.emplace_back(name, m_size, isMutable);

   if(value)
      return std::format("push {}", *value);
   else
      return "sub rsp, 8";
}

std::string Stack::pop(std::string_view reg) {
   m_stack.pop_back();
   m_size -= 8;

   return std::format("pop {}", reg);
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
   
   LOG_ERROR("Tried to get offset of a variable that doesn't exist: '{}'", name);
   return 0;   
}

std::optional<std::vector<Symbol>::iterator> Stack::get(std::string_view name) {
   auto it = std::find_if(m_stack.begin(), m_stack.end(), [name](const Symbol& symbol) { return symbol.name == name; });
   if(it != m_stack.end())
      return it;

   return std::nullopt;
}

std::optional<std::vector<Symbol>::const_iterator> Stack::get(std::string_view name) const {
   const auto it = std::find_if(m_stack.begin(), m_stack.end(), [name](const Symbol& symbol) { return symbol.name == name; });
   if(it == m_stack.end())
      return std::nullopt;

   return it;
}
