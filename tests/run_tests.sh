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

# --- sanity check: must be run from project root --------------------------
if [[ ! -d "scripts" ]]; then
   echo "${RED}Run this script from the project root (the folder containing 'scripts/').${RESET}"
   exit 1
fi

# --- compile the compiler in Release Mode, so all tests run quickly -------
cmake --preset Release
cmake --build --preset Release

# -------------------- path to built compiler binary ------------------------
COMPILER_BIN="${COMPILER_BIN:-build/Release/Aurum}"
# ---------------------------------------------------------------------------

FILTER="${1:-}"       # optional substring filter, e.g. "arithmetic"
TESTS_DIR="tests"
PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

# NASM and Clang are process-heavy; more workers can be slower due to contention.
TEST_JOBS="${AURUM_TEST_JOBS:-4}"

if ! [[ "$TEST_JOBS" =~ ^[1-9][0-9]*$ ]]; then
   echo "AURUM_TEST_JOBS must be a positive integer (got '$TEST_JOBS')." >&2
   exit 1
fi

# workers write here so their output does not get mixed together on the terminal
RESULT_DIR=$(mktemp -d)
trap 'rm -rf "$RESULT_DIR"' EXIT

# colors (fall back gracefully if terminal doesn't support them)
if [[ -t 1 ]]; then
   GREEN=$'\033[0;32m'; RED=$'\033[0;31m'; YELLOW=$'\033[0;33m'; RESET=$'\033[0m'
else
   GREEN=""; RED=""; YELLOW=""; RESET=""
fi

# --- sanity check: compiler binary exists and is executable ----------------
if [[ ! -x "$COMPILER_BIN" ]]; then
   echo "${RED}Compiler binary not found or not executable at '${COMPILER_BIN}'.${RESET}"
   echo "Build the project first, or set COMPILER_BIN=path/to/aurum before running this script."
   exit 1
fi

# --- helpers: run a single test file ----------------------------------------
# returns 0 for pass, 1 for fail, or 2 for a skipped fixture
run_one_test() {
   local aura_file="$1"
   local expected_file="${aura_file%.aura}.expected"
   local name="${aura_file#$TESTS_DIR/}"

   if [[ ! -f "$expected_file" ]]; then
      echo "${YELLOW}SKIP${RESET}  $name  (no .expected file found)"
      return 2
   fi

   # capture the compiler's own stdout+stderr and exit code
   local compiler_output
   compiler_output=$("$COMPILER_BIN" "$aura_file" --no-run 2>&1)
   local compiler_exit=$?

   # first line of the .expected file tells us which kind of test this is
   local first_line
   first_line=$(head -n1 "$expected_file" | tr -d '[:space:]')

   # convert the string to lowercase using tr
   local first_line_lower
   first_line_lower=$(echo "$first_line" | tr '[:upper:]' '[:lower:]')

   if [[ "$first_line_lower" == "fail" ]]; then
      # --- NEGATIVE TEST: compiler must refuse to compile this file ---
      if [[ $compiler_exit -eq 0 ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected compilation to fail, but it succeeded)"
         return 1
      fi

      # optional second line = substring that must appear in the error output
      local expected_substring
      expected_substring=$(sed -n '2p' "$expected_file")
      if [[ -n "$expected_substring" ]] && [[ "$compiler_output" != *"$expected_substring"* ]]; then
         echo "${RED}FAIL${RESET}  $name  (compiler failed as expected, but error text didn't mention '$expected_substring')"
         echo "        --- compiler output ---"
         echo "$compiler_output" | sed 's/^/        /'
         return 1
      fi

      echo "${GREEN}PASS${RESET}  $name"
      echo "        Compile successfully failed with output:"
      echo "$compiler_output" | sed 's/^/        /'
      return 0
   else
      # --- POSITIVE TEST: compiler must succeed, executable must exit with N ---
      local expected_exit="$first_line_lower"

      if [[ $compiler_exit -ne 0 ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected successful compile, compiler exited $compiler_exit)"
         echo "        --- compiler output ---"
         echo "$compiler_output" | sed 's/^/        /'
         return 1
      fi

      # the compiled executable lives at out/<stem>, per FileHandler's naming
      local stem
      stem=$(basename "$aura_file" .aura)
      local exe_path="out/$stem"

      if [[ ! -x "$exe_path" ]]; then
         echo "${RED}FAIL${RESET}  $name  (compiler reported success but '$exe_path' wasn't produced)"
         return 1
      fi

      "$exe_path" > /dev/null 2>&1
      local actual_exit=$?

      if [[ "$actual_exit" -ne "$expected_exit" ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected exit $expected_exit, got $actual_exit)"
         return 1
      fi

      echo "${GREEN}PASS${RESET}  $name"
      return 0
   fi
}

run_worker() {
   # 2 parameters: index (1-based), aura_file
   local index="$1"
   local aura_file="$2"

   # save output and status separately; the parent prints them in test order later
   run_one_test "$aura_file" >"$RESULT_DIR/$index.output" 2>&1
   echo "$?" >"$RESULT_DIR/$index.status"
}

# --- count the number of tests to run ---------------------------------------
TEST_COUNT=0
while IFS= read -r -d '' aura_file; do
   if [[ -n "$FILTER" ]] && [[ "$aura_file" != *"$FILTER"* ]]; then
      continue
   fi

   ((TEST_COUNT++))
done < <(find "$TESTS_DIR" -name '*.aura' -print0 | sort -z)

if [[ "$TEST_COUNT" -eq 0 ]]; then
   echo "No tests found in '$TESTS_DIR'."
   exit 0
fi

# --- discover and run every fixture -----------------------------------------
START_SECONDS=$SECONDS
TEST_INDEX=0
PIDS=() # process ids

echo "Running $TEST_COUNT tests in '$TESTS_DIR' (max $TEST_JOBS concurrent jobs)..."
while IFS= read -r -d '' aura_file; do
   if [[ -n "$FILTER" ]] && [[ "$aura_file" != *"$FILTER"* ]]; then
      continue
   fi
   ((TEST_INDEX++))
   run_worker "$TEST_INDEX" "$aura_file" & # run in background
   PIDS+=("$!") # save its PID so we can wait for it later

   # wait for this batch before starting more compiler and linker processes
   if [[ ${#PIDS[@]} -ge $TEST_JOBS ]]; then
      for pid in "${PIDS[@]}"; do
         wait "$pid"
      done
      PIDS=()
   fi
done < <(find "$TESTS_DIR" -name '*.aura' -print0 | sort -z)

# the last batch is usually not full, but still needs to finish
for pid in "${PIDS[@]-}"; do
   [[ -n "$pid" ]] && wait "$pid"
done

# workers finish at different times; replay their results in fixture order
for ((index = 1; index <= TEST_COUNT; index++)); do
   cat "$RESULT_DIR/$index.output"
   case $(<"$RESULT_DIR/$index.status") in
      0) ((PASS_COUNT++)) ;;
      1) ((FAIL_COUNT++)) ;;
      2) ((SKIP_COUNT++)) ;;
      *)
         echo "${RED}FAIL${RESET}  test worker $index exited unexpectedly"
         ((FAIL_COUNT++))
         ;;
   esac
done
ELAPSED_SECONDS=$(( SECONDS - START_SECONDS ))

# --- summary ------------------------------------------------------------
echo -e "\n-----------------------------------" # enable backslash escapes with -e
echo "${GREEN}${PASS_COUNT} passed${RESET}, ${RED}${FAIL_COUNT} failed${RESET}, ${YELLOW}${SKIP_COUNT} skipped${RESET}"
echo "${TEST_COUNT} tests completed in: ${ELAPSED_SECONDS}s"

# non-zero exit if anything failed — useful for wiring into CI
[[ $FAIL_COUNT -eq 0 ]]
