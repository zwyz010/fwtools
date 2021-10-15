#!/bin/sh
UBIIMG=$1 # ubifs image
MP="fs" # mount point
MTDBLOCK="/dev/mtd0" # MTD device file
UMNT=""
 
echo "$0" | grep unmount_ >/dev/null 2>&1
[ $? -eq 0 ] && UMNT=1
if [ $# -gt 1 -a x"$2"x = x"unmount"x ]; then
  UMNT=1
fi
 
if [ x"${UMNT}"x = x""x ]; then
  modprobe nandsim first_id_byte=0x20 second_id_byte=0xaa third_id_byte=0x00 fourth_id_byte=0x15
#  if [ ! -b ${MTDBLOCK} ] ; then
#    mknod ${MTDBLOCK} c 90 0 || exit 1
#  fi
  dd if=${UBIIMG} of=${MTDBLOCK} bs=2048
  modprobe ubi mtd=0
  [ ! -d ${MP} ] && mkdir -p ${MP}
  mount -t ubifs  /dev/ubi0_0 ${MP}
else
  umount ${MP}
  if [ $? -ne 0 ]; then
    echo "Cannot unmount UBIFS at $MP"
  fi
  modprobe -r ubifs
  modprobe -r ubi
  modprobe -r nandsim
fi
