export LIBS = 
export CFLAGS = -g -Wall -O2
export LFLAGS = 

CC=g++
xPL=-DxPLLinux

all: xplhub 

xplhub: xplhub.o
	$(CC) -o xplhub $(LIBS) $(LFLAGS) xplhub.o
	@echo -e "Successfully built xplhub.\n"

xplhub.o: xplhub.cc
	$(CC) -c -o xplhub.o $(CFLAGS) $(xPL) xplhub.cc

clean:
	rm -f xplhub *.o
