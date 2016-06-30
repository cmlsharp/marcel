CC = gcc
CFLAGS = -Wall -O0 -ggdb3 -Wextra -Werror -pipe -fstack-protector -Wl,-zrelro -Wl,-z,now -Wformat-security -std=c11
EXE = marcel
LIBS = -lreadline -lfl
SRCDIR = src
OBJDIR = obj
BINDIR = bin
SRCS = $(SRCDIR)/lexer.c $(SRCDIR)/parser.c $(wildcard $(SRCDIR)/*.c) 
OBJS := $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

all: $(EXE)

%.c %.h: %.y
	bison --defines=$(@:.c=.h) --output=$@ $<

%.c %.h: %.l
	flex --header-file=$(@:.c=.h) --outfile=$@ $<

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	rm $(SRCDIR)/parser.h $(SRCDIR)/lexer.h

$(OBJDIR)/%.o: $(SRCDIR)/%.c #$(SRCDIR)/%.l $(SRCDIR)/%.y
	$(CC) $(CFLAGS) -c -o $@ $<

# housekeeping
clean:
	rm -f core $(EXE) $(OBJDIR)/*.o 
