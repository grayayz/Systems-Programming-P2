CS 214 Spring - Project 2: Word Frequency Distribution

AUTHORS:
Janine Al-Zahiri - joa43
Antonio Pichardo - ap2694


**DESIGN OVERVIEW:**

--DATA STRUCTURES--
Our implementation uses two core structures:

wfd_node - a singly linked list node representing one unique word:
  - word (char *): heap-allocated string via strdup
  - count (int): number of times the word appears in the file
  - frequency (double): count / total_word_count, computed after all words are read
  - next (wfd_node *): pointer to the next node; list is kept in lexicographic order

file_wfd - represents one processed file:
  - file_path (char *): heap-allocated copy of the file's path
  - total_word_count (int): total number of word tokens seen in the file
  - node (wfd_node *): pointer to the head of the file's wfd_node linked list

--WORD TOKENIZATION--
process_file() reads a file using open/read/close:
  1. Reads in chunks of BUFFER_SIZE (1024) bytes into a stack buffer
  2. Accumulates word characters (letters, digits, hyphens) into a separate
     heap-allocated word_buf that lives outside the read loop, so words
     that span two read() calls are assembled correctly
  3. On a non-word character, if word_buf is non-empty, it is lowercased,
     null-terminated, and passed to add_word(); the accumulator is then reset
  4. After the read loop, a flush step handles any word remaining in word_buf
     (for files that do not end with whitespace)
  5. node and total_word_count are initialized to NULL/0 immediately after
     malloc to avoid undefined behavior on the first add_word() call

--SORTED INSERTION--
add_word() inserts or increments a word in the linked list:
  1. Traverse the list comparing the new word lexicographically to each node
  2. If an exact match is found, increment count and return
  3. Otherwise, allocate a new node with strdup(word) and splice it between
     prev and current to maintain alphabetical order
  4. If prev == NULL, the new node becomes the new head
  5. Returns the (possibly updated) head; callers must reassign:
       new_file->node = add_word(new_file->node, word_buf)

--FREQUENCY COMPUTATION--
frequency() makes a single pass over a file's linked list after all words
are inserted and computes:
  frequency = count / (double) total_word_count

--FILE COLLECTION--
collect_files() recursively traverses a path using stat/opendir/readdir:
  - Regular file: process if is_explicit (given on command line) or filename
    ends with SUFFIX (.txt); call process_file() and append to files array
  - Directory: iterate entries, skip any whose name starts with '.', build
    full child path using snprintf(), and recurse by setting is_explicit = 0
  - The files array grows dynamically by using realloc. When full, the capacity is doubled.
  - Initialized in main with capacity = 8

--MEMORY MANAGEMENT--
  - free_wfd() frees each node's word string before freeing the node itself
  - free_file_wfd() frees file_path, the wfd_node list, and the file_wfd struct
  - word_buf is freed at the end of process_file()
  - The files array and all its contents are freed at the end of main()

**TEST PLAN**

TEST 1 - Similar files (low JSD):
sports1.txt vs sports2.txt
output: 0.50000 sports1.txt sports2.txt
expected: low JSD since files share vocabulary (basketball, player, team, etc.)
result: PASS

TEST 2 - Similar files (high JSD):
cooking.txt vs tech.txt
output: 0.86623 cooking.txt tech.txt
expected: high JSD since files share no vocabulary
result: PASS

TEST 3 - Directory traversal
./compare testdir/
expected: recursively collects all .txt files, computes all pairs
result: PASS — all pairs printed, sorted by combined word count descending

TEST 4 - Single file (too few files):
./compare sports1.txt
expected: error message and non-zero exit
result: PASS — "compare: fewer than two input files after collection"

TEST 5 - Nonexistent file:
./compare fake.txt sports1.txt
expected: stat error printed, exit (fake.txt does not exist)
result: PASS

TEST 6 - Empty file:
./compare empty.txt sports1.txt
expected: warning printed, file skipped, exit due to too few files
result: PASS — "Warning: skipping empty file empty.txt"

TEST 7 - Case normalization:
file: mixed_case.txt (repeated words in varying cases)
command: ./compare mixed_case.txt repeated_words.txt
expected: all case variants merge into one entry after lowercasing
output: 0.32654 mixed_case.txt repeated_words.txt
result: PASS

TEST 8 - Hyphenated words:
file: hyphenated.txt (words containing hyphens)
command: ./compare hyphenated.txt mixed_case.txt
expected: hyphens treated as word characters, whole token kept as one word
output: 1.00000 hyphenated.txt mixed_case.txt
result: PASS

**COMPILATION AND EXECUTION**

--BUILDING--
  make          - Compile all programs
  make clean    - Remove all executables and object files

--RUNNING TESTS--
  ./compare test.txt        - Run on a single file
  ./compare testdir/        - Run on a directory recursively
  ./compare test.md         - Run on an explicit non-.txt file

--EXPECTED RESULTS--
Correctness and edge case tests:
  - Exit with status 0; print per-file word/frequency table

Error detection tests:
  - Print an error message and exit with a non-zero status


**CONTRIBUTIONS**

Janine:
  -Worked on collection phase
  -Implemented helper methods: add_word, has_suffix, free_wfd, free_file_wfd
  -Implemented data structures wfd_node and file_wfd
  -Implemented methods process_file, frequency, collect_files

Tony:
  - analysis phase: pair_analysis, analysis_pair_count, analysis_phase, analysis_exit_if_too_few_files
  - JSD: merge two sorted wfd lists, mean freq per word, KLD terms with log2, sqrt for final distance
  - qsort comparator (descending combined_word_count); asserts on frequencies in [0,1]
  - main: copy collected files into a contiguous file_wfd row, run analysis, printf("%.5f ...") per pair, free results
  - sample .txt fixtures in repo root and testdir/ for manual runs

Both partners:
  - Designed data structures together
  - Wrote and ran test cases together
  - Wrote README together
