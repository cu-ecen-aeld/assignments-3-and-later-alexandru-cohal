#!/bin/sh
# Finder script for assignment 1
# Author: Alexandru Cohal

# Get the target directory and the search string.
if [ $# -lt 2 ]
then
	echo "Not enough arguments. Exiting."
	exit 1
fi

filesdir=$1
searchstr=$2

# Check if the target directory exists.
if [ ! -d "$filesdir" ]
then
	echo "The first argument is not a directory. Exiting."
	exit 1
fi

# Get the number of files from the target directory.
numfiles=$(find "$filesdir" -type f | wc -l)

# Get the number of search string occurrences.
nummatchinglines=$(grep -r -o -F "$searchstr" "$filesdir" | wc -l)

# Print the results.
echo "The number of files are ${numfiles} and the number of matching lines are ${nummatchinglines}"
