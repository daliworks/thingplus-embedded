#
#
CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g
LDFLAGS = -shared  # linking flags
RM = rm -f  # rm command
TARGET_LIB = libtubehttp.so # target lib

SRCS = tube_http.c tube_curl.c tube_url.c sensor_driver.c# source files
OBJS = $(SRCS:.c=.o)

.PHONY: all

all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)

#all:
#	gcc sensor_driver.c tube_http.c tube_curl.c main.c tube_url.c -lcurl -ljson -g
