#ifndef _IAP_H_
#define _IAP_H_

#define IAP_LOCATION            (0x7ffffff1)
#define IAP_CMD_PREPARE         (50)
#define IAP_CMD_COPYRAMTOFLASH  (51)
#define IAP_CMD_ERASE           (52)
#define IAP_CMD_BLANKCHECK      (53)
#define IAP_CMD_READPARTID      (54)
#define IAP_CMD_READBOOTCODEVER (55)
#define IAP_CMD_COMPARE         (56)
#define IAP_CMD_REINVOKEISP     (57)

#define IAP_RESULT_CMD_SUCCESS          (0)
#define IAP_RESULT_INVALID_COMMAND      (1)
#define IAP_RESULT_SRC_ADDR_ERROR       (2)
#define IAP_RESULT_DST_ADDR_ERROR       (3)
#define IAP_RESULT_SRC_ADDR_NOT_MAPPED  (4)
#define IAP_RESULT_DST_ADDR_NOT_MAPPED  (5)
#define IAP_RESULT_COUNT_ERROR          (6)
#define IAP_RESULT_INVALID_SECTOR       (7)
#define IAP_RESULT_SECTOR_NOT_BLANK     (8)
#define IAP_RESULT_SECTOR_NOT_PREPARED  (9)
#define IAP_RESULT_COMPARE_ERROR        (10)
#define IAP_RESULT_BUSY                 (11)
#define IAP_RESULT_PARAM_ERROR          (12)
#define IAP_RESULT_ADDR_ERROR           (13)
#define IAP_RESULT_ADDR_NOT_MAPPED      (14)
#define IAP_RESULT_CMD_LOCKED           (15)
#define IAP_RESULT_INVALID_CODE         (16)
#define IAP_RESULT_INVALID_BAUD_RATE    (17)
#define IAP_RESULT_ANVALID_STOP_BIT     (18)
#define IAP_RESULT_CRP_ENABLED          (19)
#define IAP_RESULT_LAST                 (19)

#define IAP_RESULT_X_NOTSAFEREGION      (IAP_RESULT_LAST + 1)
#define IAP_RESULT_X_NOSAFEREGIONAVAIL  (IAP_RESULT_LAST + 2)

int iapSectorToAddress (int sectorNumber, unsigned long *address, int *sectorSize);
int iapAddressToSector (unsigned long address, int startSector);
int iapIsValidSector (int sector);
int iapGetErrno (void);
const char *iapStrerror (int err);
int iapPrepareSectors (int startingSector, int endingSector);
int iapFillSectors (int startingSector, int endingSector, int byte);
int iapWriteSectors (unsigned int address, const unsigned char *buffer, int bufferLen);
int iapEraseSectors (int startingSector, int endingSector);
int iapBlankCheckSectors (int startingSector, int endingSector);
unsigned int iapReadPartID (void);
unsigned int iapReadBootCodeVersion (void);
int iapCompare (unsigned int address, unsigned char *buffer, int bufferLen);

#endif // _IAP_H_
