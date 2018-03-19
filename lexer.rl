#include <stdio.h>
#include "tree.h"
#include "parser.h"
#include "grammar.h"
#include "errors.h"

#define PARSE(token) state.offset=ts;parse_token(token,&parser,&state,0,0)
#define PARSE_S(token) state.offset=ts;parse_token(token,&parser,&state,ts,te)

State lexer(char* name,char* buffer,int len){
	void* parser=0;
	
	int cs;
	char* p = buffer;
	const char* pe = buffer + len;
	const char* eof = pe;
	
	char* ts;
	char* te;
	int act;
	int top;
	int stack[8];

	State state={name,p,p,0,0,0};

%%{
	machine lexer;

	comment_multi := |*
		'\n' => {state.offset=te;++state.line;state.line_offset=te;};
		'*/' => {state.offset=te;fret;};
		any => {state.offset=ts;};
	*|;
	comment_single := |*
		'\n' => {state.offset=te;++state.line;state.line_offset=te;fret;};
		any => {state.offset=ts;};
	*|;
	
	main := |*
		[0-9]+ => {PARSE_S(TOKEN_INT);};
		[\t\v\f\r ]+ => {state.offset=ts;};
		'\n' => {state.offset=te;++state.line;state.line_offset=te;};
		',' => {PARSE(TOKEN_CMM);};
		';' => {PARSE(TOKEN_SCOL);};
		':' => {PARSE(TOKEN_COL);};
		'{' => {PARSE(TOKEN_LBRC);};
		'}' => {PARSE(TOKEN_RBRC);};
		'[' => {PARSE(TOKEN_LBRK);};
		']' => {PARSE(TOKEN_RBRK);};
		'(' => {PARSE(TOKEN_LPRN);};
		')' => {PARSE(TOKEN_RPRN);};
		'+' => {PARSE(TOKEN_ADD);};
		'->' => {PARSE(TOKEN_ARRW);};
		'-' => {PARSE(TOKEN_SUB);};
		'*' => {PARSE(TOKEN_MUL);};
		'/*' => {fcall comment_multi;};
		'//' => {fcall comment_single;};
		'/' => {PARSE(TOKEN_DIV);};
		'!=' => {PARSE(TOKEN_NE);};
		'|' => {PARSE(TOKEN_OR);};
		'&' => {PARSE(TOKEN_AND);};
		'~' => {PARSE(TOKEN_NOT);};
		'..' => {PARSE(TOKEN_DDOT);};
		'.' => {PARSE(TOKEN_DOT);};
		'#' => {PARSE(TOKEN_IDX);};
		'==' => {PARSE(TOKEN_EQ);};
		'<->' => {PARSE(TOKEN_DARRW);};
		'<=' => {PARSE(TOKEN_LE);};
		'>=' => {PARSE(TOKEN_GE);};
		'>' => {PARSE(TOKEN_GT);};
		'<' => {PARSE(TOKEN_LT);};
		'!' => {PARSE(TOKEN_EX);};
		'?' => {PARSE(TOKEN_QU);};
		'or' => {PARSE(TOKEN_OR);};
		'and' => {PARSE(TOKEN_AND);};
		'not' => {PARSE(TOKEN_NOT);};
		'div' => {PARSE(TOKEN_DIV);};
		'mod' => {PARSE(TOKEN_MOD);};
		'implies' => {PARSE(TOKEN_ARRW);};
		'iff' => {PARSE(TOKEN_DARRW);};
		'if' => {PARSE(TOKEN_IF);};
		'in' => {PARSE(TOKEN_IN);};
		'boolean' => {PARSE(TOKEN_BOOL);};
		'then' => {PARSE(TOKEN_THEN);};
		'true' => {PARSE(TOKEN_TRUE);};
		'false' => {PARSE(TOKEN_FALSE);};
		'excludes' => {PARSE(TOKEN_EXCL);};
		'requires' => {PARSE(TOKEN_REQ);};
		'_' => {PARSE_S(TOKEN_ANON);};
		[_a-zA-Z][_a-zA-Z0-9]* => {PARSE_S(TOKEN_ID);};
		any => {state.offset=ts;++state.errors;error_lex(&state); fbreak;};
	*|;
}%%

	%% write data;

	%% write init;

	%% write exec;
	
	parse_token(0,&parser,&state,0,0);
	
	return state;
}
