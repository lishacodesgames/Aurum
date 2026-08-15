#pragma once

/// @todo proper error system
/// @todo fatal errors vs errors that don't stop compilation (so multiple errors can be logged at once)
/// this distinction needs its own error handling class
/// for now both are logged the same

#define LOG_ERROR(...) std::println(stderr, "\033[38:5:161mFATAL ERROR:\033[0m {}", __VA_ARGS__)
#define FATAL_ERROR(...) do {\
   std::println(stderr, "\033[38:5:161mFATAL ERROR:\033[0m {}", __VA_ARGS__);\
   std::exit(EXIT_FAILURE);\
} while(0)
