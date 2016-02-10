#include "lookAheadCache.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define readSize arraySize	//200//2000000
#define lineSize 64
#define testing false
#define LOOKAHEAD 2
#define EXPIRED 1024

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

lookAheadCache::lookAheadCache(int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM, bool lru) {//enter pow2??

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
	numInserted = 0;
}

/**
Deconstructor for cache object.

Must free tags and usCounter
 **/

lookAheadCache::~lookAheadCache() {
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

bool lookAheadCache::cacheHit(int index) {
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
void lookAheadCache::printCache(){
	cout << endl;
	for(int i = 0; i < indexMask; i++){
		for(int j = 0; j < numWays; j++){
			printf("\t%lx\t\t\t", tags[i][j]);
		}
		cout << endl;
	}
}

bool lookAheadCache::cacheHit123(int index){

	//*******************************USE COUNTER***************************************************
	for(int i = 0; i < indexMask; i++){
		for(int j = 0; j < numWays; j++){
			useCounter[i][j]++;			//increment usecounter
			if(useCounter[i][j] >= EXPIRED){	//if expired, clear contents
				valid[i][j] = false;
				tags[i][j] = 0;
				useCounter[i][j] = 0;
			}
		}
	}

	//***********************ORIGINAL BRACKET OF HIT LOGIC********************************************
	unsigned long t = addresses[index] >> (logIndex + logBSize /*+ 2*/); //shift right by log of index + bs	
	
	int n = lineSize - (logBSize + logIndex);
	int ind = (addresses[index] << n) >> (n + logBSize);

	unsigned long blockBits = (addresses[index] << (64 - logBSize)) >> (64 - logBSize);

	if(testing){	printf("\nAddress:\t%lx\t\tTag:\t%lx\t\tIndex:\t%lx\t\tBlock Offset:\t%lx", addresses[index], t, ind, blockBits);	}

	bool hit = false;	
	int k = 0;

	for(int i = 0; i < numWays; i++){
		//useCounter[ind][i]++;	//no longer increment use counter during set accesses, increment all useCounters during every cycle
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
	//******************************END OF ALL HIT LOGIC*******************************************************


	//******************************BEGINNING OF PREDICTOR LOGIC***********************************************
	if(index < (arraySize - LOOKAHEAD)){	//stop short of end of array
		for(int j = 0; j < LOOKAHEAD; j++){
			index++;//increment index for convenience, won't effect anything since it is only a temp
			unsigned long nextTag = addresses[index] >> (logIndex + logBSize); //shift right by log of index + bs	

			int nextShift = lineSize - (logBSize + logIndex);
			int nextInd = (addresses[index] << nextShift) >> (nextShift + logBSize);

			bool insert = false;
			int inPos = 0;
			int kPos = 0;
			int use = 0;

			for(int i = 0; i < numWays; i++){
				if(!(valid[nextInd][i] || insert)){	//check if a free entry in set is available, if so predictively add next data block to cache and set its counter high;
					insert = true;
					inPos = i;
				}
				if(useCounter[nextInd][i] > use){	//get highest useCounter in set
					use = useCounter[nextInd][i];
				}
			}

			if(insert){		//insert flagged, place entry in cache
				bool notFound = true;
				for(int z = 0; z < numWays; z++){
					if(valid[nextInd][z] && (tags[nextInd][z] == nextTag)){	//check if next data access is valid and in entry
						notFound = false;
					}
				}
				if(notFound){	//next data access is not already in cache, and cache entry is available.  No reason to not insert the entry, and if we have to evict it, its useCounter will be large
					numInserted++;
					valid[nextInd][inPos] = true;
					useCounter[nextInd][inPos] = use + 1;	//setting useCounter high, so that if data instruction was mispredicted then this will be the first entry to evict
					tags[nextInd][inPos] = nextTag;
				}
			}
		}
	}
	//******************************END OF PREDICTOR LOGIC*****************************************************

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

bool lookAheadCache::cacheHit4(int index){
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
int lookAheadCache::cacheMain(){
	int index = 0;
	for (int i = 0; i < readSize; i++) {    
		if (cacheHit(i)) {
			hitCount++;
		}
	}

	printf("\nWe spared %d unnessessary cache misses.\t", numInserted);

	double hitRatio = (hitCount * 100.0) / readSize;
	return (int)(hitRatio + .5);
}
