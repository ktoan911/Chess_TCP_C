CXX = g++
CXXFLAGS = -std=c++17 -pthread -I./common -I./client -I./server -I./libraries
LDFLAGS = -lpthread

BUILD_DIR = build

SRC_SERVER = server/server_main.cpp
SRC_CLIENT = client/client_main.cpp

OBJ_SERVER = $(SRC_SERVER:.cpp=.o)
OBJ_CLIENT = $(SRC_CLIENT:.cpp=.o)

TARGET_SERVER = $(BUILD_DIR)/server_main
TARGET_CLIENT = $(BUILD_DIR)/client_main

all: $(BUILD_DIR) $(TARGET_SERVER) $(TARGET_CLIENT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET_SERVER): $(OBJ_SERVER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(OBJ_CLIENT) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

run_server:
	./$(TARGET_SERVER)

run_client:
	./$(TARGET_CLIENT)

.PHONY: all clean run_server run_client