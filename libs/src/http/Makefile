objs += tube_http.o

tube_http.o-objs += sensor_driver.o
tube_http.o-objs += tube_http.o
tube_http.o-objs += tube_curl.o
tube_http.o-objs += tube_url.o

PWD=$(shell pwd)

LIBINC_DIR=$(PWD)/../../include

CC ?= gcc
CFLAGS += -I$(LIBINC_DIR) -ggdb -Wall
#LDFLAGS += -lcurl -ljson 

SRCS = tube_http.c tube_curl.c tube_url.c sensor_driver.c# source files
LIBOFILES = $(SRCS:%.c=%.o)       

.PHONY:all clean

# all is the default rule
all : libtubehttp.a

# remove the old tapestry library and remake the new one
libtubehttp.a:	$(LIBOFILES)
	rm -f $@
	ar rc $@ $(LIBOFILES)
	ranlib $@

clean:
	rm -f libtubehttp.a

FORCE:
