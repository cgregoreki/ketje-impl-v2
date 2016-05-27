CC = gcc
CFLAGS = -g -O2
OBJECTS = main.o foo.o

main.exe : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o main.exe

main.o : main.c
	$(CC) $(CFLAGS) -c main.c

foo.o : foo.c
	$(CC) $(CFLAGS) -c foo.c