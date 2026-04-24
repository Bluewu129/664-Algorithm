# Test Suite — Shortest Superstring / Text Reconstruction

Each file contains one fragment per line. The goal is the shortest string containing all fragments as substrings.

| File | Category | Fragments | Expected output | Notes |
|------|----------|-----------|-----------------|-------|
| 01_trivial_two_fragments | trivial | `ab`, `bc` | `abc` (len 3) | minimal overlap |
| 02_trivial_one_fragment | trivial | `abc` | `abc` (len 3) | single fragment, answer = itself |
| 03_small_chain | small | `abc`,`bcd`,`cde` | `abcde` (len 5) | linear chain of overlaps |
| 04_small_nested | small | `the`,`he`,`hero`,`ero` | `hero` (len 4) | all substrings of one word |
| 05_small_no_overlap | small | `ab`,`cd`,`ef` | `abcdef` / any concat (len 6) | multiple valid orderings |
| 06_medium_long_chain | medium | 9 fragments, window-5 | `abcdefghijklm` (len 13) | sliding window chain |
| 07_medium_all_substrings | medium | `abcde` + 4 sub-fragments | `abcde` (len 5) | all are substrings of longest |
| 08_medium_cyclic | medium | `abc`,`bca`,`cab` | `abca` or `bcab` or `cabc` (len 4) | cyclic overlap, multiple solutions |
| 09_edge_all_identical | edge | `aaa` × 3 | `aaa` (len 3) | duplicates must be handled |
| 10_edge_substring_of_another | edge | `abcdef`,`bcd`,`cd` | `abcdef` (len 6) | dominated fragments |
| 11_edge_no_overlap_at_all | edge | `abcde`,`fghij`,`klmno` | any concat (len 15) | zero overlap, order matters for optimality |
| 12_edge_full_overlap_cyclic | edge | 5 rotations of `abcde` | `abcdea` or similar (len 6) | tricky cyclic structure |
