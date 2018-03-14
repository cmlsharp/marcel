# Tell make to stop removing intermediate files
.SECONDARY:

CC ?= gcc
CFLAGS = -Wall -Werror -Wextra -Wno-missing-field-initializers -pipe -fstack-protector -Wformat-security -std=c99

# marcel requires POSIX.1-2001 base specification + XSI extensions
_DEFINES = _XOPEN_SOURCE=600
DEFINES  = $(addprefix -D, $(_DEFINES))

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
$(CC) $(CFLAGS) $(DEFINES) -MMD -c -o $@ $<
endef

debug: CFLAGS += -O0 -ggdb3
debug: all

release: CFLAGS += -O3
release: all

profile: CFLAGS += -pg
profile: release

all: $(EXE)


%.c %.h: %.y 
	bison --defines=$(@:.c=.h) --output=$(@:.h=.c) $<

%.c %.h:  %.l 
	flex --header-file=$(@:.c=.h) --outfile=$(@:.h=.c) $<



$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)



$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) Makefile
	$(cc-command)

$(OBJDIR)/%.o: $(SRCDIR)/ds/%.c $(HDRS) Makefile
	$(cc-command)

-include $(wildcard $(OBJDIR)/*.d)

clean:
	rm -f core $(EXE) $(basename $(FLEX)).h $(basename $(FLEX)).c $(basename $(BSON)).h $(basename $(BSON)).c
	rm -r $(OBJDIR)

