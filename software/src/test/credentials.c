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

#include "credentials.h"

static unsigned char credCount;

int parseIdBlock(cred_t *cred, unsigned char *idBlock) {
    if(snprintf(cred->idName, 10+1, &idBlock[0]) < 0)
        return -1;
    if(snprintf(cred->idUsername, 32+1, &idBlock[10]) < 0)
        return -1;
    if(snprintf(cred->idPassword, 22+1, &idBlock[42]) < 0)
        return -1;

    return 0;
}

int update_credential(cred_t cred) {
    int memPtr;
    if(credCount == 8)
        return -1;

    // eeprom credential memory pointer
    memPtr = (credCount * 64);
}

int main(void) {

    credCount = 0;
    cred_t cred;
    unsigned char idBlockTest[64] = "idname1234idusername123456789abcdefghijklmidpassword123456789abc";
    int rval;

    rval = parseIdBlock(&cred, idBlockTest);
    printf("rval = %i\n", rval);
    printf("name = %s\n", cred.idName);
    printf("username = %s\n", cred.idUsername);
    printf("password = %s\n", cred.idPassword);

}
