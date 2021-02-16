%{
    #include <stdio.h>
    #include<stdlib.h>
%}

%option c++
%option outfile="scanner.cpp"
%option case-insensitive
%option noyywrap

LETRA [a-zA-Z]

PARAMETRO "-"{LETRA}+

MKDISK "mkdisk"

RMDISK "rmdisk"

FDISK "fdisk"

%%

{MKDISK}            {printf("rec mkdisk %s\n", yytext);}
{RMDISK}              {printf("rec rmdisk %s\n", yytext);}
{FDISK}              {printf("rec fdisk %s\n", yytext);}
{PARAMETRO}       {printf("param %s\n", yytext);}
.                   {}


%%


