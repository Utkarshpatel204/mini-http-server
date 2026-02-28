CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC = src/main.cpp
OUT = server

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)