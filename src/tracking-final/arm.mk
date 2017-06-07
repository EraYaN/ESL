BASE_TOOLCHAIN=/data/bbToolChain/usr/local/share/codesourcery
CC=$(BASE_TOOLCHAIN)/bin/arm-none-linux-gnueabi-g++
#BASE_TOOLCHAIN=/data/bbToolChain/usr/local/share/gcc-linaro-5.4.1-2017.01-x86_64_arm-linux-gnueabi
#CC=$(BASE_TOOLCHAIN)/bin/arm-linux-gnueabi-g++

DSPLINK := /data/bbToolChain/usr/local/share/bbframework/platform/beagle-linux/tools/dsplink_linux_1_65_00_03

EXEC=armMeanshiftExec
CPPSTD=c++98

SYSROOT=/data/rootfs
#SYSROOT=/data/sysroot-glibc-linaro-2.21-2017.01-arm-linux-gnueabi

LDFLAGS=-lpthread -lm --sysroot=$(SYSROOT)
LIBS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video \
		-lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann \
		$(DSPLINK)/gpp/BUILD/EXPORT/RELEASE/dsplink.lib

#   ----------------------------------------------------------------------------
#   Compiler symbol definitions
#   ----------------------------------------------------------------------------
DEFS :=        -DARMCC               \
               -DFIXEDPOINT          \
               -DDEBUGPRINT3          \
               -DOS_LINUX            \
               -DMAX_DSPS=1          \
               -DMAX_PROCESSORS=2    \
               -DID_GPP=1            \
               -DOMAPL1XX            \
               -DPROC_COMPONENT      \
               -DPOOL_COMPONENT      \
               -DNOTIFY_COMPONENT    \
               -DMPCS_COMPONENT      \
               -DRINGIO_COMPONENT    \
               -DMPLIST_COMPONENT    \
               -DMSGQ_COMPONENT      \
               -DMSGQ_ZCPY_LINK      \
               -DCHNL_COMPONENT      \
               -DCHNL_ZCPY_LINK      \
               -DZCPY_LINK           \
               -DKFILE_DEFAULT       \
               -DDA8XXGEM            \
               -DDA8XXGEM_PHYINTERFACE=SHMEM_INTERFACE

#   ----------------------------------------------------------------------------
#   Compiler include directories
#   ----------------------------------------------------------------------------
INCLUDES := -I$(DSPLINK)/gpp/inc                  \
            -I$(DSPLINK)/gpp/inc/usr              \
            -I$(DSPLINK)/gpp/inc/sys/Linux        \
            -I$(DSPLINK)/gpp/inc/sys/Linux/2.6.18 \
            -I$(BASE_TOOLCHAIN)/include           \
            -I../tracking-shared                  \
            -I./

# Modules are directories
MODULES				:= tracking-shared tracking-final
SRC_DIR				:= $(addprefix ../,$(MODULES))
OUTDIR				:= ./out/ARM
BUILD_DIR			:= $(addprefix $(OUTDIR)/,$(MODULES))

SRC_C				:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
SRC_CPP				:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
SRC					:= $(SRC_C) $(SRC_CPP)
$(info SRC is $(SRC))

OBJ_C				:= $(patsubst ../%.c,$(OUTDIR)/%.o,$(SRC_C))
OBJ_CPP				:= $(patsubst ../%.cpp,$(OUTDIR)/%.o,$(SRC_CPP))
OBJ					:= $(OBJ_C) $(OBJ_CPP)
$(info OBJ is $(OBJ))

MODULES_INCLUDES	:= $(addprefix -I../,$(MODULES))

INCLUDES			+= $(MODULES_INCLUDES)

INCLUDES			:= $(sort $(INCLUDES))

CFLAGS= -Wall -O3 -Wfatal-errors 	\
	-mlittle-endian					\
	-march=armv7-a					\
	-mtune=cortex-a8				\
	-mcpu=cortex-a8					\
	-Uarm							\
	-marm							\
	-Wno-trigraphs					\
	-fno-strict-aliasing			\
	-fno-common						\
	-fno-omit-frame-pointer			\
	-mapcs							\
	-mabi=aapcs-linux				\
	-mfloat-abi=softfp				\
    -ftree-vectorize                \
	-ffast-math						\
	-std=$(CPPSTD)					\
	--sysroot=$(SYSROOT)

all: checkdirs $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

$(OUTDIR)/%.o : ../%.cpp
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OUTDIR)/%.o : ../%.c
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $< -o $@

checkdirs: $(OUTDIR) $(BUILD_DIR)

$(OUTDIR) $(sort $(BUILD_DIR)):
	mkdir -p $@

send: $(EXEC)
	scp -oKexAlgorithms=+diffie-hellman-group1-sha1 $(EXEC) root@192.168.0.202:/home/root/esLAB/.

.PHONY: clean all
clean:
	rm -rf $(OUTDIR) $(EXEC) *~
	rm -rf /tmp/tracking_result.avi /tmp/tracking_result.coords /tmp/dynrange.csv
	
info:
	echo $(SRC)
	echo $(OBJ)