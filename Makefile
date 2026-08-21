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
	rm -rf $(OBJ_DIR) $(BIN_DIR) tests/destination_dir tests/generate_text/source_file.txt tests/generate_text/destination_file.txt



.PHONY: directories clean