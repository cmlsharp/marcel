%{
#include <errno.h> // errno
#include <string.h>

#include <fcntl.h> // read, write, O_*
#include <unistd.h> // pipe

#include "children.h" // MAX_BKG_PROC
#include "lexer.h" // yylex (in bison generated code)
#include "helpers.h" // grow_array
#include "marcel.h"
#include "macros.h" // Stopif, Free

extern size_t arg_index;
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
%token <str> WORD
%token NL OUT_T OUT_A IN BKG PIPE 

%define parse.error verbose
%parse-param {cmd *crawler}

%%

cmd_line:
    pipes io_mods bkg NL {first_run = 1;}
    | NL
    ;

bkg:
    BKG {
        Stopif(num_bkg_proc() == MAX_BKG_PROC, YYABORT, \
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
   WORD args {crawler->argv.strs[0] = $1;}
   ;

args:
    args WORD {
        if (arg_index == crawler->argv.cap - 1) {
            crawler->argv.strs = grow_array(crawler->argv.strs, &crawler->argv.cap);
            Assert_alloc(crawler->argv.strs);
        }
        crawler->argv.strs[arg_index++] = $2;
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  a b | c d | e f
        //                   ^     ^     ^    <-- reached in those places
        arg_index = 1;
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

%%

_Bool first_run = 1;
size_t arg_index = 1;
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
