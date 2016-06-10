%{
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "msh.h"
#define YYDEBUG 1
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

extern int arg_index;
extern cmd *first;

%}

%code requires {
    #include "msh.h"
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
    | error NL {yyerrok;}
    ;

bkg:
    BKG {crawler->wait = 0;}
    | 
    ;

io_mods:
    io_mods io_mod
    | 
    ;

io_mod:
    IN WORD {
        int f = open($2, O_RDONLY);
        if (f == -1) {
            perror(NAME); 
        }
        first->in = f;
    }
    | OUT_T WORD {
        if (crawler->out == 1) {
            int f = open($2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (f == -1) {
                perror(NAME); 
            }
            crawler->out = f;
        }

    }
    | OUT_A WORD {
        if (crawler->out == 1) {
            int f = open($2, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (f == -1) {
                perror(NAME); 
            }
            crawler->out = f;
        }
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
        if (arg_index == MAX_ARGS - 1) {
            fprintf(stderr, "%s: Too many arguments. Last argument read: %s", NAME, $2);
            YYABORT;
        }
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
            if (!crawler->next) {
                fprintf(stderr, "%s: Memory allocation error.", NAME);
                YYABORT;
            }
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
int yyerror (char *s)
{
    fprintf(stderr, "Parse error %s\n", s);
}

/*yydebug = 1;*/
