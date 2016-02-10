#include<iostream>
using namespace std;
class dynamicCache{
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
		
		int indexMask;
		int logIndex;

		int hitCount;
	
		int PSL;
		int LRUSig;
		int group1Last;
		int group2Last;
		int group1LastM;
		int group2LastM;
		//DATA STRUCTURES HERE FOR
		int** tags;
		int** useCounter;
		bool** valid;

		//DEFAULT EMPTY CONSTRUCTOR (WE WON'T REALLY NEED THIS)		
		dynamicCache( );


		//VALUE-SPECIFIED CONSTRUCTOR
		//bSize		<-	THE NUMBER OF BYTES PER CACHE LINE
		//nWays		<-	THE ASSOCIATIVITY FOR THE CACHE
		//cSize		<-	THE TOTAL SIZE OF THE CACHE IN BYTES
		//add		<-	A POINTER TO THE ARRAY OF ADDRESSES PARSED INTO MEMORY
		//ld		<-	A POINTER TO THE ARRAY OF INSTRUCTION TYPES PARSED INTO MEMORY
		//arrSize	<-	THE SIZE OF BOTH THE ADD & LD ARRAY
		dynamicCache( int bSize, int nWays, int cSize, unsigned long* add, bool* ld, int arrSize, bool WM );
		~dynamicCache();


		//BOOL TO RETURN IF A GIVEN LINE FROM TEXT CAUSES A HIT
		//index		<-	THE INDEX OF THE DATA ELEMENTS IN ADDRESSES & LOAD WHICH WE WISH TO DETERMINE IF CACHE HIT
		//RETURN TRUE if cache hit
		//RETURN FALSE if cache miss
		bool cacheHit( int index );
		bool cacheHit123( int index);


		bool cacheHit4( int index );

		
		//THE MAIN DRIVING CODE OF THE CACHES, THIS WILL BE INVOKED IN PROGRAM MAIN ON EACH CACHE
		int cacheMain( );
};
