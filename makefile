CC=clang++
CFFLAGS=-I.
LDFLAGS=`xcode-select --print-path`/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib \
		-rpath `xcode-select --print-path`/Toolchains/XcodeDefault.xctoolchain/usr/lib
DEBUG=-g

all:
	$(CC) $(CFFLAGS) $(LDFLAGS) NSNotificationChecker.cpp -o main.o

debug:
	$(CC) $(CFFLAGS) $(LDFLAGS) $(DEBUG) NSNotificationChecker.cpp -o main.o 

clean:
	rm -rf *.o
