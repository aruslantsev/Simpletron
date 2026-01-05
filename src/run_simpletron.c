#include <stdio.h>
#include <string.h>

#include "simpletron.h"


void show_help(char executableName[]) {
    puts("Usage: ");
    printf("\t%s\t\t\tto enter program from keyboard\n", executableName);
    printf("\t%s FILENAME\tto read program from file\n", executableName);
}


int main(const int argc, char *argv[]) {
    struct Simpletron simpletron;
    enum Status status;
    if (argc == 1) {
        input_sml(&simpletron);
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        }
        reset(&simpletron);
        read_file_sml(&simpletron, argv[1]);
        print_state(&simpletron);
    } else {
        show_help(argv[0]);
        return 0;
    }

    do {
        status = execute_operation(&simpletron);
    } while (status == SUCCESS);
    print_state(&simpletron);
    return 0;
}
