typedef enum{
	TREE_UNKNOWN,
	TREE_LIST,
	TREE_RULES,
	TREE_OPTREL,
	TREE_MANREL,
	TREE_ALTREL,
	TREE_ORREL,
	TREE_FCAR,
	TREE_GCAR,
	TREE_NAND,
	TREE_ADD,
	TREE_SUB,
	TREE_DIV,
	TREE_MUL,
	TREE_MOD,
	TREE_EQ,
	TREE_NE,
	TREE_GT,
	TREE_LT,
	TREE_GE,
	TREE_LE,
	TREE_IF,
	TREE_IFF,
	TREE_NOT,
	TREE_AND,
	TREE_OR,
	TREE_NEG,
	TREE_CAST,
	TREE_RCAST,
	TREE_EXPR,
	TREE_ID,
	TREE_UID,
	TREE_ATTR,
	TREE_INT,
	TREE_BOOL,
	TREE_IDX,
	TREE_ADECL,
	TREE_CALL,
	TREE_PARAM,
	TREE_IMPF
} TreeType;


typedef struct _Domain{
	int start;
	int end;
	struct _Domain* next;
} Domain;

typedef struct _Tree{
	TreeType type;
	union{
		int integer; //TREE_INT TREE_BOOL
		char* id; //TREE_ID TREE_UID
		struct _Tree* value; //TREE_RCAST TREE_EXPR TREE_CAST
		struct{ //TREE_LIST TREE_RULES
			struct _Tree* head;
			struct _Tree** tail;
		};
		struct{
			struct _Tree* value1;
			struct _Tree* value2;
		};
	};
	struct _Tree* next;
	union{
		Domain* cardinality; //TREE_GCAR
		Domain* domain; //TREE_ADECL
		int default_value; //TREE_PARAM
	};
} Tree;

typedef struct{
	char* file;
	char* line_offset;
	char* offset;
	Tree* root;
	int errors;
	int line;
} State;

typedef struct _Branches{
	Tree* branch;
	struct _Branches* next;
} Branches;

Tree* new_tree(TreeType type,Tree* value);
Tree* new_op(Tree* value1,TreeType type,Tree* value2);
Tree* new_int(char* str);
Tree* new_id(char* str);
Tree* new_unchecked_id(char* str);
Tree* new_bool(int val);
Tree* to_adecl(Tree* attr,Domain* domain);
Tree* to_param(Tree* attr,char* default_value);
Tree* to_rule(Tree* cexpr);
Tree* new_list(TreeType type);
Tree* append_list(Tree* list,Tree* value);

void free_tree(Tree* t);
void free_branches(Branches* l);
void add_branch(Branches** l,Tree* branch);
void reverse_branches(Branches** l);

Domain* new_domain(char* start,char* end,Domain* next);
Domain* new_domain_single(char* value,Domain* next);
void free_domain(Domain* c);

typedef int(*Topdown)(Tree*,void*);
typedef void(*Bottomup)(Tree*,void*);

void walk_tree(Tree* t,Topdown topdown,Bottomup bottomup,void* ctx);
