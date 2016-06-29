CC = gcc
CFLAGS = -Wall -O0 -ggdb3 -Wextra -Werror -pipe -fstack-protector -Wl,-zrelro -Wl,-z,now -Wformat-security -fsanitize=address -fomit-frame-pointer -std=c11 
EXE = msh
LIBS = -lreadline -lfl
SRCS = msh.tab.c lex.yy.c msh.c msh_execute.c hash_table.c msh_signals.c msh_children.c
HDRS = msh.tab.h lex.yy.h msh.h msh_execute.h hash_table.h msh_macros.h msh_signals.h msh_children.h
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
