CC = gcc
CFLAGS = -Wall -O0 -ggdb3 -Wextra -Werror -pipe -fstack-protector -Wl,-zrelro -Wl,-z,now -Wformat-security -std=c11
EXE = marcel
LIBS = -lreadline -lfl
SRCS = marcel.tab.c lex.yy.c marcel.c execute.c hash_table.c signals.c children.c
HDRS = marcel.tab.h lex.yy.h marcel.h execute.h hash_table.h macros.h signals.h children.h
OBJS = $(SRCS:.c=.o)

%.c: %.y
%.c: %.l

all: $(EXE)

marcel.tab.c marcel.tab.h: marcel.y
	bison --defines=marcel.tab.h --output=marcel.tab.c marcel.y

lex.yy.c lex.yy.h: marcel.l
	flex --header-file=lex.yy.h marcel.l

$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

$(OBJS): $(SRCS) $(HDRS)

# housekeeping
clean:
	rm -f core $(EXE) *.o *.tab.* *.yy.*
