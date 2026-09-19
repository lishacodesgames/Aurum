#pragma once

// currently only has 2, so I'm keeping a bool in Error. But might add more severities later
// enum class Severity { WARNING, FATAL };

namespace err
{
   enum class Phase { NONE, SETUP, LEXING, PARSING, GENERATING, EMITTING_ASSEMBLY, RUNNING };

   enum class Category {
      NONE,             // fallback, unitialised value (of this enum, not an error category)
      SYNTAX,           // unexpected character/token, unclosed comment
      NAME_RESOLUTION,  // undeclared identifier, redeclaration
      MUTABILITY,       // modifying immutable variable
      TYPE_MISMATCH,    // incompatible value or operation types
      SCOPING,          // eg. declaration as an if statement's then branch
   };

   struct SourceLocation {
      std::string file;
      std::uint32_t row = 1, column = 1;

      std::string to_string() const;

      /// @param fileName only base name, no path
      SourceLocation(std::string_view fileName, std::uint32_t row = 1, std::uint32_t col = 1) : file(fileName), row(row), column(col) {}
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

   bool empty() const noexcept { return m_errors.empty(); }

private:
   std::vector<Error> m_errors{};

private:
   void printAll() const;
};

inline ErrorReporter g_errors;
