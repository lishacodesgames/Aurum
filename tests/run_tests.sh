#!/usr/bin/env bash
#
# run_tests.sh — golden/end-to-end test runner for the Aurum compiler.
#
# Convention: every "<name>.aura" fixture under tests/ has a sibling
# "<name>.expected" file that describes what SHOULD happen:
#
#   - a bare integer (e.g. "5")      -> compiler must succeed, and the
#                                        PRODUCED EXECUTABLE must exit with
#                                        that code.
#   - "FAIL" on its own line,
#     optionally followed by a       -> the COMPILER ITSELF must exit
#     substring on the next line        non-zero (i.e. it must refuse to
#                                        compile this file). If a substring
#                                        is given, it must appear somewhere
#                                        in the compiler's stdout/stderr —
#                                        this catches the case where the
#                                        compiler fails, but for the WRONG
#                                        reason (regression in error text).
#
# Usage:
#   ./tests/run_tests.sh              # run every test under tests/
#   ./tests/run_tests.sh arithmetic   # only run fixtures whose path contains "arithmetic"
#
# IMPORTANT: must be run from the project root (the directory containing
# 'scripts/'), because FileHandler checks for that at construction time.

set -uo pipefail  # NOTE: deliberately not -e — a failing test must not kill the whole run

# -------------------- path to built compiler binary ------------------------
COMPILER_BIN="${COMPILER_BIN:-build/Debug/aurum}" # @todo adjust for release and symbols too
# ---------------------------------------------------------------------------

FILTER="${1:-}"       # optional substring filter, e.g. "arithmetic"
TESTS_DIR="tests"
PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

# colors (fall back gracefully if terminal doesn't support them)
if [[ -t 1 ]]; then
   GREEN=$'\033[0;32m'; RED=$'\033[0;31m'; YELLOW=$'\033[0;33m'; RESET=$'\033[0m'
else
   GREEN=""; RED=""; YELLOW=""; RESET=""
fi

# --- sanity checks ----------------------------------------------------------
if [[ ! -d "scripts" ]]; then
   echo "${RED}Run this script from the project root (the folder containing 'scripts/').${RESET}"
   exit 1
fi

if [[ ! -x "$COMPILER_BIN" ]]; then
   echo "${RED}Compiler binary not found or not executable at '${COMPILER_BIN}'.${RESET}"
   echo "Build the project first, or set COMPILER_BIN=path/to/aurum before running this script."
   exit 1
fi

# --- helper: run a single test file -----------------------------------------
run_one_test() {
   local aura_file="$1"
   local expected_file="${aura_file%.aura}.expected"
   local name="${aura_file#$TESTS_DIR/}"

   if [[ ! -f "$expected_file" ]]; then
      echo "${YELLOW}SKIP${RESET}  $name  (no .expected file found)"
      ((SKIP_COUNT++))
      return
   fi

   # capture the compiler's own stdout+stderr and exit code
   local compiler_output
   compiler_output=$("$COMPILER_BIN" "$aura_file" 2>&1)
   local compiler_exit=$?

   # first line of the .expected file tells us which kind of test this is
   local first_line
   first_line=$(head -n1 "$expected_file" | tr -d '[:space:]')

   # convert the string to lowercase using tr
   local first_line_lower
   first_line_lower=$(echo "$first_line" | tr '[:upper:]' '[:lower:]')

   if [[ "first_line_lower" == "fail" ]]; then
      # --- NEGATIVE TEST: compiler must refuse to compile this file ---
      if [[ $compiler_exit -eq 0 ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected compilation to fail, but it succeeded)"
         ((FAIL_COUNT++))
         return
      fi

      # optional second line = substring that must appear in the error output
      local expected_substring
      expected_substring=$(sed -n '2p' "$expected_file")
      if [[ -n "$expected_substring" ]] && [[ "$compiler_output" != *"$expected_substring"* ]]; then
         echo "${RED}FAIL${RESET}  $name  (compiler failed as expected, but error text didn't mention '$expected_substring')"
         echo "        --- compiler output ---"
         echo "$compiler_output" | sed 's/^/        /'
         ((FAIL_COUNT++))
         return
      fi

      echo "${GREEN}PASS${RESET}  $name"
      ((PASS_COUNT++))
   else
      # --- POSITIVE TEST: compiler must succeed, executable must exit with N ---
      local expected_exit="$first_line_lower"

      if [[ $compiler_exit -ne 0 ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected successful compile, compiler exited $compiler_exit)"
         echo "        --- compiler output ---"
         echo "$compiler_output" | sed 's/^/        /'
         ((FAIL_COUNT++))
         return
      fi

      # the compiled executable lives at out/<stem>, per FileHandler's naming
      local stem
      stem=$(basename "$aura_file" .aura)
      local exe_path="out/$stem"

      if [[ ! -x "$exe_path" ]]; then
         echo "${RED}FAIL${RESET}  $name  (compiler reported success but '$exe_path' wasn't produced)"
         ((FAIL_COUNT++))
         return
      fi

      "$exe_path" > /dev/null 2>&1
      local actual_exit=$?

      if [[ "$actual_exit" -ne "$expected_exit" ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected exit $expected_exit, got $actual_exit)"
         ((FAIL_COUNT++))
         return
      fi

      echo "${GREEN}PASS${RESET}  $name"
      ((PASS_COUNT++))
   fi
}

# --- discover and run every fixture -----------------------------------------
while IFS= read -r -d '' aura_file; do
   if [[ -n "$FILTER" ]] && [[ "$aura_file" != *"$FILTER"* ]]; then
      continue
   fi
   run_one_test "$aura_file"
done < <(find "$TESTS_DIR" -name '*.aura' -print0 | sort -z)

# --- summary ------------------------------------------------------------
echo ""
echo "-----------------------------------"
echo "${GREEN}${PASS_COUNT} passed${RESET}, ${RED}${FAIL_COUNT} failed${RESET}, ${YELLOW}${SKIP_COUNT} skipped${RESET}"

# non-zero exit if anything failed — useful once/if you wire this into CI
[[ $FAIL_COUNT -eq 0 ]]
