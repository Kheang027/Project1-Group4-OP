Project 1 — Multithreaded File Copying

Course: Operating Systems Principles, RMIT Team: Group 4 — Project1-Group4-OP Members: S4204551 · S4197581 · S4000196 · S4055262 Due: Monday, September 7, 2026, 23:59

Task Completion Status

(Update this table as you go — Section 7 requires the README to state exactly which tasks/subtasks are completed.)

Task Status
Task 1 — Multithreaded multiple file copying (mmcopier) ⬜ Not started
Task 2, Subtask 1 — Shared queue + reader/writer threads ⬜ Not started
Task 2, Subtask 2 — Mutex locking ⬜ Not started
Task 2, Subtask 3 — Condition variables (avoid busy waiting) ⬜ Not started

Replace ⬜ with ✅ (done) or 🔶 (in progress) as work is completed.

Build Instructions

This project compiles on the RMIT Core Teaching Servers:

titan.csit.rmit.edu.au
jupiter.csit.rmit.edu.au
saturn.csit.rmit.edu.au

Build:

bash
make all

Builds both mmcopier and mscopier.

Clean up compiled files:

bash
make clean
Running the Programs
Task 1 — mmcopier
bash
./mmcopier n source_dir destination_dir

Copies source1.txt ... sourceN.txt (n between 2 and 10) from source_dir to destination_dir, one thread per file.

Example:

bash
./mmcopier 3 source_dir destination_dir
Task 2 — mscopier
bash
./mscopier n source_file destination_file

Copies source_file to destination_file using n reader threads and n writer threads (n between 2 and 10), coordinated through a shared bounded queue.

Example:

bash
./mscopier 10 input output

You can generate a test source file using the provided script and dictionary:

bash
./generate_text.sh 30 > input
Design Notes

(Fill this in as the implementation progresses — briefly explain how the shared queue, mutex, and condition variables are used, so a marker can follow the design without reading all the code.)

Shared queue:
Mutex locking:
Condition variables (avoiding busy waiting):
Testing

(Note what testing was done, e.g. edge cases, valgrind results, server compilation checks.)

Compiles cleanly with -Wall -Werror on titan/jupiter/saturn
mmcopier tested against sample source_dir
mscopier tested with generated files of varying sizes
Tested with edge cases (empty file, small file, n=2, n=10)
Checked with valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all
