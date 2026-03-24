#!/bin/sh
# Writer script for assignment 1
# Author: Alexandru Cohal

# Get the write file and the write string.
if [ $# -lt 2 ]
then
	echo "Not enough arguments. Exiting."
	exit 1
fi

writefile=$1
writestr=$2

# Get the directory where the write file should be placed. If it doesn't exist, create it.
writedir=$( dirname "$writefile" )
if [ ! -d "$writedir" ]
then
	mkdir -p "$writedir"
fi

# Write to write file the write string.
echo "${writestr}" > "$writefile"

# Check if the write file was created.
if [ ! -f "$writefile" ]
then
	echo "The file could not be created. Exiting."
	exit 1
fi
