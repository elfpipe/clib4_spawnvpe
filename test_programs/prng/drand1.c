#define _XOPEN_SOURCE
#include <stdlib.h>

#include <stdio.h>

#define ASIZE 1024

int main(int argc, char *argv[]) {
    int i;
    double rand_number[ASIZE] = {0};
    unsigned short seed[3] = {155, 0, 155};

    // Initialize Seed Value For Random Number
    seed48(&seed[0]);

    for (i = 0; i < ASIZE; i++) {
        rand_number[i] = drand48();
    }

    // Sample Array Element
    printf("%f\n", rand_number[ASIZE - 1]);
    return 0;
}