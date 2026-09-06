CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -g
LDFLAGS ?= 

SRCS = main.c arena.c lexer.c parser.c sema.c ir.c opt.c codegen.c
OBJS = $(SRCS:.c=.o)

TARGET = ccompiler

.PHONY: all debug release asan clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug:
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -g -O0 -DDEBUG" all

release:
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -O3 -DNDEBUG" all

asan:
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -g -fsanitize=address,undefined" LDFLAGS="-fsanitize=address,undefined" all

test: all
	@echo "Running test suite..."
	@./tests/run_tests.sh

clean:
	rm -f *.o $(TARGET) tests/*_bin tests/*.asm tests/*.o tests/*.out
