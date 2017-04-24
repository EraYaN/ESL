BEAGLE_DIR=/home/root/esLAB/neonTest
CC=/opt/bbToolChain/usr/local/share/codesourcery/bin/arm-none-linux-gnueabi-gcc
#CC=gcc
CFLAGS=-O3 -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon
LDFLAGS=-lm 
OBJS=main.o
EXEC=exec

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $?

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

send: 
	scp ./$(EXEC) root@192.168.0.202:$(BEAGLE_DIR)/.

run:
	./$(EXEC)

clean:
	@-rm -rf $(EXEC) $(OBJS) gmon.out *~ 

