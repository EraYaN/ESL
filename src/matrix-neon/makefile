BASE_TOOLCHAIN = /data/bbToolChain/usr/local/share/codesourcery
CC = arm-none-linux-gnueabi-gcc
CFLAGS = -Wall -O3 -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon
INC = -I../matrix-include
LIBS = -lm
EXEC = matrix-mult-neon
CSRCS = main.c
OBJS = $(CSRCS:.c=.o)
REMOTE_DIR = ~/ESL/$(EXEC)

all: $(EXEC)

$(EXEC): $(OBJS)
	@$(BASE_TOOLCHAIN)/bin/$(CC) $(CFLAGS) -o $@ $? $(LIBS) $(LDFLAGS)

%.o: %.c
	@$(BASE_TOOLCHAIN)/bin/$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	@-rm -rf $(EXEC) $(OBJS) *~

send: $(EXEC)
	scp $(REMOTE_DIR)/$(EXEC) root@192.168.0.202:/home/root/esLAB/.

.PHONY: all clean send
