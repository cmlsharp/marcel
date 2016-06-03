CC = gcc
CFLAGS = -O0 -Wall -Werror
EXE = msh
HDRS = msh.h msh_execute.h
LIBS = -lreadline
SRCS = msh.c msh_execute.c
OBJS = $(SRCS:.c=.o)


$(EXE): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): $(HDRS) Makefile

# housekeeping
clean:
	rm -f core $(EXE) *.o
