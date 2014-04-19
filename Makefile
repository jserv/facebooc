run: all
	./bin/facebooc

all: src/*.c
	rm -rf bin
	mkdir -p bin
	$(CC) -o bin/facebooc -Iinc -g src/*.c
