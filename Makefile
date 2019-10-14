all: jail

jail: jail_args.o jail_conf.o jail_spawn.o jail.o
	gcc build/debug/obj/*.o -o build/debug/bin/jail -lcap -lseccomp -lpthread

jail.o: src/jail.c
	gcc -Wall -c src/jail.c -o build/debug/obj/jail.o

jail_args.o: src/jail_args.c
	gcc -Wall -c src/jail_args.c -o build/debug/obj/jail_args.o

jail_conf.o: src/jail_args.c
	gcc -Wall -c src/jail_conf.c -o build/debug/obj/jail_conf.o

jail_spawn.o: src/jail_spawn.c
	gcc -Wall -c src/jail_spawn.c -o build/debug/obj/jail_spawn.o

clean:
	rm -f build/debug/obj/* build/debug/bin/*
