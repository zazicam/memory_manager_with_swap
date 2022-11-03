inputDir="./input"
outputDir="./output"

error=false
for file in $(ls $inputDir); do
	filename=$(basename $file)
	echo -n "$filename ... " 
	if [ -f "$ouputDir/$filename" ]; then
		DIFF=$(diff "$inputDir/$filename" "$outputDir/$filename")
		if [ "$DIFF" == "" ]; then
			echo "[OK]" 
		else
			echo "[ERROR]"
			error=true	
		fi	
	else
		echo "[NO SUCH FILE]"
		error=true	
	fi
done

echo      "---------------"
if [ "$error" = true ]; then
	echo "The are errors!"
else
	echo "All right!"
fi
