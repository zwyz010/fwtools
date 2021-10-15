#/bin/sh
echo eneo PXD-2018PTZ unpacker
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
if [ ! -e mount_ubifs.sh ]; then
  echo You need mount_ubifs.sh to mount the image
  exit
fi

OFFS=`binwalk  --raw-bytes=UBI# $1 | head -n 4  | tail -n 2 | cut -s -f1`
dd if=$1 of=myubifs bs=$OFFS skip=1
mkdir fs 2>/dev/null
./mount_ubifs.sh myubifs
echo myubifs mounted at fs/
echo Do not foget to unmount with
echo ./mount_ubifs.sh myubifs unmount

