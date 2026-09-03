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

   std::string to_string() const;
};

/// @todo put into cpp
std::string to_string(Phase phase); 
std::string to_string(Category category);

struct Error {
   Phase phase;
   Category category;
   SourceLocation location;
   std::string message;
   bool isFatal = false;

   Error(Phase phase, Category category, SourceLocation location, std::string_view message, bool isFatal = false)
      : phase(phase), category(category), location(location), message(message), isFatal(isFatal) {}

   /// Formats as "[FATAL ]{CATEGORY} ERROR during {PHASE} at {file}:{row}:{col}: {message}"
   std::string to_string() const;
};

class ErrorReporter {
public:
   /// appends FIRST, then checks if it's fatal. If fatal, calls printAll and throws runtime_error with fatal's msg
   void report(Phase phase, Category category, SourceLocation location, std::string_view message, bool isFatal = false);
   void printAll() const;

   bool empty() const noexcept { return m_errors.empty(); }
   std::size_t count() const noexcept { return m_errors.size(); }

private:
   std::vector<Error> m_errors{};
};

inline ErrorReporter g_errors;
