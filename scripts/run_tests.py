#!/usr/bin/env python3
"""
run_tests.py — compile brute-force, run it on every test file, validate output.

Usage:
    python run_tests.py

Expects:
    - src/brute-force_text_reconstruction.c in the project root
    - tests/ directory in the project root
    - validate.py in the same scripts/ directory

Note: brute-force is very slow on medium/large inputs — it will time out after
TIMEOUT_SECONDS and be marked as SKIP rather than FAIL.
"""

import subprocess
import os
import sys
import time

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
BINARY      = os.path.join(PROJECT_DIR, "brute-force_text_reconstruction")
SOURCE      = os.path.join(PROJECT_DIR, "src", "brute-force_text_reconstruction.c")
TESTS_DIR   = os.path.join(PROJECT_DIR, "tests")
VALIDATE    = os.path.join(SCRIPT_DIR, "validate.py")
TIMEOUT_SECONDS = 10


def compile_brute_force():
    # -std=c2x: instructor's brute-force uses C23 `bool` keyword without <stdbool.h>.
    result = subprocess.run(
        ["gcc", "-O2", "-std=c2x", "-o", BINARY, SOURCE],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print("Compile error:\n", result.stderr)
        sys.exit(1)
    print("Compiled OK.\n")


def run_test(test_file):
    start = time.time()
    try:
        result = subprocess.run(
            [BINARY, test_file],
            capture_output=True, text=True,
            timeout=TIMEOUT_SECONDS
        )
        elapsed = time.time() - start
        output = result.stdout.strip()
        return output, elapsed, None
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start
        return None, elapsed, "TIMEOUT"


def validate_output(test_file, superstring):
    result = subprocess.run(
        [sys.executable, VALIDATE, test_file, superstring],
        capture_output=True, text=True
    )
    return result.returncode == 0, result.stdout


def main():
    compile_brute_force()

    test_files = sorted(
        os.path.join(TESTS_DIR, f)
        for f in os.listdir(TESTS_DIR)
        if f.endswith(".txt")
    )

    results = []
    for tf in test_files:
        name = os.path.basename(tf)
        print(f"--- {name} ---")

        output, elapsed, error = run_test(tf)

        if error == "TIMEOUT":
            print(f"  SKIP (timeout after {elapsed:.1f}s — input too large for brute-force)\n")
            results.append((name, "SKIP"))
            continue

        if not output:
            print(f"  FAIL — no output produced ({elapsed:.2f}s)\n")
            results.append((name, "FAIL"))
            continue

        # brute-force may print multiple solutions; take the first line
        first_solution = output.splitlines()[0]
        solutions_count = len(output.splitlines())

        ok, detail = validate_output(tf, first_solution)
        status = "PASS" if ok else "FAIL"
        print(detail.strip())
        if solutions_count > 1:
            print(f"  (brute-force found {solutions_count} solutions of same minimum length)")
        print(f"  Time: {elapsed:.2f}s\n")
        results.append((name, status))

    print("=" * 50)
    print("SUMMARY")
    print("=" * 50)
    for name, status in results:
        print(f"  {status:6s}  {name}")
    passed = sum(1 for _, s in results if s == "PASS")
    skipped = sum(1 for _, s in results if s == "SKIP")
    failed = sum(1 for _, s in results if s == "FAIL")
    print(f"\n  {passed} passed, {skipped} skipped (timeout), {failed} failed")


if __name__ == "__main__":
    main()
