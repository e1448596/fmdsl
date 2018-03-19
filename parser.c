#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tree.h"

void Parse(void *parser,int type,char* token,State* state);
void* ParseAlloc(void *(*allocator)(size_t));
void ParseFree(void *parser,void (*deallocator)(void*));

void parse_token(int type,void **parser,State* state,char* start,char* end){
	char* t=0;
	int length=end-start;
	if(start){
		t=calloc(length+1,sizeof(char));
		memcpy(t,start,length);
	}
	if(!*parser) *parser=ParseAlloc(malloc);
	Parse(*parser,type,t,state);
	if(!type){
		ParseFree(*parser,free);
		*parser=0;
	}
}

