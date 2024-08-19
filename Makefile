CFLAGS = -Wall -g -Werror -Wno-error=unused-variable
CC = g++
TARGETS = server subscriber
COMMON_OBJS = common.o

all: $(TARGETS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

server: server.cpp $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

subscriber: subscriber.cpp $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -rf $(TARGETS) *.o *.dSYM
