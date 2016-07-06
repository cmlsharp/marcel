# Tell make to stop removing intermediate files
.SECONDARY:

CC = gcc
CFLAGS = -Wall -O0 -ggdb3 -Wextra -pipe -fstack-protector -Wl,-zrelro -Wl,-z,now -Wformat-security -std=c11
EXE = marcel
LIBS = -lreadline -lfl

SRCDIR = src
OBJDIR = obj
$(shell `mkdir -p $(OBJDIR)`)

CSRCS = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/ds/*.c)
BSON = $(patsubst %.y, %.c, $(wildcard $(SRCDIR)/*.y))
FLEX = $(patsubst %.l, %.c, $(wildcard $(SRCDIR)/*.l))

SRCS =  $(BSON) $(FLEX) $(CSRCS)

HDRS = $(SRCS:.c=.h)
OBJS = $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

define cc-command
$(CC) $(CFLAGS) -MMD -c -o $@ $<
endef


all: $(EXE)


%.c %.h: %.y 
	bison --defines=$(@:.c=.h) --output=$(@:.h=.c) $<

%.c %.h:  %.l 
	flex --header-file=$(@:.c=.h) --outfile=$(@:.h=.c) $<



$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)



$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) 
	$(cc-command)

$(OBJDIR)/%.o: $(SRCDIR)/ds/%.c $(HDRS)
	$(cc-command)

-include $(wildcard $(OBJDIR)/*.d)

clean:
	rm -f core $(EXE) $(basename $(FLEX)).{h,c} $(basename $(BSON)).{h,c}
	rm -r $(OBJDIR)

