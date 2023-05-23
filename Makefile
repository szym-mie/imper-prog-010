CC = clang    # replace with your compilator of choice
CCF = -g      # extra symbols for debugging with gdb and valgrind
OUT = o1      # output file

# use 'make' to build project
$(OUT): 1.c
	$(CC) $(CCF) -o $@ $^

# use 'make clean' to remove all output files
.PHONY: clean
clean:
	rm -f $(OUT)*
