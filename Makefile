run: all
	./bin/facebooc

debug: src/*.c
	rm -rf bin
	mkdir -p bin
	$(CC) -o bin/facebooc -Iinc -lsqlite3 -g src/*.c
	gdb bin/facebooc

all: src/*.c
	rm -rf bin
	mkdir -p bin
	$(CC) -o bin/facebooc -Iinc -lsqlite3 -O2 src/*.c
