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

// I hate to use a macro for this but the lack of code duplication is worth it
#define P_ADD_ITEM(STRUCT, ENTRY)                                                           \
    do {                                                                                    \
        if (crawler->STRUCT.num == crawler->STRUCT.cap - 1) {                               \
            crawler->STRUCT.strs = grow_array(crawler->STRUCT.strs, &crawler->STRUCT.cap);  \
            Assert_alloc(crawler->STRUCT.strs);                                             \
        }                                                                                   \
        crawler->STRUCT.strs[crawler->STRUCT.num++] = ENTRY;                                \
    } while(0)

extern _Bool first_run;
extern cmd *first;

int yyerror (cmd *crawler, char const *s);
%}

// Include marcel.h in .c file as well as header
%code requires {
    #include "marcel.h"
}

%union {
    char *str;
}
%token <str> WORD ASSIGN
%token NL OUT_T OUT_A IN BKG PIPE 
%type <str> real_word

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
    IN WORD {
        if (crawler->in == 0) {
            int f = open($2, O_RDONLY);
            Stopif(f == -1, { Free($2); YYABORT; } , "%s", strerror(errno));
            first->in = f;
        }
        Free($2);
    }
    | OUT_T WORD {
        if (crawler->out == 1) {
            int f = open($2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            Stopif(f == -1, { Free($2); YYABORT; }, "%s",  strerror(errno));
            crawler->out = f;
        }
        Free($2);

    }
    | OUT_A WORD {
        if (crawler->out == 1) {
            int f = open($2, O_WRONLY | O_CREAT | O_APPEND, 0644);
            Stopif(f == -1, { Free($2); YYABORT; }, "%s", strerror(errno));
            crawler->out = f;
        }
        Free($2);
    }
    ;

pipes:
    pipes PIPE cmd
    | cmd
    ;

cmd:
   envs real_word args {crawler->argv.strs[0] = $2;}
   ;

envs:
    envs ASSIGN {
        P_ADD_ITEM(env, $2); // See top of file
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  VAR=VAL a b | c d | VAR2=VAL2 e f
        //                   ^             ^     ^    <-- reached in those places
        crawler->argv.num = 1;
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
    }
    ;

args:
    args real_word {
        P_ADD_ITEM(argv, $2); // See top of file
    }
    | {}
    ;

// Make things like `echo VAR=VAL` work as expected
real_word: WORD {$$ = $1;} | ASSIGN {$$ = $1;}

%%

_Bool first_run = 1;
cmd *first = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int yyerror (cmd *crawler, char const *s)
{
    Stopif(1, return 0, "%s", s);
}

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

/*yydebug = 1;*/
