CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lws2_32 -lwinhttp

SRCS = server.c data.c utils.c product.c sale.c purchase.c stats.c assistant.c
OBJS = $(SRCS:.c=.o)
TARGET = server

GENERATE_SRC = generate_data.c
GENERATE = generate_data

.PHONY: all clean generate run

all: generate $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

generate: $(GENERATE)
	cd .. && ./backend/$(GENERATE)

$(GENERATE): $(GENERATE_SRC)
	$(CC) $(CFLAGS) -o $@ $<

server.o: server.c data.h utils.h product.h sale.h purchase.h stats.h
data.o: data.c data.h
utils.o: utils.c utils.h
product.o: product.c data.h utils.h product.h
sale.o: sale.c data.h utils.h product.h sale.h
purchase.o: purchase.c data.h utils.h product.h purchase.h
stats.o: stats.c data.h utils.h product.h stats.h
assistant.o: assistant.c assistant.h data.h product.h sale.h utils.h

run: all
	cd .. && ./backend/$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)$(TARGET:.exe=.exe) $(GENERATE)$(GENERATE:.exe=.exe)
