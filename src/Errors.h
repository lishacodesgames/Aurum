#pragma once

// currently only has 2, so I'm keeping a bool in Error. But might add more severities later
// enum class Severity { WARNING, FATAL };

enum class Phase { NONE, SETUP, TOKENIZING, PARSING, GENERATING, EMITTING_ASSEMBLY };

enum class Category {
   NONE, // fallback, unitialised value (of this enum, not an error category)
   SYNTAX, // unexpected character/token, unclosed comment
   NAME_RESOLUTION, // undeclared identifier, redeclaration
   MUTABILITY, // modifying immutable variable
   INTERNAL // compiler-side violations; opcode vs operand mismatch, etc.
};

struct SourceLocation {
   /// @todo make input based
   std::string file = "gold.aura"; /// only base name, no path
   std::uint32_t row = 0, column = 0;

   std::string to_string() const {
      return std::format("{}:{}:{}", file, row, column);
   }
};

/// @todo put into cpp
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

struct Error {
   Phase phase;
   Category category;
   SourceLocation location;
   std::string message;
   bool isFatal = false;

   /// Formats as "[FATAL ]{CATEGORY} ERROR during {PHASE} at {file}:{row}:{col}: {message}"
   std::string to_string() const {
      return std::format(
         "\033[38:5:98m{}{} ERROR during {} at {}: {}\033[0m\n",
         isFatal ? "FATAL " : "", ::to_string(category), ::to_string(phase), location.to_string(), message);
   }

   Error(Phase phase, Category category, SourceLocation location, std::string_view message, bool isFatal = false)
      : phase(phase), category(category), location(location), message(message), isFatal(isFatal) {}
};

class ErrorReporter {
public:
   void report(Phase phase, Category category, SourceLocation location, std::string_view message, bool isFatal = false) {
      report(Error{ phase, category, location, message, isFatal });
   }

   /// appends FIRST, then checks if it's fatal. If fatal, calls printAll and throws runtime_error with fatal's msg
   void report(const Error& error) {
      m_errors.push_back(error);

      if(error.isFatal) {
         printAll();
         throw std::runtime_error(error.to_string());
      }
   }

   void printAll() const {
      for(const Error& error : m_errors)
         std::println("{}", error.to_string());
   }

   bool empty() const noexcept { return m_errors.empty(); }
   std::size_t count() const noexcept { return m_errors.size(); }

private:
   std::vector<Error> m_errors{};
};
