#!/bin/bash

echo "Converting files in folder \"data\" to C Header file data.h"

DATA="data/"
CURRDIR="$(pwd)"
TMPDIR="$CURRDIR/tmp/"
OUTFILE="$CURRDIR/data.hpp"


mkdir $TMPDIR
#change into data folder
cp $DATA/* $TMPDIR
cd $TMPDIR

yuicompressor -o '.css$:.css' *.css
yuicompressor -o '.js$:.js' *.js

sed  -i.bak ':a;N;$!ba;s/>\s*</></g' *.htm
rm *.bak
gzip *
cat > $OUTFILE <<DELIMITER
/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include <pgmspace.h>
//
// converted data/* to gzipped flash variables
//


DELIMITER

#convert contents into array of bytes
INDEX=0
for i in $(ls -1); do

	CONTENT=$(cat $i | xxd -i)
	CONTENT_LEN=$(echo $CONTENT | grep -o '0x' | wc -l)
	FILENAME=${i//[.]/_}
	printf "#define "$FILENAME"_len "$CONTENT_LEN"\n" >> $OUTFILE
	printf "const uint8_t "$FILENAME"[] PROGMEM {\n$CONTENT\n};" >> $OUTFILE
	echo >> $OUTFILE
	unset CONTENT
done
rm $TMPDIR/*
rmdir $TMPDIR
cd $CURRDIR
