BASE_TOOLCHAIN=/data/bbToolChain/usr/local/share/codesourcery
CC=$(BASE_TOOLCHAIN)/bin/arm-none-linux-gnueabi-g++

EXEC=armMeanshiftExec
CPPSTD=c++11


LDFLAGS=-lpthread -lm --sysroot=/data/rootfs
LIBS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video \
	-lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

DEFS=-DARMCC
INCLUDES=-I. -I$(BASE_TOOLCHAIN)/include

# Modules are directories
MODULES				:= tracking-shared tracking-final
SRC_DIR				:= $(addprefix ../,$(MODULES))
OUTDIR				:= ./out
BUILD_DIR			:= $(addprefix $(OUTDIR)/,$(MODULES))

SRC				    := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ					:= $(patsubst ../%.cpp,$(OUTDIR)/%.o,$(SRC))

MODULES_INCLUDES	:= $(addprefix -I,$(MODULES))

INCLUDES			+= $(MODULES_INCLUDES_CPU) 

INCLUDES			:= $(sort $(INCLUDES))


CFLAGS=$(DEFS) $(INCLUDES)          \
	  -Wall -O3 -Wfatal-errors 		\
	  --sysroot=/opt/rootfs			\
	  -mlittle-endian               \
	  -march=armv5t                 \
	  -mtune=arm9tdmi               \
	  -msoft-float                  \
	  -Uarm                         \
	  -marm                         \
	  -Wno-trigraphs                \
	  -fno-strict-aliasing          \
	  -fno-common                   \
	  -fno-omit-frame-pointer       \
	  -mapcs                        \
	  -mabi=aapcs-linux				\
	  -std=$(CPPSTD)

all: clean $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

$(OUTDIR)/%.o : ../%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all
clean:
	rm -rf out $(OBJS) $(EXEC) tracking_result.avi *~
