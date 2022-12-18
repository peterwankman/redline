SRC=src
OBJ=obj
BIN=bin

CC=gcc

#CFLAGS=-O0 -ggdb -DCHATTY_PARSER
CFLAGS=-O2

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

$(BIN)/led: $(PIECES)	
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: $(SRC)/rev.h

$(SRC)/rev.h: scripts/mkrevh.sh
	scripts/mkrevh.sh > $@

.PHONY: clean

clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
	rm -f $(SRC)/rev.h
