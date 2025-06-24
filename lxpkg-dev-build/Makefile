CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
LFLAGS = -lssl -lcrypto
SRC = src/config.cpp src/database.cpp src/package.cpp src/util.cpp src/main.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = lxpkg

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

install:
	install -Dm755 $(EXEC) /usr/bin/$(EXEC)
