/*
 * File: credentials.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-02
 * License: GNU GPL v2 (see License.txt)
 *
 *
 *  Credentials management scheme
 *  =============================
 *
 *  For the sake of simplicity the eeprom memory is divided in 8 blocks of 64 bytes.
 *
 *  Each block is called a "credential" and is structured as per table below:
 *
 *  ---------------------------------------------------------------------
 *  | idName (10 bytes) | idUsername (32 bytes) | idPassword (22 bytes) |
 *  ---------------------------------------------------------------------
 *
 *  This static memory layout has the advantage of greatly simplifying code complexity
 *  and reducing code size. The performance tradeoff of implementing dynamic memory
 *  allocation and management on the eeprom is probably not worth the effort considering
 *  the scope of this project, but is definitely a more elegant solution that could be
 *  implemented in a future release.
 *
 *  *Each idBlock component should be padded with '\0' to fill its respective block size.*
 *
 *  Typical sequence:
 *  -----------------
 *
 *  1. idBlock is received from user app.
 *
 *  2. idBlock is parsed to extract the following:
 *      a. idName (name of id to be stored)
 *      b. idUsername (username associated with id)
 *      c. idPassword (password associated with id)
 *
 *  3. Check if there is enough space in eeprom.
 *     (this is done by checking the credCount variable that can have a max value of 8)
 *
 *  4. Credential is written to eeprom memory.
 *
 *  5. credCount variable is updated.
 *
 */

#include <avr/eeprom.h>
#include "credentials.h"
#include "led.h"

unsigned char credCount = 0;
unsigned char credPtr;

int update_credential(void) {

    if(credCount == MAX_CRED)
        return -1;
    int memPtr;
    unsigned char idNameLen = sizeof(cred.idName);
    unsigned char idUsernameLen = sizeof(cred.idUsername);
    unsigned char idPasswordLen = sizeof(cred.idPassword);

    // eeprom credential memory pointer
    memPtr = (credCount * ID_BLOCK_LEN);

    // write idName to eeprom and increment memPtr to idUsername
    eeprom_update_block((const void *)cred.idName, (void *)memPtr, idNameLen);
    memPtr += ID_NAME_LEN;

    // write idUsername to eeprom and increment memPtr to idPassword
    eeprom_update_block((const void *)cred.idUsername, (void *)memPtr, idUsernameLen);
    memPtr += ID_USERNAME_LEN;

    // write idPassword to eeprom
    eeprom_update_block((const void *)cred.idPassword, (void *)memPtr, idPasswordLen);

    // increment global var credCount
    credCount++;

    return 0;
}

void getCredentialData(unsigned char idNum, unsigned char *buffer) {
    int memPtr;
    memPtr = (idNum * ID_BLOCK_LEN);

    eeprom_read_block((void *)buffer, (const void *)memPtr, 64);
}

/*
 * When iterating through credentials we need to keep track of where the ptr
 * is inside the buffer because each element is padded with '\0'. We know that
 * when we reach one it means the end of current id element so we jump to the
 * next one. If we are at the password we reset to 0 as this is the last element.
 *
 */
void updateCredPtr(void) {
    // if we sent ID_NAME we jump to ID_USERNAME
    if(credPtr <= 9)
        credPtr = 10;

    // if we sent ID_USERNAME we jump to ID_PASSWORD
    else if(credPtr >=10 && credPtr <= 41)
        credPtr = 42;

    else if(credPtr >= 42)
        credPtr = 0;
}

void clearCred(void) {
    unsigned char i;
    for(i = 0; i < sizeof(cred); i++) {
        ((unsigned char *)&cred)[i] = 0;
    }
}

