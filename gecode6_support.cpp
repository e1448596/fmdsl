public:
	virtual BoolVarBranch branchFeatureVar(){return BOOL_VAR_AFC_MAX();}
	virtual BoolValBranch branchFeatureVal(){return BOOL_VAL_MAX();}
	virtual IntVarBranch branchAttrVar(){return INT_VAR_AFC_MAX();}
	virtual IntValBranch branchAttrVal(){return INT_VAL_SPLIT_MIN();}
	virtual Search::Options searchOptions(){Search::Options o;o.threads=0;return o;}
	virtual Space* copy(){return new Model(*this);}
	
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
			DFS<Model> search(m,m->searchOptions());
			Model* solution;
			while(solution=search.next()){
				++solutions;
				delete solution;
			}
			return solutions;
		}
	};

