all: main 

main.o: main.cpp
	g++ -c main.cpp

main: main.o
	g++ -o run main.o

	


