#include <pch/Precompiled.h>
#include "Errors.h"

std::string err::SourceLocation::to_string() const {
   return std::format("{}:{}:{}", file, row, column);
}

std::string to_string(err::Phase phase) {
   using namespace err;
   switch(phase) {
      case Phase::NONE:              return "NONE";
      case Phase::SETUP:             return "SETUP";
      case Phase::TOKENIZING:        return "TOKENIZING";
      case Phase::PARSING:           return "PARSING";
      case Phase::GENERATING:        return "GENERATING";
      case Phase::EMITTING_ASSEMBLY: return "EMITTING ASSEMBLY";
      case Phase::RUNNING:           return "RUNNING";
   }
}

std::string to_string(err::Category category) {
   using namespace err;
   switch(category) {
      case Category::NONE:             return "NONE";
      case Category::SYNTAX:           return "SYNTAX";
      case Category::NAME_RESOLUTION:  return "NAME RESOLUTION";
      case Category::MUTABILITY:       return "MUTABILITY";
      case Category::TYPE_MISMATCH:    return "TYPE MISMATCH";
      case Category::INTERNAL:         return "INTERNAL";
   }
}

std::string Error::to_string() const {
   return std::format(
      "\033[38:5:98m{}{} ERROR during {} at {}:\033[0m {}",
      isFatal ? "FATAL " : "", ::to_string(category), ::to_string(phase), location.to_string(), message);
}

void ErrorReporter::report(err::Phase phase, err::Category category, err::SourceLocation location, std::string_view message, bool isFatal) {
   m_errors.emplace_back(Error{ phase, category, location, std::string(message), isFatal });

   if(isFatal)
      throwAll(phase);
}

void ErrorReporter::throwAll(err::Phase phase) const {
   printAll();
   throw std::runtime_error(std::format("\n\033[38:5:196m{} error{} generated during {}.\033[0m",
      m_errors.size(), m_errors.size() > 1 ? "s" : "", to_string(phase)));
}

void ErrorReporter::printAll() const {
   for(const Error& error : m_errors)
      std::println("{}", error.to_string());
}
