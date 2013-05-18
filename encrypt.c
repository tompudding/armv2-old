#include "rijndael-alg-fst.h"
#include <stdint.h>

int _start(void) {
    u32 rk[4*(MAXNR+1)] = {0};
    uint8_t key[]            = "aaaaaaaaaaaaaaaa";
    uint8_t plain[]          = "YELLOW SUBMARINE";
    uint8_t cipher[]         = {0};

    rijndaelKeySetupEnc(rk,key,128);
    rijndaelEncrypt(rk,10,plain,cipher);
    return ((int*)cipher)[0];
}
