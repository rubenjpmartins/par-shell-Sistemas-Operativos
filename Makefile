all: par-shell fibonacci par-shell-terminal


par-shell: main.o commandlinereader.o list.o 
	gcc -pthread -o par-shell main.o commandlinereader.o list.o 

par-shell-terminal: par-shell-terminal.o
	gcc -o par-shell-terminal par-shell-terminal.o 

commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc -c -g -Wall commandlinereader.c

list.o: list.c list.h
	gcc -Wall -g -c list.c

fibonacci: fibonacci.o
	gcc -o fibonacci fibonacci.o 

fibonacci.o: fibonacci.c 
	gcc -c -g -Wall fibonacci.c 

main.o: main.c commandlinereader.h list.h
	gcc -c -g -Wall main.c 

par-shell-terminal.o: par-shell-terminal.c
	gcc -c -g -Wall par-shell-terminal.c 

clean:
	rm -f *.o *.txt par-shell fibonacci par-shell-terminal
