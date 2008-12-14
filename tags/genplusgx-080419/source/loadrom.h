#ifndef _LOADROM_H_
#define _LOADROM_H_

#define MAXCOMPANY 64

typedef struct
{
  char consoletype[18];		/* Genesis or Mega Drive */
  char copyright[18];		/* Copyright message */
  char domestic[50];		/* Domestic name of ROM */
  char international[50];	/* International name of ROM */
  char ROMType[4];			/* Educational or Game */
  char product[14];			/* Product serial number */
  unsigned short checksum;	/* Checksum */
  char io_support[18];		/* Actually 16 chars :) */
  unsigned int romstart;
  unsigned int romend;
  char RAMInfo[14];
  unsigned int ramstart;
  unsigned int ramend;
  char modem[14];
  char memo[50];
  char country[18];
} ROMINFO;

typedef struct
{
  char companyid[6];
  char company[30];
} COMPANYINFO;

typedef struct
{
  char pID[2];
  char pName[21];
} PERIPHERALINFO;

/* Global variables */
extern ROMINFO rominfo;
extern COMPANYINFO companyinfo[MAXCOMPANY];
extern PERIPHERALINFO peripheralinfo[14];
extern uint16 realchecksum;

/* Function prototypes */
extern int load_rom(char *filename);

#endif /* _LOADROM_H_ */

