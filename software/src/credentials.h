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
    char idName[ID_NAME_LEN + 1];
    char idUsername[ID_USERNAME_LEN + 1];
    char idPassword[ID_PASSWORD_LEN + 1];
} cred_t;


// prototypes
int parseIdBlock(cred_t *cred, char *idBlock);
int update_credential(cred_t cred);

// global variables
extern unsigned char credCount;


#endif

