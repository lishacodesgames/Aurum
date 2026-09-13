#pragma once

// currently only has 2, so I'm keeping a bool in Error. But might add more severities later
// enum class Severity { WARNING, FATAL };

namespace err
{
   enum class Phase { NONE, SETUP, TOKENIZING, PARSING, GENERATING, EMITTING_ASSEMBLY, RUNNING };

   enum class Category {
      NONE, // fallback, unitialised value (of this enum, not an error category)
      SYNTAX, // unexpected character/token, unclosed comment
      NAME_RESOLUTION, // undeclared identifier, redeclaration
      MUTABILITY, // modifying immutable variable
      TYPE_MISMATCH, // incompatible value or operation types
      INTERNAL // compiler-side violations; opcode vs operand mismatch, etc.
   };

   struct SourceLocation {
      std::string file = "gold.aura"; /// only base name, no path
      std::uint32_t row = 1, column = 1;

      std::string to_string() const;
   };
}

std::string to_string(err::Phase phase); 
std::string to_string(err::Category category);

struct Error {
   err::Phase phase;
   err::Category category;
   err::SourceLocation location;
   std::string message;
   bool isFatal = false;

   /// Formats as "[FATAL ]{CATEGORY} ERROR during {PHASE} at {file}:{row}:{col}: {message}"
   std::string to_string() const;
};

class ErrorReporter {
public:
   /// appends FIRST, then checks if it's fatal. If fatal, calls printAll and throws runtime_error with fatal's msg
   void report(err::Phase phase, err::Category category, err::SourceLocation location, std::string_view message, bool isFatal = false);
   void throwAll(err::Phase phase) const;
   void printAll() const;

   bool empty() const noexcept { return m_errors.empty(); }
   std::size_t count() const noexcept { return m_errors.size(); }

private:
   std::vector<Error> m_errors{};
};

inline ErrorReporter g_errors;
