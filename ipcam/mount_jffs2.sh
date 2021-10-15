#!/bin/sh
JFFSIMG=$1 # jffs image
MP="fs" # mount point
MTDBLOCK="/tmp/mtdblock0" # MTD device file
UMNT=""
 
echo "$0" | grep unmount_ >/dev/null 2>&1
[ $? -eq 0 ] && UMNT=1
if [ $# -gt 1 -a x"$2"x = x"unmount"x ]; then
  UMNT=1
fi
 
if [ x"${UMNT}"x = x""x ]; then
  if [ ! -b ${MTDBLOCK} ] ; then
    mknod ${MTDBLOCK} b 31 0 || exit 1
  fi
  modprobe mtdblock
  modprobe mtdram total_size=65536 erase_size=16
  modprobe jffs2
  dd if=${JFFSIMG} of=${MTDBLOCK}
  [ ! -d ${MP} ] && mkdir -p ${MP}
  mount -t jffs2 -oro ${MTDBLOCK} ${MP}
else
  umount ${MP}
  if [ $? -ne 0 ]; then
    echo "Cannot unmount JFFS2 at $MP"
  fi
  modprobe -r jffs2
  modprobe -r mtdram
  modprobe -r mtdblock
  modprobe -r mtdram
  modprobe -r mtdchar
  modprobe -r mtd
fi
