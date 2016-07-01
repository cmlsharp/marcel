%{
#include <string.h> // strdup
#include "parser.h" // NL, OUT_T, OUT_A...

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"
%}

%%

\n { return NL; }
>  { return OUT_T; } 
>> { return OUT_A; }
"<" { return IN; }
"&" { return BKG; }
\| { return PIPE; }
[ \t] {}
\"[^\"]*\" {
    yylval.str = strdup(yytext + 1);
    size_t len = strlen(yylval.str);
    yylval.str[len-1] = '\0';
    return WORD;

}
[^ \t\n][^ \t\n]* {
    yylval.str = strdup(yytext);
    return WORD;
}

%%

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
