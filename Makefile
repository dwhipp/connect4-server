all: main clean-deps

CXX = clang++
override CXXFLAGS += -O3 -Wall -pedantic -pthread -std=c++20

SRCS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.cpp' -print | sed -e 's/ /\\ /g')
OBJS = $(SRCS:.cpp=.o) connect4/src-cc/connect4.so
DEPS = $(SRCS:.cpp=.d)

connect4/src-cc/connect4.so: c4_subsystem

c4_subsystem:
	cd connect4/src-cc && $(MAKE) connect4.so

%.d: %.cpp
	@set -e; rm -f "$@"; \
	$(CXX) -MM $(CXXFLAGS) "$<" > "$@.$$$$"; \
	sed 's,\([^:]*\)\.o[ :]*,\1.o \1.d : ,g' < "$@.$$$$" > "$@"; \
	rm -f "$@.$$$$"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

include $(DEPS)

main: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o "$@"

main-debug: $(OBJS)
	$(CXX) $(CXXFLAGS) -O0 $(OBJS) -o "$@"

clean:
	rm -f $(OBJS) $(DEPS) main

clean-deps:
	rm -f $(DEPS)