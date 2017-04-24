BEAGLE_DIR=/home/root/esLAB/neonTest
CC=/opt/bbToolChain/usr/local/share/codesourcery/bin/arm-none-linux-gnueabi-gcc
#CC=gcc
CFLAGS=-O3 
LDFLAGS=-lm 
CSRCS=main.c
OBJS=$(CSRCS:.c=.o)
ASMS=$(CSRCS:.c=.s)
EXEC=exec

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LIBS) -o $@ $? $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

asm: $(ASMS)
asm: CFLAGS+=-S -fverbose-asm
%.s: %.c
	$(CC) $(CFLAGS) -c $< -o $@

soft: CFLAGS=-O3 
soft: clean all

vfp: CFLAGS=-O3 -march=armv7-a -mtune=cortex-a8 -mfpu=vfp -mfloat-abi=softfp
vfp: clean all

neon: CFLAGS=-O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -funsafe-math-optimizations
neon: clean all


send: 
	scp ./$(EXEC) root@192.168.0.202:$(BEAGLE_DIR)/.

run:
	./$(EXEC)

clean:
	@-rm -rf $(EXEC) $(OBJS) gmon.out *~ $(ASMS)

