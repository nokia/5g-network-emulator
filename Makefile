CXX := g++
C := gcc
CXX_FLAGS := -O3 -std=c++11 -pthread -lmnl -lnetfilter_queue
#CXX_FLAGS := -std=c++11 -pthread -lmnl -lnetfilter_queue
C_FLAGS :=	-pthread -lmnl -lnetfilter_queue
SRCFLAGS = $(wildcard $(SRC)/*.cpp)  $(wildcard $(SRC_MAC_LAYER)/*.cpp)  $(wildcard $(SRC_MOBILITY_MODEL)/*.cpp)  $(wildcard $(SRC_PDCP_LAYER)/*.cpp)  $(wildcard $(SRC_PHY_LAYER)/*.cpp)  $(wildcard $(SRC_PHY_SHARED)/*.cpp)  $(wildcard $(SRC_MAP_HANDLER)/*.cpp) $(wildcard $(SRC_UE)/*.cpp)
SRCNETFILTER = $(wildcard $(SRC_NETFILTER)/*.c)
HEADERS = $(wildcard $(INCLUDE)/*/*.h) 
LDFLAGS = -Llib -static
BIN     			:=	bin
SRC     			:=	src
LIB     			:=	lib
SRC_MAC_LAYER     	:=	src/mac_layer
SRC_NETFILTER    	:=	src/netfilter
SRC_MOBILITY_MODEL 	:=	src/mobility_models
SRC_PDCP_LAYER     	:=	src/pdcp_layer
SRC_PHY_LAYER     	:=	src/phy_layer
SRC_PHY_SHARED      :=  src/phy_shared
SRC_MAP_HANDLER     :=  src/map
SRC_UE     			:=	src/ue
INCLUDE 			:=	include

NETFILTER_LIB 	:= libnetfilter.a
LIBRARIES   := $(LIB)/$(NETFILTER_LIB)
EXECUTABLE  :=	main

OBJFILES = $(patsubst %.cpp, $(BIN)/%.o, $(notdir $(SRCFLAGS)))

all:	$(BIN)/$(EXECUTABLE)

run:	clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(BIN)/netfilter.o $(BIN)/main.o $(OBJFILES)
	$(CXX) $(CXX_FLAGS) -g $(OBJFILES) -I$(INCLUDE) $(BIN)/main.o $(BIN)/netfilter.o $(C_FLAGS) -o $@ 

$(BIN)/main.o: main.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -o $(BIN)/main.o -c main.cpp

$(BIN)/netfilter.o: $(SRCNETFILTER) $(HEADERS)
	mkdir -p $(BIN)  
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -o $(BIN)/netfilter.o -c $(SRC_NETFILTER)/netfilter_interface.cpp

$(BIN)/%.o: $(SRC_MAC_LAYER)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_MOBILITY_MODEL)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_PDCP_LAYER)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_PHY_LAYER)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_PHY_SHARED)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_MAP_HANDLER)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC_UE)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@

$(BIN)/%.o: $(SRC)/%.cpp $(HEADERS)
	$(CXX) $(CXX_FLAGS) -g -c $< -I$(INCLUDE) -o $@



clean:
	-rm $(BIN)/*

# Print OBJFILES
print-objfiles:
	@echo $(OBJFILES)

.PHONY: all clean print-objfiles