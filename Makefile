run: all
	./bin/facebooc

debug: src/*.c
	rm -rf bin
	mkdir -p bin
	$(CC) -o bin/facebooc -Wall -Iinc -g src/*.c src/**/*.c -lsqlite3
	gdb bin/facebooc

all: src/*.c
	rm -rf bin
	mkdir -p bin
	$(CC) -o bin/facebooc -Wall -Iinc -O2 src/*.c src/**/*.c -lsqlite3

clean:
	rm -f bin/facebooc
