/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

%{
#include <errno.h> // errno
#include <string.h>

#include <fcntl.h> // read, write, O_*
#include <unistd.h> // pipe

#include "ds/proc.h" // proc, job
#include "lexer.h" // yylex (in bison generated code)
#include "macros.h" // Stopif, Free

#define P_TRUNCATE (O_WRONLY | O_TRUNC | O_CREAT)
#define P_APPEND (O_WRONLY | O_APPEND | O_CREAT)

// First must be set to NULL to ensure next command runs successfully
#define ABORT_PARSE do { p_crawler = NULL; YYABORT; } while (0)

// I hate to use a macro for this but the lack of code duplication is worth it
#define P_add_item(STRUCT, ENTRY)                                                                   \
    do {                                                                                            \
        if (p_crawler->STRUCT->num == p_crawler->STRUCT->cap - 1) {                                 \
            Assert_alloc(grow_dyn_array(p_crawler->STRUCT) == 0);                                   \
        }                                                                                           \
        ((char **) p_crawler->STRUCT->data)[p_crawler->STRUCT->num++] = ENTRY;                      \
    } while(0)

// See above
#define Add_io_mod(PATH, FD, OFLAG)                                                                 \
    do {                                                                                            \
        if (!p_job->io[FD].path) {                                                                  \
            p_job->io[FD] = (proc_io) {.path = PATH, .oflag = OFLAG};                               \
        } else {                                                                                    \
            Err_msg("Taking/sending IO to/from more than one source not supported. "                \
                    "Skipping \"%s\"", PATH);                                                       \
            Free(PATH);                                                                             \
        }                                                                                           \
    } while (0)
extern proc *p_crawler;

int yyerror (job *w, char const *s);

%}

// Include marcel.h in .c file as well as header
%code requires {
    #include "ds/proc.h"
}

%union {
    char *str;
}

%token <str> WORD ASSIGN 
%token OUT_T OUT_ERR_T OUT_A OUT_ERR_A ERR_T ERR_A IN 
%token NL PIPE BKG

%type <str> real_arg

%define parse.error verbose
%parse-param {job *p_job}

%%

cmd_line:
    pipes io_mods bkg NL {p_crawler = NULL;}
    | NL
    ;

bkg:
    BKG {
        p_job->bkg = 1;
    }
    | 
    ;

io_mods:
    io_mods io_mod
    | 
    ;

io_mod:
    IN real_arg {
        Add_io_mod($2, STDIN_FILENO, O_RDONLY);
    }
    | OUT_T real_arg {
        Add_io_mod($2, STDOUT_FILENO, P_TRUNCATE);

    }
    | OUT_ERR_T real_arg {
        Add_io_mod($2, STDOUT_FILENO, P_TRUNCATE);
        Add_io_mod($2, STDERR_FILENO, P_TRUNCATE);
    }
    | OUT_A real_arg {
        Add_io_mod($2, STDOUT_FILENO, P_APPEND);
    }
    | OUT_ERR_A real_arg {
        Add_io_mod($2, STDOUT_FILENO, P_APPEND);
        Add_io_mod($2, STDERR_FILENO, P_APPEND);
    }
    | ERR_A real_arg {
        Add_io_mod($2, STDERR_FILENO, P_APPEND);
    }
    | ERR_T real_arg {
        Add_io_mod($2, STDERR_FILENO, P_TRUNCATE);
    }
    ;

pipes:
    pipes PIPE cmd
    | cmd
    ;

cmd:
   envs WORD args {((char **) p_crawler->argv->data)[0] = $2;}
   ;

envs:
    envs ASSIGN {
        P_add_item(env, $2); // See top of file
    }
    | { // This is reached ONLY before the first arg of each pipe 
        // E.g. the command:  VAR=VAL a b | c d | VAR2=VAL2 e f
        //                   ^             ^     ^    <-- reached in those places
        if (!p_crawler) { 
            p_job->root = new_proc();
            p_crawler = p_job->root;
        } else {
            p_crawler->next = new_proc();
            p_crawler = p_crawler->next;
        }
        p_crawler->argv->num = 1;
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
proc *p_crawler = NULL;


int yyerror (job *w, char const *s)
{
    (void) w;
    p_crawler = NULL;
    Err_msg("%s", s);
    return 0;
}

/*yydebug = 1;*/
