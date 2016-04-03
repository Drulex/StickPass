/*
 * File: credentials.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-02
 * License: GNU GPL v3 (see LICENSE)
 *
 */

#include <avr/eeprom.h>
#include "credentials.h"
#include <string.h>
#include "led.h"

// init credCount to 0
unsigned char credCount = 0;

/*
 *  Get the credential count and append credential to EEPROM memory
 *  Return 0 on success
 *  Return -1 if no more space is available
 *
 */
int update_credential(cred_t cred) {
    getCredCount();

    if(credCount == MAX_CRED) {
        // signal that something is wrong
        LED_HIGH();
        return -1;
    }

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

/*
 * Set the master key in EEPROM memory
 * We erase memory whenever we set the key for security purposes
 *
 */
void setMasterKey(char *masterKey) {
    eeprom_update_block((const void *)masterKey, (void *)MASTERKEY_LOCATION, MASTERKEY_LEN);
}

/*
 * Get credential information from EEPROM memory
 * Take an ID number and a cred_t structer and update the credential
 * from EEPROM in the cred structure
 *
 */
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

/*
 * Clear memory before using cred_t structure
 *
 */
void clearCred(cred_t *cred) {
    memset(cred->idName, 0, ID_NAME_LEN);
    memset(cred->idUsername, 0, ID_USERNAME_LEN);
    memset(cred->idPassword, 0, ID_PASSWORD_LEN);
}

/*
 * Clear EEPROM by writing 0xFF on all 512 bytes
 *
 */
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

/*
 * Obtain the credential count from the EEPROM memory
 * This is located at address 0x1F8
 *
 */
void getCredCount(void) {
    unsigned char data;
    eeprom_read_block(&data, (const void*)CREDCOUNT_LOCATION, 1);
    credCount = data;
}
