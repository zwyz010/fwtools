/******************************************
 * Appro Firmware decrypter V1.0          *
 * Models specified in list below         *
 *                                        *
 * Version 1.00                04/2017    *
 * leecher@dose.0wnz.at                   *
 * http://www.hardwarefetish.com          *
 ******************************************/
#include <stdio.h>

struct {
    char *pszModel;
    unsigned long dwCode;
} m_mpimModel[] = {
    /* Appro */
    {"DMS-3011", 0x1234},
    {"DMS-3014", 0x1240},
    {"LC-7211" , 0x5678},
    {"LC-7213" , 0x1588},
    {"LC-7214" , 0x1688},
    {"LC-7215" , 0x1788},
    {"NVR-2018", 0x1250},
    {"NVR-2028", 0x1260},
    {"DMS-3016", 0x2234},
    {"PVR-3031", 0x1270},
    {"LC-7224" , 0x1689},
    {"LC-7225" , 0x1789},
    {"DMS-3009", 0x2235},
    {"DMS-3004", 0x2236},
    /* D-Link */
    {"DNR-202L", 0x3300}
};

/* This would be approx. the original decoding function which operates on a buffer, but
 * regarding memory usage, this is not memory efficient, therefore we made
 * our own function which operates on a fp
void aSysUpdate_UnPackedFile(short machineCode, short * pTarget, unsigned long data_size)
{
    int i, crypt = machineCode;// (data_size%32==0)?((machineCode>>8)|(machineCode<<8)):machineCode;

    for (i=0; i<(data_size>>1); i++)
    {
	pTarget[i] = crypt ^ pTarget[i] ^ pTarget[i+1];
	crypt = crypt%2==0?crypt>>1:(crypt>>1)|0x8000;
    }
    pTarget[i] = crypt ^ pTarget[i+1];
}
*/

void unpack_file(short machineCode, FILE *fpIn, FILE *fpOut)
{
	int i, crypt = machineCode; 
	short cur, next;

	fread(&cur, sizeof(cur), 1, fpIn);
	for (i=0; fread(&next, sizeof(next), 1, fpIn); i++)
	{
		cur ^= crypt ^ next;
		fwrite(&cur, sizeof(cur), 1, fpOut);
		crypt = crypt%2==0?crypt>>1:(crypt>>1)|0x8000;
		cur = next;
	}
}

static void usage(char *pszApp)
{
	int i;
	fprintf (stderr, "%s <Machine code hex> <firmware file> <output file>\n\nCurrently known machine codes:\n", pszApp);
	
	for (i=0; i<sizeof(m_mpimModel)/sizeof(m_mpimModel[0]); i++)
	    fprintf (stderr, "\t0x%04X\t%s\n", (unsigned int)m_mpimModel[i].dwCode, m_mpimModel[i].pszModel);
	fprintf (stderr, "\nExample: %s 0x3300 DNR-202L_A2_V2.04.03.bin decrypted.bin\n", pszApp);
}

int main(int argc, char **argv)
{
	FILE *fpIn, *fpOut;
	unsigned int iMachineCode;

	printf ("Appro/D-Link firmware decrypter V1.00\nhttp://www.hardwarefetish.com\n\n");
	if (argc<4)
	{
		usage(argv[0]);
		return -1;
	}

	if (!sscanf(argv[1], "%04X", &iMachineCode) || !iMachineCode || iMachineCode>0xFFFF)
	{
		fprintf(stderr, "Invalid machine code supplied\n");
		return -1;
	}

	if (!(fpIn = fopen(argv[2], "rb")))
	{
		perror("Cannot open input file");
		return -1;
	}

	if (!(fpOut = fopen(argv[3], "w+b")))
	{
		fclose (fpIn);
		perror("Cannot open output file");
		return -1;
	}

	unpack_file(iMachineCode, fpIn, fpOut);

	fclose(fpIn);
	fclose(fpOut);

	return 0;
}
