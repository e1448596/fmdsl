#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

Tree* new_op(Tree* value1,TreeType type,Tree* value2){
	Tree* t=malloc(sizeof(Tree));
	t->value1=value1;
	t->value2=value2;
	t->type=type;
	t->domain=0;
	t->next=0;
	return t;
}

Tree* new_tree(TreeType type,Tree* value){
	Tree* t=new_op(0,type,0);
	t->value=value;
	return t;
}

Tree* new_bool(int val){
	Tree* t=new_tree(TREE_BOOL,0);
	t->integer=val;
	return t;
}

Tree* new_int(char* str){
	Tree* t=new_tree(TREE_INT,0);
	t->integer=strtol(str,0,10);
	free(str);
	return t;
}

Tree* new_id(char* str){
	Tree* t=new_tree(TREE_ID,0);
	t->id=str;
	return t;
}

Tree* new_unchecked_id(char* str){
	Tree* t=new_id(str);
	t->type=TREE_UID;
	return t;
}

Tree* to_rule(Tree* cexpr){
	if(cexpr->type==TREE_ID || cexpr->type==TREE_IDX){
		cexpr=new_op(cexpr,TREE_RCAST,0);
	}
	return new_tree(TREE_EXPR,cexpr);
}

Tree* to_adecl(Tree* attr,Domain* domain){
	attr->type=TREE_ADECL;
	attr->domain=domain;
	return attr;
}

Tree* to_param(Tree* attr,char* default_value){
	attr->type=TREE_PARAM;
	if(default_value){
		attr->default_value=strtol(default_value,0,10);
		free(default_value);
	}
	return attr;
}

Tree* new_list(TreeType type){
	Tree* t=new_tree(type,0);
	t->tail=&(t->head);
	return t;
}

Tree* append_list(Tree* list,Tree* value){
	*(list->tail)=value;
	list->tail=&(value->next);
	return list;
}

Domain* new_domain(char* start,char* end,Domain* next){
	Domain* d=malloc(sizeof(Domain));
	d->start=strtol(start,0,10);
	d->end=strtol(end,0,10);
	d->next=next;
	free(start);
	free(end);
	return d;
}

Domain* new_domain_single(char* value,Domain* next){
	Domain* d=malloc(sizeof(Domain));
	d->start=strtol(value,0,10);
	d->end=d->start;
	d->next=next;
	free(value);
	return d;
}

void free_domain(Domain* d){
	while(d){
		Domain* next=d->next;
		free(d);
		d=next;
	}
}

void walk_tree(Tree* t,Topdown topdown,Bottomup bottomup,void* ctx){
	if(!t) return;
	Tree* v1=t->value1;
	Tree* v2=t->value2;
	Tree* head=t->head;
	TreeType type=t->type;
	
	if(topdown){
		if(topdown(t,ctx)) return;
	}
	
	switch(type){
	case TREE_LIST:
	case TREE_RULES:
		while(head){
			Tree* next=head->next;
			walk_tree(head,topdown,bottomup,ctx);
			head=next;
		}
		break;
	case TREE_UID:
	case TREE_ID:
	case TREE_INT:
	case TREE_BOOL:
		break;
	default:
		walk_tree(v1,topdown,bottomup,ctx);
		walk_tree(v2,topdown,bottomup,ctx);
		break;
	}
	
	if(bottomup){
		bottomup(t,ctx);
	}
}

static void free_node(Tree* t,void* ctx){
	switch(t->type){
	case TREE_ID:
	case TREE_UID:
		free(t->id);
		break;
	case TREE_GCAR:
		free_domain(t->cardinality);
		break;
	case TREE_ADECL:
		free_domain(t->domain);
		break;
	default:
		break;
	}
	free(t);
}

void free_tree(Tree* t){
	walk_tree(t,0,free_node,0);
}

void free_branches(Branches* l){
	Branches* ln;
	while(l){
		ln=l->next;
		free(l);
		l=ln;
	}
}

void add_branch(Branches** l,Tree* branch){
	Branches* nl=calloc(1,sizeof(Branches));
	nl->branch=branch;
	nl->next=*l;
	*l=nl;
}

void reverse_branches(Branches** l){
	Branches* head=*l;
	*l=0;
	Branches* next;
	while(head){
		next=head->next;
		head->next=*l;
		*l=head;
		head=next;
	}
}