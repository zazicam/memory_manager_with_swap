if [ $# -eq 0 ]; then
	numTests=1
else
	numTests=$1
fi

folder_size_kb=$(du 'input/' | awk '{print $1}')
let "folder_size_mb = folder_size_kb / 1024 + 1"
echo "input folder size: $folder_size_mb Mb" 

let "use_ram_size = folder_size_mb / 20 + 1"
echo "selected ram size: $use_ram_size Mb" 
echo

mkdir -p logs

echo "Run $numTests tests in a row."
echo "You can change number of tests using command:"
echo "   $0 [number of tests]"
echo

rm ./logs/*
for i in $(seq 1 $numTests); do 
	echo -n "# $i ... "; 
	./build/memory_manager_test $use_ram_size > ./logs/log$i.txt && ./check_result.sh >> ./logs/log$i.txt
	if [ $? -eq 0 ]; then
		echo "[+]"
	else 
		echo "[ERROR]"
	fi
done

