CXX = g++
CXXFLAGS = -Wall -Werror -g
# Change the above to CC and CCFLAGS if using c
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
	$(CXX) $(CXXFLAGS) -o $@ $(MMCOPIER_OBJS) $(LDLIBS)

$(MSCOPIER_BIN): directories $(MSCOPIER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(MSCOPIER_OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: directories clean