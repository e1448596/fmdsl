#include <iostream>
using namespace std;

/**EMIT**/

class Inventory{
	int frame_data[7][4]={
		{0,0,0,0},
		{1,0,7950,100},
		{1,1,7999,150},
		{2,0,6000,500},
		{2,1,6050,600},
		{3,0,4000,950},
		{3,1,4050,1000}
	};
	
	int brake_data[3][3]={
		{0,0,0},
		{1,25,50},
		{2,100,200}
	};
	
	int rim_data[6][5]={
		{0,0,0,0,0},
		{2,16,36,150,50},
		{2,26,36,300,30},
		{2,26,32,300,40},
		{3,26,36,100,300},
		{3,26,32,100,300}
	};
	
	int hub_data[18][8]={
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
	};

	int tire_data[3][3]={
		{0,0,0},
		{26,150,20},
		{16,80,40}
	};

	int solidtire_data[3][3]={
		{0,0,0},
		{26,300,300},
		{16,150,280}
	};

	int chain_data[2][2]={
		{0,0},
		{150,100}
	};
	
	int belt_data[2][2]={
		{0,0},
		{50,400}
	};

	int shifter_data[5][2]={
		{0,0},
		{2,30},
		{3,40},
		{7,70},
		{10,100}
	};
	
	int seatpost_data[4][3]={
		{0,0,0},
		{1,300,50},
		{2,200,100},
		{3,100,150}
	};

	int saddle_data[3][2]={
		{0,0},
		{1000,100},
		{500,300}
	};
	
	int sprockets_data[5][2]={
		{0,0},
		{1,50},
		{2,60},
		{7,150},
		{10,200}
	};
	
#define FILL(table) for(int r=0;r<sizeof(table##_data)/sizeof(table##_data[0]);++r){table.add(IntArgs(sizeof(table##_data[0])/sizeof(int),table##_data[r]));}table.finalize()
public:
	TupleSet sprockets;
	TupleSet saddle;
	TupleSet shifter;
	TupleSet seatpost;
	TupleSet belt;
	TupleSet chain;
	TupleSet solidtire;
	TupleSet tire;
	TupleSet hub;
	TupleSet rim;
	TupleSet brake;
	TupleSet frame;
	
	Inventory(){
		FILL(frame);
		FILL(brake);
		FILL(rim);
		FILL(hub);
		FILL(tire);
		FILL(solidtire);
		FILL(chain);
		FILL(belt);
		FILL(shifter);
		FILL(seatpost);
		FILL(saddle);
		FILL(sprockets);
	}
};

class Bike:public Model<Bike,Inventory>{
public:
	void inventory(TupleSet& table,IntVarArgs& row){
		extensional(*this,row,table,IPL_MEMORY);
	}
};

int main(int argc,char** argv){
	Bike bike;
	Bike::Analysis a(&bike);
	cout<<a.count()<<endl;
	return 0;
}
