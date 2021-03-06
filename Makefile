CFLAGS=-Wall -std=c99 -g
SRCS=$(wildcard *.c)
INCLUDES=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

mdcc: $(OBJS)

test: mdcc
	./test.sh

$(OBJS): mdcc.h

format:
	clang-format -i $(SRCS) $(INCLUDES)

clean:
	rm -f mdcc *.o a.out tmp*

.PHONY: clean test format
