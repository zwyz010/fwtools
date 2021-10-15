#/bin/sh
echo D-Link DCS-56x5, DCS-3411, DCS-3430 Firmware unpacker
echo www.hardwarefetish.com
echo
if [ -z $1 ]; then 
  echo Usage: $0 Firmwarefile
  exit
fi
which binwalk >/dev/null
if [ $? -eq 1 ]; then
  echo You need the binwalk utility to unpack
  echo http://code.google.com/p/binwalk/
  exit
fi

OFFS=`binwalk -y cramfs $1  | tail -n 2 | cut -s -f1`
if [ -z $OFFS ]; then
    which gcc >/dev/null
    if [ $? -eq 1 ]; then
      echo Please install gcc
      exit
    fi
    gcc -o unpfw -m32 -xc - <<EOF 
#include <stdio.h>

static int skip_package_script (FILE *fp)
{
    char szBuf[256];

    while (fgets(szBuf, sizeof(szBuf), fp))
    {
        if (strncmp(szBuf, "=== Firmware Boundary ===\n", 26)==0)
            return 0;
    }
    return -1;
}

static void usage(char *pszApp)
{
    fprintf (stderr, "%s <firmware file> <output file>\n", pszApp);
}

int main(int argc, char **argv)
{
    FILE *fp, *fpDst;
    int iRet = -1;

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

    if (!(fpDst = fopen(argv[2], "wb")))
    {
        fclose (fp);
        perror("Cannot open output file");
        return -1;
    }

    if (skip_package_script(fp) == 0)
    {
        unsigned long buf[256];
        int i, read;

        fseek (fp, 0x400, SEEK_CUR);
        while (read = fread (buf, 1, sizeof(buf), fp))
        {
            for (i=0; i<sizeof(buf)/sizeof(long); i++) buf[i]=~buf[i];
            fwrite (buf, 1, read, fpDst);
        }
        printf ("%s written.\n", argv[2]);
        iRet = 0;
    } else fprintf (stderr, "Firmware boundary not found.\n");
    fclose (fp);
    fclose (fpDst);
    return iRet;
}
EOF
DUMPF=fwfixed.dmp
./unpfw $1 $DUMPF
rm unpfw
else
DUMPF=$1
fi

binwalk -y cramfs --dd=cramfs:cramfs:1 -q $DUMPF
rm fwfixed.dmp 2>/dev/null
mkdir fs 2>/dev/null
mount -o loop,ro *.cramfs fs/
echo *.cramfs mounted at fs/
echo Do not foget to umount fs
