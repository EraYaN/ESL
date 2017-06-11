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

.PHONY: clean all rebuild
clean:
	rm -rf $(OUTDIR) $(EXEC) *~
	rm -rf /tmp/tracking_result.avi /tmp/tracking_result.coords /tmp/dynrange.csv

rebuild: clean all
	
info:
	echo $(SRC)
	echo $(OBJ)