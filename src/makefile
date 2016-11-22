CC=g++
INC=-I./utils
CFLAGS=-c -DNDEBUG -Wall -std=c++0x -O2
LDFLAGS=-pthread
SOURCES=bellman_ford.cc main.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=asynch_bellman_ford

all:	$(SOURCES) $(EXECUTABLE)

$(EXECUTABLE):	$(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm *.o asynch_bellman_ford
