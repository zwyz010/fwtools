/*
 *  Repacker for tenvis Camera firmware images
 *
 *  (C)oded by leecher@dose.0wnz.at        01/2014
 *  http://www.hardwarefetish.com
 *
 *  gcc -O3 -Wall -o tenvis_pack tenvis_pack.c
 *
 *
 *  Sample usage to repack tenvis.pk2 firmware image:
 *  --------------------------------------------------------------------
 *  mkdir unpack
 *  ./tenvis_pack -u -f tenvis.pk2 -d unpack/ -c tenvis.ctl
 *  vi tenvis.ctl
 *  ./tenvis_pack -p -f myfw.pk2 -d unpack/ -c tenvis.ctl
 *
 *
 *  .ctl file format:
 * ---------------------------------------------------------------------
 * Each line contains a 4 byte command word followed by data.
 * The file has to start with the TYPE command word.
 * The bracktes [] in the examples are just to show line start and
 * ending, do not write them!
 * 
 * TYPE: Specifies the camera chip type in the header. This is
 *       followed by an integer specifying the type, usually this is one
 *       of the values in function cam_type().
 *       Example:  [TYPE8128]
 *
 * CMD : Specifies a shell command to execute during the update. 
 *       As the command word is 4 bytes, note the BLANK after CMD, so
 *       it is CMD[blank]!
 *       The line has to have a blank as the last character in the line
 *       before CR!
 *       Example: [CMD killall monitor.sh ]
 *
 * FILE: Specifies a file that needs to be included in the firmware image.
 *       Note that the path has to be exactly the path that it needs to
 *       be put in in the target system!
 *       Only use absolute paths starting with /
 *       Normally you may want to write to /mnt/mtd/
 *       The path will be taken relative to the unpack directory you 
 *       specified on command line
 *       Example: [FILE/mnt/mtd/gmdvr_mem.cfg.720p]
 *       If you start with "-d unpack", file will be searched in 
 *       unpack/mnt/mtd/gmdvr_mem.cfg.720p
 *
 * Short meaningless sample .ctl file:
 * TYPE8128
 * CMD killall monitor.sh 
 * FILE/mnt/mtd/boot.sh
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/stat.h>

static const unsigned char xortab[8] = {0xA1, 0x83, 0x24, 0x78, 0xB3, 0x41, 0x43, 0x56};

#pragma pack(1)

/* File header */
typedef struct {
	uint32_t dwFileHeader;      // PK2
	uint32_t dwCameraType;      // Camera type
	uint32_t tCreation;           // Creation timestamp of firmware update file
	unsigned char version[4];   // Version number of firmware, seems to be 0.0.0.0
	unsigned char reserved[8];  // Some reserved space
	uint32_t dwSections;        // Number of sections in file
} FW_FILE_HDR;

/* Section header */
typedef struct {
	uint32_t dwSectHdr;         // Section header, either FILE or CMD
	unsigned char hash[16];     // Hash for section
	uint32_t cbSect;            // Length of section in bytes
} FW_SECT_HDR;
/* For dwSectHdr == "FILE":
 * The next DWORD is the length of the filename.
 * The next bytes are the file name with the length given in dwLen
 * This is finally followed by a DWORD with the content length and the real file content
 * "encrypted" via XOR
 */
#pragma pack()

/******** The MD5 stuff, code taken from md5.c by Colin Plumb ********/
struct MD5Context 
{
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
};

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

static void 
MD5Transform(uint32_t buf[4],
	     uint32_t in[16])
{
  uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}


/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void MD5Init(struct MD5Context *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
MD5Update(struct MD5Context *ctx,
	  const void *data,
	  unsigned len)
{
    const unsigned char *buf = data;
    uint32_t t;

    /* Update bitcount */

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++; 	/* Carry from low to high */
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += t;
	len -= t;
    }
    /* Process data in 64-byte chunks */

    while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += 64;
	len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void MD5Final(unsigned char digest[16],
	      struct MD5Context *ctx)
{
    unsigned count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in, 0, 56);
    } else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
    }

    /* Append length in bits and transform */
    ((uint32_t *) ctx->in)[14] = ctx->bits[0];
    ((uint32_t *) ctx->in)[15] = ctx->bits[1];

    MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(struct MD5Context));        /* In case it's sensitive */
}
/* end of md5.c */


/******** And here we go ... *******/

static char *cam_type(uint32_t dwCameraType)
{
	static char szCam[8];

	switch(dwCameraType)
	{
	case 8126: return "GM8126";
	case 8128: return "GM8128";
	case 3516: return "Hi3516";
	case    0: return "Hi3510";
	case  365: return "DM365";
	case 3512: return "Hi3512";
	default  : sprintf (szCam, "%d", dwCameraType); 
	}
	return szCam;
}

static void show_fw_info(FW_FILE_HDR *file_hdr)
{
	time_t t = file_hdr->tCreation;
	struct tm *tm;

	tm = localtime(&t);
	printf ("Firmware created: %d-%02d-%02d %02d:%02d:%02d\n"
		"Camera type: %s\n"
		"Version: %d.%d.%d.%d\nSections: %d\n",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		cam_type(file_hdr->dwCameraType),
		file_hdr->version[0], file_hdr->version[1], file_hdr->version[2], file_hdr->version[3],
		file_hdr->dwSections
	);
}

static int unpack(FILE *fp, FILE *fpCtl, char *pszUnpDir)
{
	struct MD5Context ctx;
	FILE *fpOut = NULL;
	FW_FILE_HDR file_hdr;
	FW_SECT_HDR sect_hdr;
	unsigned int i;
	unsigned char buf[4000], *pPayload=&buf[4], digest[16];
	char  szFile[PATH_MAX]={0};
	uint32_t *pcbPayload = (uint32_t*)buf, len_data;

	if (!fread(&file_hdr, sizeof(file_hdr), 1, fp))
	{
		fprintf (stderr, "Cannot read entire firmware file header.\n");
		return 1;
	}
	if (file_hdr.dwFileHeader != *(uint32_t*)"PK2")
	{
		fprintf (stderr, "Not a valid PK2 file (header mismatch).\n");
		return 1;
	}
	show_fw_info(&file_hdr);
	if (!file_hdr.dwSections)
	{
		fprintf (stderr, "There are no sections to unpack in the file\n");
		return 1;
	}
	if (fpCtl) fprintf (fpCtl, "TYPE%d\n",file_hdr.dwCameraType);

	for (i=0; i<file_hdr.dwSections; i++)
	{
		if (!fread(&sect_hdr, sizeof(sect_hdr), 1, fp))
		{
			fprintf (stderr, "Cannot read section header @%08lX\n", ftell(fp));
			return 1;
		}
		MD5Init(&ctx);
		if (sect_hdr.dwSectHdr == *(uint32_t*)"FILE")
		{
			uint32_t *pcbData;

			if (!fread(pcbPayload, 4, 1, fp))
			{
				fprintf (stderr, "Cannot read size of filename in section %d @%08lX\n", i, ftell(fp));
				return 1;
			}
			if (*pcbPayload > sizeof(buf)-4 || !fread(pPayload, *pcbPayload, 1, fp))
			{
				fprintf (stderr, "Cannot read filename in section %d @%08lX\n", i, ftell(fp));
				return 1;
			}
			printf ("Processing file %s\n", pPayload);
			if (fpCtl) fprintf(fpCtl,"FILE%s\n",pPayload);
			snprintf (szFile, sizeof(szFile), "%s%s", pszUnpDir, pPayload);
			pcbData=(uint32_t*)((unsigned char*)pPayload+*pcbPayload);
			if (!fread(pcbData, 4, 1, fp))
			{
				fprintf (stderr, "Cannot read size of data in section %d @%08lX\n", i, ftell(fp));
				return 1;
			}
			if (*pcbData + *pcbPayload + 8 != sect_hdr.cbSect)
			{
				fprintf (stderr, "Section length differs from section header + file length (%d!=%d) in section %d @%08lX\n", 
					*pcbData + *pcbPayload + 8, sect_hdr.cbSect, i, ftell(fp));
				return 1;
			}
			len_data = *pcbData;
			MD5Update(&ctx, buf, *pcbPayload + 8);
		}
		else if (sect_hdr.dwSectHdr == *(uint32_t*)"CMD")
		{
			len_data = sect_hdr.cbSect;
			printf ("CMD ");
			if (fpCtl) fprintf(fpCtl,"CMD ");
		}
		else
		{
			fprintf (stderr, "Unknown section header: %08X\n", sect_hdr.dwSectHdr);
			continue;
		}

		/* Decrypt contents */
		while (len_data > 0)
		{
			uint32_t cbRead = len_data<=4000?len_data:4000, j;

			if ((cbRead = fread(buf, 1, cbRead, fp)) <= 0) break;
			for (j=0; j<cbRead; j++)
				buf[j] ^= xortab[j%8];
			MD5Update(&ctx, buf, cbRead);
			len_data -= cbRead;
			/* If content type is file, write decrypted stuff to output directory */
			if (sect_hdr.dwSectHdr == *(uint32_t*)"FILE")
			{
				if (!fpOut)
				{
					char szDir[PATH_MAX]={0}, *p;

					/* Create target directory */
					for (p=szFile+strlen(pszUnpDir)+1; *p && (p=strchr(p, '/')); p++)
					{
						strncpy (szDir, szFile, p-szFile);
						mkdir (szDir, S_IRWXU);
					}
					/* Create file in target directory */
					if (!(fpOut = fopen(szFile, "w")))
					{
						fprintf(stderr, "Aborting: Cannot create output file %s: %s\n", szFile, strerror(errno));
						return 1;
					}
				}
				if (!fwrite (buf, cbRead, 1, fpOut))
				{
					fprintf(stderr, "Aborting: Cannot write data to output file %s%s: %s\n", 
						pszUnpDir, pPayload, strerror(errno));
					return 1;
				}
			}
			else if (sect_hdr.dwSectHdr == *(uint32_t*)"CMD")
			{
				printf ("%.*s", cbRead, buf);
				if (fpCtl) fprintf(fpCtl,"%.*s", cbRead, buf);
			}
		}
		if (fpOut)
		{
			fclose(fpOut);
			fpOut = NULL;
		}

		/* Validate MD5 digest */
		MD5Final(digest, &ctx);
		if (memcmp(digest, sect_hdr.hash, sizeof(digest)))
		{
			int j;

			fprintf (stderr, "Error decoding section %d: MD5 Digests do not match:\nCalculated: ", i);
			for (j=0; j<sizeof(digest); j++) fprintf (stderr, "%02X", digest[j]);
			fprintf (stderr, "\nShould be: ");
			for (j=0; j<sizeof(sect_hdr.hash); j++) fprintf (stderr, "%02X", sect_hdr.hash[j]);
			fprintf (stderr, "\n");
		}
	}
	printf ("All sections processed.\n");
	return 0;
}

static int pack(FILE *fp, FILE *fpCtl, char *pszUnpDir)
{
	struct MD5Context ctx;
	FW_FILE_HDR file_hdr={0};
	FW_SECT_HDR sect_hdr;
	unsigned int i;
	char buf[4000], *pPayload=&buf[4];
	uint32_t *pcbPayload = (uint32_t*)buf;
	time_t t;

	/* Read camera type from control file */
	if (!fgets (buf, sizeof(buf), fpCtl))
	{
		fprintf (stderr, "Control file doesn't contain a single line\n");
		return 1;
	}
	if (*pcbPayload != *(uint32_t*)"TYPE")
	{
		fprintf (stderr, "First declaration in control file must be camera TYPE\n");
		return 1;
	}

	/* Prepare file header and write it to file, sections will be updated later */
	sscanf(pPayload, "%d", &file_hdr.dwCameraType);
	time(&t);
	file_hdr.tCreation=(uint32_t)t;
	file_hdr.dwFileHeader = *(uint32_t*)"PK2";
	if (!fwrite (&file_hdr, sizeof(file_hdr), 1, fp))
	{
		perror("Cannot write file header");
		return 1;
	}

	while (fgets(buf, sizeof(buf), fpCtl))
	{
		memset (&sect_hdr, 0, sizeof(sect_hdr));
		if (*pcbPayload == *(uint32_t*)"CMD ")
		{
			/* Write a CMD packet section header */
			printf ("Writing CMD %s", pPayload);
			sect_hdr.dwSectHdr = *(uint32_t*)"CMD";
			sect_hdr.cbSect    = strlen(pPayload)+1;
			MD5Init(&ctx);
			MD5Update(&ctx, pPayload, sect_hdr.cbSect);
			MD5Final(sect_hdr.hash, &ctx);
			if (!fwrite (&sect_hdr, sizeof(sect_hdr), 1, fp))
			{
				perror("Cannot write section header");
				return 1;
			}

			/* Write encrypted CMD */
			for (i=0; i<sect_hdr.cbSect; i++)
				pPayload[i] ^= xortab[i%8];
			if (!fwrite(pPayload, sect_hdr.cbSect, 1, fp))
			{
				perror("Cannot write CMD packet");
				return 1;
			}
			file_hdr.dwSections++;
		}
		else if (*pcbPayload == *(uint32_t*)"FILE")
		{
			long sect_pos;
			FILE *fpIn = NULL;
			size_t len;
			char  szFile[PATH_MAX]={0}, *p;
			uint32_t len_data;

			/* Write a FILE packet section header */
			printf ("Writing FILE %s", pPayload);
			sect_hdr.dwSectHdr = *(uint32_t*)"FILE";
			for (p = pPayload+strlen(pPayload)-1; *p=='\r' || *p=='\n'; p--) *p=0;
			snprintf (szFile, sizeof(szFile), "%s%s", pszUnpDir, pPayload);
			if (!(fpIn = fopen(szFile, "r")))
			{
				fprintf (stderr, "Cannot open required input file %s\n", szFile);
				continue;
			}
			if (fseek(fpIn, 0, SEEK_END)<0 || !(sect_pos = ftell(fpIn)))
			{
				fprintf (stderr, "Cannot get file length of input file %s: %s\n", szFile, strerror(errno));
				continue;
			}
			len_data=(uint32_t)sect_pos;
			fseek(fpIn, 0, SEEK_SET);
			*pcbPayload = strlen(pPayload)+1;
			sect_hdr.cbSect = *pcbPayload+len_data+8;
			sect_pos = ftell(fp);
			if (!fwrite (&sect_hdr, sizeof(sect_hdr), 1, fp) ||
				!fwrite (buf, *pcbPayload+4, 1, fp) ||
				!fwrite (&len_data, 4, 1, fp))
			{
				perror("Cannot write section header");
				fclose(fpIn);
				return 1;
			}
			MD5Init(&ctx);
			MD5Update(&ctx, buf, *pcbPayload+4);
			MD5Update(&ctx, &len_data, 4);
			while ((len=fread(buf, 1, sizeof(buf), fpIn)))
			{
				MD5Update(&ctx, buf, len);
				for (i=0; i<len; i++)
					buf[i] ^= xortab[i%8];
				if (!fwrite (buf, len, 1, fp))
				{
					perror("Cannot write file contents");
					fclose(fpIn);
					return 1;
				}
			}
			fclose(fpIn);
			MD5Final(sect_hdr.hash, &ctx);
			fseek(fp, sect_pos, SEEK_SET);
			if (!fwrite (&sect_hdr, sizeof(sect_hdr), 1, fp))
			{
				perror("Cannot update section header");
				return 1;
			}
			fseek(fp, 0, SEEK_END);
			file_hdr.dwSections++;
		}
		else
		{
			fprintf(stderr, "Unknown identifier in control file in line: %s\n", buf);
		}
	}
	fseek(fp, 0, SEEK_SET);
	if (!fwrite (&file_hdr, sizeof(file_hdr), 1, fp))
	{
		perror("Cannot update file header");
		return 1;
	}
	return 0;
}

static int usage(char *pszMe)
{
	fprintf (stderr, "Usage:\n%s <-u|-p> <-f Firmwarefile> [-d Directory] [-c Control-File]\n\n"
	"\t-u\tUnpack firmware\n"
	"\t\t-f\tPK2 firmware file to unpack\n"
	"\t\t-d\tDirectory where unpacked files are written to,\n\t\t\tmust exist, default is .\n"
	"\t\t-c\tOptional control-file to create for repacking\n"
	"\t-p\tPack new firmware\n"
	"\t\t-f\tPK2 firmware file to create\n"
	"\t\t-d\tDirectory where files to be packed reside, paths\n\t\t\tin control-file are relative to this, default is .\n"
	"\t\t-c\tMandatory control-file that contains cam-type, files and cmds\n",
	pszMe);
	return 1;
}

int main (int argc, char **argv)
{
	FILE *fp, *fpCtl=NULL;
	int i, ret;
	char mode=0, *pszFW=NULL, *pszDir=".", *pszCTL=NULL;

	printf ("TENVIS firmware unpacker V0.1 - leecher@dose.0wnz.at\n\n");
	/* Parse commandline */
	if (argc<3) return usage(argv[0]);
	for (i=1; i<argc; i++)
	{
		if (argv[i][0]=='-')
		{
			switch(argv[i][1])
			{
			case 'h':
			case '?':
				return usage(argv[0]);
			case 'u':
			case 'p':
				mode=argv[i][1];
				break;
			case 'f':
				if (argc<=++i)
				{
					fprintf (stderr, "Missing firmwarefile argument!\n\n");
					return usage(argv[0]);
				}
				pszFW=argv[i];
				break;
			case 'd':
				if (argc<=++i)
				{
					fprintf (stderr, "Missing directory argument!\n\n");
					return usage(argv[0]);
				}
				pszDir=argv[i];
				break;
			case 'c':
				if (argc<=++i)
				{
					fprintf (stderr, "Missing control file argument!\n\n");
					return usage(argv[0]);
				}
				pszCTL=argv[i];
				break;
			}
		}
	}

	/* Input validation */
	if (!pszFW)
	{
		fprintf (stderr, "You need to specify a valid firmware file!\n\n");
		return usage(argv[0]);
	}

	switch (mode)
	{
	case 'u':
		break;
	case 'p':
		if (!pszCTL)
		{
			fprintf (stderr, "You need a control file for packing an firmware image!\n\n");
			return usage(argv[0]);
		}
		break;
	default:
		fprintf (stderr, "You need to tell me what you want me to do!\n\n");
		return usage(argv[0]);
	}

	if (access(pszDir, F_OK)<0)
	{
		fprintf (stderr, "Cannot access directory %s\n", pszDir);
		return 1;
	}

	/* Now do it! */
	if (!(fp=fopen(pszFW, mode=='u'?"r":"w")))
	{
		fprintf(stderr, "Cannot open firmware file %s for %s: %s\n", pszFW, mode=='u'?"reading":"writing", strerror(errno));
		return 1;
	}
	if (pszCTL)
	{
		if (!(fpCtl=fopen(pszCTL, mode=='u'?"w":"r")))
		{
			fprintf(stderr, "Cannot open control file %s for %s: %s\n", pszCTL, mode=='u'?"writing":"reading", strerror(errno));
			fclose(fp);
			return 1;
		}
	}
	if (mode=='u')	ret = unpack(fp, fpCtl, pszDir);
	else ret = pack(fp, fpCtl, pszDir);
	fclose(fp);
	if (fpCtl) fclose(fpCtl);
	return ret;
}
