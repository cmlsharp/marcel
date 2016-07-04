# Tell make to stop removing intermediate files
.SECONDARY:

CC = gcc
CFLAGS = -Wall -O0 -ggdb3 -Wextra -Werror -pipe -fstack-protector -Wl,-zrelro -Wl,-z,now -Wformat-security -std=c11
EXE = marcel
LIBS = -lreadline -lfl

SRCDIR = src
OBJDIR = obj
$(shell `mkdir -p $(OBJDIR)`)

CSRCS = $(wildcard $(SRCDIR)/*.c) 
BSON = $(patsubst %.y, %.c, $(wildcard $(SRCDIR)/*.y))
FLEX = $(patsubst %.l, %.c, $(wildcard $(SRCDIR)/*.l))


SRCS =  $(BSON) $(FLEX) $(CSRCS)

HDRS = $(SRCS:.c=.h)
OBJS = $(addprefix obj/,$(notdir $(SRCS:.c=.o)))


all: $(EXE)


%.c %.h: %.y 
	bison --defines=$(@:.c=.h) --output=$(@:.h=.c) $<

%.c %.h:  %.l 
	flex --header-file=$(@:.c=.h) --outfile=$(@:.h=.c) $<



$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)



$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) 
	$(CC) $(CFLAGS) -MMD -c -o $@ $<


-include $(wildcard $(OBJDIR)/*.d)

clean:
	rm -f core $(EXE) $(basename $(FLEX)).{h,c} $(basename $(BSON)).{h,c}
	rm -r $(OBJDIR)

