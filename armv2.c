#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "armv2.h"

#define LOG(...) printf(__VA_ARGS__)

int init_armv2(uint32_t memsize, armv2_t *cpu) {
    if(NULL == cpu) {
        LOG("%s error, NULL cpu\n",__func__);
        return -1;
    }
    if(memsize > MAX_MEMORY) {
        LOG("%s error, request memory size(%u) too big\n",__func__,memsize);
        return -2;
    }
    memset(cpu,0,sizeof(armv2_t));
    return 0;
}

int cleanup_armv2(armv2_t *cpu) {
    if(NULL == cpu) {
        return 0;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    armv2_t armv2;
    int result = 0;

    if(0 != (result = init_armv2((1<<20),&armv2))) {
        LOG("Error %d creating\n",result);
        return result;
    }
    goto cleanup;

cleanup:
    cleanup_armv2(&armv2);
    return result;
}
