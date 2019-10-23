CC=gcc
CFLAGS=
BUILD_DIR=

release: CFLAGS += -O2
release: BUILD_DIR += build/release
release: jail

debug: CFLAGS += -g -Wall -DDEBUG
debug: BUILD_DIR += build/debug
debug: jail

jail: jail_args.o jail_conf.o jail_spawn.o jail_loop.o jail_seccomp.o jail_capability.o jail_cgroup.o jail_rlimit.o jail.o
	$(CC) $(BUILD_DIR)/obj/*.o -o $(BUILD_DIR)/jail -lcap -lseccomp

jail.o: src/jail.c
	$(CC) $(CFLAGS) -c src/jail.c -o $(BUILD_DIR)/obj/jail.o

jail_args.o: src/jail_args.c
	$(CC) $(CFLAGS) -c src/jail_args.c -o $(BUILD_DIR)/obj/jail_args.o

jail_conf.o: src/jail_args.c
	$(CC) $(CFLAGS) -c src/jail_conf.c -o $(BUILD_DIR)/obj/jail_conf.o

jail_spawn.o: src/jail_spawn.c
	$(CC) $(CFLAGS) -c src/jail_spawn.c -o $(BUILD_DIR)/obj/jail_spawn.o

jail_loop.o: src/jail_loop.c
	$(CC) $(CFLAGS) -c src/jail_loop.c -o $(BUILD_DIR)/obj/jail_loop.o

jail_seccomp.o: src/jail_seccomp.c
	$(CC) $(CFLAGS) -c src/jail_seccomp.c -o $(BUILD_DIR)/obj/jail_seccomp.o

jail_capability.o: src/jail_capability.c
	$(CC) $(CFLAGS) -c src/jail_capability.c -o $(BUILD_DIR)/obj/jail_capability.o

jail_cgroup.o: src/jail_cgroup.c
	$(CC) $(CFLAGS) -c src/jail_cgroup.c -o $(BUILD_DIR)/obj/jail_cgroup.o

jail_rlimit.o: src/jail_rlimit.c
	$(CC) $(CFLAGS) -c src/jail_rlimit.c -o $(BUILD_DIR)/obj/jail_rlimit.o
.PHONY: clean

clean:
	rm -f build/debug/obj/* build/debug/jail build/release/obj/* build/release/jail
