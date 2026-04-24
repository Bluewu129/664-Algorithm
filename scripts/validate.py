#!/usr/bin/env python3
"""
validate.py — check that a superstring output contains all input fragments.

Usage:
    python validate.py <input_fragments_file> <output_string>
    python validate.py <input_fragments_file> -  (reads output from stdin)

Exit codes:
    0  PASS
    1  FAIL (missing fragments or wrong usage)
"""

import sys


def read_fragments(path):
    with open(path, "r") as f:
        return [line.rstrip("\r\n") for line in f if line.strip()]


def validate(fragments, superstring):
    failed = []
    for frag in fragments:
        if frag not in superstring:
            failed.append(frag)
    return failed


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <fragments_file> <superstring | ->", file=sys.stderr)
        sys.exit(1)

    fragments_file = sys.argv[1]
    output_arg = sys.argv[2]

    fragments = read_fragments(fragments_file)

    if output_arg == "-":
        superstring = sys.stdin.read().rstrip("\r\n")
    else:
        superstring = output_arg.rstrip("\r\n")

    print(f"Fragments file : {fragments_file}")
    print(f"Fragment count : {len(fragments)}")
    print(f"Superstring    : {repr(superstring)}")
    print(f"Superstring len: {len(superstring)}")
    print()

    failed = validate(fragments, superstring)

    if not failed:
        print(f"PASS — all {len(fragments)} fragments found in superstring.")
        sys.exit(0)
    else:
        print(f"FAIL — {len(failed)} fragment(s) NOT found in superstring:")
        for f in failed:
            print(f"  missing: {repr(f)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
