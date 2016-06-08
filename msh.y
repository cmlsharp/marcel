%{
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "msh.h"

#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

extern int num_args;
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
    | error NL{yyerrok;}
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
            exit(1);
        }
        first->in = f;
    }
    | OUT_T WORD {
        int f = open($2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (f == -1) {
            perror(NAME); 
            exit(1);
        }
        crawler->out = f;

    }
    | OUT_A WORD {
        int f = open($2, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (f == -1) {
            perror(NAME); 
            exit(1);
        }
        crawler->out = f;
    }
    ;

pipes:
    pipes PIPE args 
    | args 
    ;

args:
    args WORD {
        if (num_args == 3) {
            fprintf(stderr, "%s: Too many arguments. Last argument read: %s", NAME, $2);
            exit(1);
        }
        crawler->argv[num_args++] = $2;
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  a b | c d | e f
        //                   ^     ^     ^    <-- reached in those places
        num_args = 0;
        if (sentinel) { 
            sentinel = 0;
            first = crawler;
        } else {
            crawler->next = def_cmd();
            if (!crawler->next) exit(1);
            int fd[2];
            pipe(fd);
            crawler->out = fd[1];
            crawler->next->in = fd[0];
            crawler = crawler->next;
        }
    }
    ;

%%

int num_args = 0;
cmd *first = NULL;
int yyerror (char *s)
{
    /*fprintf(stderr, "Parse error %s\n", s);*/
}
