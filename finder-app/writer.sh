#!/bin/sh
# assignment 1
# Author: Sherif Omar


if [ $# -lt 2 ]
then
	if [ $# -eq 0 ]
	then
		echo "No parameters specified. Pass writefile and writestr as parameters."
		exit 1
	elif [ $# -eq 1 ] 
	then
		echo "Second Parameter 'writestr' is missing."
		exit 1
	 
	fi
else
	echo "File Path:" $1
	echo "String:" $2
	
	touch $1
	
	if [ -f $1 ]
	then
		echo "File created successfully."
		echo $2 >> $1
		exit 0
	else
		echo "File is not created, exiting."
		exit 1
	fi
	exit 0
fi
