CC = clang
CCF = -g
OUT = o1

$(OUT): 1.c
	$(CC) $(CCF) -o $@ $^

.PHONY: clean
clean:
	rm -f $(OUT)*
