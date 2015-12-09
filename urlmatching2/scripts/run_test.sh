#!/bin/bash
PROG1='./urls3'
File="$1"
OTXT="results/oUnix.$1.txt"
DELIMI="-l ."

if [ -z "$1" ]
  then
    echo "ERROR: date file not supplied"
	exit 1
fi

mkdir -p results
echo " -- " > $OTXT

echo "Started at" ; date
SLEEPP="sleep 4"
# article n
if [ -z "$2" ] || [ "$2" -eq 1 ]
 then
	ACSV="results/datUnix.Online.$1.csv"
	ARGS="-k 8 -r 0.9"
	for TWICE in 1 2
	do
		echo "$ACSV : $ARGS"
		$PROG1 article -o $ACSV -f $File -n 1170 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 2287 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 3364 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 4401 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 5448 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 6474 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 7498 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 8510 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 9491 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 10500 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 11550 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 12600 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 13600 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 14650 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 15700 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 16800 $ARGS >>$OTXT ; $SLEEPP
		$PROG1 article -o $ACSV -f $File -n 17900 $ARGS >>$OTXT ; $SLEEPP
		ARGS="$ARGS $DELIMI"
		ACSV="results/datUnix.Online.$1.LPM.csv"
	done
fi
	
# test r
CMD="article"
ACSV="results/datUnix.K.$1.csv"
if [ -z "$2" ] || [ "$2" -eq 2 ]
 then
	for N in 1000 4000 9000 12000 15000
	do
		ARGS="-n $N -r 0.9"
		echo "$CMD $ACSV : $ARGS"
		$PROG1 $CMD -o $ACSV -f $File -k 4 $ARGS -a >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 8 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 12 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 16 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 20 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 24 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 32 $ARGS >>$OTXT
	done
fi

# test kgram
ACSV="results/datUnix.N.$1.csv"
if [ -z "$2" ] || [ "$2" -eq 3 ]
 then
	for K in 4 8 12 16 20 
	do
		ARGS="-k $K -r 0.9"
		echo "$CMD $ACSV : $ARGS"
		$PROG1 $CMD -o $ACSV -f $File -n 500 $ARGS -a >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 1000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 3000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 6000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 9000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 12000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 15000 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 18000 $ARGS >>$OTXT
	done
fi

# test kgram

FILESHUF="tmp.shuffled"
if [ -z "$2" ] || [ "$2" -eq 4 ]
 then
	ACSV="results/datUnix.URLS.$1.csv"
	AA="-a"
	for U in 1 2 4 8 16 32 64 128 256 512 1024 2048 
	do
		ARGS="-k 4 -r 0.9 -n 16000"
		echo "$CMD $ACSV $U : $ARGS"
		shuf $File | head -n$(($U*1000)) > $FILESHUF
		$PROG1 $CMD -o $ACSV -f $FILESHUF $ARGS $AA >>$OTXT
		$SLEEPP
		AA=""
	done
	ACSV="results/datUnix.URLS.$1.LPM.csv"
	AA="-a"
	FileComp="tmp.components"
	cat $File | tr '.' '\n' > $FileComp
	for U in 1 2 4 8 16 32 64 128 256 512 1024 2048 
	do
		ARGS="-k 4 -r 0.9 -n 16000"
		echo "$CMD $ACSV $U : $ARGS"
		shuf $FileComp | head -n$(($U*1000)) > $FILESHUF
		$PROG1 $CMD -o $ACSV -f $FILESHUF $ARGS $AA >>$OTXT
		$SLEEPP
		AA=""
	done
fi
echo "Finished at "; date
