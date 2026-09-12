#!/usr/bin/env bash

#
# run_tests.sh — golden/end-to-end test runner for the Aurum compiler.
#
# Convention: every "<name>.aura" fixture under `tests/` declares its own
# expectation via a `$$` line-comment header at the top of the file. The
# compiler's own tokenizer strips these before parsing, so the header never
# reaches the parser or affects the test program itself:
#
#   $$ EXPECT: <int>                -> compiler must succeed, and the
#                                       PRODUCED EXECUTABLE must exit with
#                                       that code.
#
#   $$ EXPECT: FAIL                  -> the COMPILER ITSELF must exit
#   $$ CONTAINS: <substring>            non-zero (i.e. it must refuse to
#   (CONTAINS line is optional)         compile this file). If given, the
#                                       substring must appear somewhere in
#                                       the compiler's stdout/stderr — this
#                                       catches the case where the compiler
#                                       fails, but for the WRONG reason
#                                       (a regression in error text).
#
# A .aura file with no `$$ EXPECT:` header is SKIPPED, not failed.
#
# Usage:
#   ./tests/run_tests.sh                     # run every test under tests/
#   ./tests/run_tests.sh arithmetic          # only run fixtures whose path contains "arithmetic"
#   AURUM_TEST_JOBS=8 ./tests/run_tests.sh   # run up to 8 tests concurrently (default: 4)
#
# IMPORTANT: must be run from the project root (the directory containing
# 'scripts/'), because FileHandler checks for that at construction time.

set -uo pipefail  # NOTE: deliberately not -e — a failing test must not kill the whole run

# bash's builtin 'time' keyword reports according to this format when timing a command.
# %3R = "real" elapsed wall-clock time, 3 digits after the decimal point (milliseconds).
# using bash's builtin instead of `date +%N` because macOS's BSD `date` doesn't support
# nanosecond formatting at all (that's a GNU coreutils extension) — this way stays portable.
TIMEFORMAT='%3R'

# colors (fall back gracefully if terminal doesn't support them)
if [[ -t 1 ]]; then
   GREEN=$'\033[0;32m'; RED=$'\033[0;31m'; YELLOW=$'\033[0;33m'; RESET=$'\033[0m'
else
   GREEN=""; RED=""; YELLOW=""; RESET=""
fi

# --- sanity check: must be run from project root --------------------------
if [[ ! -d "scripts" ]]; then
   echo "${RED}Run this script from the project root (the folder containing 'scripts/').${RESET}"
   exit 1
fi

# --- compile the compiler in Release Mode, so all tests run quickly -------
echo "Compiling the compiler in Release mode..."
cmake --preset Release
cmake --build --preset Release
echo -e "Done compiling the compiler.\n\n"

# -------------------- path to built compiler binary ------------------------
COMPILER_BIN="${COMPILER_BIN:-build/Release/Aurum}"
# ---------------------------------------------------------------------------

# --- parse arguments ---------------------------------------------------
# supports: ./run_tests.sh [filter] [-j N | --jobs N]
FILTER=""
JOBS_OVERRIDE=""
while [[ $# -gt 0 ]]; do
   case "$1" in
      -j|--jobs)
         JOBS_OVERRIDE="$2"
         shift 2
         ;;
      *)
         FILTER="$1"
         shift
         ;;
   esac
done

TEST_JOBS="${JOBS_OVERRIDE:-${AURUM_TEST_JOBS:-4}}"
TESTS_DIR="tests"
PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

# maps test index -> its display name, so the slowest-tests report at the end
# can print names instead of just index numbers. Populated during dispatch below.
declare -A TEST_NAMES

if ! [[ "$TEST_JOBS" =~ ^[1-9][0-9]*$ ]]; then
   echo "AURUM_TEST_JOBS must be a positive integer (got '$TEST_JOBS')." >&2
   exit 1
fi

# workers write here so their output does not get mixed together on the terminal
RESULT_DIR=$(mktemp -d)
trap 'rm -rf "$RESULT_DIR"' EXIT

# --- sanity check: compiler binary exists and is executable ----------------
if [[ ! -x "$COMPILER_BIN" ]]; then
   echo "${RED}Compiler binary not found or not executable at '${COMPILER_BIN}'.${RESET}"
   echo "Build the project first, or set COMPILER_BIN=path/to/aurum before running this script."
   exit 1
fi

# --- helpers: run a single test file ----------------------------------------
# returns 0 for pass, 1 for fail, or 2 for skip
run_one_test() {
   local aura_file="$1"
   local expect_line
   expect_line=$(grep -m1 '^\$\$ *EXPECT:' "$aura_file")
   local name="${aura_file#$TESTS_DIR/}"

   if [[ -z "$expect_line" ]]; then
      echo "${YELLOW}SKIP${RESET}  $name  (no \$\$ EXPECT: header found)"
      return 2
   fi

   # declaration and assignment must be separate for these locals so we can capture the exit code of the compiler
   # capture the compiler's own stdout+stderr and exit code
   local compiler_output
   compiler_output=$("$COMPILER_BIN" "$aura_file" --no-run 2>&1)
   local compiler_exit=$?

   # first line of the aurum file tells us which kind of output to expect
   local first_line
   first_line=$(echo "$expect_line" | sed 's/^\$\$ *EXPECT: *//' | tr -d '[:space:]')

   # convert the string to lowercase
   local first_line_lower
   first_line_lower=${first_line,,}

   if [[ "$first_line_lower" == "fail" ]]; then
      # --- NEGATIVE TEST: compiler must refuse to compile this file ---
      if [[ $compiler_exit -eq 0 ]]; then
         echo "${RED}FAIL${RESET}  $name  (expected compilation to fail, but it succeeded)"
         return 1
      fi

      local expected_substrings
      mapfile -t expected_substrings < <(grep '^\$\$ *CONTAINS:' "$aura_file" | sed 's/^\$\$ *CONTAINS: *//')

      for expected_substring in "${expected_substrings[@]}"; do
         if [[ -n "$expected_substring" ]] && [[ "$compiler_output" != *"$expected_substring"* ]]; then
            echo "${RED}FAIL${RESET}  $name  (compiler failed as expected, but error text didn't mention '$expected_substring')"
            echo "        --- compiler output ---"
            echo "$compiler_output" | sed 's/^/        /'
            return 1
         fi
      done

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
      stem=${aura_file##*/}
      stem=${stem%.aura}
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
   #
   # the inner `2>&1` only redirects run_one_test's OWN stdout/stderr into .output.
   # `time`'s report is emitted separately, after run_one_test finishes, to whatever
   # fd 2 the *outer* { } block points to — which the trailing `2>` sends to .time.
   # this is the standard way to capture a `time` report without it mixing into the
   # timed command's own output.
   { time run_one_test "$aura_file" >"$RESULT_DIR/$index.output" 2>&1 ; } 2> "$RESULT_DIR/$index.time"
   echo "$?" >"$RESULT_DIR/$index.status"
}

# --- count the number of tests to run ---------------------------------------
mapfile -d '' -t all_aura_files < <(find "$TESTS_DIR" -name '*.aura' -print0 | sort -z)
test_files=()
for aura_file in "${all_aura_files[@]}"; do
   if [[ -z "$FILTER" ]] || [[ "$aura_file" == *"$FILTER"* ]]; then
      test_files+=("$aura_file")
   fi
done

TEST_COUNT=${#test_files[@]}
if [[ "$TEST_COUNT" -eq 0 ]]; then
   echo "No tests found in '$TESTS_DIR'."
   exit 0
fi

# --- discover and run every fixture -----------------------------------------
START_SECONDS=$SECONDS
TEST_INDEX=0
PIDS=() # process ids

echo "Running $TEST_COUNT tests in '$TESTS_DIR' (max $TEST_JOBS concurrent jobs)..."
for aura_file in "${test_files[@]}"; do
   ((TEST_INDEX++))
   TEST_NAMES[$TEST_INDEX]="${aura_file#$TESTS_DIR/}"
   run_worker "$TEST_INDEX" "$aura_file" & # run in background
   PIDS+=("$!") # save its PID so we can wait for it later

   # wait for this batch before starting more compiler and linker processes
   if [[ ${#PIDS[@]} -ge $TEST_JOBS ]]; then
      for pid in "${PIDS[@]}"; do
         wait "$pid"
      done
      PIDS=()
   fi
done

# the last batch is usually not full, but still needs to finish
for pid in "${PIDS[@]-}"; do
   [[ -n "$pid" ]] && wait "$pid"
done

# workers finish at different times; replay their results in fixture order
for ((index = 1; index <= TEST_COUNT; index++)); do
   cat "$RESULT_DIR/$index.output"

   if [[ -s "$RESULT_DIR/$index.time" ]]; then
      echo "        took $(<"$RESULT_DIR/$index.time")s"
   fi

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

# sum of each individual test's own elapsed time. NOTE: since tests run in parallel
# batches of $TEST_JOBS, this can legitimately be LARGER than the wall-clock
# ELAPSED_SECONDS above — the gap between the two is a rough signal of how much
# parallelism you're actually getting (a big gap = jobs mostly running concurrently;
# a small gap = jobs mostly serialized, e.g. contention or TEST_JOBS=1).
TOTAL_TEST_SECONDS=$(cat "$RESULT_DIR"/*.time 2>/dev/null | awk '{sum += $1} END {printf "%.3f", sum}')

# --- summary ------------------------------------------------------------
echo -e "\n-----------------------------------" # enable backslash escapes with -e
echo "${GREEN}${PASS_COUNT} passed${RESET}, ${RED}${FAIL_COUNT} failed${RESET}, ${YELLOW}${SKIP_COUNT} skipped${RESET}"
echo "${TEST_COUNT} tests completed in: ${ELAPSED_SECONDS}s wall-clock (${TOTAL_TEST_SECONDS}s summed across tests)"

# --- slowest tests, for perf work -----------------------------------------
echo -e "\nSlowest tests:"

# save everything to a temporary variable
RAW_TIMING_DATA=$(
   for index in "${!TEST_NAMES[@]}"; do
      # Read the time file using the matching index number
      test_time=$(cat "$RESULT_DIR/$index.time" 2>/dev/null) || test_time="0.000"

      # Print the time (padded with spaces) followed by the name
      printf "%-8s %s\n" "${test_time}s" "${TEST_NAMES[$index]}"
   done
)

# sort the saved text and display the top 5
echo "$RAW_TIMING_DATA" | sort -rn | head -n 5

# non-zero exit if anything failed — useful for wiring into CI
[[ $FAIL_COUNT -eq 0 ]]
