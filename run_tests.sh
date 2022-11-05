if [ $# -eq 0 ]; then
	numTests=5
else
	numTests=$1
fi

mkdir -p logs

echo "Run $numTests tests in a row."
rm ./logs/*
for i in $(seq 1 $numTests); do 
	echo -n "# $i ... "; 
	./build/memory_manager_test 1 > ./logs/log$i.txt && ./check_result.sh >> ./logs/log$i.txt
	if [ $? -eq 0 ]; then
		echo "[+]"
	else 
		echo "[ERROR]"
	fi
done

