#include "conflictCache.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include<time.h>

#define readSize arraySize	//2000000
#define lineSize 64
#define testing false
#define conflictSize 8
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

conflictCache::conflictCache(int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM, bool lru) {//enter pow2??

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

	conflictTags = new unsigned long[ conflictSize ];
	conflictUseCounter = new int[ conflictSize ];
	conflictValid = new bool[ conflictSize ];

	for(int i = 0; i < conflictSize; i++){	//initializing conflict cache
		conflictTags[i] = 0;
		conflictUseCounter[i] = 0;
		conflictValid[i] = false;
	}

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

conflictCache::~conflictCache() {
	for (int i = 0; i < indexMask; i++){
		delete[] tags[i];
		delete[] useCounter[i];
		delete[] valid[i];
	}
	delete[] tags;
	delete[] useCounter;
	delete[] valid;

	delete[] conflictTags;
	delete[] conflictUseCounter;
	delete[] conflictValid;
}

/**
Returns true if searched line hits into cache as per perameters
of questions 123.

Invoked by cacheHit.

index      INDEX OF add, ld TO CHECK FOR HIT
 **/
void conflictCache::printCache(){
	cout << endl;
	for(int i = 0; i < indexMask; i++){
		for(int j = 0; j < numWays; j++){
			printf("\t%lx\t\t\t", tags[i][j]);
		}
		cout << endl;
	}
}

bool conflictCache::cacheHit(int index){
	unsigned long t = addresses[index] >> (logIndex + logBSize);
	
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
	if(!hit){	//check conflictCache for contents,
			//if there do nothing, otherwise
			//place into primary cache
			//and place evicted contents (if any)
			//into conflictCache
		if(!conflictCacheHit( index )){
			if(valid[ind][k]){	//valid bit indicates an entry is present at k
				unsigned long oldTag = tags[ind][k];	//get old tag to replace
				tags[ind][k] = t;
				useCounter[ind][k] = 0;						//placing new cache entry at that index
				valid[ind][k] = true;
				insertConflictCache( oldTag, ind );					//placing old cache data into conflictCache
			}else{			//no conflict, just insert into primary cache
				tags[ind][k] = t;
				useCounter[ind][k] = 0;
				valid[ind][k] = true;
			}
		}else{
			hit = true;
		}
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


//	1.)	get tag 
//	2.)	check whole array for tag
//	3.)	report hit miss
bool conflictCache::conflictCacheHit( int index ){
	unsigned long tag = addresses[index] >> logBSize;

	for(int i = 0; i < conflictSize; i++){
		if(conflictTags[i] == tag){	//contents in conflictCache
			conflictUseCounter[i] = 0;			
			return true;
		}else{
			conflictUseCounter[i]++;
		}
	}//having left loop item is not in conflict cache
	return false;
}

void conflictCache::insertConflictCache( unsigned long tag, unsigned long index ){
	int k = 0;
	//cout << "Tag:\t" << tag << "\tindex:\t" << index << endl;
	unsigned long oldTag = (tag << logIndex) + index;
	//cout << "Inserted:\t" << oldTag << endl;
	for(int i = 1; i < conflictSize; i++){
		if(conflictUseCounter[i] < conflictUseCounter[k]){
			k = i;
		}
	}	//k now references oldest entry in conflictCache
	if(!LRU){	//if not using LRU, we use random
		k = rand() % conflictSize;	//just change k-index to random number
	}
	conflictTags[k] = oldTag;
	conflictUseCounter[k] = 0;
	conflictValid[k] = true;
}

/**
Driving method.
Evoked for each cache.
 **/
int conflictCache::cacheMain(){
	cout << "Conflict Cache Implementation of size: " << conflictSize;
	int index = 0;
	for (int i = 0; i < readSize; i++) {    
		if (cacheHit(i)) {
			hitCount++;
		}
	}

	double hitRatio = (hitCount * 100.0) / readSize;
	return (int)(hitRatio + .5);
}
