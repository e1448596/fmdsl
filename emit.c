#include <stdlib.h>
#include "tree.h"
#include "emit.h"

Name new_name(char* name){
	Name n={0,name};
	return n;
}

Name new_shared_name(char* name){
	Name n={1,name};
	return n;
}

static void free_name(Name n){
	if(!n.shared) free(n.str);
}

void free_emit(Emit* emit){
	while(emit){
		Emit* next=emit->next;
		switch(emit->type){
		case EMIT_INTDECL:
		case EMIT_BOOLDECL:
		case EMIT_VAR:
		case EMIT_INTVAR:
		case EMIT_BOOLVAR:
			free_name(emit->name);
			break;
		case EMIT_CARDN:
			free_name(emit->name);
			free_emit(emit->vars);
			break;
		case EMIT_OP:
			free_emit(emit->left);
			free_emit(emit->right);
			break;
		case EMIT_KV:
			free_emit(emit->key);
			free_emit(emit->val);
			break;
		case EMIT_PRED:
			free_name(emit->name);
			free_emit(emit->params);
			free_emit(emit->body);
			break;
		case EMIT_DICT:
			free_name(emit->name);
		case EMIT_LIST:
			free_emit(emit->items);
			break;
		default:
			break;
		}
		free(emit);
		emit=next;
	}
}

static Emit* new_emit(EmitType type,Name* name){
	Emit* e=calloc(1,sizeof(Emit));
	e->type=type;
	if(name){
		e->name.str=name->str;
		e->name.shared=name->shared;
		name->shared=1;
	}
	e->tail=&(e->next);
	return e;
}

Emit* new_emit_var(Name* name){
	return new_emit(EMIT_VAR,name);
}

Emit* new_emit_intvar(Name* name){
	return new_emit(EMIT_INTVAR,name);
}

Emit* new_emit_boolvar(Name* name){
	return new_emit(EMIT_BOOLVAR,name);
}

Emit* new_emit_intdecl(Name* name,int start,int end){
	Emit* i=new_emit(EMIT_INTDECL,name);
	i->start=start;
	i->end=end;
	return i;
}

Emit* new_emit_booldecl(Name* name){
	return new_emit(EMIT_BOOLDECL,name);
}

Emit* new_emit_int(int i){
	Emit* e=new_emit(EMIT_INT,0);
	e->value=i;
	return e;
}

Emit* new_emit_bool(int i){
	Emit* e=new_emit(EMIT_BOOL,0);
	e->value=i;
	return e;
}

Emit* new_emit_op(Emit* left,TreeType op,Emit* right){
	Emit* e=new_emit(EMIT_OP,0);
	e->left=left;
	e->right=right;
	e->op=op;
	return e;
}

Emit* new_emit_list(){
	Emit* list=new_emit(EMIT_LIST,0);
	list->depth=1;
	return list;
}

Emit* new_emit_cardn(Name* name){
	return new_emit(EMIT_CARDN,name);
}

Emit* new_emit_dict(Name* shape){
	return new_emit(EMIT_DICT,shape);
}

Emit* new_emit_pred(Name* shape){
	return new_emit(EMIT_PRED,shape);
}

static void emit_add(Emit* head,Emit* last){
	*(head->tail)=last;
	head->tail=last->tail;
}

void emit_add_param(Emit* pred,Emit* param){
	if(!pred->params){
		pred->params=param;
	}
	else{
		emit_add(pred->params,param);
	}
}

void emit_add_body(Emit* pred,Emit* body){
	if(!pred->body){
		pred->body=body;
	}
	else{
		emit_add(pred->body,body);
	}
}

void emit_add_list(Emit* list,Emit* item){
	if(!list->items){
		list->items=item;
	}
	else{
		emit_add(list->items,item);
	}
	if(item->type==EMIT_LIST && item->depth>=list->depth){
		list->depth=1+item->depth;
	}
}

void emit_add_dict(Emit* dict,Emit* key,Emit* val){
	Emit* kv=new_emit(EMIT_KV,0);
	kv->key=key;
	kv->val=val;
	if(!dict->items){
		dict->items=kv;
	}
	else{
		emit_add(dict->items,kv);
	}
}

void emit_add_var(Emit* crd,Emit* var){
	if(!crd->vars){
		crd->vars=var;
	}
	else{
		emit_add(crd->vars,var);
	}
}
