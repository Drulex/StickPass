/*
 * File: credentials.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-02
 * License: GNU GPL v2 (see License.txt)
 *
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string.h>
#include <stdio.h>

#define ID_BLOCK_LEN 63
#define ID_NAME_LEN 10
#define ID_USERNAME_LEN 32
#define ID_PASSWORD_LEN 21
#define MAX_CRED 8
// credcnt var is kept at eeprom location 0x1F8 (504)
#define CREDCOUNT_LOCATION 0x1F8

typedef struct {
    char idName[ID_NAME_LEN + 1];
    char idUsername[ID_USERNAME_LEN + 1];
    char idPassword[ID_PASSWORD_LEN + 1];
} cred_t;

extern unsigned char credCount;

// prototypes
int update_credential(cred_t cred);
void getCredentialData(unsigned char idNum, cred_t *cred);
void clearCred(cred_t *cred);
void clearEEPROM(void);
void getCredCount(void);

#endif

