help:
	@echo "Select a rule:"
	@echo "    make app ..... to build the user app"
	@echo "    make clean ... to delete objects"

clean:
	rm -rf stickapp stickapp.o

app:
	gcc -O -g -Wall -c stickapp.c
	gcc -o stickapp stickapp.o -L/usr/lib -lusb

