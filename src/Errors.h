#pragma once

/// @todo proper error system

/// just logs the error to stderr stream
#define LOG_ERROR(...) do {\
   std::print(stderr, "\033[38:5:98mFATAL ERROR:\033[0m");\
   std::println(__VA_ARGS__);\
} while(0)

/// logs error to stderr stream and exits with 1
#define FATAL_ERROR(...) do {\
   std::print(stderr, "\033[38:5:98mFATAL ERROR:\033[0m");\
   std::println(__VA_ARGS__);\
   std::exit(EXIT_FAILURE);\
} while(0)
