#include "cache.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include<time.h>

#define readSize arraySize	//200//2000000
#define lineSize 64
#define testing false

using namespace std;

/**
Explicit constructor for cache object.
  blockSize = bSize;        THE NUMBER OF BYTES PER CACHE LINE but pass in the power of 2
                            for example:
			        if we want 1 byte per line, we pass in 0
				if we want 64 bytes per line, pass in 6
  numWays = nWays;          ASSOCIATIVITY OF THE CACHE but pass in the power of 2
                            
  cacheSize = cSize;        TOTAL CACHE SIZE IN BYTES pow 2
  addresses = add;          POINTER TO ARRAY OF ADDRESSES FROM FILE
  load = ld;                POINTER TO ARRAY OF INSTS FROM FILE
  arraySize = arrSize;      SIZE OF ADD AND INST ARRAY
  writeThrough = WM;        true FOR 123, false FOR 4

  indexMask =               CAPTURES INDEX BITS FOR CACHE SIZE
  blockSize/cacheSize/numWays-1

  logIndex                             pow 2

  hitCount = 0              UPDATED EACH HIT, WILL DIVIDE BY readSize
 **/

cache::cache(int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM, bool lru) {//enter pow2??

	blockSize = 1 <<  bSize; //not used, would be used to grab the data we want at a given point
	logBSize = bSize;

	if(testing){	cout << "\nBlock:\t\t" << blockSize << "\tLog Block:\t" << logBSize;	}

	numWays = 1 << nWays;
	logNumWays = nWays;

	if(testing){	cout << "\nNumWays:\t" << numWays << "\tLog NumWays:\t" << logNumWays;	}

	cacheSize = 1 << cSize;
	logCacheSize = cSize;

	if(testing){	cout << "\ncacheSize:\t" << cacheSize << "\tLog Cache:\t" << logCacheSize;	}

	addresses = add;
	load = ld;
	arraySize = arrSize;
	writeThrough = WM;
	LRU = lru;	//new bool for LRU
	srand(time(NULL));

	indexMask = cacheSize / (blockSize * numWays); //NO MINUS 1 *******switched from bS/cs/nw
	logIndex = cSize - (logNumWays + logBSize);

	if(testing){	cout << "\nindexSize:\t" << indexMask << "\tLog Index:\t" << logIndex;	}

	if(testing){	cout << "\nInput Size:\t" << arraySize << endl;	}

	hitCount = 0;

	tags = new unsigned long* [indexMask];
	useCounter = new int*[indexMask];
	valid = new bool*[indexMask];
	
	for (int i = 0; i < indexMask; i++){
		tags[i] = new unsigned long[numWays];
		useCounter[i] = new int[numWays];
		valid[i] = new bool[numWays];
		
		for (int j = 0; j < numWays; j++){
			tags[i][j]=0;
			useCounter[i][j] = 0;
			valid[i][j] = false;
		}
	}
}

/**
Deconstructor for cache object.

Must free tags and usCounter
 **/

cache::~cache() {
	for (int i = 0; i < indexMask; i++){
		delete[] tags[i];
		delete[] useCounter[i];
		delete[] valid[i];
	}
	delete[] tags;
	delete[] useCounter;
	delete[] valid;
}

/**
Returns true if searched line yields a hit into cache.

index      INDEX OF add, ld TO CHECK FOR HIT
 **/

bool cache::cacheHit(int index) {
	if(writeThrough || load[index]){
		return  cacheHit123(index);
	}else{
		return cacheHit4(index);
	}
}

/**
Returns true if searched line hits into cache as per perameters
of questions 123.

Invoked by cacheHit.

index      INDEX OF add, ld TO CHECK FOR HIT
 **/
void cache::printCache(){
	cout << endl;
	for(int i = 0; i < indexMask; i++){
		for(int j = 0; j < numWays; j++){
			printf("\t%lx\t\t\t", tags[i][j]);
		}
		cout << endl;
	}
}

bool cache::cacheHit123(int index){
	unsigned long t = addresses[index] >> (logIndex + logBSize /*+ 2*/); //shift right by log of index + bs
	
	int n = lineSize - (logBSize + logIndex);
	int ind = (addresses[index] << n) >> (n + logBSize);

	unsigned long blockBits = (addresses[index] << (64 - logBSize)) >> (64 - logBSize);

	if(testing){	printf("\nAddress:\t%lx\t\tTag:\t%lx\t\tIndex:\t%lx\t\tBlock Offset:\t%lx", addresses[index], t, ind, blockBits);	}

	bool hit = false;	
	int k = 0;

	for(int i = 0; i < numWays; i++){
		useCounter[ind][i]++;
		if ((tags[ind][i] == t) && valid[ind][k]){
			hit = true;
			useCounter[ind][i] = 0;
		}else if(useCounter[ind][i] > useCounter[ind][k]){
			k = i;
		}
	}
	if(!LRU){	//if not using LRU, we use random
		if(testing){	printf("\tRANDOM");	}
		k = rand() % numWays;	//just change k-index to random number
	}
	if(!hit){
		tags[ind][k] = t;
		useCounter[ind][k] = 0;
		valid[ind][k] = true;
	}
	if(testing){
		if(hit){
			printf("\tHITS (CH123)\n");
		}else{
			printf("\tMISSES (CH123)\n");
		}
		printCache();
		cout << endl;
	}
	return hit;
}

/**
Returns true if searched line hits into cache as per peramaters
of question 4.

The difference between this and the first cacheHit is that unless the
instruction bit is a store, nothing new gets written into the cache.
We pretend it gets written directly into memory.

Invoked by cacheHit.

index      INDEX OF add, ld TO CHECK FOR HIT
**/

bool cache::cacheHit4(int index){
	unsigned long t = addresses[index] >> (logIndex + logBSize /*+ 2*/); //shift right by log of index + bs
	
	int n = lineSize - (logBSize + logIndex);
	int ind = (addresses[index] << n) >> (n + logBSize);

	unsigned long blockBits = (addresses[index] << (64 - logBSize)) >> (64 - logBSize);

	if(testing){	printf("\nAddress:\t%lx\t\tTag:\t%lx\t\tIndex:\t%lx\t\tBlock Offset:\t%lx", addresses[index], t, ind, blockBits);	}

	bool hit = false;	
	int k = 0;

	for(int i = 0; i < numWays; i++){
		useCounter[ind][i]++;
		if ((tags[ind][i] == t) && valid[ind][k]){
			hit = true;
			useCounter[ind][i] = 0;
		}else if(useCounter[ind][i] > useCounter[ind][k]){
			k = i;
		}
	}
	if(testing){
		if(hit){
			printf("\tHITS (CH4)\n");
		}else{
			printf("\tMISSES (CH4)\n");
		}
		printCache();
		cout << endl;
	}
	return hit;
}


/**
Driving method.
Evoked for each cache.
 **/
int cache::cacheMain(){
	int index = 0;
	for (int i = 0; i < readSize; i++) {    
		if (cacheHit(i)) {
			hitCount++;
		}
	}

	double hitRatio = (hitCount * 100.0) / readSize;
	return (int)(hitRatio + .5);
}
