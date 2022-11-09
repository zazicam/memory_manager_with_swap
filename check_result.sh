#!/bin/bash

inputDir="./input"
outputDir="./output"

function help {
	echo
	echo "This script checks that all files from input folder ('$inputDir' by default)"
	echo "were copied to the output folder ('$outputDir' by default)." 
	echo "You can give it pathes to other folders using comand line arguments."
	echo "Example: "
	echo "	$0 [input folder] [output folder]"
}

if [ $# -eq 2 ]; then
	inputDir=$1
	outputDir=$2
fi

if [ ! -d "$inputDir" ]; then
	echo "$inputDir does not exist."
	help
	exit 1
fi

if [ ! -d "$outputDir" ]; then
	echo "$outputDir does not exist."
	help
	exit 1
fi

error=false
for file in $inputDir/*; do
	filename=$(basename "$file")
	inputFile="$inputDir/$filename"
	outputFile="$outputDir/$filename"
	echo -n "$filename ... " 
	if [ -f "$inputFile" ]; then
		if [ -f "$outputFile" ]; then
			DIFF=$(diff "$inputFile" "$outputFile")
			if [ "$DIFF" == "" ]; then
				echo "[+]" 
			else
				echo "[ERROR: wrong file]"
				error=true	
			fi	
		else
			echo "[ERROR: no such file]"
			error=true	
		fi
	else
		echo "[skip - not a file]"
	fi
done

echo
if [ "$error" = true ]; then
	echo "The are errors!"
	exit 1
else
	echo "[Ok]"
	exit 0
fi
