%{
#include <errno.h> // errno
#include <string.h>

#include <fcntl.h> // read, write, O_*
#include <unistd.h> // pipe

#include "children.h" // MAX_BKG_CHILD, num_bkg_child
#include "ds/cmd.h" // cmd, cmd_wrapper
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
        if (!wrap->io[FD].path) {                                                                   \
            wrap->io[FD] = (cmd_io) {.path = PATH, .oflag = OFLAG};                                 \
        } else {                                                                                    \
            Err_msg("Taking/sending IO to/from more than one source not supported. "                \
                    "Skipping \"%s\"", PATH);                                                       \
            Free(PATH);                                                                             \
        }                                                                                           \
    } while (0)
extern cmd *p_crawler;

int yyerror (cmd_wrapper *w, char const *s);

// Opens path with the flag and mode specified by oflag and m and adds the file 
// descriptor to the apropriate field in c
int modify_io(char const *path, int oflag, mode_t m, int fd, cmd *c);

%}

// Include marcel.h in .c file as well as header
%code requires {
    #include "ds/cmd.h"
}

%union {
    char *str;
}

%token <str> WORD ASSIGN 
%token OUT_T OUT_ERR_T OUT_A OUT_ERR_A ERR_T ERR_A IN 
%token NL PIPE BKG

%type <str> real_arg

%define parse.error verbose
%parse-param {cmd_wrapper *wrap}

%%

cmd_line:
    pipes io_mods bkg NL {p_crawler = NULL;}
    | NL
    ;

bkg:
    BKG {
        Stopif(num_bkg_child() == MAX_BKG_CHILD, ABORT_PARSE,
        "Maximum background processes reached. Aborting.");
        p_crawler->wait = 0;
    }
    | 
    ;

io_mods:
    io_mods io_mod
    | 
    ;

io_mod:
    IN real_arg {
        Add_io_mod($2, 0, O_RDONLY);
    }
    | OUT_T real_arg {
        Add_io_mod($2, 1, P_TRUNCATE);

    }
    | OUT_ERR_T real_arg {
        Add_io_mod($2, 1, P_TRUNCATE);
        Add_io_mod($2, 2, P_TRUNCATE);
    }
    | OUT_A real_arg {
        Add_io_mod($2, 1, P_APPEND);
    }
    | OUT_ERR_A real_arg {
        Add_io_mod($2, 1, P_APPEND);
        Add_io_mod($2, 2, P_APPEND);
    }
    | ERR_A real_arg {
        Add_io_mod($2, 2, P_APPEND);
    }
    | ERR_T real_arg {
        Add_io_mod($2, 2, P_TRUNCATE);
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
            wrap->root = new_cmd();
            p_crawler = wrap->root;
        } else {
            p_crawler->next = new_cmd();
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
cmd *p_crawler = NULL;


int yyerror (cmd_wrapper *w, char const *s)
{
    (void) w;
    p_crawler = NULL;
    Stopif(1, return 0, "%s", s);
}

/*yydebug = 1;*/
