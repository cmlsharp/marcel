%{
#include <string.h> // strdup
#include <ctype.h>
#include "parser.h" // NL, OUT_T, OUT_A...

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"
char *skip_spaces(char *str, int inc);
%}

L_WORD [^ \t\n]+
SPACE [ \t]
S_WORD {SPACE}*{L_WORD}
BKG_WORD {L_WORD}{SPACE}*\&

%%


\n { return NL; }
>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+1, 1));
    return OUT_T; 
} 

&>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+2, 1));
    return OUT_ERR_T;
}

&>>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+3, 1));
    return OUT_ERR_A;
}

2>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+2, 1));
    return ERR_T; 
}

2>>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+3, 1));
    return ERR_A; 
}
>>{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+2, 1));
    return OUT_A; 
}

\<{S_WORD} { 
    yylval.str = strdup(skip_spaces(yytext+1, 1));
    return IN; 
}

\& {return BKG;}

\| { return PIPE; }

{SPACE} {}


\"[^\"]*\" |
\'[^\']*\' {
    yylval.str = strdup(yytext + 1);
    yylval.str[yyleng-2] = '\0';
    return WORD;

}

[a-zA-Z_]+={L_WORD} {
   yylval.str = strdup(yytext);
   return ASSIGN;

}

{L_WORD} {
    yylval.str = strdup(yytext);
    return WORD;
}

%%

char *skip_spaces(char *str, int inc)
{
    while(isspace(*str)) {str+=inc;}
    str-=inc; 
    return str;
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
