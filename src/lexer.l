%{
#include <string.h> // strdup
#include "parser.h" // NL, OUT_T, OUT_A...

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"

%}

L_WORD [^ \t\n]+

%%

\n { return NL; }
>  { return OUT_T; } 
&> { return OUT_ERR_T;}
&>> { return OUT_ERR_A;}
2> { return ERR_T; }
2>> { return ERR_A; }
>> { return OUT_A; }
\< { return IN; }
& { return BKG; }
\| { return PIPE; }
[ \t] {}

\"[^\"]*\" |
\'[^\']*\' {
    yylval.str = strdup(yytext + 1);
    size_t len = strlen(yylval.str);
    yylval.str[len-1] = '\0';
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

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
