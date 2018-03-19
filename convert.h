typedef struct {
	int length;
	int* index;
} Selector;

typedef struct _Attribute{
	char* name;
	Domain* domain;
	struct _Attribute* next;
} Attribute;

typedef struct _Feature{
	char* name;
	int cardinality;
	struct _Attribute* attributes;
	struct _Feature* parent;
} Feature;

typedef struct _Params{
	Feature* feature;
	Attribute* attr;
	int default_value;
	Selector selector;
	struct _Params* next;
} Params;

typedef struct _FeatureList{
	struct _Feature* feature;
	struct _FeatureList* next;
} FeatureList;

typedef void(*CloneCB)(Feature* f,Selector selector,void*);

Emit* convert(State* s);
