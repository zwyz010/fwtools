#/bin/sh
echo Trendnet TV-IP572PI, D-Link DCS-942L, DCS-5211L, DCS-5222L Firmware unpacker
echo www.hardwarefetish.com
echo
if [ -z $1 ]; then 
  echo Usage: $0 Firmwarefile
  exit
fi
which gcc >/dev/null
if [ $? -eq 1 ]; then
  echo Please install gcc
  exit
fi

gcc -o unpfw -m32 -xc - <<EOF 
#include <stdio.h>
#include <byteswap.h>

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
            for (i=0; i<sizeof(buf)/sizeof(long); i++) buf[i]=__bswap_32(buf[i]);
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
./unpfw $1 zImage
rm unpfw

if [ -e ./repack-zImage.sh ]; then
  ./repack-zImage.sh -u zImage
  rm zImage
else
  echo You should really use repack-zImage from http://forum.xda-developers.com/showthread.php?t=901152
  echo to unpack and also repack the zImage.
  echo
  echo Will try to use other method...
  echo

  which binwalk >/dev/null
  if [ $? -eq 1 ]; then
    echo No binwalk utility available, alternative unpacking also failed.
    echo Get binwalk at http://code.google.com/p/binwalk/
    exit
  fi
  which dd >/dev/null
  if [ $? -eq 1 ]; then
    echo Please install dd
    exit
  fi
  which gunzip >/dev/null
  if [ $? -eq 1 ]; then
    echo Please install gunzip
    exit
  fi
  which cpio >/dev/null
  if [ $? -eq 1 ]; then
    echo Please install cpio
    exit
  fi

  OFFS=`binwalk -y gzip fwfixed.dmp  | tail -n 2 | cut -s -f1`
  dd if=fwfixed.dmp of=myfw.gz bs=$OFFS skip=1
  rm fwfixed.dmp
  gunzip myfw.gz
  OFFS=`binwalk -y gzip myfw  | tail -n 2 | cut -s -f1`
  dd if=myfw of=mypkg.gz bs=$OFFS skip=1
  rm myfw
  gunzip mypkg.gz
  mkdir fs
  cd fs
  cpio -ivd --no-absolute-filenames -F../mypkg
  cd ..
  rm mypkg
  echo unpacked filesystem in directory fs
fi
