CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I include
SRC      = main.cpp
BIN      = pricer

.PHONY: all clean run benchmark

# Default: compile with both std::thread and OpenMP
all:
	$(CXX) $(CXXFLAGS) -fopenmp $(SRC) -o $(BIN) -lpthread
	@echo "Built with OpenMP + std::thread support"

# Without OpenMP (fallback)
no-omp:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN) -lpthread
	@echo "Built without OpenMP (std::thread only)"

run: all
	./$(BIN)

clean:
	rm -f $(BIN)
