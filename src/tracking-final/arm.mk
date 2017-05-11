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


CFLAGS= -Wall -O3 -Wfatal-errors 	\
	--sysroot=/data/rootfs			\
	-mlittle-endian					\
	-march=armv7-a					\
	-mtune=cortex-a8				\
	-msoft-float					\
	-Uarm							\
	-marm							\
	-Wno-trigraphs					\
	-fno-strict-aliasing			\
	-fno-common						\
	-fno-omit-frame-pointer			\
	-mapcs							\
	-mabi=aapcs-linux				\ 
	-Wdeclaration-after-statement	\
	-Wstrict-prototypes				\
	-mapcs							\  
	-mfpu=neon						\
	-ftree-vectorize				\
	-ffast-math						\
			
	  -std=$(CPPSTD)

all: clean $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

$(OUTDIR)/%.o : ../%.cpp
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $< -o $@

.PHONY: clean all
clean:
	rm -f $(OBJS) $(EXEC) tracking_result.avi *~
