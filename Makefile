CC = gcc
CFLAGS = -O0 -Wall -ggdb3 -pedantic
EXE = msh
HDRS = msh.h msh_execute.h hash_table.h lex.yy.h msh.tab.h
LIBS = -lreadline -lfl
SRCS = msh.c msh_execute.c hash_table.c msh.tab.c lex.yy.c
OBJS = $(SRCS:.c=.o)

all: $(EXE)

msh.tab.c msh.tab.h: msh.y
	bison --defines=msh.tab.h --output=msh.tab.c msh.y

lex.yy.c lex.yy.h: msh.l
	flex --header-file=lex.yy.h msh.l

$(EXE): $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

# housekeeping
clean:
	rm -f core $(EXE) *.o *.tab.* *.yy.*
