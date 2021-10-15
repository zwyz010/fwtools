#/bin/sh
echo D-Link DCS-6111, DCS3110 Firmware unpacker
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
which dd >/dev/null
if [ $? -eq 1 ]; then
  echo Please install dd
  exit
fi

RDFIL=myinitrd
OFFS=`binwalk -y gzip $1 | grep initrd  | tail -n 2 | cut -s -f1`
if [ -z $OFFS ]; then
  echo Trendnet cams have Minix rootfs, but better use fwunpack.pl from http://hackingthetrendnet312w.blogspot.co.at/
  OFFS=`binwalk -y gzip $1 | grep rootfs | tail -n 2 | cut -s -f1`
  if [ -z $OFFS ]; then
    echo No rootfs and no initrd found in $1
    exit
  fi
  RDFIL=myrootfs
fi
dd if=$1 of=$RDFIL.gz bs=$OFFS skip=1
gunzip $RDFIL.gz
mkdir fs 2>/dev/null
mount $RDFIL fs/
echo $RDFIL mounted at fs/
echo Do not foget to umount fs
