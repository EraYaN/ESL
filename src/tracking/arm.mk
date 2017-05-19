BASE_TOOLCHAIN=/data/bbToolChain/usr/local/share/codesourcery
CC=$(BASE_TOOLCHAIN)/bin/arm-none-linux-gnueabi-g++
#BASE_TOOLCHAIN=/data/bbToolChain/usr/local/share/gcc-linaro-5.4.1-2017.01-x86_64_arm-linux-gnueabi
#CC=$(BASE_TOOLCHAIN)/bin/arm-linux-gnueabi-g++

EXEC=armMeanshiftExec
CPPSTD=c++0x

SYSROOT=/data/rootfs
#SYSROOT=/data/sysroot-glibc-linaro-2.21-2017.01-arm-linux-gnueabi

LDFLAGS=-lpthread -lm --sysroot=$(SYSROOT)
LIBS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video \
	-lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

DEFS=-DARMCC
INCLUDES=-I. -I$(BASE_TOOLCHAIN)/include

# Modules are directories
MODULES				:= tracking-shared tracking
SRC_DIR				:= $(addprefix ../,$(MODULES))
OUTDIR				:= ./out/ARM
BUILD_DIR			:= $(addprefix $(OUTDIR)/,$(MODULES))

SRC				    := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ					:= $(patsubst ../%.cpp,$(OUTDIR)/%.o,$(SRC))

MODULES_INCLUDES	:= $(addprefix -I../,$(MODULES))

INCLUDES			+= $(MODULES_INCLUDES) 

INCLUDES			:= $(sort $(INCLUDES))

CFLAGS= -Wall -O3 -Wfatal-errors 		\
	  --sysroot=$(SYSROOT)			\
	  -mlittle-endian               \
	  -march=armv5t                 \
	  -mtune=arm9tdmi               \
	  -Uarm                         \
	  -marm                         \
	  -Wno-trigraphs                \
	  -fno-strict-aliasing          \
	  -fno-common                   \
	  -fno-omit-frame-pointer       \
	  -mapcs                        \
	  -mabi=aapcs-linux				\
	  -std=$(CPPSTD) \
	  -pg

all: checkdirs $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -pg -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

$(OUTDIR)/%.o : ../%.cpp
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $< -o $@

checkdirs: $(OUTDIR) $(BUILD_DIR)

$(OUTDIR) $(sort $(BUILD_DIR)):
	mkdir -p $@

send: $(EXEC)
	scp -oKexAlgorithms=+diffie-hellman-group1-sha1 $(EXEC) root@192.168.0.202:/home/root/esLAB/.

.PHONY: clean all
clean:
	rm -rf $(OUTDIR) $(EXEC) tracking_result.avi *~

info:
	echo $(SRC)
	echo $(OBJ)