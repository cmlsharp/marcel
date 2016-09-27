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
#include <string.h> // strdup
#include <ctype.h>
#include "parser.h" // NL, OUT_T, OUT_A...

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wint-conversion"
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


char *esc_strdup(char *str)
{
    size_t len = strlen(str);
    char *ret = malloc((len+1) * sizeof *ret);
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
#pragma GCC diagnostic pop
