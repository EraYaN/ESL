PIN_DIR=$(PIN_ROOT)
MCPROF_DIR=~/mcprof

CC = gcc
EXEC=test
CMD=./$(EXEC)
CFLAGS=-O2
INCS=-I.
LIBS=-lm
LFLAGS=
LDFLAGS=

SRCS = vectOps.c
OBJS=$(SRCS:.c=.o)

all: clean $(EXEC)
$(EXEC):  $(OBJS)
	$(CC) $(LFLAGS) -o $(EXEC) $^ $(LIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

run: $(EXEC)
	$(CMD)


MCPROF_OPT:=-RecordStack 0  -TrackObjects 1 -Engine 1
compile4mcprof: CFLAGS= -O2 -g -fomit-frame-pointer
compile4mcprof: clean $(EXEC)

mcprof: compile4mcprof
	$(PIN_DIR)/pin -t $(MCPROF_DIR)/obj-intel64/mcprof.so $(MCPROF_OPT) -- $(CMD)
	$(MCPROF_DIR)/scripts/dot2pdf.sh communication.dot
	$(MCPROF_DIR)/scripts/plotScript.sh
	$(MCPROF_DIR)/scripts/callgraph2pdf.sh

compile4gprof: CFLAGS= -O2 -g -pg -fomit-frame-pointer
compile4gprof: LDFLAGS= -pg
compile4gprof: clean $(EXEC)
gprof: compile4gprof
	$(CMD)
	gprof -b $(EXEC) > gprof.txt

clean:
	rm -f *.o $(EXEC) $(OBJS) *~ *.log gmon.out

.PHONY: clean all run mcprof.compile mcprof.execute
