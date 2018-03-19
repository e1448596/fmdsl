
	protected org.chocosolver.solver.constraints.Constraint sum_eq(org.chocosolver.solver.variables.IntVar s,org.chocosolver.solver.variables.IntVar[] vars){
		return sum(vars,"=",s);
	}

	protected org.chocosolver.solver.constraints.Constraint all_different(org.chocosolver.solver.variables.IntVar[] vars){
		return allDifferent(vars);
	}
	
	protected org.chocosolver.solver.constraints.Constraint all_different_0(org.chocosolver.solver.variables.IntVar[] vars){
		return allDifferentExcept0(vars);
	}
	
	protected org.chocosolver.solver.constraints.Constraint min_eq(org.chocosolver.solver.variables.IntVar m,org.chocosolver.solver.variables.IntVar[] vars){
		return min(m,vars);
	}
	
	protected org.chocosolver.solver.constraints.Constraint max_eq(org.chocosolver.solver.variables.IntVar m,org.chocosolver.solver.variables.IntVar[] vars){
		return max(m,vars);
	}
	
	public class Analysis{
		private org.chocosolver.solver.constraints.Constraint[] filters;
		private org.chocosolver.solver.constraints.Constraint[] extra=new org.chocosolver.solver.constraints.Constraint[0];
		private org.chocosolver.solver.Solver solver=getSolver();
		private java.util.Map<String,org.chocosolver.solver.variables.IntVar> featuremap=new java.util.LinkedHashMap<String,org.chocosolver.solver.variables.IntVar>();
		
		{
			for(java.lang.reflect.Field f:Features.class.getFields()){
				try{
					featuremap.put(f.getName(),(org.chocosolver.solver.variables.IntVar)f.get(features));
				}
				catch(IllegalAccessException e){}
			}
		}

		public Analysis(org.chocosolver.solver.expression.discrete.relational.ReExpression... filters){
			org.chocosolver.solver.constraints.Constraint[] cfilters=new org.chocosolver.solver.constraints.Constraint[filters.length];
			for(int i=0;i<filters.length;++i){
				cfilters[i]=filters[i].decompose();
			}
			this.filters=cfilters;
		}
		
		private void begin(org.chocosolver.solver.expression.discrete.relational.ReExpression... expr){
			solver.reset();
			post(filters);
			extra=new org.chocosolver.solver.constraints.Constraint[expr.length];
			for(int i=0;i<expr.length;++i){
				extra[i]=expr[i].decompose();
			}
			post(extra);
		}
		
		private void end(){
			unpost(extra);
			unpost(filters);
		}
		
		private java.util.Map<String,org.chocosolver.solver.variables.IntVar> featuresCheck(boolean core){
			java.util.Map<String,org.chocosolver.solver.variables.IntVar> result=new java.util.LinkedHashMap<String,org.chocosolver.solver.variables.IntVar>();
			for(java.util.Map.Entry<String,org.chocosolver.solver.variables.IntVar> e:featuremap.entrySet()){
				org.chocosolver.solver.variables.IntVar f=e.getValue();
				begin(f.eq(core ? 0 : 1));
				if(!solver.solve()) result.put(e.getKey(),f);
				end();
			}
			return result;
		}
		
		public java.util.Map<String,org.chocosolver.solver.variables.IntVar> featuresCore(){
			return featuresCheck(true);
		}
		
		public java.util.Map<String,org.chocosolver.solver.variables.IntVar> featuresOptional(){
			java.util.Map<String,org.chocosolver.solver.variables.IntVar> core=featuresCore();
			java.util.Map<String,org.chocosolver.solver.variables.IntVar> dead=featuresDead();
			java.util.Map<String,org.chocosolver.solver.variables.IntVar> result=new java.util.LinkedHashMap<String,org.chocosolver.solver.variables.IntVar>();
			for(java.util.Map.Entry<String,org.chocosolver.solver.variables.IntVar> e:featuremap.entrySet()){
				String k=e.getKey();
				if(!core.containsKey(k) && !dead.containsKey(k)) result.put(k,e.getValue());
			}
			return result;
		}
		
		public java.util.Map<String,org.chocosolver.solver.variables.IntVar> featuresDead(){
			return featuresCheck(false);
		}
		
		public long count(){
			valid(null);
			return solver.getSolutionCount();
		}
		
		public void valid(org.chocosolver.solver.search.loop.monitors.IMonitorSolution callback){
			if(callback!=null) solver.plugMonitor(callback);
			begin();
			while(solver.solve());
			end();
			if(callback!=null) solver.unplugMonitor(callback);
		}
		
		public boolean check(org.chocosolver.solver.expression.discrete.relational.ReExpression... expr){
			begin(expr);
			boolean result=solver.solve();
			end();
			return result;
		}
	}
