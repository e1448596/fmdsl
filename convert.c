#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tree.h"
#include "emit.h"
#include "convert.h"
#include "errors.h"

static int check_domain(int* errors,Domain* domain){
	Domain* p=0;
	Domain* d=domain;
	while(d){
		if(
			(d->start > d->end) ||
			(p && (p->start < d->end ))
		){
			error_domain_bounds(domain);
			++(*errors);
			return 0;
		}
		p=d;
		d=d->next;
	}
	return 1;
}

static void reverse_attributes(Attribute** l){
	Attribute* head=*l;
	*l=0;
	Attribute* next;
	while(head){
		next=head->next;
		head->next=*l;
		*l=head;
		head=next;
	}
}

static void reverse_params(Params** l){
	Params* head=*l;
	*l=0;
	Params* next;
	while(head){
		next=head->next;
		head->next=*l;
		*l=head;
		head=next;
	}
}

static void free_attributes(Attribute* a){
	Attribute* an;
	while(a){
		an=a->next;
		free(a);
		a=an;
	}
}

static Attribute* find_attribute(Feature* f,char* attr){
	Attribute* a=f->attributes;
	while(a){
		if(0==strcmp(a->name,attr)) return a;
		a=a->next;
	}
	return 0;
}

static void add_attribute(int* errors,Feature* f,char* attr,Domain* domain){
	if(!check_domain(errors,domain)) return;
	Attribute* a=find_attribute(f,attr);
	if(a){
		++(*errors);
		error_attribute_dupe(f->name,attr);
		return;
	}
	a=calloc(1,sizeof(Attribute));
	a->name=attr;
	a->domain=domain;
	a->next=f->attributes;
	f->attributes=a;
}

static void reverse_features(FeatureList** l){
	FeatureList* head=*l;
	*l=0;
	FeatureList* next;
	while(head){
		reverse_attributes(&(head->feature->attributes));
		next=head->next;
		head->next=*l;
		*l=head;
		head=next;
	}
}

static Feature* find_feature(FeatureList* l,char* name){
	while(l){
		if(0==strcmp(name,l->feature->name)) return l->feature;
		l=l->next;
	}
	return 0;
}

static Feature* add_parent_feature(FeatureList** l,char* name){
	Feature* existing=find_feature(*l,name);
	if(existing) return existing;
	
	FeatureList* nl=calloc(1,sizeof(FeatureList));
	nl->next=*l;
	*l=nl;
	nl->feature=calloc(1,sizeof(Feature));
	nl->feature->name=name;
	return nl->feature;
}

static void add_child_feature(int* errors,FeatureList** l,Feature* parent,char* name,int cardinality){
	Feature* existing=find_feature(*l,name);
	if(existing){
		if(existing->parent){
			error_feature_dupe(name,existing->parent->name,parent->name);
			++(*errors);
			return;
		}
	}
	else{
		FeatureList* nl=calloc(1,sizeof(FeatureList));
		nl->next=*l;
		*l=nl;
		nl->feature=calloc(1,sizeof(Feature));
		nl->feature->name=name;
		existing=nl->feature;
	}
	existing->cardinality=cardinality;
	existing->parent=parent;
}

static void free_feature_list(FeatureList* l){
	FeatureList* ln;
	while(l){
		ln=l->next;
		free_attributes(l->feature->attributes);
		free(l->feature);
		free(l);
		l=ln;
	}
}

static Selector new_selector(int length){
	Selector s={length,length ? calloc(length,sizeof(int)) : 0};
	return s;
}

static void free_selector(Selector s){
	if(s.length) free(s.index);
}

static Selector get_selector(Tree* t,char** name){
	int slength=0;
	*name=0;
	Tree* tt=t;
	int i;
	while(tt->type==TREE_IDX){
		++slength;
		tt=tt->value1;
	}
	*name=tt->id;
	Selector s=new_selector(slength);
	
	tt=t;
	for(i=0;i<slength;++i){
		s.index[slength-1-i]=tt->value2->integer;
		tt=tt->value1;
	}
	return s;
}

static int check_clone(Feature* feature,Selector selector){
	if(!feature) return 0;
	Feature* f=feature;
	int last=0;
	while(f){
		if(f->cardinality) ++last;
		f=f->parent;
	}
	if(last!=selector.length) return 0;
	if(!selector.length) return 1;
	f=feature;
	while(f){
		int cardinality=f->cardinality;
		if(cardinality){
			--last;
			if(selector.index[last]>=cardinality) return 0;
		}
		f=f->parent;
	}
	return 1;
}

static char* get_name(char* prefix,char* name1,Selector selector,char* name2,int idx){
	int written=strlen(name1);
	if(prefix){
		written+=snprintf(0,0,"%s_",prefix);
	}
	int i;
	for(i=0;i<selector.length;++i){
		written+=snprintf(0,0,"_%d",selector.index[i]);
	}
	if(name2){
		written+=snprintf(0,0,"_%s",name2);
	}
	if(idx>=0){
		written+=snprintf(0,0,"_%d",idx);
	}
	char* str=calloc(written+1,1);
	int offset=0;
	if(prefix){
		offset=sprintf(str,"%s_",prefix);
	}
	offset+=sprintf(str+offset,name1);
	for(i=0;i<selector.length;++i){
		offset+=sprintf(str+offset,"_%d",selector.index[i]);
	}
	if(name2){
		offset+=sprintf(str+offset,"_%s",name2);
	}
	if(idx>=0){
		sprintf(str+offset,"_%d",idx);
	}
	return str;
}

static void foreach_clone(Feature* feature,CloneCB cb,void* ctx){
	Feature* f=feature;
	int selector_length=0;
	while(f){
		if(f->cardinality) ++selector_length;
		f=f->parent;
	}
	if(!selector_length){
		cb(feature,new_selector(0),ctx);
		return;
	}
	int i=selector_length;
	Selector last_selector=new_selector(selector_length);
	Selector current_selector=new_selector(selector_length);
	int clones=1;
	f=feature;
	while(f){
		int cardinality=f->cardinality;
		if(cardinality){
			--i;
			clones*=cardinality;
			last_selector.index[i]=cardinality-1;
		}
		f=f->parent;
	}
	while(clones){
		--clones;
		cb(feature,current_selector,ctx);
		for(int j=0;j<selector_length;++j){
			if(current_selector.index[j]<last_selector.index[j]){
				++current_selector.index[j];
				break;
			}
			else current_selector.index[j]=0;
		}
	}
	free_selector(current_selector);
	free_selector(last_selector);
}

static void free_params(Params* l){
	Params* ln;
	while(l){
		ln=l->next;
		free_selector(l->selector);
		free(l);
		l=ln;
	}
}

typedef void(*ParamCB)(Feature* f,Selector selector,Attribute* a,int default_value,void* ctx);

typedef struct {
	ParamCB cb;
	int default_value;
	void* ctx;
	Attribute* attr;
} FPCtx;

static void _foreach_param(Feature* f,Selector selector,FPCtx* ctx){
	ctx->cb(f,selector,ctx->attr,ctx->default_value,ctx->ctx);
}

static void foreach_param(FeatureList* features,Tree* t,ParamCB cb,void* ctx){
	Tree* fbranch=t->value1;
	Tree* abranch=t->value2;
	Feature* f=0;
	Attribute* a;
	Selector selector;
	char* fname=0;
	if(fbranch && abranch){
		selector=get_selector(fbranch,&fname);
		f=find_feature(features,fname);
		a=find_attribute(f,abranch->id);
		cb(f,selector,a,t->default_value,ctx);
		free_selector(selector);
	}
	else if(fbranch){
		selector=get_selector(fbranch,&fname);
		f=find_feature(features,fname);
		a=f->attributes;
		while(a){
			cb(f,selector,a,t->default_value,ctx);
			a=a->next;
		}
		free_selector(selector);
	}
	else if(abranch){
		while(features){
			FPCtx fpctx={cb,t->default_value,ctx,0};
			a=find_attribute(features->feature,abranch->id);
			if(a){
				fpctx.attr=a;
				foreach_clone(features->feature,(CloneCB)_foreach_param,&fpctx);
			}
			features=features->next;
		}
	}
}

static char* get_feature_key(char* fname,Selector selector){
	return get_name(0,fname,selector,0,-1);
}

static char* get_attr_shape(char* fname,Selector selector){
	return get_name("Attr",fname,selector,0,-1);
}

static char* get_feature_name(char* fname,Selector selector){
	return get_name("F",fname,selector,0,-1);
}

static char* get_clone_name(char* fname,Selector selector,int idx){
	return get_name("F",fname,selector,0,idx);
}

static char* get_attr_name(char* fname,Selector selector,char* aname){
	return get_name("A",fname,selector,aname,-1);
}

static char* get_param_name(char* fname,Selector selector,char* aname,int default_value){
	return get_name("V",fname,selector,aname,default_value);
}

typedef struct{
	int* errors;
	FeatureList** features;
	Feature* current;
	char* attr;
	int cardinality;
} FCtx;

static int _extract_attrs(Tree* t,FCtx* ctx){
	if(t->type!=TREE_ADECL) return 0;
	Feature* f=find_feature(*(ctx->features),t->value1->id);
	if(f){
		add_attribute(ctx->errors,f,t->value2->id,t->domain);
	}
	else{
		++(*(ctx->errors));
		error_attr_feature_missing(t->value1->id,t->value2->id);
	}
	return 1;
}

static void _extract_child(Tree* t,FCtx* ctx){
	if(t->type!=TREE_ID) return;
	add_child_feature(ctx->errors,ctx->features,ctx->current,t->id,ctx->cardinality);
}

static int _extract_parent(Tree* t,FCtx* ctx){
	ctx->cardinality=0;
	switch(t->type){
	case TREE_FCAR:
		if(check_domain(ctx->errors,t->cardinality)){
			ctx->cardinality=t->cardinality->end;
		}
	case TREE_GCAR:
	case TREE_OPTREL:
	case TREE_MANREL:
	case TREE_ALTREL:
	case TREE_ORREL:
		ctx->current=add_parent_feature(ctx->features,t->value1->id);
		walk_tree(t->value2,0,(Bottomup)_extract_child,ctx);
		return 1;
	default:
		break;
	}
	return 0;
}

static void check_multiple_roots(int* errors,FeatureList *l){
	int roots=0;
	char* first=0;
	while(l){
		if(l->feature->parent==0){
			++roots;
			switch(roots){
			case 1:
				first=l->feature->name;
				break;
			case 2:
				error_multiple_roots(first);
				++(*errors);
			default:
				error_multiple_roots(l->feature->name);
				++(*errors);
			}
		}
		l=l->next;
	}
}

static void check_cycles(int* errors,FeatureList* l){
	int features=0;
	FeatureList* ll=l;
	while(ll){
		++features;
		ll=ll->next;
	}
	while(l){
		int loop=0;
		Feature* current=l->feature;
		Feature* parent=current->parent;
		while(parent){
			++loop;
			if(loop>=features){
				++(*errors);
				error_cycle(current->name);
				break;
			}
			parent=parent->parent;
		}
		l=l->next;
	}
}

static void extract_features(int* errors,FeatureList** l,Tree* root){
	FCtx ctx={errors,l,0,0,0};
	walk_tree(root,(Topdown)_extract_parent,0,&ctx);
	check_cycles(errors,*l);
	check_multiple_roots(errors,*l);
	walk_tree(root,(Topdown)_extract_attrs,0,&ctx);
	reverse_features(l);
}

typedef struct {
	int* errors;
	FeatureList* features;
	Branches** branches;
} BCtx;

static void _count_ids(Tree* t,int* count){
	if(t->type==TREE_ID) ++(*count);
}

static void _extract_rule(Tree* t,BCtx* ctx){
	int children=-1;
	switch(t->type){
	case TREE_GCAR:
		walk_tree(t,0,(Bottomup)_count_ids,&children);
		if(children<t->cardinality->end || t->cardinality->start>t->cardinality->end){
			++(*(ctx->errors));
			error_wrong_cardinality(t->cardinality,children);
		}
	case TREE_FCAR:
	case TREE_OPTREL:
	case TREE_MANREL:
	case TREE_ALTREL:
	case TREE_ORREL:
		add_branch(ctx->branches,t);
	default:
		break;
	}
}

static void extract_rules(int* errors,Branches** rules,Tree* root){
	BCtx ctx={errors,0,rules};
	walk_tree(root,0,(Bottomup)_extract_rule,&ctx);
	reverse_branches(rules);
}

static int check_attribute(Feature* f,char* attr){
	if(!f) return 0;
	if(!attr) return 1;
	return !!find_attribute(f,attr);
}

static void _extract_expr(Tree* t,BCtx* ctx){
	if(t->type!=TREE_EXPR) return;
	add_branch(ctx->branches,t->value);
}

static void _extract_calls(Tree* t,BCtx* ctx){
	if(t->type!=TREE_CALL) return;
	add_branch(ctx->branches,t);
}

static int _check_var(Tree* t,BCtx* ctx){
	Feature* f=0;
	char* attr=0;
	char* fname=0;
	FeatureList* features=ctx->features;
	switch(t->type){
	case TREE_ATTR:
	case TREE_PARAM:
		attr=t->value2 ? t->value2->id : 0;
		t=t->value1;
	case TREE_ID:
	case TREE_IDX:
		if(!t) return 1;
		Selector selector=get_selector(t,&fname);
		f=find_feature(features,fname);
		if(!f || !check_clone(f,selector) || !check_attribute(f,attr)){
			++(*(ctx->errors));
			error_expr_undefined(fname,selector.index,selector.length,attr);
		}
		free_selector(selector);
		return 1;
	default:
		return 0;
	}
}

static void check_vars(int* errors,FeatureList* features,Branches* exprs){
	BCtx ctx={errors,features,0};
	while(exprs){
		walk_tree(exprs->branch,(Topdown)_check_var,0,&ctx);
		exprs=exprs->next;
	}
}

static void extract_exprs(int* errors,Branches** exprs,FeatureList* features,Tree* root){
	BCtx ctx={errors,features,exprs};
	walk_tree(root,0,(Bottomup)_extract_expr,&ctx);
	reverse_branches(exprs);
	check_vars(errors,features,*exprs);
}

static void extract_calls(int* errors,Branches** calls,FeatureList* features,Tree* root){
	BCtx ctx={errors,features,calls};
	walk_tree(root,0,(Bottomup)_extract_calls,&ctx);
	reverse_branches(calls);
	check_vars(errors,features,*calls);
}

typedef struct{
	Params** params;
	FeatureList* features;
} PCtx;

static void _add_param(Feature* f,Selector selector,Attribute* a,int default_value,Params** params){
	Params* tmp=*params;
	outer:while(tmp){
		if(tmp->feature!=f ||
				tmp->attr!=a ||
					tmp->default_value!=default_value ||
						tmp->selector.length!=selector.length
		){
			tmp=tmp->next;
		}
		else{
			for(int i=0;i<selector.length;++i){
				if(selector.index[i]!=tmp->selector.index[i]){
					tmp=tmp->next;
					goto outer;
				}
			}
			return;
		}
	}
	
	Params* v=calloc(1,sizeof(Params));
	v->next=*params;
	*params=v;
	v->feature=f;
	v->attr=a;
	v->default_value=default_value;
	v->selector=new_selector(selector.length);
	for(int j=0;j<selector.length;++j) v->selector.index[j]=selector.index[j];
}

static int _extract_param(Tree* t,PCtx* ctx){
	if(t->type!=TREE_PARAM) return 0;
	foreach_param(ctx->features,t,(ParamCB)_add_param,ctx->params);
	return 1;
}

static void extract_params(Params** params,FeatureList* features,Tree* root){
	PCtx pctx={params,features};
	walk_tree(root,(Topdown)_extract_param,0,&pctx);
	reverse_params(params);
}

static void _add_feature_var(Feature* f,Selector selector,Emit* list){
	Name v=new_name(get_feature_name(f->name,selector));
	emit_add_list(list,new_emit_boolvar(&v));
}

static void _add_feature_kv(Feature* f,Selector selector,Emit* dict){
	Name k=new_name(get_feature_key(f->name,selector));
	Name v=new_name(get_feature_name(f->name,selector));
	emit_add_dict(dict,new_emit_var(&k),new_emit_boolvar(&v));
}

static void declare_feature_list(FeatureList* features,Emit* list){
	while(features){
		Feature* f=features->feature;
		foreach_clone(f,(CloneCB)_add_feature_var,list);
		features=features->next;
	}
}

static void declare_feature_dict(FeatureList* features,Emit* dict){
	while(features){
		Feature* f=features->feature;
		foreach_clone(f,(CloneCB)_add_feature_kv,dict);
		features=features->next;
	}
}

static void _add_attr_var(Feature* f,Selector selector,Emit* list){
	Attribute* a=f->attributes;
	Name v;
	while(a){
		v=new_name(get_attr_name(f->name,selector,a->name));
		emit_add_list(list,new_emit_intvar(&v));
		a=a->next;
	}
}

static void _add_attr_dict(Feature* f,Selector selector,Emit* dict){
	Attribute* a=f->attributes;
	Name k;
	Name v;
	if(!a) return;
	
	Name shape=new_name(get_attr_shape(f->name,selector));
	Emit* adict=new_emit_dict(&shape);
	while(a){
		k=new_shared_name(a->name);
		v=new_name(get_attr_name(f->name,selector,a->name));
		//attr-A
		emit_add_dict(adict,new_emit_var(&k),new_emit_intvar(&v));
		a=a->next;
	}
	
	k=new_name(get_feature_key(f->name,selector));
	//f-[..]
	emit_add_dict(dict,new_emit_var(&k),adict);
}

static void declare_attribute_list(FeatureList* features,Emit* list){
	while(features){
		Feature* f=features->feature;
		foreach_clone(f,(CloneCB)_add_attr_var,list);
		features=features->next;
	}
}

static void declare_attribute_dict(FeatureList* features,Emit* dict){
	while(features){
		Feature* f=features->feature;
		foreach_clone(f,(CloneCB)_add_attr_dict,dict);
		features=features->next;
	}
}

static void declare_params_list(Params* params,Emit* list){
	while(params){
		Params* h=params;
		Name v=new_name(get_param_name(h->feature->name,h->selector,h->attr->name,h->default_value));
		emit_add_list(list,new_emit_intvar(&v));
		params=h->next;
	}
}

static void declare_feature(Feature* f,Selector selector,Emit* pred){
	Name n=new_name(get_feature_name(f->name,selector));
	emit_add_body(pred,new_emit_booldecl(&n));
}

static int declare_domain(Emit* pred,Domain* domain,Name* name){
	Domain* d=domain;
	if(!domain){
		//X in [true,false]
		emit_add_body(pred,new_emit_booldecl(name));
		return 0;
	}
	
	int start;
	int end=d->end;
	while(d){
		start=d->start;
		d=d->next;
	}

	// X in [start..end]
	emit_add_body(pred,new_emit_intdecl(name,start,end));

	d=domain;
	Domain* n=d->next;
	while(n){
		//  X=<nextend \/ X>=prevstart
		emit_add_body(pred,new_emit_op(
			new_emit_op(new_emit_intvar(name),TREE_GE,new_emit_int(d->start)),
			TREE_OR,
			new_emit_op(new_emit_intvar(name),TREE_LE,new_emit_int(n->end))
		));
		d=n;
		n=n->next;
	}
	return start;
}

static void declare_attributes(Feature* f,Selector selector,Emit* pred){
	Attribute* a=f->attributes;
	Name fname=new_name(get_feature_name(f->name,selector));
	Name aname;
	Domain intbool={0,1,0};
	while(a){
		aname=new_name(get_attr_name(f->name,selector,a->name));
		// A in [min..max],..
		int min=declare_domain(pred,a->domain ? a->domain : &intbool,&aname);
		// \F => A=min
		emit_add_body(pred,new_emit_op(
			new_emit_op(0,TREE_NOT,new_emit_boolvar(&fname)),
			TREE_IF,
			new_emit_op(new_emit_intvar(&aname),TREE_EQ,new_emit_int(min))
		));
		a=a->next;
	}
}

static void declare_params(Params* params,Emit* pred){
	while(params){
		int dv=params->default_value;
		
		Name fvar=new_name(get_feature_name(params->feature->name,params->selector));
		Name avar=new_name(get_attr_name(params->feature->name,params->selector,params->attr->name));
		Name pvar=new_name(get_param_name(params->feature->name,params->selector,params->attr->name,dv));

		// V in [0..max]
		Domain* d=params->attr->domain;
		int start=0;
		int end=d ? d->end : 1;
		while(d){
			start=d->start;
			d=d->next;
		}
		emit_add_body(pred,new_emit_intdecl(&pvar,start<dv ? start : dv,end>dv ? end : dv));
		
		// (F /\ V=A) \/ (\F /\ V=default)
		emit_add_body(pred,new_emit_op(
			new_emit_op(
				new_emit_boolvar(&fvar),
				TREE_AND,
				new_emit_op(
					new_emit_intvar(&pvar),
					TREE_EQ,
					new_emit_intvar(&avar)
				)
			),
			TREE_OR,
			new_emit_op(
				new_emit_op(0,TREE_NOT,new_emit_boolvar(&fvar)),
				TREE_AND,
				new_emit_op(
					new_emit_intvar(&pvar),
					TREE_EQ,
					new_emit_int(dv)
				)
			)
		));
		params=params->next;
	}
}

typedef struct{
	Emit* pred;
	Feature* child;
	TreeType op;
} SCtx;

static void convert_simple_rel(Feature* parent,Selector selector,SCtx* ctx){
	Name cn=new_name(get_feature_name(ctx->child->name,selector));
	Name pn=new_name(get_feature_name(parent->name,selector));
	emit_add_body(ctx->pred,new_emit_op(
		new_emit_boolvar(&cn),
		ctx->op,
		new_emit_boolvar(&pn)
	));
}

typedef struct{
	Emit* pred;
	Tree* children;
	int min;
	int max;
} GCtx;

static void convert_group_rel(Feature* parent,Selector selector,GCtx* ctx){
	Emit* crd=0;
	Name tmp;
	char cstr[32]={0};
	
	Tree* c=ctx->children;
	int count=0;
	while(c){
		++count;
		c=c->next;
	}
	if(ctx->max<0) ctx->max=count;
	
	Name pname=new_name(get_feature_name(parent->name,selector));
	
	if(ctx->min==1 && ctx->max==1){
		//fd_cardinality([..],F)
		crd=new_emit_cardn(&pname);
		emit_add_body(ctx->pred,crd);
	}
	else{
		sprintf(cstr,"%p",ctx->children);
		tmp=new_name(get_name("G",cstr,selector,0,-1));
		
		//G in [0..end]
		emit_add_body(ctx->pred,new_emit_intdecl(&tmp,0,ctx->max));
		//fd_cardinality([..],G)
		crd=new_emit_cardn(&tmp);
		emit_add_body(ctx->pred,crd);

		// (F /\ G>=min) \/ (\F /\ G=0) 
		emit_add_body(ctx->pred,new_emit_op(
			new_emit_op(
				new_emit_boolvar(&pname),
				TREE_AND,
				new_emit_op(
					new_emit_intvar(&tmp),
					TREE_GE,
					new_emit_int(ctx->min)
				)
			),
			TREE_OR,
			new_emit_op(
				new_emit_op(0,TREE_NOT,new_emit_boolvar(&pname)),
				TREE_AND,
				new_emit_op(
					new_emit_intvar(&tmp),
					TREE_EQ,
					new_emit_int(0)
				)
			)
		));
	}
	
	c=ctx->children;
	while(c){
		tmp=new_name(get_feature_name(c->id,selector));
		emit_add_var(crd,new_emit_boolvar(&tmp));
		c=c->next;
	}
}

typedef struct{
	Emit* pred;
	Feature* child;
	Domain* cardinality;
} CCtx;

static void convert_clone_rel(Feature* parent,Selector selector,CCtx* ctx){
	int i;
	Name pname=new_name(get_feature_name(parent->name,selector));
	int max=ctx->child->cardinality;
	Name* clones=calloc(max,sizeof(Name));
	for(i=0;i<max;++i){
		clones[i]=new_name(get_clone_name(ctx->child->name,selector,i));
	}
	
	Domain* d=ctx->cardinality;
	int min=0;
	while(d){
		int start=d->start;
		min=start;
		for(i=d->end-1;i>(start-2);--i){
			if(i<0) break;
			if(i+1>=max) continue;
			// F_i+1 => F_i
			emit_add_body(ctx->pred,new_emit_op(
				new_emit_boolvar(clones+i+1),
				TREE_IF,
				new_emit_boolvar(clones+i)
			));
		}
		Domain* n=d->next;
		int end=n ? n->end : 0;
		for(i=start-2;i>(end-1);--i){
			if(i+1>=max) continue;
			// F_i+1 <=> F_i
			emit_add_body(ctx->pred,new_emit_op(
				new_emit_boolvar(clones+i+1),
				TREE_IFF,
				new_emit_boolvar(clones+i)
			));
		}
		d=n;
	}
	if(min){
		// P <=> F_min
		emit_add_body(ctx->pred,new_emit_op(
			new_emit_boolvar(&pname),
			TREE_IFF,
			new_emit_boolvar(clones+min-1)
		));
	}
	else if(max){
		//F_0 => P
		emit_add_body(ctx->pred,new_emit_op(
			new_emit_boolvar(clones+0),
			TREE_IF,
			new_emit_boolvar(&pname)
		));
	}
	else{
		// P <=> 0
		emit_add_body(ctx->pred,new_emit_op(
			new_emit_boolvar(&pname),
			TREE_IFF,
			new_emit_bool(0)
		));
	}
	free(clones);
}

static int _add_implicits(Tree* t,Emit** result){
	char* fname=0;

	if(t->type!=TREE_ATTR) return 0;

	Selector selector=get_selector(t->value1,&fname);
	Name name=new_name(get_feature_name(fname,selector));
	free_selector(selector);
	
	// .. /\ F
	*result=new_emit_op(*result,TREE_AND,new_emit_boolvar(&name));
	return 1;
}

static Emit* convert_var(Tree* t){
	char* fname=0;
	char* attr=0;
	char* var;
	Name n;
	Selector selector;
	switch(t->type){
	case TREE_UID:
		n=new_shared_name(t->id);
		return new_emit_var(&n);
	case TREE_ATTR:
		attr=t->value2->id;
		t=t->value1;
	case TREE_IDX:
	case TREE_ID:
		selector=get_selector(t,&fname);
		var=attr ? get_attr_name(fname,selector,attr) : get_feature_name(fname,selector);
		free_selector(selector);
		n=new_name(var);
		return attr ? new_emit_intvar(&n) : new_emit_boolvar(&n);
	default:
		return 0;
	}
}

static Emit* convert_expr(Tree* t,FeatureList* features);

static Emit* convert_list(Tree* t,FeatureList* features){
	Emit* list=new_emit_list();
	Tree* item=t->head;
	while(item){
		emit_add_list(list,convert_expr(item,features));
		item=item->next;
	}
	return list;
}

static Emit* convert_call(Tree* t,FeatureList* features){
	Name name=new_shared_name(t->value1->id);
	Emit* r=new_emit_pred(&name);
	Tree* p=t->value2->head;
	while(p){
		emit_add_param(r,convert_expr(p,features));
		p=p->next;
	}
	return r;
}

static void _add_param_list(Feature* f,Selector selector,Attribute* a,int default_value,Emit* list){
	Name pvar=new_name(get_param_name(f->name,selector,a->name,default_value));
	emit_add_list(list,new_emit_intvar(&pvar));
}

static Emit* convert_param_list(Tree* t,FeatureList* features){
	Emit* list=new_emit_list();
	foreach_param(features,t,(ParamCB)_add_param_list,list);
	return list;
}

static Emit* convert_param(Tree* t,FeatureList* features){
	Tree* fbranch=t->value1;
	Tree* abranch=t->value2;
	char* fname=0;
	if(fbranch && abranch){
		Selector selector=get_selector(fbranch,&fname);
		Name pvar=new_name(get_param_name(fname,selector,abranch->id,t->default_value));
		return new_emit_intvar(&pvar);
	}
	
	return convert_param_list(t,features);
}

static Emit* convert_expr(Tree* t,FeatureList* features){
	Emit* r;
	switch(t->type){
	case TREE_INT:
		return new_emit_int(t->integer);
	case TREE_BOOL:
		return new_emit_bool(t->integer);
	case TREE_UID:
	case TREE_ATTR:
	case TREE_IDX:
	case TREE_ID:
		return convert_var(t);
	case TREE_LIST:
		return convert_list(t,features);
	case TREE_PARAM:
		return convert_param(t,features);
	case TREE_CALL:
		return convert_call(t,features);
	case TREE_IMPF:
		r=convert_expr(t->value1,features);
		walk_tree(t->value1,(Topdown)_add_implicits,0,&r);
		return r;
	default:
		r=new_emit_op(0,t->type,0);
		if(t->value1){
			r->left=convert_expr(t->value1,features);
		}
		if(t->value2){
			r->right=convert_expr(t->value2,features);
		}
		return r;
	}
}

static Emit* convert_model(FeatureList* features,Branches* rules,Branches* exprs){
	FeatureList* tfl;
	Branches* tmp;
	
	Name pname=new_shared_name("product_model");
	Emit* result=new_emit_pred(&pname);
	
	Feature* root=features->feature;
	while(root->parent) root=root->parent;
	Name rname=new_name(get_feature_name(root->name,new_selector(0)));
	emit_add_param(result,new_emit_boolvar(&rname));
	
	Emit* fparam=new_emit_list();
	Emit* aparam=new_emit_list();
	
	declare_feature_list(features,fparam);
	declare_attribute_list(features,aparam);
	
	emit_add_param(result,fparam);
	emit_add_param(result,aparam);
	
	tfl=features;
	while(tfl){
		Feature* f=tfl->feature;
		foreach_clone(f,(CloneCB)declare_feature,result);
		tfl=tfl->next;
	}
	
	tfl=features;
	while(tfl){
		Feature* f=tfl->feature;
		foreach_clone(f,(CloneCB)declare_attributes,result);
		tfl=tfl->next;
	}

	tmp=rules;
	while(tmp){
		Tree* rule=tmp->branch;
		Feature* parent=find_feature(features,rule->value1->id);
		SCtx sctx={result,0,0};
		GCtx gctx={result,rule->value2->head,0,0};
		CCtx cctx={result,0,rule->cardinality};
		switch(rule->type){
		case TREE_OPTREL:
			sctx.op=TREE_IF;
			sctx.child=find_feature(features,rule->value2->id);
			foreach_clone(parent,(CloneCB)convert_simple_rel,&sctx);
			break;
		case TREE_MANREL:
			sctx.op=TREE_IFF;
			sctx.child=find_feature(features,rule->value2->id);
			foreach_clone(parent,(CloneCB)convert_simple_rel,&sctx);
			break;
		case TREE_ORREL:
			gctx.min=1;
			gctx.max=-1;
			foreach_clone(parent,(CloneCB)convert_group_rel,&gctx);
			break;
		case TREE_ALTREL:
			gctx.min=1;
			gctx.max=1;
			foreach_clone(parent,(CloneCB)convert_group_rel,&gctx);
			break;
		case TREE_GCAR:
			gctx.min=rule->cardinality->start;
			gctx.max=rule->cardinality->end;
			foreach_clone(parent,(CloneCB)convert_group_rel,&gctx);
			break;
		case TREE_FCAR:
			cctx.child=find_feature(features,rule->value2->id);
			foreach_clone(parent,(CloneCB)convert_clone_rel,&cctx);
			break;
		default:
			break;
		}
		tmp=tmp->next;
	}

	tmp=exprs;
	while(tmp){
		emit_add_body(result,convert_expr(tmp->branch,features));
		tmp=tmp->next;
	}

	return result;
}

static Emit* convert_call_params(FeatureList* features,Params* params,Branches* calls){
	Branches* tmp;
	
	Name pname=new_shared_name("product_hook");
	
	Emit* result=new_emit_pred(&pname);
	Emit* fparam=new_emit_list();
	Emit* aparam=new_emit_list();
	Emit* vparam=new_emit_list();
	
	declare_feature_list(features,fparam);
	declare_attribute_list(features,aparam);
	declare_params_list(params,vparam);
	
	emit_add_param(result,fparam);
	emit_add_param(result,aparam);
	emit_add_param(result,vparam);

	declare_params(params,result);
	
	tmp=calls;
	while(tmp){
		emit_add_body(result,convert_expr(tmp->branch,features));
		tmp=tmp->next;
	}
	
	return result;
}

static Emit* convert_descriptor(FeatureList* features){
	Name pname=new_shared_name("product_descriptor");
	
	Emit* result=new_emit_pred(&pname);
	Emit* param;
	
	param=new_emit_list();
	declare_feature_list(features,param);
	emit_add_param(result,param);
	
	param=new_emit_list();
	declare_attribute_list(features,param);
	emit_add_param(result,param);
	
	Name shape=new_shared_name("Features");
	param=new_emit_dict(&shape);
	declare_feature_dict(features,param);
	emit_add_param(result,param);
	
	shape=new_shared_name("Attributes");
	param=new_emit_dict(&shape);
	declare_attribute_dict(features,param);
	emit_add_param(result,param);
	
	Feature* root=features->feature;
	while(root->parent) root=root->parent;
	Name rname=new_name(get_feature_name(root->name,new_selector(0)));
	emit_add_body(result,new_emit_op(new_emit_boolvar(&rname),TREE_CAST,0));

	return result;
}

Emit* convert(State* s){
	FeatureList* features=0;
	Params* params=0;
	Branches* rules=0;
	Branches* exprs=0;
	Branches* calls=0;
	Emit* result=0;
	
	extract_features(&(s->errors),&features,s->root);
	extract_rules(&(s->errors),&rules,s->root);
	extract_exprs(&(s->errors),&exprs,features,s->root);
	extract_calls(&(s->errors),&calls,features,s->root);
	
	if(!s->errors){
		extract_params(&params,features,s->root);
		
		result=convert_model(features,rules,exprs);
		result->next=convert_call_params(features,params,calls);
		result->next->next=convert_descriptor(features);
	}
	
	free_branches(exprs);
	free_branches(calls);
	free_branches(rules);
	free_params(params);
	free_feature_list(features);
	return result;
}
