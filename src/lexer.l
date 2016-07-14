%{
#include <string.h> // strdup
#include <ctype.h>
#include "parser.h" // NL, OUT_T, OUT_A...
#include "macros.h" // Cast

#ifdef __cplusplus
extern "C" int yylex(void);
#endif

GCC_PUSH;
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#ifndef __cplusplus
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif
char *esc_strdup(char *str);
%}

R_CHARS [ \n\t\<>\|&\\] 
NO_R_CHARS [^ \n\t\<>\|&\\] 
L_WORD ({NO_R_CHARS}|\\{R_CHARS})+
%%


\n      {return NL;}
>       {return OUT_T;} 
&>      {return OUT_ERR_T;}
&>>     {return OUT_ERR_A;}
2>      {return ERR_T;}
2>>     {return ERR_A; }
>>      {return OUT_A;}
\<      {return IN;}
\|      {return PIPE;}
&       {return BKG;}
[ \t]   {}

\"[^\"]*\" |
\'[^\']*\' {
    yylval.str = strdup(yytext + 1);
    yylval.str[yyleng-2] = '\0';
    return WORD;

}

[a-zA-Z_]+={L_WORD} {
   yylval.str = esc_strdup(yytext);
   return ASSIGN;

}

{L_WORD} {
    yylval.str = esc_strdup(yytext);
    return WORD;
}

%%
GCC_POP;

char *esc_strdup(char *str)
{
    size_t len = strlen(str);
    char *ret = Cast(char*) malloc((len+1) * sizeof *ret);
    _Bool prev = 0;
    for (size_t i = 0, j = 0; i < len + 1; i++) {
        if (str[i] == '\\' && prev == 0) {
            prev = 1;
            continue;
        }
        ret[j++] = str[i];
        prev = 0;
    }
    return ret;
}
