%{
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "marcel.h"
#include "macros.h"
#include "children.h"
#include "marcel.tab.h"
#include "lex.yy.h"

extern int arg_index;
extern cmd *first;

int yyerror (cmd *crawler, int sentinel, char const *s);
%}

%code requires {
    #include "marcel.h"
}

%union {
    char *str;
}
%token <str> WORD
%token NL OUT_T OUT_A IN BKG PIPE 

%define parse.error verbose
%parse-param {cmd *crawler} {int sentinel}

%%

cmd_line:
    pipes io_mods bkg NL {}
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
   WORD args {crawler->argv[0] = $1;}
   ;

args:
    args WORD {
        Stopif(arg_index == (MAX_ARGS - 1), YYABORT, "Too many arguments. Last argument read: %s", NAME);
        crawler->argv[arg_index++] = $2;
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  a b | c d | e f
        //                   ^     ^     ^    <-- reached in those places
        arg_index = 1;
        if (sentinel) { 
            sentinel = 0;
            first = crawler;
        } else {
            crawler->next = def_cmd();
            int fd[2];
            pipe(fd);
            crawler->out = fd[1];
            crawler->next->in = fd[0];
            crawler = crawler->next;
        }
    }
    ;

%%

int arg_index = 1;
cmd *first = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wunused-parameter"

int yyerror (cmd *crawler, int sentinel, char const *s)
{
    fprintf(stderr, "%s: %s\n", NAME, s);
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

/*yydebug = 1;*/
