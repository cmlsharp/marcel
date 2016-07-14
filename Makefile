# Tell make to stop removing intermediate files
.SECONDARY:

## For compiling with g++
#CC = g++ $(G++FLAGS)
#STD = c++11
#G++FLAGS = -fkeep-inline-functions 

CC = gcc
STD = c11
DEBUG = -O0 -ggdb3 
CFLAGS = -Wall $(DEBUG) -Wextra -Werror -pipe -fstack-protector -Wformat-security -Wno-missing-field-initializers -Wno-unused-function -std=$(STD)
EXE = marcel
LDFLAGS = -lreadline -lfl

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
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) 
	$(cc-command)

$(OBJDIR)/%.o: $(SRCDIR)/ds/%.c $(HDRS)
	$(cc-command)

-include $(wildcard $(OBJDIR)/*.d)

clean:
	rm -f core $(EXE) $(basename $(FLEX)).{h,c} $(basename $(BSON)).{h,c}
	rm -r $(OBJDIR)

