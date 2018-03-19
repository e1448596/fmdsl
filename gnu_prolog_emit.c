#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"
#include "emit.h"

static char* op_name(TreeType op){
	switch(op){
	case TREE_NEG:return "-";
	case TREE_ADD:return "+";
	case TREE_SUB:return "-";
	case TREE_DIV:return "//";
	case TREE_MUL:return "*";
	case TREE_MOD:return "rem";
	case TREE_EQ:return "#=";
	case TREE_NE:return "#\\=";
	case TREE_GT:return "#>";
	case TREE_LT:return "#<";
	case TREE_GE:return "#>=";
	case TREE_LE:return "#=<";
	case TREE_CAST:return "#>0";
	case TREE_RCAST:return "#<=>1";
	case TREE_NOT:return "#\\";
	case TREE_AND:return "#/\\";
	case TREE_NAND: return "#\\/\\";
	case TREE_OR:return "#\\/";
	case TREE_IF:return "#==>";
	case TREE_IFF:return "#<=>";
	default: return "";
	}
}

static void print_expr(FILE* out,Emit* r,int newline,char* join){
	while(r){
		switch(r->type){
		case EMIT_INT:
		case EMIT_BOOL:
			fprintf(out,"%s%d",newline ? "\t":"",r->value);
			break;
		case EMIT_VAR:
		case EMIT_INTVAR:
		case EMIT_BOOLVAR:
			fprintf(out,"%s%s",newline ? "\t":"",r->name.str);
			break;
		case EMIT_INTDECL:
			fprintf(out,"fd_domain(%s,%d,%d)",r->name.str,r->start,r->end);
			break;
		case EMIT_BOOLDECL:
			fprintf(out,"fd_domain_bool(%s)",r->name.str);
			break;
		case EMIT_CARDN:
			fprintf(out,"fd_cardinality([");
			print_expr(out,r->vars,0,",");
			fprintf(out,"],%s)",r->name.str);
			break;
		case EMIT_OP:
			fprintf(out,"(");
			print_expr(out,r->left,0,",");
			fprintf(out," %s ",op_name(r->op));
			print_expr(out,r->right,0,",");
			fprintf(out,")");
			break;
		case EMIT_KV:
			fprintf(out,"\t");
			print_expr(out,r->left,0,",");
			fprintf(out," - ");
			print_expr(out,r->right,0,",");
			break;
		case EMIT_PRED:
			fprintf(out,"%s(",r->name.str);
			print_expr(out,r->params,0,",");
			fprintf(out,")");
			if(r->body){
				fprintf(out,":-\n");
				print_expr(out,r->body,1,",");
			}
			break;
		case EMIT_LIST:
		case EMIT_DICT:
			fprintf(out,"[\n");
			print_expr(out,r->items,1,",");
			fprintf(out,"\n]");
			break;
		}
		
		if(r->next){
			fprintf(out,join);
			if(newline) fprintf(out,"\n");
		}
		r=r->next;
	}
}

#include "gnu_prolog_support_string.h"

void gnu_prolog_emit(FILE* out,Emit* r){
	print_expr(out,r,1,".\n");
	fprintf(out,".\n");
	fprintf(out,gnu_prolog_support);
}
