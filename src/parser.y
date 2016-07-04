%{
#include <errno.h> // errno
#include <string.h>

#include <fcntl.h> // read, write, O_*
#include <unistd.h> // pipe

#include "children.h" // MAX_BKG_CHILD, num_bkg_child
#include "lexer.h" // yylex (in bison generated code)
#include "helpers.h" // grow_array
#include "marcel.h" // cmd
#include "macros.h" // Stopif, Free

#define M_TRUNC (O_WRONLY | O_TRUNC | O_CREAT)
#define M_APPEND (O_WRONLY | O_APPEND | O_CREAT)
#define M_MODE 0644

// I hate to use a macro for this but the lack of code duplication is worth it
#define P_add_item(STRUCT, ENTRY)                                                           \
    do {                                                                                    \
        if (crawler->STRUCT.num == crawler->STRUCT.cap - 1) {                               \
            crawler->STRUCT.strs = grow_array(crawler->STRUCT.strs, &crawler->STRUCT.cap);  \
            Assert_alloc(crawler->STRUCT.strs);                                             \
        }                                                                                   \
        crawler->STRUCT.strs[crawler->STRUCT.num++] = ENTRY;                                \
    } while(0)

// Again, I hate making this a macro but I think reducing the noise makes the 
// code somewhat clearer. This macro has side effects (will abort parsing and
// free PATH if modify_io failes) which is why its name is in all caps
#define MODIFY_IO(PATH, OFLAG, MODE, FD, C)                                                 \
        Stopif(modify_io(PATH, OFLAG, MODE, FD, C) > 0,                                     \
               Free(PATH); YYABORT,                                                         \
               "%s: %s",                                                                    \
               (errno) ? strerror(errno) : "Setting multiple inputs/outputs not supported", PATH)                                             \

extern _Bool first_run;
extern cmd *first;

int yyerror (cmd *c, char const *s);

// Opens path with the flag and mode specified by oflag and m and adds the file 
// descriptor to the apropriate field in c
int modify_io(char const *path, int oflag, mode_t m, int fd, cmd *c);

%}

// Include marcel.h in .c file as well as header
%code requires {
    #include "marcel.h"
}

%union {
    char *str;
}

%token <str> WORD ASSIGN 
%token <str> OUT_T OUT_ERR_T OUT_A OUT_ERR_A ERR_T ERR_A IN 
%token NL PIPE BKG

%type <str> real_arg

%define parse.error verbose
%parse-param {cmd *crawler}

%%

cmd_line:
    pipes io_mods bkg NL {first_run = 1;}
    | NL
    ;

bkg:
    BKG {
        Stopif(num_bkg_child() == MAX_BKG_CHILD, YYABORT, \
        "Maximum background processes reached. Aborting.");
        crawler->wait = 0;
    }
    | 
    ;

io_mods:
    io_mods io_mod
    | 
    ;

io_mod:
    IN {
        MODIFY_IO($1, O_RDONLY, M_MODE, 0, first);
        Free($1);
    }
    | OUT_T {
        MODIFY_IO($1, M_TRUNC, M_MODE, 1, crawler);
        Free($1);

    }
    | OUT_ERR_T {
        for (int i = 1; i <= 2; i++) {
            MODIFY_IO($1, M_TRUNC, M_MODE, i,  crawler);
        }
        Free($1);
    }
    | OUT_A {
        MODIFY_IO($1, M_APPEND, M_MODE, 1, crawler);
        Free($1);
    }
    | OUT_ERR_A {
        for (int i = 1; i <= 2; i++) {
            MODIFY_IO($1, M_APPEND, M_MODE, i, crawler);
        }
        Free($1);
    }
    | ERR_A {
        MODIFY_IO($1, M_APPEND, M_MODE, 2, crawler);
        Free($1);
    }
    | ERR_T {
        MODIFY_IO($1, M_TRUNC, M_MODE, 2, crawler);
        Free($1);
    }
    ;

pipes:
    pipes PIPE cmd
    | cmd
    ;

cmd:
   envs WORD args {crawler->argv.strs[0] = $2;}
   ;

envs:
    envs ASSIGN {
        P_add_item(env, $2); // See top of file
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  VAR=VAL a b | c d | VAR2=VAL2 e f
        //                   ^             ^     ^    <-- reached in those places
        if (first_run) { 
            first_run = 0;
            first = crawler;
        } else {
            crawler->next = new_cmd();
            int fd[2];
            pipe(fd);
            crawler->out = fd[1];
            crawler->next->in = fd[0];
            crawler = crawler->next;
        }
        crawler->argv.num = 1;
    }
    ;

args:
    args real_arg {
        P_add_item(argv, $2); // See top of file
    }
    | {}
    ;

// Make things like `echo VAR=VAL` work as expected
real_arg: WORD {$$ = $1;} | ASSIGN {$$ = $1;}

%%

_Bool first_run = 1;
cmd *first = NULL;


int yyerror (cmd *c, char const *s)
{
    (void) c;
    Stopif(1, return 0, "%s", s);
}

int modify_io(char const *path, int oflag, mode_t m, int fd, cmd *c)
{
    int *ptr;
    switch (fd) {
        case 0: ptr = &c->in; break;
        case 1: ptr = &c->out; break;
        case 2: ptr = &c->err; break;
        default: return -1; break; // Function used incorrectly
    }
    if (*ptr == fd) {
        int f = open(path, oflag, m);
        if (f == -1) return 1;
        *ptr = f;
        return 0;
    }
    return 1;
}

/*yydebug = 1;*/
