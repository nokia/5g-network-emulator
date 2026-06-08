CXX := g++

BIN_DIR := bin
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TEST_OBJ_DIR := $(BUILD_DIR)/tests
TOOLS_OBJ_DIR := $(BUILD_DIR)/tools

TARGET := $(BIN_DIR)/fikore
TOOLS_TARGETS := $(BIN_DIR)/udp_tos_probe

CPPFLAGS := -Iinclude
CXXFLAGS := -O3 -g -std=c++11 -pthread
DEPFLAGS := -MMD -MP
LDFLAGS :=
LDLIBS := -pthread -lmnl -lnetfilter_queue

SRC_DIR := src
TEST_DIR := tests

APP_SOURCES := main.cpp $(shell find $(SRC_DIR) -name '*.cpp' | sort)
APP_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(APP_SOURCES))
APP_DEPS := $(APP_OBJECTS:.o=.d)

LIB_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' | sort)
LIB_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(LIB_SOURCES))

TEST_SOURCES := $(shell find $(TEST_DIR) -name '*_test.cpp' 2>/dev/null | sort)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.cpp,$(TEST_OBJ_DIR)/%.o,$(TEST_SOURCES))
TEST_BINS := $(patsubst $(TEST_DIR)/%_test.cpp,$(TEST_OBJ_DIR)/%_test,$(TEST_SOURCES))
TEST_DEPS := $(TEST_OBJECTS:.o=.d)
TOOLS_SOURCES := $(shell find tools -name '*.cpp' 2>/dev/null | sort)
TOOLS_OBJECTS := $(patsubst tools/%.cpp,$(TOOLS_OBJ_DIR)/%.o,$(TOOLS_SOURCES))
TOOLS_DEPS := $(TOOLS_OBJECTS:.o=.d)

.PHONY: all tools run test smoke clean print-objects
.SECONDARY: $(TEST_OBJECTS)

all: $(TARGET) $(TOOLS_TARGETS)

tools: $(TOOLS_TARGETS)

run: all
	./$(TARGET)

$(TARGET): $(APP_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/%_test: $(TEST_OBJ_DIR)/%_test.o $(LIB_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TOOLS_OBJ_DIR)/%.o: tools/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR)/udp_tos_probe: $(TOOLS_OBJ_DIR)/udp_tos_probe.o
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

test: $(TEST_BINS)
	@set -e; \
	for test_bin in $(TEST_BINS); do \
		echo "[TEST] $$test_bin"; \
		$$test_bin; \
	done

smoke: all
	./$(TARGET) tests/smoke_sim.ini

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET) $(TOOLS_TARGETS)

print-objects:
	@printf '%s\n' $(APP_OBJECTS)

-include $(APP_DEPS) $(TEST_DEPS) $(TOOLS_DEPS)
