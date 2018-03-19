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
	case TREE_DIV:return "/";
	case TREE_MUL:return "*";
	case TREE_MOD:return "%";
	case TREE_EQ:return "==";
	case TREE_NE:return "!=";
	case TREE_GT:return ">";
	case TREE_LT:return "<";
	case TREE_GE:return ">=";
	case TREE_LE:return "<=";
	case TREE_CAST:return ">0";
	case TREE_RCAST:return "==1";
	case TREE_NOT:return "!";
	case TREE_AND:return "&&";
	case TREE_NAND: return "&&";//append not later
	case TREE_OR:return "||";
	case TREE_IF:return ">>";
	case TREE_IFF:return "==";
	default: return "?";
	}
}

static void print_expr(FILE* out,Emit* r);

static void print_op(FILE* out,Emit* r){
	if(r->op==TREE_NAND) fprintf(out,"!");
	fprintf(out,"(");
	if(r->left) print_expr(out,r->left);
	fprintf(out," %s ",op_name(r->op));
	if(r->right) print_expr(out,r->right);
	fprintf(out,")");
}

static void print_list(FILE* out,Emit* items){
	Emit* i=items;
	while(i){
		switch(i->type){
		case EMIT_BOOLVAR:
			fprintf(out,"<<channel(*this,%s)",i->name.str);
			break;
		case EMIT_LIST:
			print_expr(out,i);
			break;
		default:
			fprintf(out,"<<");
			print_expr(out,i);
		}
		i=i->next;
	}
}

static void print_items(FILE* out,Emit* items){
	Emit* i=items;
	while(i){
		if(i->type==EMIT_LIST){
			fprintf(out,"IntVarArgs()");
			print_list(out,i->items);
		}
		else{
			print_expr(out,i);
		}
		i=i->next;
		if(i) fprintf(out,",");
	}
}

static void print_expr(FILE* out,Emit* r){
	switch(r->type){
	case EMIT_INT:
	case EMIT_BOOL:
		fprintf(out,"%d",r->value);
		break;
	case EMIT_INTVAR:
	case EMIT_BOOLVAR:
		fprintf(out,r->name.str);
		break;
	case EMIT_VAR:
		fprintf(out,"_that()->%s",r->name.str);
		break;
	case EMIT_OP:
		print_op(out,r);
		break;
	case EMIT_LIST:
		print_list(out,r->items);
		break;
	default:
		break;
	}
}

static void print_properties(FILE* out,Emit* r){
	while(r){
		switch(r->type){
		case EMIT_INTDECL:
			fprintf(out,"\tIntVar %s;\n",r->name.str);
			break;
		case EMIT_BOOLDECL:
			fprintf(out,"\tBoolVar %s;\n",r->name.str);
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
			fprintf(out,"\t\trel(*this,sum(BoolVarArgs()");
			Emit* v=r->vars;
			while(v){
				fprintf(out,"<<%s",v->name.str);
				v=v->next;
			}
			fprintf(out,")==channel(*this,%s));\n",r->name.str);
			break;
		case EMIT_OP:
			fprintf(out,"\t\trel(*this,");
			print_op(out,r);
			fprintf(out,");\n");
			break;
		case EMIT_PRED:
			fprintf(out,"\t\t_that()->%s(",r->name.str);
			print_items(out,r->params);
			fprintf(out,");\n");
			break;
		default:
			break;
		}
		r=r->next;
	}
}

static void print_method(FILE* out,Emit* r){
	print_properties(out,r->body);
	fprintf(out,"\tvoid %s(){\n",r->name.str);
	print_statements(out,r->body);
	fprintf(out,"\t}\n");
}

static void print_properties_init(FILE* out,Emit* r,int* first){
	Emit* body=r->body;
	while(body){
		switch(body->type){
		case EMIT_INTDECL:
			fprintf(out,"%c\n\t\t%s(*this,%d,%d)",*first?':':',',body->name.str,body->start,body->end);
			*first=0;
			break;
		case EMIT_BOOLDECL:
			fprintf(out,"%c\n\t\t%s(*this,0,1)",*first?':':',',body->name.str);
			*first=0;
			break;
		default:
			break;
		}
		body=body->next;
	}
}

static void print_properties_update(FILE* out,Emit* r){
	Emit* body=r->body;
	while(body){
		switch(body->type){
		case EMIT_BOOLDECL:
		case EMIT_INTDECL:
			fprintf(out,"\t\t%s.update(*this,_share,_model.%s);\n",body->name.str,body->name.str);
			break;
		default:
			break;
		}
		body=body->next;
	}
}

static void print_properties_branch(FILE* out,Emit* r){
	int bools=0;
	int ints=0;
	fprintf(out,"\t\t{\n\t\t\tBoolVarArgs bargs;\n");
	fprintf(out,"\t\t\tIntVarArgs iargs;\n");
	Emit* body=r->body;
	while(body){
		switch(body->type){
		case EMIT_BOOLDECL:
			fprintf(out,"\t\t\tbargs<<%s;\n",body->name.str);
			++bools;
			break;
		case EMIT_INTDECL:
			fprintf(out,"\t\t\tiargs<<%s;\n",body->name.str);
			++ints;
			break;
		default:
			break;
		}
		body=body->next;
	}
	if(bools){
		fprintf(out,"\t\t\tbranch(*this,bargs,_that()->branchFeatureVar(),_that()->branchFeatureVal());\n");
	}
	if(ints){
		fprintf(out,"\t\t\tbranch(*this,iargs,_that()->branchAttrVar(),_that()->branchAttrVal());\n");
	}
	fprintf(out,"\t\t}\n");
}

#define NT(L) for(int i=0;i<L;++i)fprintf(out,"\t");
static void print_class(FILE* out,Emit* r,char* instance,int nest){
	char* cname=r->name.str;
	NT(nest) fprintf(out,"struct %s{\n",cname);
	Emit* kv=r->items;
	int first=1;
	while(kv){
		switch(kv->val->type){
		case EMIT_DICT:
			print_class(out,kv->val,kv->key->name.str,nest+1);
			break;
		case EMIT_INTVAR:
			NT(nest+1) fprintf(out,"IntVar %s;\n",kv->key->name.str);
			break;
		case EMIT_BOOLVAR:
			NT(nest+1) fprintf(out,"BoolVar %s;\n",kv->key->name.str);
			break;
		default:
			break;
		}
		kv=kv->next;
	}
	NT(nest+1) fprintf(out,"%s(Model* _model)",cname);
	kv=r->items;
	while(kv){
		switch(kv->val->type){
		case EMIT_DICT:
			fprintf(out,"%c\n",first?':':',');
			NT(nest+2) fprintf(out,"%s(_model)",kv->key->name.str);
			first=0;
			break;
		case EMIT_INTVAR:
		case EMIT_BOOLVAR:
			fprintf(out,"%c\n",first?':':',');
			NT(nest+2) fprintf(out,"%s(_model->%s)",kv->key->name.str,kv->val->name.str);
			first=0;
			break;
		default:
			break;
		}
		kv=kv->next;
	}
	fprintf(out,"\n");
	NT(nest+1) fprintf(out,"{}\n");
	NT(nest) fprintf(out,"};\n");
	NT(nest) fprintf(out,"%s %s;\n",cname,instance);
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

static void print_instances_init(FILE* out,Emit* r){
	Emit* param=r->params;
	while(param){
		if(param->type==EMIT_DICT){
			char* instance=strdup(param->name.str);
			instance[0]+=' ';
			fprintf(out,",\n\t\t%s(this)",instance);
			free(instance);
		}
		param=param->next;
	}
}

#include "gecode5_support_string.h"

void gecode5_emit(FILE* out,Emit* r){
	fprintf(out,"#include <gecode/minimodel.hh>\n#include <gecode/search.hh>\nusing namespace Gecode;\ntemplate <typename T,typename ...M> class Model:public M...,public Space{\n\tfriend T;\n");

	fprintf(out,gecode5_support);
	
	Emit* tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t/*unsupported construct*/\n");
		else print_classes(out,tmp);
		tmp=tmp->next;
	}
	
	fprintf(out,"private:\n\tT* _that(){return static_cast<T*>(this);}\n\n");
	
	tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t/*unsupported construct*/\n");
		else print_method(out,tmp);
		tmp=tmp->next;
	}

	fprintf(out,"\tModel()");
	tmp=r;
	int first=1;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t\t/*unsupported construct*/\n");
		else{
			print_properties_init(out,tmp,&first);
			print_instances_init(out,tmp);
		}
		tmp=tmp->next;
	}
	fprintf(out,"\n\t{\n");
	tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t\t/*unsupported construct*/\n");
		else fprintf(out,"\t\t%s();\n",tmp->name.str);
		tmp=tmp->next;
	}
	tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t\t/*unsupported construct*/\n");
		else print_properties_branch(out,tmp);
		tmp=tmp->next;
	}
	fprintf(out,"\t}\n\tModel(bool _share,Model& _model):Space(_share,_model)");
	tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t\t/*unsupported construct*/\n");
		else print_instances_init(out,tmp);
		tmp=tmp->next;
	}
	fprintf(out,"\n\t{\n");
	tmp=r;
	while(tmp){
		if(tmp->type!=EMIT_PRED) fprintf(out,"\t\t/*unsupported construct*/\n");
		else print_properties_update(out,tmp);
		tmp=tmp->next;
	}
	fprintf(out,"\t}\n};\n");
}
