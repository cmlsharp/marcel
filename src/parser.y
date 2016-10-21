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

#include "execute.h" // builtin, lookup_table
#include "ds/proc.h" // proc, job
#include "ds/vec.h" // vappend
#include "lexer.h" // yylex (in bison generated code)
#include "macros.h" // Stopif, Free

#define P_TRUNCATE (O_WRONLY | O_TRUNC | O_CREAT)
#define P_APPEND (O_WRONLY | O_APPEND | O_CREAT)
#define P_LAST (p_job->procs[vlen(p_job->procs)-1])

// I hate to use a macro for this but the lack of code duplication is worth it
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

int yyerror (job *w, char const *s);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

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

/*full_line:*/
    /*envs pipes_line {*/
        /*// Process has variables set but no command, meaning that its just an assigment*/
        /*if (!p_job->procs[0]->argv[0]) {*/
            /*size_t env_len = vlen(p_job->procs[0]->env);*/
            /*for (size_t i = 0; i < env_len; i++) {*/
                /*builtin *b = malloc(sizeof *b);*/
                /*Assert_alloc(b);*/
                /*b->type = VAR;*/
                /*b->var =  p_job->procs[0]->env[i] + strlen(p_job->procs[0]->env[i]) + 1;*/
                /*add_node(p_job->procs[0]->env[i], b, lookup_table);*/
                /*Err_msg("Warning: accessing local variables unimplemented");*/
            /*}*/
        /*}*/
    /*}*/
    /*;*/

pipes_line:
    pipes io_mods bkg {p_job->valid = 1;}
    | 
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
   envs WORD args {P_LAST->argv[0] = $2;}
   ;

envs:
    envs ASSIGN {vappend(&($2), sizeof (char*), &(P_LAST->env));}
    | { // this is reached only before the first arg of each pipe 
        // e.g. the command:  var=val a b | c d | var2=val2 e f
        //                   ^             ^     ^    <-- reached in those places
        proc *p = new_proc();
        vappend(&p, sizeof (proc *), &(p_job->procs));
        char *null = NULL;
        vappend(&null, sizeof (char *), &(P_LAST->argv));
    }
    ;

args:
    args real_arg {vappend(&($2), sizeof (char*), &(P_LAST->argv));}
    | {}
    ;

// Make things like `echo VAR=VAL` work as expected
real_arg: WORD {$$ = $1;} | ASSIGN {$$ = $1;}

%%

int yyerror (job *w, char const *s)
{
    (void) w;
    Err_msg("%s", s);
    return 0;
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

/*yydebug = 1;*/
