all: jail

jail: jail_cli.o jail.o
	gcc build/debug/obj/*.o -o build/debug/bin/jail -lcap -lseccomp -lpthread

jail.o: src/jail.c
	gcc -Wall -c src/jail.c -o build/debug/obj/jail.o

jail_cli.o: src/jail_cli.c
	gcc -Wall -c src/jail_cli.c -o build/debug/obj/jail_cli.o

clean:
	rm -f build/debug/obj/* build/debug/bin/*
