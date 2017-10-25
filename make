CC=g++
CFLAGS=-lstd++ -c

master: master.o
	$(CC) master.o -o master

master.o: master.cpp
	$(CC) $(CFLAGS) master.cpp

.PHONY: clean

clean:
	rm -f *.exe
	rm -f *.o
	rm -f master
	rm -f *.txt
	rm -f *.out
	rm -f slave
