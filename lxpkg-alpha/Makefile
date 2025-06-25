CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
LDFLAGS = -lspdlog -pthread -lcrypto -lcurl -lnlohmann_json
SRC = src/config.cpp src/database.cpp src/main.cpp src/package/package.cpp src/package/source.cpp src/package/build.cpp src/package/install.cpp src/package/dependency.cpp src/package/manifest.cpp src/package/search.cpp src/package/conflict.cpp src/package/sync.cpp src/util.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = lxpkg
TEST_SRC = tests/test_database.cpp tests/test_package.cpp
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TEST_TARGET = run_tests

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJ) $(TEST_OBJ)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ) $(filter-out src/main.o, $(OBJ))
	$(CXX) $(TEST_OBJ) $(filter-out src/main.o, $(OBJ)) -o $@ $(LDFLAGS) -lgtest -lgtest_main

install:
	install -m 755 $(TARGET) /usr/bin/
	install -d /etc/lxpkg
	install -m 644 configs/lxpkg.conf /etc/lxpkg/lxpkg.conf

.PHONY: all clean test install