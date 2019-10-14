#include <stdio.h>
#include <stdlib.h>

#include "jail.h"

int main(int argc, char *argv[]) {
    jail_args_t args = jail_args_parse(argc, argv);

    jail_args_dump(&args);
    printf("Hello, Jail!\n");

    return EXIT_SUCCESS;
}
