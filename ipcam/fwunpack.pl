#!/usr/bin/perl
# TrendNet firmware unpacker
# Matt Brain (matt.brain@gmail.com)
#
# Usage: fwunpack.pl
#
# Extracts: prostub.bin
# vmlinuz
# minix.gz
# autoboot.bat
# footer.bin
#
# Writes log to fwunpack.log

use strict;

my $scriptversion=1; #Version of this script

my $headerPointer=0x04; #Header reference in firmware
my $versionLocation=0x08; #Version location in firmware
my $md5Location=0x0c; #md5 location in firmware
my $md5Size=16; #md5 length
my $crcLocation=$md5Location+$md5Size; #crc location
my $crcSize=4; #crc length
my $pkgSizeLocation=$crcLocation+$crcSize; #package size location
my $headerSize=4096; #size of header found at $headerPointer

my $model="Unknown";
my $headerLocation=0x00;
my $tVersion=0x00;
my @version=(0,0,0,0);

my @chunkStart=(0);
my @chunkLength=(0);
my @chunkName=("vmlinuz", "minix.gz", "autoboot.bat");
my @chunk;

my $footer="";
my $prostub="";
my $numArgs=$#ARGV+1;




sub Usage {
print "Usage: fwunpack \n\n";
exit;
}



if ($numArgs<1) {
&Usage;
}

print "** fwunpack version $scriptversion **\n\n";
print "Opening $ARGV[0]\n";
open(FIRMWARE,"<$ARGV[0]");
binmode(FIRMWARE);

#read the header location
seek(FIRMWARE,$headerPointer,0);

read(FIRMWARE,$headerLocation,4) or die "FATAL: Couldn't read the header location\n";

#firmware location is stored in little endian format so need to fiddle the bits
$headerLocation = unpack('L<',$headerLocation);
printf "Header location at 0x%x\n",$headerLocation;

seek(FIRMWARE,$versionLocation,0);
read(FIRMWARE,$tVersion,4) or die "FATAL: Couldn't read the version\n";
@version=unpack('cccc',$tVersion);
printf "Firmware version $version[3].$version[2].$version[1].$version[0]\n";
#read the model name
seek(FIRMWARE,$headerLocation+4088,0);
read(FIRMWARE,$model,8);
print "Model $model\n";

my $chunkCursor=$headerLocation+4096;
#load the header
for (my $counter=0;$counter < 511;$counter++) {
seek(FIRMWARE,$headerLocation+($counter*8),0);
read(FIRMWARE,$chunkStart[$counter],4);
read(FIRMWARE,$chunkLength[$counter],4);
$chunkStart[$counter] = unpack('L<',$chunkStart[$counter]);
$chunkLength[$counter] = unpack('L<',$chunkLength[$counter]);
if ($chunkStart[$counter]>0) {
printf "%s found, deploy at:0x%x length:%d\n",$chunkName[$counter],$chunkStart[$counter],$chunkLength[$counter];
seek(FIRMWARE,$chunkCursor,0);
read(FIRMWARE,$chunk[$counter],$chunkLength[$counter]);
$chunkCursor = tell(FIRMWARE);
open(CHUNK,">$chunkName[$counter]") or die $!;
binmode CHUNK;
print CHUNK $chunk[$counter];
close CHUNK;
}
}

seek(FIRMWARE,$chunkCursor,0);
print "writing footer.bin\n";

read(FIRMWARE,$footer,64);
open(FOOTER,'>footer.bin');
binmode FOOTER;
print FOOTER $footer;
close FOOTER;

print "writing prostub.bin\n";

seek(FIRMWARE,0,0);
read(FIRMWARE,$prostub,$headerLocation);
open(PROSTUB,'>prostub.bin');
binmode PROSTUB;
print PROSTUB $prostub;
close PROSTUB;

close FIRMWARE;