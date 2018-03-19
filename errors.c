#include <stdio.h>
#include <string.h>
#include "tree.h"

void error_syntax(State* state){
	fprintf(stderr,"%s:%d:%d: Syntax error.\n",state->file,1+state->line,(int)(state->offset-state->line_offset)+1);
}

void error_lex(State* state){
	fprintf(stderr,"%s:%d:%d: Invalid character.\n",state->file,1+state->line,(int)(state->offset-state->line_offset)+1);
}

void error_attr_feature_missing(char* feature,char* attr){
	fprintf(stderr,"Feature %s is not found for attribute %s.\n",feature,attr);
}
void error_feature_dupe(char* feature,char* parent,char* newparent){
	fprintf(stderr,"Feature %s with parent %s is redefined",feature,parent);
	if(0!=strcmp(parent,newparent)){
		fprintf(stderr," as child of %s",newparent);
	}
	fprintf(stderr,".\n");
}

void error_multiple_roots(char* name){
	fprintf(stderr,"Multiple root feature %s.\n",name);
}

void error_cycle(char* name){
	fprintf(stderr,"Feature %s parents itself.\n",name);
}

void error_domain_bounds(Domain* d){
	fprintf(stderr,"Domain bounds are not linear ");
	while(d){
		fprintf(stderr,"[%d..%d]",d->end,d->start);
		d=d->next;
	}
	fprintf(stderr,".\n");
}

void error_attribute_dupe(char* feature,char* attr){
	fprintf(stderr,"Attribute %s.%s is redefined.\n",feature,attr);
}

void error_wrong_cardinality(Domain* d,int max){
	fprintf(stderr,"Group cardinality must fit [0..%d] but [%d..%d] is given.\n",max,d->start,d->end);
}

void error_expr_undefined(char* feature,int* selector,int slength,char* attr){
	int i;
	fprintf(stderr,"%s",feature);
	for(i=0;i<slength;++i) fprintf(stderr,"#%d",selector[i]);
	if(attr) fprintf(stderr,".%s",attr);
	fprintf(stderr," is not defined.\n");
}

void error_has_no_attr(char* feature){
	fprintf(stderr,"Feature %s has no attributes.\n",feature);
}

void error_attr_undefined(char* attr){
	fprintf(stderr,"Attribute %s is not defined in any feature.\n",attr);
}
