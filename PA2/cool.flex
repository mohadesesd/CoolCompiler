/* 
*  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");



/* to assemble string constants */
char string_buf[MAX_STR_CONST];
char *string_buf_ptr;
extern int curr_lineno;
extern int verbose_flag;
extern YYSTYPE cool_yylval;


int comment_depth = 0;
 
int string_length;

bool checkIfstrIsTooLong();
void resetStringBuffer();
void setErrorMassage(char* msg);
int strLengthError();

%}

%x COMMENT IN_LINE_COMMENT STRING  STRING_ERR

DASH		"-"
STARTCOM	"(*"
ENDCOM		"*)"
NEWLINE		\n
DIGIT          [0-9]
ALPHANUMERIC    [a-zA-Z0-9_]
TYPEID          [A-Z]{ALPHANUMERIC}*
OBJECTID        [a-z]{ALPHANUMERIC}*
DARROW          =>
LE              <=
ASSIGN          <-
INTEGERS	{DIGIT}+
SYMBOL		[:;(=).+\-*/<,~{@}]
WHITESPACE	[ \f\r\t\v]

%%

{STARTCOM}               {
                        comment_depth++;
                        BEGIN(COMMENT);
                    }
<COMMENT>{
	{STARTCOM}       {   comment_depth++; }
	{NEWLINE}       {   curr_lineno++; }
	{ENDCOM}       {
                        comment_depth--;
                        if (comment_depth == 0) {
                            BEGIN(INITIAL);
                        }
			}
	.          {}
	<<EOF>>    {
                        setErrorMassage("EOF in comment");
                        BEGIN(INITIAL);
                        return ERROR;
	                }
}
{ENDCOM}                {
                        setErrorMassage("Unmatched *)");
                        BEGIN(INITIAL);
                        return ERROR;
	                }
{DASH}.{DASH}             {   BEGIN(IN_LINE_COMMENT); }
<IN_LINE_COMMENT>{
	.   {}
	{NEWLINE} {
                        curr_lineno++;
                        BEGIN(INITIAL);
                    }
}



{INTEGERS}   {
                    cool_yylval.symbol = inttable.add_string(yytext);
                    return INT_CONST;
	            }
{DARROW}		{   return DARROW; }
{LE}            {   return LE; }
{ASSIGN}        {   return ASSIGN; }
{SYMBOL}	{ return int(yytext[0]);}


 


(?i:class)      {   return (CLASS); }
(?i:inherits)   {   return (INHERITS); }
(?i:isvoid)     {   return (ISVOID); }
(?i:new)        {   return (NEW); }
(?i:if)         {   return (IF); }
(?i:then)       {   return (THEN); }
(?i:else)       {   return (ELSE); }
(?i:fi)         {   return (FI); }
(?i:of)         {   return (OF); }
(?i:not)        {   return (NOT); }
(?i:in)         {   return (IN); }
(?i:case)       {   return (CASE); }
(?i:esac)       {   return (ESAC); }
(?i:let)        {   return (LET); }
(?i:loop)       {   return (LOOP); }
(?i:pool)       {   return (POOL); }
(?i:while)      {   return (WHILE); }

t(?i:rue)       {   
	                cool_yylval.boolean = true;
	                return (BOOL_CONST);
	            }
f(?i:alse)      {   
	                cool_yylval.boolean = false;
	                return (BOOL_CONST);
	            }

{TYPEID}        {
                    cool_yylval.symbol = idtable.add_string(yytext);
                    return (TYPEID);
	            }
{OBJECTID}      {
                    cool_yylval.symbol = idtable.add_string(yytext);
                    return (OBJECTID);
	            }


\"              {
                    BEGIN(STRING);
                    string_length = 0;
	            }
<STRING>{
	\"      {
                    cool_yylval.symbol = stringtable.add_string(string_buf);
                    resetStringBuffer();
                    BEGIN(INITIAL);
                    return (STR_CONST);
	            }
         \0      {
                    setErrorMassage("String contains null character");
                    resetStringBuffer();
                    BEGIN(STRING_ERR);
                    return ERROR;
	            }
	\\\0    {
                    setErrorMassage("String contains escaped null character.");
                    resetStringBuffer();
                    BEGIN(STRING_ERR);
                    return ERROR;
	            }
       {NEWLINE}      {
                    setErrorMassage("Unterminated string constant");
                    resetStringBuffer();
                    curr_lineno++;
                    BEGIN(INITIAL);
                    return ERROR;
	            }
	\\n     {
	             
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length = string_length + 2;
                     strcat(string_buf, "\n");
		     }
	\\\n    {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
                    curr_lineno++;
                    strcat(string_buf, "\n");
                }
	\\t     {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
		   strcat(string_buf, "\t");
                    
                }
	\\b     {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
                    strcat(string_buf, "\b");
	            }
         \\f     {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
                    strcat(string_buf, "\f");
	            }

          \\.     {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
                    strcat(string_buf, &strdup(yytext)[1]);
	            }
          <<EOF>> {
	                setErrorMassage("EOF in string constant");
	                curr_lineno++;
                    BEGIN(INITIAL);
                    return ERROR;
	            }
          .       {
                    if (checkIfstrIsTooLong()) { return strLengthError(); }
                    string_length++;
                    strcat(string_buf, yytext);
	            }
}

<STRING_ERR>{
	\"  {
                    BEGIN(INITIAL);
	            }
        \\\n {
	                curr_lineno++;
                    BEGIN(INITIAL);
                }
	{NEWLINE}  {
	                curr_lineno++;
                    BEGIN(INITIAL);
	            }
        .   {}
}


{NEWLINE}            {   curr_lineno++; }
{WHITESPACE}     {}
.               {   
	                setErrorMassage(yytext);
                    return ERROR;
                }

%%

bool checkIfstrIsTooLong() {
	if (string_length + 1 >= MAX_STR_CONST) {
		BEGIN(STRING_ERR);
        return true;
    }
    return false;
}

void resetStringBuffer() {
    string_buf[0] = '\0';
}

void setErrorMassage(char* msg) {
    cool_yylval.error_msg = msg;
}

int strLengthError() {
    resetStringBuffer();
    setErrorMassage("String constant too long");
    return ERROR;
}
