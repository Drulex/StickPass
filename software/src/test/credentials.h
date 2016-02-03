#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string.h>
#include <stdio.h>

#define ID_BLOCK_LEN 64
#define ID_NAME_LEN 10
#define ID_USERNAME_LEN 32
#define ID_PASSWORD_LEN 22
#define MAX_CRED 512/ID_BLOCK_LEN

typedef struct {
    unsigned char idName[ID_NAME_LEN + 1];
    unsigned char idUsername[ID_USERNAME_LEN + 1];
    unsigned char idPassword[ID_PASSWORD_LEN + 1];
} cred_t;


#endif

