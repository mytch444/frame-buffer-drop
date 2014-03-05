FILES=game.c input.c draw.c
EXEC=game

all: main

main: $(FILES)
	gcc -o $(EXEC) game.c -lpthread

clean:
	rm $(EXEC)
