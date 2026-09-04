# Project 1 — Multithreaded File Copying

**Course:** Operating Systems Principles, RMIT  
**Team:** Group 4 — Project1-Group4-OP  
**Members:** S4204551 · S4197581 · S4000196 · S4055262  
**Due:** Monday, September 7, 2026, 23:59

---

## Task Completion Status

Update this table as work progresses. The README must state exactly which tasks and subtasks have been completed.

| Task | Status |
|---|---|
| Task 1 — Multithreaded multiple file copying (`mmcopier`) | ✅ Completed |
| Task 2, Subtask 1 — Shared queue + reader/writer threads | ✅ Completed |
| Task 2, Subtask 2 — Mutex locking | ✅ Completed |
| Task 2, Subtask 3 — Condition variables (avoid busy waiting) | ✅ Completed |

### Status Indicators

- ⬜ Not started
- 🔶 In progress
- ✅ Completed

Replace ⬜ with ✅ or 🔶 as work is completed.

---

## Build Instructions

This project can be compiled and run on any of the RMIT Core Teaching Servers:

- `jupiter.csit.rmit.edu.au`
- `saturn.csit.rmit.edu.au`
- `titan.csit.rmit.edu.au`

### Download and Compile

```bash
ssh <user>@<rmit-server>
wget https://github.com/Kheang027/Project1-Group4-OP/archive/refs/heads/final.zip
unzip final.zip
cd Project1-Group4-OP-final
make all
```

This builds both `mmcopier` and `mscopier`.

---

## Running the Programs

### Task 1 — `mmcopier`

```bash
./bin/mmcopier n source_dir destination_dir
```

Copies `source1.txt` through `sourceN.txt` from `source_dir` to `destination_dir`, using one thread per file.

**Constraints:**

* `n` must be between 2 and 10.
* The source and destination directories must be different.

**Example:**

```bash
mkdir -p tests/tmp/destination_dir
./bin/mmcopier 3 tests/source_dir tests/tmp/destination_dir
```

---

### Task 2 — `mscopier`

```bash
./bin/mscopier n source_file destination_file
```

Copies `source_file` to `destination_file` using `n` reader threads and `n` writer threads.

**Constraints:**

* `n` must be between 2 and 10.

**Example:**

```bash
shuf -n 10000 tests/generate_text/wordlist.10000 > tests/tmp/source_file.txt
./bin/mscopier 10 tests/tmp/source_file.txt tests/tmp/destination_file.txt
```

---

### Extra Makefile Functions

#### Clean Up Compiled Files

```bash
make clean
```

#### Test Binaries

##### All Tests
```bash
make test
```

##### Test `mmcopier`
```bash
make test-mmcopier
```

##### Test `mscopier`

```bash
make test-mscopier
```

#### Benchmark Binaries

```bash
make benchmark
```

---

## Memory Testing

Run Valgrind using:

```bash
valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./mscopier 10 source_file.txt destination_file.txt
```

Run Leaks using:

```bash
leaks -atExit -- ./mscopier 10 source_file.txt destination_file.txt
```
