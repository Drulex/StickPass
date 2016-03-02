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
 *  For the sake of simplicity the eeprom memory is divided in 8 blocks of 63 bytes.
 *
 *  Each block is called a "credential" and is structured as per table below:
 *
 *  ---------------------------------------------------------------------
 *  | idName (10 bytes) | idUsername (32 bytes) | idPassword (21 bytes) |
 *  ---------------------------------------------------------------------
 *
 *  This static memory layout has the advantage of greatly simplifying code complexity
 *  and reducing code size. The performance tradeoff of implementing dynamic memory
 *  allocation and management on the eeprom is probably not worth the effort considering
 *  the scope of this project, but is definitely a more elegant solution that could be
 *  implemented in a future release.
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
#include <string.h>

unsigned char credCount = 0;

int update_credential(cred_t cred) {
    getCredCount();

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
    eeprom_update_block((const void*)&credCount, (void *)CREDCOUNT_LOCATION, 1);

    return 0;
}

/*
 * Get the master key from EEPROM memory
 * This is not really good security at least we could spread the key
 * around in obscure locations to improve security but for the scope
 * of this project it's ok
 *
 */
void getMasterKey(char *masterKey) {
    eeprom_read_block(masterKey, (const void*)MASTERKEY_LOCATION, MASTERKEY_LEN);
}

void getCredentialData(unsigned char idNum, cred_t *cred) {
    int memPtr;
    if(idNum == 0)
        memPtr = 0;
    else
        memPtr = ((idNum - 1) * ID_BLOCK_LEN);

    // read idName
    eeprom_read_block(cred->idName, (const void *)memPtr, ID_NAME_LEN);
    cred->idName[ID_NAME_LEN] = '\0';

    // read idUsername
    memPtr += ID_NAME_LEN;
    eeprom_read_block(cred->idUsername, (const void *)memPtr, ID_USERNAME_LEN);
    cred->idUsername[ID_USERNAME_LEN] = '\0';

    // read idPassword
    memPtr += ID_USERNAME_LEN;
    eeprom_read_block(cred->idPassword, (const void *)memPtr, ID_PASSWORD_LEN);
    cred->idPassword[ID_PASSWORD_LEN] = '\0';
}

void clearCred(cred_t *cred) {
    memset(cred->idName, 0, ID_NAME_LEN);
    memset(cred->idUsername, 0, ID_USERNAME_LEN);
    memset(cred->idPassword, 0, ID_PASSWORD_LEN);
}

void clearEEPROM(void) {
void clearEEPROM(unsigned char flagResetKey) {
    // backup masterKey
    char masterKey[7];
    getMasterKey(&masterKey[0]);

    unsigned char i;
    int memPtr;
    unsigned char clear[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    for(i = 0; i < 64; i++) {
        memPtr = (i * MAX_CRED);
        eeprom_update_block((const void *)clear, (void *)memPtr, 8);
    }
    credCount = 0;
    // set credCount to 0 in memory
    eeprom_update_block((const void*)&credCount, (void *)CREDCOUNT_LOCATION, 1);

    // restore the master key if key reset flag is false
    if(!flagResetKey)
        setMasterKey(&masterKey[0]);
}

void getCredCount(void) {
    unsigned char data;
    eeprom_read_block(&data, (const void*)CREDCOUNT_LOCATION, 1);
    credCount = data;
}
