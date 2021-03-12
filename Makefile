CXXFLAGS = -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion
CXXFLAGS += -std=c++17

CXXFLAGS += -fsanitize=address,leak
#CXXFLAGS += -fthread
#CXXFLAGS += -fmemory

# debug setting
CXXFLAGS += -O0 -g

# perf setting
# CXXFLAGS += -O3 -flto -DNDEBUG

all: project

project: project.cc

fmt:
	@clang-format -i -style=file project.cc

tidy:
	@clang-tidy project.cc

test: project
	@sbin/test.sh

clean:
	@rm -f project project.o
