BASE_TOOLCHAIN = /data/bbToolChain/usr/local/share/codesourcery
CC = arm-none-linux-gnueabi-gcc
CFLAGS= -O3 -Wall
LIBS= -lm
LDFLAGS=
EXEC = matrix-mult
CSRCS = main.c
OBJS = $(CSRCS:.c=.o)

INCLUDES := -I../matrix-include \
			-I$(BASE_TOOLCHAIN)/include \
			-I./

all: $(EXEC)

$(EXEC): $(OBJS)
	@echo Compiling for ARM...
	@$(BASE_TOOLCHAIN)/bin/$(CC) -o $@ $? $(LIBS) $(LDFLAGS)

%.o: %.c
	@$(BASE_TOOLCHAIN)/bin/$(CC) $(ALL_CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@-rm -rf $(EXEC) $(OBJS) *~

send: $(EXEC)
	scp $(EXEC) root@192.168.0.202:/home/root/esLAB/.

rebuild: clean all

.PHONY: all clean send rebuild
