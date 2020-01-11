# makefile for deque

# Dependencies.
CATCH2_DIR ?=

# Options.
EXTRA_CFLAGS += -Wall -Wextra -Wshadow -Wcast-align -Werror
EXTRA_CFLAGS += -Wno-clobbered
EXTRA_CFLAGS += -Wno-missing-field-initializers
EXTRA_CFLAGS += -Og -g -fPIC
EXTRA_LDLIBS += -ldl -rdynamic
EXTRA_CFLAGS += -fprofile-arcs -ftest-coverage
EXTRA_LDLIBS += -lgcov

EXTRA_CFLAGS += -finstrument-functions
EXTRA_CFLAGS += -fno-omit-frame-pointer

EXTRA_CXXFLAGS += $(if $(CATCH2_DIR),-I$(CATCH2_DIR)/single_include)

CPPFLAGS := $(EXTRA_CPPFLAGS)
CFLAGS := -std=c11 -MMD -MP -I. -I../../include $(EXTRA_CFLAGS)
CXXFLAGS := -std=c++11 -MMD -MP -I. -I../../include $(EXTRA_CXXFLAGS)
LDFLAGS := $(EXTRA_LDFLAGS)
CXXLDLIBS := -latomic -lpthread $(EXTRA_LDLIBS)

CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld

TEST := deque_test
OBJS := mempool.o deque.o deque_test.o utils.o test_runner.o
DEPS := $(OBJS:.o=.d)
GCDAS := $(OBJS:.o=.gcda)
GCNOS := $(OBJS:.o=.gcno)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

.PHONY: all $(TEST) clean test

all: $(TEST)

$(TEST): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(CXXLDLIBS)

clean:
	rm -rf $(TEST) $(OBJS) $(DEPS) $(GCDAS) $(GCNOS)

test: $(TEST)
	./$(TEST) -r compact -s --durations yes $(TAGS)

-include $(DEPS)
