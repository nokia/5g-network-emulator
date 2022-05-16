CXX       :=	g++
C      :=	gcc
CXX_FLAGS := -O3 -std=c++11 -pthread -lmnl -lnetfilter_queue
C_FLAGS :=	-pthread -lmnl -lnetfilter_queue
SRCFLAGS = $(wildcard $(SRC)/*.cpp)  $(wildcard $(SRC_MAC_LAYER)/*.cpp)  $(wildcard $(SRC_MOBILITY_MODEL)/*.cpp)  $(wildcard $(SRC_PDCP_LAYER)/*.cpp)  $(wildcard $(SRC_PHY_LAYER)/*.cpp)  $(wildcard $(SRC_UE)/*.cpp)
SRCNETFILTER = $(wildcard $(SRC_NETFILTER)/*.c)
LDFLAGS = -Llib -static
BIN     			:=	bin
SRC     			:=	src
LIB     			:=	lib
SRC_MAC_LAYER     	:=	src/mac_layer
SRC_NETFILTER    	:=	src/netfilter
SRC_MOBILITY_MODEL 	:=	src/mobility_models
SRC_PDCP_LAYER     	:=	src/pdcp_layer
SRC_PHY_LAYER     	:=	src/phy_layer
SRC_UE     			:=	src/ue
INCLUDE 			:=	include

NETFILTER_LIB 	:= libnetfilter.a
LIBRARIES   := $(LIB)/$(NETFILTER_LIB)
EXECUTABLE  :=	main


all:	$(BIN)/$(EXECUTABLE)

run:	clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): netfilter.o main.o
	$(CXX) $(CXX_FLAGS) -g $(SRCFLAGS) -I$(INCLUDE) $(BIN)/main.o $(BIN)/netfilter.o $(C_FLAGS) -o $@ 

main.o: $(SRCFLAGS)
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -o $(BIN)/main.o -c main.cpp

netfilter.o: $(SRCNETFILTER)
	mkdir -p $(BIN)  
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -o $(BIN)/netfilter.o -c $(SRC_NETFILTER)/netfilter_interface.cpp

clean:
	-rm $(BIN)/*
	-rm *.o
	-rm lib/*
