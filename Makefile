CFLAGS := -g -Wall

SRC := grammar.c lexer.c $(wildcard *.c)

.PHONY: all
all: fmdsl

fmdsl: $(SRC:%.c=%.o)
	$(CC) $^ -o $@

makestring: makestring._c
	$(CC) -x c $< -o $@

-include deps

deps: $(SRC)
	$(CC) -MM -MG $^ > $@

%.h %.c: %.lm
	lemon -s $<

%.c: %.rl
	ragel $<

%_string.h: %.* makestring
	./makestring $< $@ $*

.PHONY: clean
clean:
	-rm *.o
	-rm *_string.h
	-rm makestring
	-rm fmdsl

.PHONY: veryclean
veryclean: clean
	-rm grammar.h
	-rm grammar.c
	-rm grammar.out
	-rm lexer.c
	-rm deps
	$(MAKE) -C tests/bike/ clean