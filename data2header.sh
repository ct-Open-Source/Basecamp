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

# :a;N;$!ba; will move all lines into the pattern space (internal buffer of sed)
# so that the next command will also remove the line breaks (which are also whitespaces)
# s/>\s*</></g' will remove all whitespaces between the HTML tags
# example: 
#"> 
#		<" becomes "><"
sed  -i ':a;N;$!ba;s/>\s*</></g' *.htm
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
