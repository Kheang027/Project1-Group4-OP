#include <stdio.h>

int main(int argc, char **argv) {
    // Parse args
    if (argc != 4) {
        printf("Error: Wrong number of args\n");
        return 1;
    }

    int n = argv[1];
    if (n < 2 || n > 10) {
        printf("Error: n has to be an integer between 2 and 10\n");
    }
    char *src_file = argv[2];
    char *dst_file = argv[3];
    // Create n threads and copy file
    return 0;
}