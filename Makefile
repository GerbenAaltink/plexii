all: build run

build:
	gcc plexii.c -o plexii.o

serve:
	gcc pserver.c -o pserver.o
	./pserver.o 8888

run:
	./plexii.o

dumb:
	gcc dumb.c -o dumb.o
	./dumb.o

install:
	cp ./plexii /usr/local/bin
