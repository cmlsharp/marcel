CC = gcc
CFLAGS = -O0 -Wall -ggdb3 -pedantic
EXE = msh
HDRS = msh.h msh_execute.h hash_table.h
LIBS = -lreadline
SRCS = msh.c msh_execute.c hash_table.c
OBJS = $(SRCS:.c=.o)


$(EXE): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): $(HDRS) Makefile

# housekeeping
clean:
	rm -f core $(EXE) *.o
