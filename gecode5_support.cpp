public:
	BoolVarBranch branchFeatureVar(){return BOOL_VAR_DEGREE_MAX();}
	BoolValBranch branchFeatureVal(){return BOOL_VAL_MAX();}
	IntVarBranch branchAttrVar(){return INT_VAR_SIZE_MIN();}
	IntValBranch branchAttrVal(){return INT_VAL_MIN();}
	virtual Space* copy(bool _share){return new Model(_share,*this);}
	
	void sum_eq(IntVar s,IntVarArgs& a){
		rel(*this,s==sum(a));
	}
	
	void min_eq(IntVar m,IntVarArgs& a){
		min(*this,a,m);
	}

	void max_eq(IntVar m,IntVarArgs& a){
		max(*this,a,m);
	}
	
	void all_different(IntVarArgs& a){
		distinct(*this,a);
	}
	
	void all_different_0(IntVarArgs& a){
		distinct(*this,a,0);
	}
	
	class Analysis{
	private:
		Model* m;
		Analysis(){};
	public:
		Analysis(Model* model):m(model){}
		int count(){
			int solutions=0;
			DFS<Model> search(m);
			Model* solution;
			while(solution=search.next()){
				++solutions;
				delete solution;
			}
			return solutions;
		}
	};

