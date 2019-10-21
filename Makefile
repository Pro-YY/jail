CC=gcc
CFLAGS=-g -Wall -DDEBUG
#CFLAGS= -O2

all: jail

jail: jail_args.o jail_conf.o jail_spawn.o jail_loop.o jail_seccomp.o jail_capability.o jail.o
	$(CC) build/debug/obj/*.o -o build/debug/bin/jail -lcap -lseccomp

jail.o: src/jail.c
	$(CC) $(CFLAGS) -c src/jail.c -o build/debug/obj/jail.o

jail_args.o: src/jail_args.c
	$(CC) $(CFLAGS) -c src/jail_args.c -o build/debug/obj/jail_args.o

jail_conf.o: src/jail_args.c
	$(CC) $(CFLAGS) -c src/jail_conf.c -o build/debug/obj/jail_conf.o

jail_spawn.o: src/jail_spawn.c
	$(CC) $(CFLAGS) -c src/jail_spawn.c -o build/debug/obj/jail_spawn.o

jail_loop.o: src/jail_loop.c
	$(CC) $(CFLAGS) -c src/jail_loop.c -o build/debug/obj/jail_loop.o

jail_seccomp.o: src/jail_seccomp.c
	$(CC) $(CFLAGS) -c src/jail_seccomp.c -o build/debug/obj/jail_seccomp.o

jail_capability.o: src/jail_capability.c
	$(CC) $(CFLAGS) -c src/jail_capability.c -o build/debug/obj/jail_capability.o


.PHONY: clean

clean:
	rm -f build/debug/obj/* build/debug/bin/*
