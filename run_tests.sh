#!/bin/bash

if [ $# -eq 0 ]; then
	numTests=1
else
	numTests=$1
fi

folder_size_kb=$(du 'input/' | tail -n 1 | awk '{print $1}')

let "folder_size_mb = folder_size_kb / 1024 + 1"
echo "input folder size: $folder_size_mb Mb" 

let "use_ram_size = folder_size_mb / 20 + 1"
echo -n "selected ram size: $use_ram_size Mb" 
if [ $use_ram_size -eq 1 ]; then 
	echo " (it's minimum possible value)"
fi
echo

echo "Run $numTests tests in a row."
echo "You can change number of tests using command:"
echo "   $0 [number of tests]"
echo

logTestsDir=log_tests
echo "You can find log for each test in '$logTestsDir' folder"
echo

rm -rf "$logTestsDir" 
mkdir -p "$logTestsDir" 
for i in $(seq 1 $numTests); do 
	echo -n "# $i ... "; 

	logTestFile="$logTestsDir"/log$i.txt
	./build/source/memory_manager_test $use_ram_size > "$logTestFile" && 
	./check_result.sh >> "$logTestFile" 

	if [ $? -eq 0 ]; then
		echo "[+]"
	else 
		echo "[ERROR]"
	fi
done
