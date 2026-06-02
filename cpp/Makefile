CXX = clang++
CXXFLAGS = -std=c++20 -O3 -I/opt/homebrew/Cellar/taskflow/4.0.0/include -I/opt/homebrew/opt/libomp/include -Xclang -fopenmp
LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp

TARGETS = \
	histogram/openmp histogram/taskflow \
	mergesort/openmp mergesort/taskflow \
	stencil/openmp stencil/taskflow \
	bfs/openmp bfs/taskflow

all: $(TARGETS)

histogram/openmp: histogram/openmp.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ histogram/openmp.cpp $(LDFLAGS)

histogram/taskflow: histogram/taskflow.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ histogram/taskflow.cpp $(LDFLAGS)

mergesort/openmp: mergesort/openmp.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ mergesort/openmp.cpp $(LDFLAGS)

mergesort/taskflow: mergesort/taskflow.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ mergesort/taskflow.cpp $(LDFLAGS)

stencil/openmp: stencil/openmp.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ stencil/openmp.cpp $(LDFLAGS)

stencil/taskflow: stencil/taskflow.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ stencil/taskflow.cpp $(LDFLAGS)

bfs/openmp: bfs/openmp.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ bfs/openmp.cpp $(LDFLAGS)

bfs/taskflow: bfs/taskflow.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o $@ bfs/taskflow.cpp $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
