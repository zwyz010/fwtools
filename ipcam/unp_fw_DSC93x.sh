#/bin/sh
echo D-Link DCS-930, DCS-932, Trendnet TV-IP551, TV-IP651  Firmware unpacker
echo www.hardwarefetish.com
echo
if [ -z $1 ]; then 
  echo Usage: $0 Firmwarefile
  exit
fi
which binwalk >/dev/null
if [ $? -eq 1 ]; then
  echo You need the binwalk utility to unpack
  echo https://github.com/mirror/firmware-mod-kit/tree/master/src/binwalk-0.4.1
  exit
fi
if [ `binwalk 2>&1 | grep "Binwalk v" | cut -b 10` -gt 0 ]; then
    echo Note: Your binwalk version is above version 0, there is no alignment parameter
    echo available anymore, so this may not be working properly!
    echo Please use original C-based version instead of the Python-Rewrite.
    echo https://github.com/mirror/firmware-mod-kit/tree/master/src/binwalk-0.4.1
    ALIGN=""
else
    ALIGN="-b 0x1000"
fi
which dd >/dev/null
if [ $? -eq 1 ]; then
  echo Please install dd
  exit
fi
which lzma >/dev/null
if [ $? -eq 1 ]; then
  echo Please install lzma
  exit
fi

OFFS=`binwalk -y uimage $1  | tail -n 2 | cut -s -f1`
if [ -z $OFFS ]; then
  echo No Uimage found in $1
  exit
fi
dd if=$1 of=myfw.lzma bs=$(($OFFS + 64)) skip=1
lzma -d myfw.lzma
OFFS=`binwalk -y lzma $ALIGN myfw  | tail -n 2 | cut -s -f1`
if [ -z $OFFS ]; then
  echo No lzma compressed rom image found in $1
  rm myfw
  exit
fi
dd if=myfw of=myromfs.lzma bs=$OFFS skip=1
rm myfw
lzma -d myromfs.lzma
mkdir fs
cd fs
cpio -ivd --no-absolute-filenames -F../myromfs
cd ..
rm myromfs
echo unpacked filesystem in directory fs
