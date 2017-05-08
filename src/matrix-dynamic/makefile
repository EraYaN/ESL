BASE_TOOLCHAIN = /data/bbToolChain/usr/local/share/codesourcery
CC = arm-none-linux-gnueabi-gcc
CFLAGS = -DPROFILE
LDFLAGS := -lpthread
LIBS = -lm
EXEC = matrix-mult-dynamic
CSRCS = main.c
OBJS = $(CSRCS:.c=.o)

#   ----------------------------------------------------------------------------
#   Compiler include directories 
#   ----------------------------------------------------------------------------
INCLUDES := -I../matrix-include \
			-I$(BASE_TOOLCHAIN)/include \
			-I./

#   ----------------------------------------------------------------------------
#   All compiler options to be passed to the command line
#   ----------------------------------------------------------------------------
ALL_CFLAGS := $(INCLUDES)				  \
			-mlittle-endian               \
            -march=armv7-a                \
            -mtune=cortex-a8              \
            -mfloat-abi=softfp            \
            -Uarm                         \
            -Wdeclaration-after-statement \
            -marm -Wall					  \
            -Wstrict-prototypes           \
            -Wno-trigraphs                \
            -fno-strict-aliasing          \
            -fno-common                   \
            -fno-omit-frame-pointer       \
            -mapcs                        \
            -c                            \
            -mabi=aapcs-linux             \
			-O3							  \
			$(CFLAGS)

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