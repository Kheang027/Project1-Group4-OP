CXX = g++
CXXFLAGS = -Wall -Werror -g
LDLIBS = -lpthread

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

MMCOPIER_OBJS = $(OBJ_DIR)/mmcopier.o
MSCOPIER_OBJS = $(OBJ_DIR)/mscopier.o

MMCOPIER_BIN = $(BIN_DIR)/mmcopier
MSCOPIER_BIN = $(BIN_DIR)/mscopier

all: directories $(BIN_DIR)/mmcopier $(BIN_DIR)/mscopier

$(MMCOPIER_BIN): directories $(MMCOPIER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(MMCOPIER_OBJS) $(LDLIBS)

$(MSCOPIER_BIN): directories $(MSCOPIER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(MSCOPIER_OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
directories: $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p $(BUILD_DIR); \
	mkdir -p $(OBJ_DIR); \
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: directories clean