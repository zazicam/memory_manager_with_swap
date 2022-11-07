inputDir="./input"
outputDir="./output"

error=false
for file in $(ls $inputDir); do
	filename=$(basename $file)
	inputFile="$inputDir/$filename"
	outputFile="$outputDir/$filename"
	echo -n "$filename ... " 
	if [ -f $outputFile ]; then
		DIFF=$(diff $inputFile $outputFile)
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
done

echo
if [ "$error" = true ]; then
	echo "The are errors!"
	exit 1
else
	echo "[Ok]"
	exit 0
fi
