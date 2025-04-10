#!/bin/bash

# Fallback objcopy path.
FBOBJCOPY=/usr/bin/objcopy
# It isn't guaranteed that this is set.
if [ -n "$STEAM_RUNTIME_PATH" ]; then
	OBJCOPY=$STEAM_RUNTIME_PATH/bin/objcopy
	if [ ! -f "$OBJCOPY" ]; then
		OBJCOPY=$FBOBJCOPY
	fi
else
	OBJCOPY=$FBOBJCOPY
fi

function usage {
	echo "$0 /path/to/input/file [-o /path/to/output/file ]"
	echo ""
}

if [ $# == 0 ]; then
	usage
	exit 2
fi

if [ $(basename $1) == $1 ]; then
	INFILEDIR=$PWD
else
	INFILEDIR=$(cd ${1%/*} && echo $PWD)
fi
INFILE=$(basename $1)

OUTFILEDIR=$INFILEDIR
OUTFILE=$INFILE.dbg

while getopts "o:" opt; do
	case $opt in
		o)
			OUTFILEDIR=$(cd ${OPTARG%/*} && echo $PWD)
			OUTFILE=$(basename $OPTARG)
			;;
	esac
done

if [ "$OUTFILEDIR" != "$INFILEDIR" ]; then
	INFILE=${INFILEDIR}/${INFILE}
	OUTFILE=${OUTFILEDIR}/${OUTFILE}
fi

pushd "$INFILEDIR"	
$OBJCOPY "$INFILE" "$OUTFILE"
$OBJCOPY --add-gnu-debuglink="$OUTFILE" "$INFILE"
popd


