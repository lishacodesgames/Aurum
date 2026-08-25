#pragma once

/// @todo type info, once you have more than one type
/// @todo SymbolKind kind; if functions/variables need distinguishing (func vs var)
struct Symbol { // for now, can only be a variable
   std::uint32_t offset = 0; // offset from rbp, in BYTES
   bool isMutable; // TRUE = bar, FALSE = mint.
};
