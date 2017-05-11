# set the path to pin if not set in .bachrc
PIN_DIR=$(PIN_ROOT)

# set the path to mcprof if not set in .bachrc
MCPROF_DIR=~/mcprof

CC=g++
EXEC=pcMeanshiftExec
INCLUDES = -I. -I$(MCPROF_DIR)
CPPSTD=c++11

# Modules are directories
MODULES				:= tracking-shared tracking-final
SRC_DIR				:= $(addprefix ../,$(MODULES))
OUTDIR				:= ./out
BUILD_DIR			:= $(addprefix $(OUTDIR)/,$(MODULES))

SRC				    := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ					:= $(patsubst ../%.cpp,$(OUTDIR)/%.o,$(SRC))

MODULES_INCLUDES	:= $(addprefix -I../,$(MODULES))

INCLUDES			+= $(MODULES_INCLUDES) 

INCLUDES					:= $(sort $(INCLUDES))

CFLAGS = -Wall -O3 -Wfatal-errors -std=$(CPPSTD)
LIBS = -lm -lpthread `pkg-config --libs opencv`
CMD=./$(EXEC) ../tracking-shared/car.avi

all: checkdirs $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS) $(LDFLAGS)

$(OUTDIR)/%.o : ../%.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

run: $(EXEC)
	$(CMD)

debug: CFLAGS= -g -Wall -Wfatal-errors -std=$(CPPSTD)
debug: checkdirs $(EXEC)
	gdb --args $(CMD)

valgrind: CFLAGS= -g -Wall -Wfatal-errors
valgrind: clean $(EXEC)
	valgrind --leak-check=full --tool=memcheck $(CMD)

gprof: CFLAGS=-O2 -g -pg -Wall -Wfatal-errors
gprof: LDFLAGS=-pg
gprof: clean $(EXEC)
	$(CMD)
	gprof -b $(EXEC) > gprof.txt
	$(MCPROF_DIR)/scripts/gprof2pdf.sh gprof.txt

MCPROF_OPT:=-RecordStack 0  -TrackObjects 1 -Engine 1 -TrackStartStop 1 -TrackZones 0 -Threshold 100
mcprof: CFLAGS= -O2 -g -fno-omit-frame-pointer -Wall -Wfatal-errors
mcprof: clean $(EXEC)
	time $(PIN_DIR)/pin -t $(MCPROF_DIR)/obj-intel64/mcprof.so $(MCPROF_OPT) -- $(CMD)
	$(MCPROF_DIR)/scripts/callgraph2pdf.sh
	$(MCPROF_DIR)/scripts/dot2pdf.sh
	$(MCPROF_DIR)/scripts/dotfilter.py communication_graph.dat 0.1 0.01

checkdirs: $(OUTDIR) $(BUILD_DIR)

$(OUTDIR) $(sort $(BUILD_DIR)):
	mkdir -p $@

clean:
	rm -rf out/ $(EXEC) *~

dist-clean: clean
	rm -rf tracking_result.avi gmon.out gprof.txt \
	callgraphAll.dot callgraphAll.pdf callgraph.dat callgraph.json callgraphTop.dot \
	callgraphTop.pdf communication.dot communication_graph.dat communication.pdf \
	execProfile.dat locations.dat matrix.dat memProfile.dat recursivefunctions.dat \
	symbols.dat taskgraph.dat traces.dat gprof.dot gprof.pdf communication_filtered_* \
	pin.log

info:
	echo $(SRC)
	echo $(OBJ)