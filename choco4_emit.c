#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"
#include "emit.h"

static char* op_name(TreeType op){
	switch(op){
	case TREE_NEG:return "neg()";
	case TREE_ADD:return "add";
	case TREE_SUB:return "sub";
	case TREE_DIV:return "div";
	case TREE_MUL:return "mul";
	case TREE_MOD:return "mod";
	case TREE_EQ:return "eq";
	case TREE_NE:return "ne";
	case TREE_GT:return "gt";
	case TREE_LT:return "lt";
	case TREE_GE:return "ge";
	case TREE_LE:return "le";
	case TREE_CAST:return "gt(0)";
	case TREE_RCAST:return "iff(boolVar(true))";
	case TREE_NOT:return "not()";
	case TREE_AND:return "and";
	case TREE_NAND: return "and";//append not later
	case TREE_OR:return "or";
	case TREE_IF:return "imp";
	case TREE_IFF:return "iff";
	default: return "?";
	}
}

static void print_expr(FILE* out,Emit* r);
static void print_op(FILE* out,Emit* r){
	Emit* op1=r->left ? r->left : r->right;
	Emit* op2=r->left ? r->right : r->left;
	print_expr(out,op1);
	fprintf(out,".%s",op_name(r->op));
	if(op2){
		fprintf(out,"(");
		print_expr(out,op2);
		fprintf(out,")");
	}
	if(r->op==TREE_NAND) fprintf(out,".not()");
}

static void print_items(FILE* out,Emit* items){
	while(items){
		print_expr(out,items);
		items=items->next;
		if(items) fprintf(out,",");
	}
}

static void print_list(FILE* out,Emit* items,int depth){
	fprintf(out,"new org.chocosolver.solver.variables.IntVar");
	for(int i=0;i<depth;++i) fprintf(out,"[]");
	fprintf(out,"{");
	while(items){
		int item_depth=items->type==EMIT_LIST ? items->depth : 0;
		for(int i=item_depth+1;i<depth;++i) fprintf(out,"{");
		print_expr(out,items);
		for(int i=item_depth+1;i<depth;++i) fprintf(out,"}");
		items=items->next;
		if(items) fprintf(out,",");
	}
	fprintf(out,"}");
}

static void print_expr(FILE* out,Emit* r){
	switch(r->type){
	case EMIT_INT:
		fprintf(out,"intVar(%d)",r->value);
		break;
	case EMIT_BOOL:
		fprintf(out,"boolVar(%s)",r->value ? "true" : "false");
		break;
	case EMIT_VAR:
	case EMIT_INTVAR:
	case EMIT_BOOLVAR:
		fprintf(out,r->name.str);
		break;
	case EMIT_OP:
		print_op(out,r);
		break;
	case EMIT_LIST:
		print_list(out,r->items,r->depth);
		break;
	default:
		break;
	}
}

static void print_properties(FILE* out,Emit* r){
	while(r){
		switch(r->type){
		case EMIT_INTDECL:
			fprintf(out,"\tprivate org.chocosolver.solver.variables.IntVar %s=intVar(\"%s\",%d,%d,true);\n",r->name.str,r->name.str,r->start,r->end);
			break;
		case EMIT_BOOLDECL:
			fprintf(out,"\tprivate org.chocosolver.solver.variables.BoolVar %s=boolVar(\"%s\");\n",r->name.str,r->name.str);
			break;
		default:
			break;
		}
		r=r->next;
	}
}

static void print_statements(FILE* out,Emit* r){
	while(r){
		switch(r->type){
		case EMIT_CARDN:
			fprintf(out,"\t\tsum(new org.chocosolver.solver.variables.BoolVar[]{");
			print_items(out,r->vars);
			fprintf(out,"},\"=\",%s).post();\n",r->name.str);
			break;
		case EMIT_OP:
			fprintf(out,"\t\t");
			print_op(out,r);
			fprintf(out,".post();\n");
			break;
		case EMIT_PRED:
			fprintf(out,"\t\t%s(",r->name.str);
			print_items(out,r->params);
			fprintf(out,").post();\n");
			break;
		default:
			break;
		}
		r=r->next;
	}
}

static void print_method(FILE* out,Emit* r){
	print_properties(out,r->body);
	fprintf(out,"\tprivate void %s(){\n",r->name.str);
	print_statements(out,r->body);
	fprintf(out,"\t}\n");
}

#define NT(L) for(int i=0;i<L;++i)fprintf(out,"\t");
static void print_class(FILE* out,Emit* r,char* instance,int nest){
	char* cname=r->name.str;
	NT(nest) fprintf(out,"public class %s{\n",cname);
	Emit* kv=r->items;
	while(kv){
		switch(kv->val->type){
		case EMIT_DICT:
			print_class(out,kv->val,kv->key->name.str,nest+1);
			break;
		case EMIT_INTVAR:
			NT(nest+1) fprintf(out,"public org.chocosolver.solver.variables.IntVar %s=%s;\n",kv->key->name.str,kv->val->name.str);
			break;
		case EMIT_BOOLVAR:
			NT(nest+1) fprintf(out,"public org.chocosolver.solver.variables.BoolVar %s=%s;\n",kv->key->name.str,kv->val->name.str);
			break;
		default:
			break;
		}
		kv=kv->next;
	}
	NT(nest) fprintf(out,"}\n");
	NT(nest) fprintf(out,"public %s %s=new %s();\n",cname,instance,cname);
}

static void print_classes(FILE* out,Emit* r){
	Emit* param=r->params;
	while(param){
		if(param->type==EMIT_DICT){
			char* instance=strdup(param->name.str);
			instance[0]+=' ';
			print_class(out,param,instance,1);
			free(instance);
		}
		param=param->next;
	}
}

#include "choco4_support_string.h"

void choco4_emit(FILE* out,Emit* r){
	Emit* tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED){
			fprintf(out,"\t/*unsupported construct*/\n");
		}
		else{
			print_classes(out,tmp);
			print_method(out,tmp);
		}
		tmp=tmp->next;
	}

	tmp=r;

	fprintf(out,"\t{\n");
	while(tmp){
		if(tmp->type!=EMIT_PRED){
			fprintf(out,"\t\t/*unsupported construct*/\n");
		}
		else{
			fprintf(out,"\t\t%s();\n",tmp->name.str);
		}
		tmp=tmp->next;
	}
	fprintf(out,"\t}\n");
	
	fprintf(out,choco4_support);
}
