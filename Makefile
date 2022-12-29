SRC=src
OBJ=obj
BIN=bin

NATIVE_CC=gcc
AFL_CC=afl-gcc

CC=$(NATIVE_CC)

CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-O0 -ggdb
CFLAGS_AFL=-DAFL_BUILD $(CFLAGS_DEBUG)

CFLAGS=$(CFLAGS_DEBUG)

PIECES=\
$(SRC)/rev.h \
$(OBJ)/dynarr.o \
$(OBJ)/ermac.o \
$(OBJ)/getopt.o \
$(OBJ)/lexer.o \
$(OBJ)/main.o \
$(OBJ)/parser.o \
$(OBJ)/repl.o \
$(OBJ)/util.o

.PHONY: all, debug, verbose, clean, release, afl, $(SRC)/rev.h

$(BIN)/fred: $(PIECES)	
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC)/rev.h: scripts/mkrevh.sh
	scripts/mkrevh.sh > $@

all:
	make debug
	make release
	make afl

clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
	rm -f $(SRC)/rev.h

debug:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_DEBUG)" $(BIN)/fred
	cp $(BIN)/fred $(BIN)/fred-debug

verbose:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_DEBUG) -DCHATTY_PARSER" $(BIN)/fred
	cp $(BIN)/fred $(BIN)/fred-verbose

release:
	rm -f $(OBJ)/*
	make CFLAGS="$(CFLAGS_RELEASE)" $(BIN)/fred
	cp $(BIN)/fred $(BIN)/fred-release
	strip $(BIN)/fred-release

afl:
	rm -f $(OBJ)/*
	make CC=$(AFL_CC) CFLAGS="$(CFLAGS_AFL)" $(BIN)/fred
	cp $(BIN)/fred $(BIN)/fred-afl
