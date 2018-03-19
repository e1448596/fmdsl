package product;

import org.chocosolver.solver.Model;
import org.chocosolver.solver.variables.IntVar;
import org.chocosolver.solver.constraints.Constraint;
import org.chocosolver.solver.constraints.extension.Tuples;

public class Bike extends Model{
	
	private Tuples frame=new Tuples(new int[][]{
		{0,0,0,0},
		{1,0,7950,100},
		{1,1,7999,150},
		{2,0,6000,500},
		{2,1,6050,600},
		{3,0,4000,950},
		{3,1,4050,1000}
	},true);
	
	private Tuples brake=new Tuples(new int[][]{
		{0,0,0},
		{1,25,50},
		{2,100,200}
	},true);
	
	private Tuples rim=new Tuples(new int[][]{
		{0,0,0,0,0},
		{2,16,36,150,50},
		{2,26,36,300,30},
		{2,26,32,300,40},
		{3,26,36,100,300},
		{3,26,32,100,300}
	},true);
	
	private Tuples hub=new Tuples(new int[][]{
		{0,0,0,0,0,0,0,0},
		{1,5,36,0,0,0,50,30},
		{1,5,32,0,0,0,50,35},
		{1,5,36,1,0,0,55,40},
		{1,5,32,1,0,0,55,45},
		{2,7,36,0,0,0,500,250},
		{2,7,36,1,0,0,500,250},
		{3,5,36,0,0,7,100,100},
		{3,5,32,0,0,7,100,110},
		{3,5,36,1,0,7,110,110},
		{3,5,32,1,0,7,110,120},
		{3,5,36,0,0,10,110,120},
		{3,5,32,0,0,10,110,130},
		{3,5,36,1,0,10,120,140},
		{3,5,32,1,0,10,120,150},
		{4,10,36,0,3,1,800,280},
		{4,10,36,0,7,1,1000,300},
		{4,10,36,0,3,2,900,290}
	},true);
	
	private Tuples tire=new Tuples(new int[][]{
		{0,0,0},
		{26,150,20},
		{16,80,40}
	},true);
	
	private Tuples solidtire=new Tuples(new int[][]{
		{0,0,0},
		{26,300,300},
		{16,150,280}
	},true);
	
	private Tuples chain=new Tuples(new int[][]{
		{0,0},
		{150,100}
	},true);
	
	private Tuples belt=new Tuples(new int[][]{
		{0,0},
		{50,400}
	},true);
	
	private Tuples shifter=new Tuples(new int[][]{
		{0,0},
		{2,30},
		{3,40},
		{7,70},
		{10,100}
	},true);
	
	private Tuples seatpost=new Tuples(new int[][]{
		{0,0,0},
		{1,300,50},
		{2,200,100},
		{3,100,150}
	},true);
	
	private Tuples saddle=new Tuples(new int[][]{
		{0,0},
		{1000,100},
		{500,300}
	},true);
	
	private Tuples sprockets=new Tuples(new int[][]{
		{0,0},
		{1,50},
		{2,60},
		{7,150},
		{10,200}
	},true);

	private Constraint inventory(Tuples data,IntVar[] row){
		return table(row,data);
	}

/**EMIT**/
}