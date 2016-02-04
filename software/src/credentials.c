/*  File: credentials.c
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


int parseIdBlock(cred_t *cred, char *idBlock) {
    if(snprintf(cred->idName, ID_NAME_LEN + 1, &idBlock[0]) < 0)
        return -1;
    if(snprintf(cred->idUsername, ID_USERNAME_LEN + 1, &idBlock[0 + ID_NAME_LEN]) < 0)
        return -1;
    if(snprintf(cred->idPassword, ID_PASSWORD_LEN + 1, &idBlock[ID_NAME_LEN + ID_USERNAME_LEN]) < 0)
        return -1;

    return 0;
}

int update_credential(cred_t cred) {
    /*
     * TODO
     *
     * Add logic to verify if a credential exists in which case we only
     * need to update the idUsername and/or idPassword
     *
     */

    int memPtr;
    if(credCount == MAX_CRED)
        return -1;

    // eeprom credential memory pointer
    memPtr = (credCount * ID_BLOCK_LEN);

    // write idName to eeprom and increment memPtr to idUsername
    eeprom_update_block((const void *)cred.idName, (void *)memPtr, ID_NAME_LEN);
    memPtr += ID_NAME_LEN;

    // wirte idUsername to eeprom and increment memPtr to idPassword
    eeprom_update_block((const void *)cred.idUsername, (void *)memPtr, ID_USERNAME_LEN);
    memPtr += ID_USERNAME_LEN;

    // write idPassword to eeprom
    eeprom_update_block((const void *)cred.idPassword, (void *)memPtr, ID_PASSWORD_LEN);

    // increment global var credCount
    credCount++;

    return 0;
}

/*
int main(void) {

    credCount = 0;
    cred_t cred;
    char idBlockTest[ID_BLOCK_LEN] = "linkedin\0\0alexandru.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    int rval;

    rval = parseIdBlock(&cred, idBlockTest);
    printf("rval = %i\n", rval);
    printf("name = %s\n", cred.idName);
    printf("username = %s\n", cred.idUsername);
    printf("password = %s\n", cred.idPassword);

}
*/
