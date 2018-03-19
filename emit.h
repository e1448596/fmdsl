typedef enum{
	EMIT_PRED,
	EMIT_INTDECL,
	EMIT_BOOLDECL,
	EMIT_CARDN,
	EMIT_OP,
	EMIT_INT,
	EMIT_BOOL,
	EMIT_VAR,
	EMIT_INTVAR,
	EMIT_BOOLVAR,
	EMIT_LIST,
	EMIT_DICT,
	EMIT_KV
} EmitType;

typedef struct{
	int shared;
	char* str;
} Name;

typedef struct _Emit{
	EmitType type;
	union{
		//EMIT_VAR EMIT_INTVAR EMIT_BOOLVAR EMIT_INTDECL EMIT_BOOLDECL EMIT_CARDN EMIT_PRED EMIT_DICT
		Name name;
		
		//EMIT_INT EMIT_BOOL
		int value;
		
		//EMIT_LIST
		int depth;
		
		//EMIT_OP
		TreeType op;
	};
	union{
		//EMIT_PRED
		struct{
			struct _Emit* params;
			struct _Emit* body;
		};
		
		//EMIT_INTDECL
		struct{
			int start;
			int end;
		};
		
		//EMIT_LIST EMIT_DICT
		struct _Emit* items;
		
		//EMIT_CARDN
		struct _Emit* vars;
		
		//EMIT_OP
		struct{
			struct _Emit* left;
			struct _Emit* right;
		};
		
		//EMIT_KV
		struct{
			struct _Emit* key;
			struct _Emit* val;
		};
	};
	struct _Emit* next;
	struct _Emit** tail;
} Emit;

void free_emit(Emit* e);
Emit* new_emit_var(Name* name);
Emit* new_emit_intvar(Name* name);
Emit* new_emit_boolvar(Name* name);
Emit* new_emit_intdecl(Name* name,int start,int end);
Emit* new_emit_booldecl(Name* name);
Emit* new_emit_pred(Name* name);
Emit* new_emit_int(int i);
Emit* new_emit_bool(int i);
Emit* new_emit_op(Emit* left,TreeType op,Emit* right);
Emit* new_emit_dict(Name* shape);
Emit* new_emit_list();
Emit* new_emit_cardn(Name* name);
void emit_add_param(Emit* pred,Emit* param);
void emit_add_body(Emit* pred,Emit* body);
void emit_add_list(Emit* list,Emit* item);
void emit_add_dict(Emit* dict,Emit* key,Emit* val);
void emit_add_var(Emit* crd,Emit* var);

Name new_name(char* name);
Name new_shared_name(char* name);
