CC = gcc
CCFLAGS = -Wall -Werror
LDLIBS = -lpthread

OBJ_DIR = obj
BIN_DIR = bin


MMCOPIER_OBJS = $(OBJ_DIR)/mmcopier.o
MSCOPIER_OBJS = $(OBJ_DIR)/mscopier.o

MMCOPIER_BIN = $(BIN_DIR)/mmcopier
MSCOPIER_BIN = $(BIN_DIR)/mscopier

all: directories $(BIN_DIR)/mmcopier $(BIN_DIR)/mscopier

directories:
	@mkdir -p $(OBJ_DIR); \
	mkdir -p $(BIN_DIR)

$(MMCOPIER_BIN): directories $(MMCOPIER_OBJS)
	$(CC) $(CCFLAGS) -o $@ $(MMCOPIER_OBJS) $(LDLIBS)

$(MSCOPIER_BIN): directories $(MSCOPIER_OBJS)
	$(CC) $(CCFLAGS) -o $@ $(MSCOPIER_OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c | directories
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) tests/tmp

test: $(BIN_DIR)/mmcopier $(BIN_DIR)/mscopier
	@echo "========================================"; \
	echo "          Testing mmcopier"; \
	echo "========================================"; \
	echo ""; \
	echo "=== 1. Destination directory exists ==="; \
	rm -rf tests/tmp/destination_dir; \
	mkdir -p tests/tmp/destination_dir; \
	$(BIN_DIR)/mmcopier 10 tests/source_dir tests/tmp/destination_dir; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	for file in tests/source_dir/*; do diff $$file tests/tmp/destination_dir/$$(basename $$file) && echo "$$(basename $$file): OK" || echo "$$(basename $$file): FAIL"; done; \
	echo ""; \
	echo "=== 2. Destination directory does not exist ==="; \
	rm -rf tests/tmp/destination_dir; \
	$(BIN_DIR)/mmcopier 10 tests/source_dir tests/tmp/destination_dir; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	for file in tests/source_dir/*; do diff $$file tests/tmp/destination_dir/$$(basename $$file) && echo "$$(basename $$file): OK" || echo "$$(basename $$file): FAIL"; done; \
	echo ""; \
	echo "=== 3. Minimum thread count (n = 2) ==="; \
	rm -rf tests/tmp/destination_dir; \
	$(BIN_DIR)/mmcopier 2 tests/source_dir tests/tmp/destination_dir; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	for file in tests/source_dir/*; do if [ -f tests/tmp/destination_dir/$$(basename $$file) ]; then diff $$file tests/tmp/destination_dir/$$(basename $$file) && echo "$$(basename $$file): OK" || echo "$$(basename $$file): FAIL"; fi; done; \
	echo ""; \
	echo "=== 4. Maximum thread count (n = 10) ==="; \
	rm -rf tests/tmp/destination_dir; \
	$(BIN_DIR)/mmcopier 10 tests/source_dir tests/tmp/destination_dir; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	for file in tests/source_dir/*; do diff $$file tests/tmp/destination_dir/$$(basename $$file) && echo "$$(basename $$file): OK" || echo "$$(basename $$file): FAIL"; done; \
	echo ""; \
	echo "=== 5. Invalid thread count (n = 1) ==="; \
	$(BIN_DIR)/mmcopier 1 tests/source_dir tests/tmp/destination_dir 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== 6. Invalid thread count (n = 11) ==="; \
	$(BIN_DIR)/mmcopier 11 tests/source_dir tests/tmp/destination_dir 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== 7. Source directory does not exist ==="; \
	$(BIN_DIR)/mmcopier 10 tests/tmp/nonexistent tests/tmp/destination_dir 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== 8. Source and destination directories are the same ==="; \
	$(BIN_DIR)/mmcopier 10 tests/source_dir tests/source_dir 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "========================================"; \
	echo "          Testing mscopier"; \
	echo "========================================"; \
	echo ""; \
	echo "=== 1. Normal file ==="; \
	shuf -n 10000 tests/generate_text/wordlist.10000 > tests/tmp/source_file.txt; \
	$(BIN_DIR)/mscopier 10 tests/tmp/source_file.txt tests/tmp/destination_file.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/source_file.txt tests/tmp/destination_file.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 2. Empty file ==="; \
	touch tests/tmp/empty_file.txt; \
	$(BIN_DIR)/mscopier 10 tests/tmp/empty_file.txt tests/tmp/empty_destination.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/empty_file.txt tests/tmp/empty_destination.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 3. Small file ==="; \
	printf "Hello, world!\n" > tests/tmp/small_file.txt; \
	$(BIN_DIR)/mscopier 2 tests/tmp/small_file.txt tests/tmp/small_destination.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/small_file.txt tests/tmp/small_destination.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 4. Exactly BUFFER_SIZE ==="; \
	dd if=/dev/zero of=tests/tmp/exact_file.txt bs=4096 count=1 2>/dev/null; \
	$(BIN_DIR)/mscopier 10 tests/tmp/exact_file.txt tests/tmp/exact_destination.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/exact_file.txt tests/tmp/exact_destination.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 5. Larger than BUFFER_SIZE ==="; \
	dd if=/dev/zero of=tests/tmp/large_file.txt bs=4096 count=100 2>/dev/null; \
	$(BIN_DIR)/mscopier 10 tests/tmp/large_file.txt tests/tmp/large_destination.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/large_file.txt tests/tmp/large_destination.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 6. Minimum thread count (n = 2) ==="; \
	$(BIN_DIR)/mscopier 2 tests/tmp/source_file.txt tests/tmp/destination_file.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/source_file.txt tests/tmp/destination_file.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 7. Maximum thread count (n = 10) ==="; \
	$(BIN_DIR)/mscopier 10 tests/tmp/source_file.txt tests/tmp/destination_file.txt; \
	[ $$? -eq 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	diff tests/tmp/source_file.txt tests/tmp/destination_file.txt && echo "Files: OK" || echo "Files: FAIL"; \
	echo ""; \
	echo "=== 8. Invalid thread count (n = 1) ==="; \
	$(BIN_DIR)/mscopier 1 tests/tmp/source_file.txt tests/tmp/destination_file.txt 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== 9. Invalid thread count (n = 11) ==="; \
	$(BIN_DIR)/mscopier 11 tests/tmp/source_file.txt tests/tmp/destination_file.txt 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== 10. Source file does not exist ==="; \
	$(BIN_DIR)/mscopier 10 tests/tmp/nonexistent.txt tests/tmp/destination_file.txt 2>/dev/null; \
	[ $$? -ne 0 ] && echo "Program: OK" || echo "Program: FAIL"; \
	echo ""; \
	echo "=== Cleanup ==="; \
	
	rm -rf tests/tmp

.PHONY: directories clean