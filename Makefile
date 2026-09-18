CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -I.

LDFLAGS = -lsqlite3 -lpthread


TARGET = tcp_sqlite_server


# ==============================
# Source Files
# ==============================

SRCS = \
	app/main.c \
	core/gateway.c \
	transport/tcp/tcp_server.c \
	protocol/tcp/tcp_parser.c \
	service/sensor_service.c \
	service/data_service.c \
	storage/sqlite/sqlite_db.c \
	utils/log/log.c


OBJS = $(SRCS:.c=.o)


# ==============================
# Build
# ==============================

.PHONY: all

all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)


# ==============================
# Compile
# ==============================

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# ==============================
# Clean
# ==============================

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)