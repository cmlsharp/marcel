CC = gcc
CFLAGS = -O0 -Wall -ggdb3 -pedantic
EXE = msh
LIBS = -lreadline -lfl
SRCS = msh.tab.c lex.yy.c msh.c msh_execute.c hash_table.c 
OBJS = $(SRCS:.c=.o)

%.c: %.y
%.c: %.l

all: $(EXE)

msh.tab.c msh.tab.h: msh.y
	bison --defines=msh.tab.h --output=msh.tab.c msh.y

lex.yy.c lex.yy.h: msh.l
	flex --header-file=lex.yy.h msh.l

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

# housekeeping
clean:
	rm -f core $(EXE) *.o *.tab.* *.yy.*
