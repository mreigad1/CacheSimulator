#include<iostream>
using namespace std;
class conflictCache{
	public:
		int cacheSize;
		int logCacheSize;

		int blockSize;
		int logBSize;

		int numWays;
		int logNumWays;

		unsigned long* addresses;
		bool* load;
		int arraySize;
		bool writeThrough;
		bool LRU;

		int indexMask;
		int logIndex;

		int hitCount;
		

		//Data for primary cache
		unsigned long** tags;
		int** useCounter;
		bool** valid;

		//Data for conflict cache
		unsigned long* conflictTags;
		int* conflictUseCounter;
		bool* conflictValid;		

		//DEFAULT EMPTY CONSTRUCTOR (WE WON'T REALLY NEED THIS)		
		conflictCache( );

		//VALUE-SPECIFIED CONSTRUCTOR
		//bSize		<-	THE NUMBER OF BYTES PER CACHE LINE
		//nWays		<-	THE ASSOCIATIVITY FOR THE CACHE
		//cSize		<-	THE TOTAL SIZE OF THE CACHE IN BYTES
		//add		<-	A POINTER TO THE ARRAY OF ADDRESSES PARSED INTO MEMORY
		//ld		<-	A POINTER TO THE ARRAY OF INSTRUCTION TYPES PARSED INTO MEMORY
		//arrSize	<-	THE SIZE OF BOTH THE ADD & LD ARRAY
		conflictCache( int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM, bool lru );
		~conflictCache();

		bool cacheHit( int index);
		bool conflictCacheHit( int index );
		void insertConflictCache( unsigned long tag, unsigned long index );
	
		//THE MAIN DRIVING CODE OF THE CACHES, THIS WILL BE INVOKED IN PROGRAM MAIN ON EACH CACHE
		int cacheMain( );
		void printCache( );
};
