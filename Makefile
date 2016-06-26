CC = gcc
CFLAGS = -O0 -Wall -ggdb3 -Werror
EXE = msh
LIBS = -lreadline -lfl
SRCS = msh.tab.c lex.yy.c msh.c msh_internals.c hash_table.c 
HDRS = msh.tab.h lex.yy.h msh.h msh_internals.h hash_table.h msh_macros.h
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

$(OBJS): $(SRCS) $(HDRS)

# housekeeping
clean:
	rm -f core $(EXE) *.o *.tab.* *.yy.*
