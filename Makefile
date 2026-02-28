CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC = $(wildcard src/*.cpp)
OUT = server
UNIT_TEST_BIN = unit_tests

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

unit-test:
	$(CXX) $(CXXFLAGS) -Isrc tests/unit_tests.cpp src/http.cpp src/files.cpp -o $(UNIT_TEST_BIN)
	./$(UNIT_TEST_BIN)

test: all unit-test
	./scripts/test.sh

clean:
	rm -f $(OUT) $(UNIT_TEST_BIN)
