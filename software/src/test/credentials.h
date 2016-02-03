#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string.h>
#include <stdio.h>

typedef struct {
    unsigned char idName[10+1];
    unsigned char idUsername[32+1];
    unsigned char idPassword[22+1];
} cred_t;


#endif

