/*
 *  Unpacker for Smtsec Camera firmware images
 *
 *  (C)oded by leecher@dose.0wnz.at        01/2017
 *  http://www.hardwarefetish.com
 *
 *  gcc -O3 -Wall -o smtpack smtpack.c
 *
 * 10.12.2017	-	Added repacking functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dirent.h>

#define BUF_SIZE 	32678	/* 32kb stack buffer for copying */

#define MAGIC	0x12345678	/* the very creative magic */

#pragma pack(1)
typedef struct
{
	unsigned int magic;		/* Always 0x12345678 */
	unsigned int unk1;      /* Always 0x10 */
	unsigned int unk2;      /* Always 0x01 */
	unsigned int blksize;	/* Block size? Usually 0x400, not used */
	unsigned int nImages;	/* Number of files following header */
	unsigned int filesize;	/* Total size of upgrade file in bytes */
	unsigned int unk3;	/* Always 0x01 */
	unsigned int padding[8];
	unsigned int magic2;	/* Always 0x12345678 */
} FLS_HDR;

typedef struct
{
	unsigned int magic;	/* Always 0x12345678 */
	unsigned int unk1;	/* Always 0x10 */
	unsigned int padding[13];
	unsigned int magic2;	/* Always 0x12345678 */
} FLS_FOOTER;

typedef struct
{
	char szFileName[128];	/* Name of file */
	unsigned int size;	/* size of file */
	unsigned int offset;	/* Offset of file */
	unsigned int padding[30];
} FLS_BLK_HDR;
#pragma pack()

typedef struct
{
	int lOffRoot;		/* Offset of root directory within full path of file */
	int nImages;		/* Number of images allocated in memory buffer */
	FLS_BLK_HDR *rgHdrs;	/* Array of headers to write */
	FLS_HDR hdr;		/* File header */
} REPACK;


static int usage(char *pszProg)
{
	fprintf (stderr, "Usage: %s [-u <FW file> <output directory>]\n"
		"\t[-r <Old FW File> <New FW file> <input directory>]\n"
		"\t[-p <<New FW file> <input directory>]\n\n"
		"\t-u\tUnpack\n"
		"\t-r\tRepack\n"
		"\t-p\tPack\n\n", pszProg);
	return EXIT_FAILURE;
}

static int read_hdr(FILE *fpOld, char *pszFile, FLS_HDR *hdr)
{
	if (fread(hdr, 1, sizeof(FLS_HDR), fpOld) != sizeof(FLS_HDR))
	{
		fprintf (stderr, "Cannot read header from FW file %s: %s\n", pszFile, strerror(errno));
		return -1;
	}
	if (hdr->magic != MAGIC)
	{
		fprintf(stderr, "Header magic of FW file %s doesn't match!\n", pszFile);
		return -1;
	}
	if (hdr->magic2 != MAGIC)
	{
		fprintf(stderr, "Header magic of Block following header of FW file %s doesn't match!\n", pszFile);
		return -1;
	}
	return 0;
}

static void remove_slash(char *pszDir)
{
	for (pszDir+=strlen(pszDir)-1; *pszDir=='/'; pszDir--) *pszDir=0;
}

static int seek_file(FILE *fpOld, long lOffsHdrTbl, int i, FLS_BLK_HDR *blkhdr)
{
	if (i && fseek(fpOld, lOffsHdrTbl+i*sizeof(FLS_BLK_HDR), SEEK_SET))
	{
		fprintf(stderr, "Cannot seek to FAT entry #%d: %s\n", i, strerror(errno));
		return -1;
	}
	if (fread(blkhdr, 1, sizeof(FLS_BLK_HDR), fpOld) != sizeof(FLS_BLK_HDR))
	{
		fprintf(stderr, "Cannot read FAT entry #%d @%08X: %s\n", i, (unsigned int)ftell(fpOld), strerror(errno));
		return -1;
	}
	return 0;
}


static int enum_dir(char *pszDir, REPACK *rp)
{
	DIR *dir;
	int i;

	/* Now enumerate all files and add them to the list, if necessary */
	if ((dir = opendir(pszDir)))
	{
		struct dirent *dp;
		struct stat st;
		char szInfile[PATH_MAX];

		while ((dp = readdir(dir)))
		{
			if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
			{
				snprintf(szInfile, sizeof(szInfile), "%s/%s", pszDir, dp->d_name);
				if (stat(szInfile, &st) == 0)
				{
					if (S_ISDIR(st.st_mode)) enum_dir(szInfile, rp);
					else if (S_ISREG(st.st_mode))
					{
						for (i=0; i<rp->hdr.nImages; i++)
							if (strcmp(rp->rgHdrs[i].szFileName, szInfile+rp->lOffRoot) == 0) break;
						if (i!=rp->hdr.nImages) continue;
						if (rp->hdr.nImages+1 >= rp->nImages)
						{
							FLS_BLK_HDR *rgHdrs;

							if (!(rgHdrs = realloc(rp->rgHdrs, (rp->nImages+32)*sizeof(FLS_BLK_HDR))))
							{
								fprintf(stderr, "Error allocating 32 more header entries.\n");
								return -1;
							}
							rp->rgHdrs = rgHdrs;
							rp->nImages += 32;
						}
						strncpy(rp->rgHdrs[rp->hdr.nImages].szFileName, 
							szInfile+rp->lOffRoot, sizeof(rp->rgHdrs[0].szFileName));
						rp->rgHdrs[rp->hdr.nImages++].size = st.st_size;
						rp->hdr.filesize += sizeof(rp->rgHdrs[0]) + st.st_size;
					}
				}
			}
		}
		closedir(dir);
	}
	return 0;
}

static int copy_file(FILE *fpOld, FILE *fpOut, FLS_BLK_HDR *blkhdr)
{
	unsigned char buffer[BUF_SIZE];
	unsigned int toread, left;

	for (left=blkhdr->size; left; left-=toread)
	{
		toread=left>sizeof(buffer)?sizeof(buffer):left;
		if (fread(buffer, 1, toread, fpOld)!=toread)
		{
			fprintf(stderr, "Error reading %d bytes from input file: %s\n", toread, strerror(errno));
			return -1;
		}
		if (fwrite(buffer, 1, toread, fpOut)!=toread)
		{
			fprintf(stderr, "Error writing %d bytes to output file: %s\n", toread, strerror(errno));
			return -1;
		}
	}
	return left;
}

int repack(char *pszOld, char *pszNew, char *pszDir)
{
	REPACK rp = {0};
	FILE *fpOld = NULL;
	int iRet = -1;
	unsigned int i;
	char szOutFile[PATH_MAX];
	FLS_FOOTER footer;

	remove_slash(pszDir);
	/* Open template file, if available */
	if (pszOld)
	{
		if (!(fpOld=fopen(pszOld, "rb")))
		{
			fprintf(stderr, "Cannot open source FW file %s for reading: %s\n", pszOld, strerror(errno));
			return -1;
		}
	}

	/* Init default header, if needed */
	if (!fpOld)
	{
		rp.hdr.magic = MAGIC;
		rp.hdr.unk1 = 0x10;
		rp.hdr.unk2 = 0x01;
		rp.hdr.unk3 = 0x01;
		rp.hdr.blksize = 0x400;
		rp.hdr.magic2 = MAGIC;

		footer.magic = MAGIC;
		footer.magic2 = MAGIC;
		footer.unk1 = 0x10;
	}
	else
	{
		FLS_BLK_HDR blkhdr;
		struct stat st;

		if (read_hdr(fpOld, pszOld, &rp.hdr)<0) return -1;

		/* Reconstruct in original order, if possible */
		if (!(rp.rgHdrs = calloc(sizeof(FLS_BLK_HDR), (rp.nImages = rp.hdr.nImages))))
		{
			fprintf (stderr, "Out of emory allocating %d cached headers\n", rp.hdr.nImages);
			return -1;
		}
		rp.hdr.nImages = 0;
		rp.hdr.filesize = 0;
		for (i=0; i<rp.nImages; i++)
		{
			if (seek_file (fpOld, 0, 0, &blkhdr)<0) break;
			sprintf(szOutFile, "%s%s", pszDir, blkhdr.szFileName);
			if (stat(szOutFile, &st) == 0 && S_ISREG(st.st_mode))
			{
				blkhdr.size = st.st_size;
				memcpy(&rp.rgHdrs[rp.hdr.nImages], &blkhdr, sizeof(blkhdr));
				rp.hdr.nImages++;
				rp.hdr.filesize += sizeof(blkhdr) + blkhdr.size;
			}
		}
		fseek(fpOld, -sizeof(footer), SEEK_END);
		if (!fread(&footer, sizeof(footer), 1, fpOld))
			fprintf (stderr, "Error reading footer of original file!\n");
		fclose(fpOld);
	}

	rp.lOffRoot = strlen(pszDir);
	rp.hdr.filesize += sizeof(rp.hdr) + sizeof(footer);
	if (enum_dir(pszDir, &rp) == 0)
	{
		/* Now that we collected all files, write new file */
		FILE *fpOut;
		unsigned int left;

		if ((fpOut = fopen(pszNew, "wb")))
		{
			fwrite(&rp.hdr, sizeof(rp.hdr), 1, fpOut);
			fseek(fpOut, rp.hdr.nImages*sizeof(rp.rgHdrs[0]), SEEK_CUR);
			for (i=0; i<rp.hdr.nImages; i++)
			{
				sprintf(szOutFile, "%s%s", pszDir, rp.rgHdrs[i].szFileName);
				printf ("%s\n", szOutFile);
				if (!(fpOld = fopen(szOutFile, "rb")))
				{
					fprintf(stderr, "Cannot open file %s, aborted: %s\n", szOutFile, strerror(errno));
					break;
				}
				rp.rgHdrs[i].offset = ftell(fpOut);
				left = copy_file(fpOld, fpOut, &rp.rgHdrs[i]);
				fclose(fpOld);
				if (left) break;
			}
			fwrite(&footer, sizeof(footer), 1, fpOut);
			if (i==rp.hdr.nImages)
			{
				fseek(fpOut, sizeof(rp.hdr), SEEK_SET);
				fwrite(rp.rgHdrs, rp.hdr.nImages, sizeof(rp.rgHdrs[0]), fpOut);
				iRet=0;
			}
			fclose(fpOut);
		}
		else
		{
			fprintf(stderr, "Cannot create file %s: %s\n", pszNew, strerror(errno));
		}
	}
	free (rp.rgHdrs);
	return iRet;
}

int unpack(char *pszFile, char *pszDir)
{
	FILE *fpOld, *fpOut;
	int iRet=-1;
	unsigned int i, left;
	char szOutFile[PATH_MAX], *p;
	long lOffsHdrTbl;
	FLS_HDR hdr;
	FLS_BLK_HDR blkhdr;

	remove_slash(pszDir);
	if ((fpOld=fopen(pszFile, "rb")))
	{
		if (read_hdr(fpOld, pszFile, &hdr) == 0)
		{
			for (i=0, lOffsHdrTbl=ftell(fpOld); i<hdr.nImages; i++)
			{
				if (seek_file (fpOld, lOffsHdrTbl, i, &blkhdr)<0) break;
				sprintf(szOutFile, "%s/%s", pszDir, blkhdr.szFileName);
				if ((p=strrchr(blkhdr.szFileName, '/')))
				{
					for (p=szOutFile+strlen(pszDir)+1; p&&*p; p=strchr(p+1,'/'))
					{
						*p=0;
						mkdir(szOutFile, 0777);
						*p='/';
					}
				}
				if (!(fpOut = fopen(szOutFile, "w")))
				{
					fprintf(stderr, "Cannot create file %s: %s\n", szOutFile, strerror(errno));
					continue;
				}
				printf("%s\n", szOutFile);
				if (fseek(fpOld, blkhdr.offset, SEEK_SET))
				{
					fprintf(stderr, "Cannot seek to offset %08X of FAT entry #%d: %s\n", blkhdr.offset, i, strerror(errno));
					break;
				}
				left = copy_file(fpOld, fpOut, &blkhdr);
				fclose(fpOut);
				if (left) break;
			}
			if (i==hdr.nImages) iRet=0;
		}
		fclose(fpOld);
	}
	else
		fprintf(stderr, "Cannot open FW file %s for reading: %s\n", pszFile, strerror(errno));
	return iRet;
}

int main(int argc, char **argv)
{

	printf ("Simple Smtsec repacker V0.2 - leecher@dose.0wnz.at\n\n");
	if (argc>=3 && argv[1][0]=='-')
	{
		switch (argv[1][1])
		{
		case 'u':	return unpack(argv[2], argv[3])<0?EXIT_FAILURE:EXIT_SUCCESS;
		case 'r':
			if (argc>=4) return repack(argv[2], argv[3], argv[4])<0?EXIT_FAILURE:EXIT_SUCCESS;
			break;
		case 'p':
			return repack(NULL, argv[2], argv[3])<0?EXIT_FAILURE:EXIT_SUCCESS;
		}
	}
	return usage(argv[0]);
}
