#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "lexer.h"
#include "parser.h"
#include "emit.h"
#include "convert.h"
#include "gnu_prolog_emit.h"
#include "choco4_emit.h"
#include "gecode5_emit.h"
#include "gecode6_emit.h"

typedef struct {
	char* input;
	char* output;
	char* template;
	char* default_template;
	void(*cb)(FILE*,Emit*);
} Options;

static const char* MARKER="/**EMIT**/";

static int print_template(char* template,int length,void(*cb)(FILE*,Emit*),FILE* out,Emit* r){
	if(!length){
		cb(out,r);
		return 1;
	}
	char* marker=strstr(template,MARKER);
	if(!marker) return 0;
	if(marker-template) fwrite(template,1,marker-template,out);
	cb(out,r);
	fputs(marker+strlen(MARKER),out);
	return 1;
}

static char* default_template(char* path,char* name){
	char* end=strrchr(path,'/');
	if(!end) end=strrchr(path,'\\');
	int pathsize=(end ? end-path+1 : 0);
	char* template=calloc(1,strlen(name)+pathsize+1);
	if(pathsize) strncpy(template,path,pathsize);
	strcpy(template+pathsize,name);
	return template;
}

static void read_all(FILE* in,char** content,int* length){
	if(!in){
		*content=0;
		*length=0;
		return;
	}
	fseek(in,0,SEEK_END);
	*length=ftell(in);
	rewind(in);
	*content=calloc(1,*length+1);
	fread(*content,1,*length,in);
	fclose(in);
}

static Options get_options(int argc,char** argv){
	Options o={0,0,0,0,gnu_prolog_emit};
	for(int i=1;i<argc;++i){
		if(argv[i][0]=='-'){
			if(0==strcmp(argv[i],"-t")){
				++i;
				if(i<argc) o.template=argv[i];
			}
			else if(0==strcmp(argv[i],"-p")){
				o.cb=gnu_prolog_emit;
				o.default_template=0;
			}
			else if(0==strcmp(argv[i],"-c4")){
				o.cb=choco4_emit;
				o.default_template=default_template(argv[0],"choco4_template.java");
			}
			else if(0==strcmp(argv[i],"-g5")){
				o.cb=gecode5_emit;
				o.default_template=0;
			}
			else if(0==strcmp(argv[i],"-g6")){
				o.cb=gecode6_emit;
				o.default_template=0;
			}
			else fprintf(stderr,"Unrecognized option %s. Ignored.\n",argv[i]);
		}
		else if(!o.input){
			o.input=argv[i];
		}
		else if(!o.output){
			o.output=argv[i];
		}
	}
	return o;
}

int main(int argc,char** argv){
	State state;
	Emit* result=0;
	
	char* content=0;
	int length=0;

	if(argc<2){
		fprintf(stderr,"Usage: %s [-t template][-p][-c4][-g5][-g6] input [output]\nOptions:\n\t-t\tTemplate file with marker %s replaced with the output.\n"
		"\t-p\tOutput in GNU prolog. (default)\n"
		"\t-c4\tOutput in java for Choco 4.\n"
		"\t-g5\tOutput in C++ for Gecode 5.\n"
		"\t-g6\tOutput in C++ for Gecode 6.\n"
		"\n",argv[0],MARKER);
		return 0;
	}

	Options options=get_options(argc,argv);
	if(!options.input){
		fprintf(stderr,"No input file given\n");
		return 1;
	}

	FILE* fin=fopen(options.input,"rb");
	if(!fin){
		fprintf(stderr,"Input file missing: %s\n",options.input);
		return 1;
	}

	FILE* fout;
	if(options.output){
		fout=fopen(options.output,"wb");
		if(!fout){
			fprintf(stderr,"Ouput file cannot be opened: %s\n",options.output);
			return 1;
		}
	}
	else{
		fout=stdout;
	}
	
	FILE* ftpl=0;
	
	read_all(fin,&content,&length);

	state=lexer(options.input,content,length);

	result=convert(&state);
	if(result){
		if(options.template){
			ftpl=fopen(options.template,"rb");
			if(!ftpl){
				fprintf(stderr,"Template file missing: %s\n",options.template);
				return 1;
			}
		}
		else if(options.default_template){
			ftpl=fopen(options.default_template,"rb");
		}
		read_all(ftpl,&content,&length);
		if(!print_template(content,length,options.cb,fout,result)){
			fprintf(stderr,"Template file missing %s marker: %s\n",MARKER,options.template);
			return 1;
		}
	}
	if(options.output) fclose(fout);
	
	if(state.errors) fprintf(stderr,"%d errors.\n",state.errors);
	return state.errors ? 1 : 0;
}
