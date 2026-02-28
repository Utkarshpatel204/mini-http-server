CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC = $(wildcard src/*.cpp)
OUT = server

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

test: all
	./scripts/test.sh

clean:
	rm -f $(OUT)
