#include <pch/Precompiled.h>
#include "Errors.h"

std::string SourceLocation::to_string() const {
   return std::format("{}:{}:{}", file, row, column);
}

std::string to_string(Phase phase) {
   switch(phase) {
      case Phase::NONE:              return "NONE";
      case Phase::SETUP:             return "SETUP";
      case Phase::TOKENIZING:        return "TOKENIZING";
      case Phase::PARSING:           return "PARSING";
      case Phase::GENERATING:        return "GENERATING";
      case Phase::EMITTING_ASSEMBLY: return "EMITTING ASSEMBLY";
   }
}

std::string to_string(Category category) {
   switch(category) {
      case Category::NONE:             return "NONE";
      case Category::SYNTAX:           return "SYNTAX";
      case Category::NAME_RESOLUTION:  return "NAME RESOLUTION";
      case Category::MUTABILITY:       return "MUTABILITY";
      case Category::INTERNAL:         return "INTERNAL";
   }
}

std::string Error::to_string() const {
   return std::format(
      "\033[38:5:98m{}{} ERROR during {} at {}:\033[0m {}\n",
      isFatal ? "FATAL " : "", ::to_string(category), ::to_string(phase), location.to_string(), message);
}

void ErrorReporter::report(Phase phase, Category category, SourceLocation location, std::string_view message, bool isFatal) {
   m_errors.emplace_back(phase, category, location, message, isFatal);

   if(isFatal) {
      printAll();
      throw std::runtime_error(std::string(message));
   }
}

void ErrorReporter::printAll() const {
   for(const Error& error : m_errors)
      std::println("{}", error.to_string());
}
