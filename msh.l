%{
#include <string.h>
#include "msh.tab.h"
#include "msh.h"
#pragma GCC diagnostic ignored "-Wunused-function"
%}
%%
\n   { return NL; }
">"  { return OUT_T; } 
">>" { return OUT_A; }
"<"  { return IN; }
"&"  { return BKG; }
\| {return PIPE; }
[ \t] {}
[^ \t\n][^ \t\n]* {
    yylval.str = strdup(yytext);
    return WORD;
}
