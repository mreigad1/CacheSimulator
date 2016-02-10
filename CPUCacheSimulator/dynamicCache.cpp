#include "dynamicCache.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#define readSize 2000000
#define lineSize 64
#define testing 0
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
dynamicCache::dynamicCache(int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM) {//enter pow2??
  //blocksize = 2^bSize
  blockSize = 1 <<  bSize; //not used, would be used to grab the data we want at a given point
  logBSize = bSize;

  numWays = 1 << nWays;
  logNumWays = nWays;

  cacheSize = 1 << cSize;
  logCacheSize = cSize;

  addresses = add;
  load = ld;
  arraySize = arrSize;
  writeThrough = WM;

  indexMask = cacheSize/(blockSize*numWays); 
  logIndex = cSize - (logNumWays + logBSize);

  hitCount = 0;
	
  //***********************************************************************vvvvvvvvdifferent from regular implementation. Used for Extra Credit 3.2
  
  PSL = 512;
  LRUSig = 512;
  group1Last = (cacheSize/lineSize/numWays) / 16;
  group1LastM = group1Last/2;
  group2Last = (cacheSize/lineSize/numWays)/8;
  group2LastM = group2Last - group1Last/2;
//**************************************************************************^^^^^^^^^^
  /**
     Create array of pointers. Pointer num = set number.
     Each of these pointers points to an array of integers of length
     nWays, representing the number of ways found at this set. Index 0
     corresponds to way zero and so on. Each entry in this array is
     an address held as an integer. So, tags[2][3] accesses the fourth
     way of the third set.

     Also create an identical array that holds use counters as opposed
     to address information. Each time the address in way 0 is accessed,
     the counter at index useCounter[i][0] is incremented.
   **/
  tags = new int* [indexMask];
  useCounter = new int*[indexMask];
  valid = new bool*[indexMask];
  for (int i = 0; i < indexMask; i++){

    tags[i] = new int[numWays];
    useCounter[i] = new int[numWays];
    valid[i] = new bool[numWays];
    //initialize tags and counters to 0
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
dynamicCache::~dynamicCache() {

  for (int i = 0; i < indexMask; i++){

    delete[] tags[i];
    delete[] useCounter[i];
    delete[] valid[i];
  
  }

  delete[] tags;
  delete[] useCounter;
  delete [] valid;
  }


/**
Returns true if searched line yields a hit into cache.

index      INDEX OF add, ld TO CHECK FOR HIT
 **/
bool dynamicCache::cacheHit( int index) { //possible combination

  //printf("cacheHit\n");
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
bool dynamicCache::cacheHit123(int index){

	
  int blockOffsetBitNum = logBSize;
  int blockOffsetMask = (2 << blockOffsetBitNum)-1;
	if(index > readSize || index < 0) {

	printf("ind:%lx\n", index);
}
  unsigned long noBlockOffset = addresses[index] >> blockOffsetBitNum;


  int indexBitNum = logCacheSize - (logNumWays + logBSize);
  int indexBitMask = ((2 << indexBitNum) -1 ) ^ blockOffsetMask;
  unsigned long noIndexOrOffset = addresses[index] >> (blockOffsetBitNum + indexBitNum);

  int tagBitNum = blockSize - indexBitNum - blockOffsetBitNum;
  unsigned long justTagBits = noIndexOrOffset;
  
  int tagOnly = (int) justTagBits;
  int indexOnly = (addresses[index] << tagBitNum) >> (tagBitNum + blockOffsetBitNum);
	 bool hit = false;	
	int k = 0;

	

for(int i = 0; i < numWays; i++){ //**************************************************************************vvvvvv Lots of logic changed for section 3.2

	  if (tags[indexOnly][i] == tagOnly && valid[indexOnly][k]==true){ //if tags match and tag is valid

			if (indexOnly < group1Last)  { //if index falls in first group, inc PSL. 

				if (testing) {	  
					printf("First group last index: %lx\t indexOnly %lx\n", group1Last, indexOnly);
				 }
				PSL ++;
				LRUSig ++; //increment LRUSig each time. if index is in MRU zone, will decrease by two. otherwise, LRU zone
				if (LRUSig > 2047) { 
				//printf("inc LRU and PSL\n");
					LRUSig = 2047;
				}	

				if (PSL > 2047) {
					PSL = 2047; 
			
				}


				if(indexOnly < group1LastM) { // index is in MRU zone. decrease LRUSig twice to account for extra addition
		  			LRUSig--;
		  			LRUSig--;
			  		if (LRUSig <0) {
						LRUSig = 0;
					}
				}

		} else if (indexOnly < group2Last && indexOnly > group1Last)  { //index falls in second group, dec PSL.
	  			if(testing) {
					printf("Second group last index: %lx\t indexOnly %lx\n", group2Last, indexOnly);
				}
				PSL --;
				LRUSig++; //inc LRUSig. if in MRU zone, will compensate
				if (LRUSig > 2047) { 
					LRUSig = 2047;
				}
	
				if(testing) {
					printf("Decrementing PSL to %d\n", PSL);
				}
				if (PSL <0) { 
					PSL = 0; 
					if(testing) {	
						printf("PSL reset to 0.\n"); 
					 }
				}
	  	
 
				if(indexOnly < group2LastM) { // index in MRU zone. compensating for extra addition by double subtraction
					    LRUSig--;
				            LRUSig--;
			   		 if(LRUSig <0 ) { 
						LRUSig = 0; 
					}
			  	}
 
	
		} else { //index falls into group where we pick using PSL

	  //DO nothing
  

	}               //BEGIN FRESH
			hit = true;
			valid[indexOnly][i] = true;
			useCounter[indexOnly][i] --;
			useCounter[indexOnly][i]--; //CHANGED TO subtraction. Higher counter means distant future. if we use it, more liekly to use again
			if (useCounter[indexOnly][i] < 0) { 
				useCounter[indexOnly][i] = 0; 
				if(testing){ 
					printf("Resest useCounter to seven2047.\n");
				}	
			 }		

	  
	}else if ((indexOnly < group1Last && indexOnly > group1LastM) || (indexOnly < group2Last && indexOnly > group2LastM) || (indexOnly > group2Last && LRUSig > 1024)) { // if tag was not found or was not valid, pick the replacement. IF high LRU, use LRU method
		if (useCounter[indexOnly][i] > useCounter[indexOnly][k]){ //PICK REPLACEMENT. < for == regular CHANGED

	/**
	We can abide by two replacement policies: MRU, where we account for scans by taking out the most recent addition. Favors pattern reps
						  LRU, where we assume newer additions are more important. Favor localities
	**/
				k = i;
				
		}
	  } else if(1){ //if low LRU, use MRU method of replacement
		if (useCounter[indexOnly][i] < useCounter[indexOnly][k]) { 
			k = i;
		 }
	  }


else {
	//printf("NOTHING\n");
}
	}
	
	if(!hit){
	

		double setNum2 = cacheSize/lineSize;
		if(testing) {
			printf("setNum2: %d\t cacheSize: %d\n", setNum2, cacheSize);
		}
		if (indexOnly < group1Last)  { // if index falls in first group
	
		          if(testing) {printf("First group last index: %d\t indexOnly %d\n", group1Last, indexOnly);	
			  }
	
			tags[indexOnly][k] = tagOnly;
			valid[indexOnly][k] = true;
			useCounter[indexOnly][k] = 1024; // this effectively predicts a use in the DISTANT FUTURE
					      


		} else if (indexOnly < group2Last)  { //index falls in second group
	  		if(testing) {printf("Second group last index: %lx\t indexOnly %lx\n",group2Last, indexOnly);
			}

			tags[indexOnly][k] = tagOnly;
			valid[indexOnly][k] = true;
			useCounter[indexOnly][k] = 512; // this effectively predicts a use in the not near, but not distant future
					
	


		} else { //index falls into group where we pick using PSL
			  if(testing) { 
				printf("Index is in final group. Should be bigger than group 2.\tindexOnly: %lx\t 2*sqrtOfSetNum - 1: %x\n", indexOnly, group2Last);
                                printf("PSL: %lx\n", PSL);
      			   }
		  if (PSL >= 1024) { //if most sig bit is one, choose counter initializer  
    			if(testing) {
				printf("PSL has sig 1 bit; choose to set very distant. PSL >> 4: %lx\n", PSL >> 4);
			}
			tags[indexOnly][k] = tagOnly;
			valid[indexOnly][k] = true;
			useCounter[indexOnly][k] =1024 ; // this effectively predicts a use in the DISTANT FUTURE

		  } else if(PSL < 1024) { //if most sig bit is 0, use initializer B
    			if(testing) { printf("PSL has sig 0 bit; choose to set long. PSL >> 4: %lx\n", PSL >> 4);
			}

			tags[indexOnly][k] = tagOnly;
			valid[indexOnly][k] = true;
			useCounter[indexOnly][k] = 512; // this effectively predicts a use in the not near, but not distant future
					


		} else {

		      printf("Problem in cacheHit123. PSL  >> 4 == %d\n", PSL);

		}

	}

}

	/**
	We have to increment every counter that wasn't hit. The counter that was hit was incremented by 2 to account for this.
	**/
	for (int i = 0; i < indexMask; i++){

   
	    	for (int j = 0; j < numWays; j++){

	      		useCounter[i][j]++;
			if (useCounter[i][j]> 2047) { 
				useCounter[i][j] == 2047;
			 }
  
	    }
  
    }
	
//***********************************************************************************************************************************^^^^^^^^ Nothing below this line is different from normal implementation
	
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

bool dynamicCache::cacheHit4(int index){
  int t = addresses[index] >> (logIndex + logBSize); //shift right by log of index + bs
  int n = lineSize - (logBSize + logIndex);
  int ind = (addresses[index] << n) >> (n+logBSize);//(~(~0 * indexMask)) & t; //all ones multiplied by index mask shifts left by the index mask. Inverting again, mask with addresses at index(from read array)
	bool hit = false;	
	int k = 0;

	for(int i = 0; i < numWays; i++){
		useCounter[ind][i]++;
		if (tags[ind][i] == t){
			hit = true;
			useCounter[ind][i] = 0;
		}else if(useCounter[ind][i] > useCounter[ind][k]){
			k = i;
		}
	}	//k now references the index of the oldest and we know if hit
	
	return hit;
}


/**
Driving method.
Evoked for each cache.
 **/
int dynamicCache::cacheMain(){
//printf("G1L: %d\t G1LM: %d\t G2L: %d G2LM: %d\t SetNum: %d\n", group1Last, group1LastM, group2Last, group2LastM, cacheSize/lineSize/numWays);
//printf("LRUSig: %d\tPSL: %d\n", LRUSig, PSL);
  //initialize index to zero so we don't create it a bajillion times
  int index = 0;
  //For every address in the file array
  for (int i = 0; i < readSize; i++) {
    
    /**
       Calculate the index.
       
       Number of index bits in an address = total cache size / line size/number of ways.
       This gives the number of sets in the cache, and thus the number of
       unique bits required to identify a specific set.

       Isolate the index by masking the least significant N bits, where
       N = (cache size / line size)/nWays - 1.

       EXAMPLE:
               4KB cache = 4026 bytes
	       line size = 64 bytes

	       4026/64 = 64 sets in cache 
	       64/1 way for DMC = 64 sets
	       must identify 64 unique numbers, 2^6 = 64
	       need 6 bits for mask 111111, this = 63 = 64 - 1
     **/
    
    if (cacheHit(i)) {

      //update hit ratio, rounded to the nearest whole number
      //printf("We had a hit. Incrementing.\n");
      hitCount++;
    }
  }

  //after ever line has been read and checked into the cache, return
  //the hitRatio rounded to the nearest integer
  //  printf("hc: %lx\n", hitCount);
  double hitRatio = (hitCount*100.0)/(readSize);
	//printf("LRUSig: %d\tPSL: %d\n", LRUSig, PSL);
  return (int)(hitRatio + .5);

}

