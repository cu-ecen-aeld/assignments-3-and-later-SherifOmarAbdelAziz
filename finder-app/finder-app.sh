#!/bin/sh
# assignment 1
# Author: Sherif Omar


if [ $# -lt 2 ]
then
	if [ $# -eq 0 ]
	then
		echo "No parameters specified. Pass filesdir and searchstr as parameters."
		exit 1
	elif [ $# -eq 1 ] 
	then
		echo "Second Parameter 'searchstr' is missing."
		exit 1
	 
	fi
else
	echo "Directory Path:" $1
	echo "Search String:" $2
	
	if [ -d $1 ]
	then
		echo "Valid directory path, directory exists."
		
		NumberOfMatchingFiles=$(grep -r -m 1 $2 $1 | wc -l)
		NumberofMatchingLines=$(grep -r $2 $1 | wc -l)
		
		echo "The number of files are $NumberOfMatchingFiles and the number of matching lines are $NumberofMatchingLines"
		exit 0
	else
		echo "Invalid directory path, directory does not exist."
		exit 1
	fi
	exit 0
fi
			
