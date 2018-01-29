CC = gcc
CFLAGS = -ansi -Wall -g -O0 -Wwrite-strings -Wshadow -pedantic-errors -fstack-protector-all
PROGS = d8sh 

.PHONY: all clean

all: $(PROGS)


d8sh:	d8sh.c parser.tab.o lexer.o executor.o d8sh.o
		$(CC) -o d8sh parser.tab.o lexer.o executor.o d8sh.o -lreadline

parser.tab.o:	command.h parser.tab.c
		$(CC) $(CFLAGS) -c parser.tab.c

lexer.o:	lexer.c parser.tab.h parser.tab.c command.h
		$(CC) $(CFLAGS) -c lexer.c

executor.o:	command.h executor.h executor.c
		$(CC) $(CFLAGS) -c executor.c

d8sh.o:		executor.h lexer.h d8sh.c command.h
		$(CC) $(CFLAGS) -c d8sh.c
clean:
	@echo "Cleaning my system"
	@rm -f *.o $(PROGS) a.out
