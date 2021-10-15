/*
 *  Repacker for Hisilicon (FLS) Camera firmware images
 *
 *  (C)oded by leecher@dose.0wnz.at        09/2014
 *  http://www.hardwarefetish.com
 *
 *  gcc -O3 -Wall -lz -o flspack flspack.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>	/* Needed for CRC32, you can also add your own crc32 */

#define MAGIC	0x53564448	/* HDVS */

typedef struct
{
	unsigned int magic;		/* Always HDVS */
	unsigned int filesize;	/* Total size of upgrade file in bytes */
	unsigned int nImages;	/* Number of images following header, usually 1 */
	unsigned int blksize;	/* Block size? Usually 0x10000, not used */
	unsigned int imagesize;	/* Size of file without the header (header=this + followig headers) */
	unsigned char padding[0x80];
} FLS_HDR;

typedef struct
{
	char szFileName[32];	/* Name of file in this section */
	char szApptype[128];	/* Usually "APPLICATION" */
	unsigned int blsize;	/* blksize from header, usually 0x10000 */
	unsigned int imagesize;	/* imagesize of this image, if 1 image, identical to imagesize from header */
	unsigned int CRC32;		/* CRC of this image */
	unsigned int unk1;		/* Unknown, always 0 */
	unsigned int imageNr;	/* Image number, starting with 0 */
	unsigned char unk2[72];	/* Unknown stuff, doesn't seem to get used by updater? */
} FLS_BLK_HDR;

static int usage(char *pszProg)
{
	fprintf (stderr, "Usage: %s <Old FW file> <Your repacked JFFS> <output FW file>\n\n", pszProg);
	return EXIT_FAILURE;
}

static int read_hdr(FILE *fpOld, char *pszFile, FLS_HDR *hdr, FLS_BLK_HDR *blkhdr)
{
	if (fread(hdr, 1, sizeof(FLS_HDR), fpOld) != sizeof(FLS_HDR))
	{
		fprintf (stderr, "Cannot read header from old FW file %s: %s\n", pszFile, strerror(errno));
		return -1;
	}
	if (hdr->magic != MAGIC)
	{
		fprintf(stderr, "Header magic of old FW file %s doesn't match!\n", pszFile);
		return -1;
	}
	if (hdr->nImages != 1)
	{
		fprintf(stderr, "Currently only images with 1 subimage are supported. Support can easily be added, contact me!\n");
		return -1;
	}
	if (fread(blkhdr, 1, sizeof(FLS_BLK_HDR), fpOld) != sizeof(FLS_BLK_HDR))
	{
		fprintf(stderr, "Cannot read block header from old FW file %s: %s", pszFile, strerror(errno));
		return -1;
	}
	return 0;
}

unsigned int CRC_JFFS(FILE *fpJFFS, FILE *fpOut, unsigned int *cbFile)
{
	unsigned char buffer[0x1000];
	size_t sz;
	unsigned int crc = 0;

	while ((sz = fread(buffer, 1, sizeof(buffer), fpJFFS)))
	{
		fwrite(buffer, 1, sz, fpOut);
		crc = crc32(crc, buffer, sz);
	}
	*cbFile = ftell(fpJFFS);
	rewind(fpJFFS);
	return crc;
}

int main(int argc, char **argv)
{
	FILE *fpOld, *fpJFFS, *fpOut;
	int iRet=EXIT_FAILURE;
	FLS_HDR hdr;
	FLS_BLK_HDR blkhdr;

	printf ("FLS simple repacker V0.1 - leecher@dose.0wnz.at\n\n");
	if (argc<4) return usage(argv[0]);
	
	if ((fpOld=fopen(argv[1], "rb")))
	{
		if (read_hdr(fpOld, argv[1], &hdr, &blkhdr) == 0)
		{
			if ((fpJFFS=fopen(argv[2], "rb")))
			{
				if ((fpOut=fopen(argv[3], "wb")))
				{
					fseek(fpOut, ftell(fpOld), SEEK_SET);
					blkhdr.CRC32 = CRC_JFFS(fpJFFS, fpOut, &blkhdr.imagesize);
					hdr.imagesize = blkhdr.imagesize;
					hdr.filesize = hdr.imagesize + ftell(fpOld);
					rewind(fpOut);
					fwrite(&hdr, 1, sizeof(hdr), fpOut);
					fwrite(&blkhdr, 1, sizeof(blkhdr), fpOut);
					iRet = EXIT_SUCCESS;
					printf ("New image written to %s\n", argv[3]);
					fclose(fpOut);
				}
				fclose(fpJFFS);
			}
			else
				fprintf(stderr, "Cannot open your repacked JFFS file %s for reading: %s\n", argv[2], strerror(errno));
		}
		fclose(fpOld);
	}
	else
		fprintf(stderr, "Cannot open old FW file %s for reading: %s\n", argv[1], strerror(errno));
	return iRet;
}
