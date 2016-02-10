#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include "cache.h"
#include "conflictCache.h"
#include "lookAheadCache.h"
#include "dynamicCache.h"
using namespace std;
#define linesize 64
#define N 2000000
#define testing false	//change testing static here, DO NOT change testing static in any of the class implementations

unsigned long* addresses;
bool* load;

unsigned long* createTest(){
	unsigned long* retVal = new unsigned long[ N ];

	retVal[0] = 0xfffffffffffff000;
	retVal[1] = 0xfffffffffffff100;	//0xcceeffaacceeffe9;
	retVal[2] = 0xfffffffffffff200;
	retVal[3] = 0xfffffffffffff300;
	retVal[4] = 0xfffffffffffff400;
	retVal[5] = 0xfffffffffffff500;

	for(int i = 6; i < N; i++){
		retVal[i] = retVal[i - 6];
	}

	return retVal;
}

void initialize(){		//initializes the state of global variables
	addresses = new unsigned long[ N ];
	load = new bool[ N ];
	for (int i = 0; i < N; i++){
		addresses[i] = 0;
		load[i] = false;
	}
}

void parseFile(FILE* input){
	//textLines are 20 characters long
	//fileLine[0] = L/S
	//fileLine[1] = ' '
	//fileLine[2] = '0'
	//fileLine[3] = 'x'
	//fileLine[4] = 64-bit (16 character) address
	initialize();
	int i = 0;
	char line[24];
	for(int j = 0; j < 24; j++){
		line[j] = 0; 
	}
	while(!feof(input) && (i < N)){
		fgets(line, 24, input);	//read up to 24 characters from file into buffer, we only need 20, but
					//we want to ensure that we advance the file pointer to the next line
		if (line[0] == 'L'){
			load[i] = true;
		}else{
			load[i] = false;
		}		
		for(int j = 0; j < 20; j++){
			line[j] = line[j + 4];
		}
		//cout << line << endl;
		addresses[i] = strtol(line, NULL, 16);
		
		i++;
	}	//file parsed successfully at this point
}

void print(){
	for(int i = 0; i < N; i++){
		printf("%lx\t", addresses[i]);
		if(load[i]){
			cout << "TRUE" << endl;
		}else{
			cout << "FALSE" << endl;
		}
	}	
}

/*int percent(int i, int j){
	double temp = (((double) i ) / j) * 100;
	temp += .5;
	i = temp;
	return i;
}*/

int main(int argc, char* argv[]){
  	FILE* input = fopen(argv[1], "r");

	parseFile(input); //calls initialize to create load and address arrays
	
	unsigned long* ptr = createTest();

	//print();
	//cache(int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM)

	int outPut[16] = { 0, 0, 0, 0,
			   0, 0, 0, 0,
			   0, 0, 0, 0,
			   0, 0, 0, 0 };
	int outIndTracker = 0;
	cache* firstDmc;

	for(int i = 12; i < 17; i++){	//Direct Mapped data pull
		firstDmc = new cache(6, 0, i, addresses, load, N, true, true);
		int alpha = 1 << i;
		int ret = firstDmc->cacheMain();
		outPut[outIndTracker] = ret;
		if(testing){	printf("DM Hit Ratio (%d):\t%d\n", alpha, outPut[outIndTracker]);	}
		outIndTracker++;
		delete firstDmc;
	}

	for(int i = 1; i < 5; i++){	//Set Associative data pull
		firstDmc = new cache(6, i, 16, addresses, load, N, true, true);
		int alpha = 1 << i;
		int ret = firstDmc->cacheMain();
		outPut[outIndTracker] = ret;
		if(testing){	printf("SA Hit Ratio (%d):\t%d\n", alpha, outPut[outIndTracker]);	}
		outIndTracker++;
		delete firstDmc;
	}

	//Fully Associative data pull with LRU
	firstDmc = new cache(6, 10, 16, addresses, load, N, true, true);
	int ret = firstDmc->cacheMain();
	outPut[outIndTracker] = ret;
	if(testing){	printf("FA-LRU Hit Ratio:\t%d\n", outPut[outIndTracker]);	}
	outIndTracker++;
	delete firstDmc;

	//Fully Associative data pull with Random
	firstDmc = new cache(6, 10, 16, addresses, load, N, true, false);
	ret = firstDmc->cacheMain();
	outPut[outIndTracker] = ret;
	if(testing){	printf("FA-RAND Hit Ratio:\t%d\n", ret);	}
	outIndTracker++;
	delete firstDmc;

	for(int i = 1; i < 5; i++){	//Set Associative (NO WRITE THROUGH) data pull
		firstDmc = new cache(6, i, 16, addresses, load, N, false, true);
		int alpha = 1 << i;
		int ret = firstDmc->cacheMain();
		outPut[outIndTracker] = ret;
		if (testing){	printf("NW Hit Ratio (%d):\t%d\n", alpha, outPut[outIndTracker]);	}
		outIndTracker++;
		delete firstDmc;
	}

	if(testing){	//when testing print in shell
		for(int i = 0; i < 15; i++){
			switch(i){		
				case 5:
				case 9:
				case 10:
				case 11:
				case 15:
					cout << endl;
				default:
					cout << outPut[i] << " ";
				break;
			}
		}
		cout << endl;
	}else if(argc >= 3){		//when not testing print in output file
		FILE* output = fopen(argv[2], "w");
		if(output){
			for(int i = 0; i < 15; i++){
			switch(i){		
				case 5:
				case 9:
				case 10:
				case 11:
				case 15:
					fprintf(output, "\n");
				default:
					fprintf(output, "%d ", outPut[i]);
				break;
			}
		}
		fprintf(output, "\n");
		}
		fclose(output);
	}

	if(testing){
		//USE THIS FOR CONFLICTCACHE TESTING	
		cout << "\nTest for conflict caching implementation (Performed on Direct mapped caches of the same size as part I):\n\n";
		for(int i = 12; i < 17; i++){
			conflictCache* bc = new conflictCache(6, 0, i, addresses, load, N, true, false);
			cout << "\tconflictCache performance for cache size " << (1<<i) <<" bytes):\t" << bc->cacheMain() << endl;	//
			delete bc;
		}

		//USE THIS FOR LOOKAHEAD TESTING
		cout << "\nTest of lookahead caching implementation (Performed on Set Associative Caches of the same size as part II):\n";
		for(int i = 1; i < 5; i++){
			lookAheadCache* bc = new lookAheadCache(6, i, 16, addresses, load, N, true, true);
			cout << "\tlookAheadCache performance for (" << (1<<i) <<" ways):\t" << bc->cacheMain();	//
			delete bc;
		}
		cout << endl;
		//USE THIS FOR DYNAMIC PREDICTION TESTING		
		cout << "\nTest of Dynamic caching implementation (Performed on Set Associative Caches of the same size as part II):\n"; 
		for(int i = 1; i < 5; i++){
			dynamicCache* bc = new dynamicCache(6, i, 16, addresses, load, N, true);
			cout << "dynamicCache performance for (" << (1<<i) <<" ways):\t" << bc->cacheMain() << endl;	//
			delete bc;
		}
		cout << endl;
	}
	fclose(input);
	delete[] addresses;
	delete[] load;
	delete[] ptr;
}
