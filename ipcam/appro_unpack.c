/******************************************
 * Appro Firmware Unpacker V1.0           *
 *                                        *
 * Version 1.00                04/2017    *
 * leecher@dose.0wnz.at                   *
 * http://www.hardwarefetish.com          *
 ******************************************/
#include <stdio.h>
#include <limits.h>

/* For unpacking, we only implement simple unpacker and don't
 * implement re-packer, as this is already available in the D-Link 
 * GPL-Packages, so no need to do extra work
 */

// Stolen from appro headers
typedef unsigned long UINT32;
typedef char CHAR;
typedef unsigned char UINT8;
typedef unsigned short UINT16;

typedef struct update_global
{
	UINT32	nSection;		/* 此檔案有包含幾個成員 */
	CHAR*	base;
	UINT16	csum;
	UINT16	type;			/* machine code*/
	UINT32	reserved;
  
} UPDATE_GLOBAL;


/********************************************************/
/* 解開bin檔的檔頭2 */
/********************************************************/
typedef struct update_section
{
	CHAR*	shift;			/* 組成 bin 檔的成員在bin 檔中的位置 */
	CHAR*	offset;			/* 要燒入 flash 的位址, 從BIN_MAKER得來 */
	UINT32	size;			/* 檔案大小 */
	UINT8	group;			/* bin 檔的 id */
	UINT8	flags;
	UINT16	reserved;
	CHAR	filename[16];
  
} UPDATE_SECTION;

static void usage(char *pszApp)
{
	int i;
	fprintf (stderr, "%s <firmware file> <output directory>\n\nExample: %s decrypted.bin .\n", pszApp, pszApp);
}

int main(int argc, char **argv)
{
	FILE *fp, *fpOut;
	UPDATE_GLOBAL hdr;
	UPDATE_SECTION sect;
	char szFile[PATH_MAX];
	long pos;
	int i;

	printf ("Appro/D-Link firmware unpacker V1.00\nhttp://www.hardwarefetish.com\n\n");
	if (argc<3)
	{
		usage(argv[0]);
		return -1;
	}

	if (!(fp = fopen(argv[1], "rb")))
	{
		perror("Cannot open input file");
		return -1;
	}

	if (!(fread(&hdr, sizeof(hdr), 1, fp)))
	{
		perror("Error reading file header");
		fclose(fp);
		return -1;
	}

	for (i=0; i<hdr.nSection; i++)
	{
		if (!(fread(&sect, sizeof(sect), 1, fp)))
		{
			perror("Error reading section");
			fclose(fp);
			return -1;
		}
		sprintf(szFile, "%s/%s", argv[2], sect.filename);
		printf ("Dumping %s...", sect.filename);
		pos = ftell(fp);
		fseek(fp, (long)sect.shift, SEEK_SET);
		if (fpOut = fopen(szFile, "wb"))
		{
			int size, read=0;
			unsigned char buf[1024];

			for (size=sect.size; size>0; size-=read)
			{
				read = fread(buf, 1, size>sizeof(buf)?sizeof(buf):size, fp);
				if (!read) break;
				fwrite (buf, 1, read, fpOut);
			}
			printf ("OK\n");
			fclose(fpOut);
		}
		else
		{
		    perror("Dumping failed");
		}
		fseek(fp, pos, SEEK_SET);
	}

	fclose(fp);

	return 0;
}
