CC = gcc
OUT = heat

all: main.o
	$(CC) -o $(OUT) main.o

clean:
	rm -f *.o heat

%.o : %.cpp %.h
	$(CC) -c $<

%.o : %.cpp
	$(CC) -c $<

% : %.cpp
	$(CC) -o $@ $<