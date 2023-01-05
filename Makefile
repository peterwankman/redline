SRC=src
OBJ=obj
BIN=bin

NATIVE_CC=gcc
AFL_CC=afl-gcc

CC=$(NATIVE_CC)

CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-O0 -ggdb -D_DEBUG -rdynamic
CFLAGS_VERBOSE=$(CFLAGS_DEBUG) -DDEBUG_VERBOSE
CFLAGS_AFL=-DAFL_BUILD $(CFLAGS_DEBUG)

CFLAGS=$(CFLAGS_DEBUG)

PIECES=\
$(SRC)/rev.h \
$(OBJ)/dynarr.o \
$(OBJ)/ermac.o \
$(OBJ)/getopt.o \
$(OBJ)/lexer.o \
$(OBJ)/main.o \
$(OBJ)/mem.o \
$(OBJ)/mem_bst.o \
$(OBJ)/parser.o \
$(OBJ)/repl.o \
$(OBJ)/util.o

.PHONY: all, afl, debug, release, verbose, clean, $(SRC)/rev.h

release:
	make $(BIN)/edison-release
	mv $(BIN)/edison-release $(BIN)/edison

debug:
	make $(BIN)/edison-debug
	mv $(BIN)/edison-debug $(BIN)/edison

verbose:
	make $(BIN)/edison-verbose
	mv $(BIN)/edison-verbose $(BIN)/edison

afl:
	make $(BIN)/edison-afl
	cp $(BIN)/edison-afl $(BIN)/edison

all:
	make $(BIN)/edison-afl
	make $(BIN)/edison-debug
	make $(BIN)/edison-release
	make $(BIN)/edison-verbose

$(BIN)/edison-afl:
	rm -f $(OBJ)/*
	make CC=$(AFL_CC) CFLAGS="$(CFLAGS_AFL)" $(BIN)/edison
	mv $(BIN)/edison $(BIN)/edison-afl

$(BIN)/edison-debug:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_DEBUG)" $(BIN)/edison
	mv $(BIN)/edison $(BIN)/edison-debug

$(BIN)/edison-release:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_RELEASE)" $(BIN)/edison
	mv $(BIN)/edison $(BIN)/edison-release
	strip $(BIN)/edison-release

$(BIN)/edison-verbose:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_VERBOSE)" $(BIN)/edison
	mv $(BIN)/edison $(BIN)/edison-verbose

$(BIN)/edison: $(PIECES)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC)/rev.h: scripts/mkrevh.sh
	scripts/mkrevh.sh > $@

clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
	rm -f $(SRC)/rev.h
