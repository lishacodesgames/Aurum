#pragma once

/// @todo proper error system

/// just logs the error to stderr stream
#define LOG_ERROR(...) std::println(stderr, "\033[38:5:161mERROR:\033[0m {}", __VA_ARGS__)

/// logs error to stderr stream and exits with 1
#define FATAL_ERROR(...) do {\
   std::println(stderr, "\033[38:5:98mFATAL ERROR:\033[0m {}", __VA_ARGS__);\
   std::exit(EXIT_FAILURE);\
} while(0)
