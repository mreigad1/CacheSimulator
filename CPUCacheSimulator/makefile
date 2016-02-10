GXX=g++ -g

all: cache-sim

cache-sim: main.o cache.o conflictCache.o lookAheadCache.o dynamicCache.o
	$(GXX) main.o cache.o conflictCache.o lookAheadCache.o dynamicCache.o -o cache-sim

main.o: main.cpp 
	$(GXX)  -c  main.cpp

cache.o: cache.cpp 
	$(GXX) -c cache.cpp

conflictCache.o: conflictCache.cpp
	$(GXX) -c conflictCache.cpp

lookAheadCache.o: lookAheadCache.cpp
	$(GXX) -c lookAheadCache.cpp

dynamicCache.o: dynamicCache.cpp
	$(GXX) -c dynamicCache.cpp

clean:
	rm -f *.o *~ core test.exe cache-sim driv
