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
SLEEPP="sleep 5"
# article n
if [ -z "$2" ] || [ "$2" -eq 1 ]
 then
	ACSV="results/datUnix.Online.$1.csv"
	ARGS="-k 3 -r 0.5"
	for LPMONOFF in 1 2
	do		
		for NN in 50 100 200 500 1170 2287 4401 6474 8510 9491 10500 11550 12600 13600 14650 16800 17900
		do
			for TIMESS in `seq 1 30`
			do
				echo "$ACSV -n $(($NN)): $ARGS"
				$PROG1 article -o $ACSV -f $File -n $(($NN)) $ARGS >>$OTXT
				$SLEEPP
			done
		done
		
		ARGS="$ARGS $DELIMI"
		ACSV="results/datUnix.Online.$1.LPM.csv"
	done
fi
	
# test r
CMD="article"
ACSV="results/datUnix.K.$1.csv"
if [ -z "$2" ] || [ "$2" -eq 2 ]
 then
	for N in 100 1170 4401 9491 12600
	do
		ARGS="-n $N -r 0.5"
		echo "$CMD $ACSV : $ARGS"
		$PROG1 $CMD -o $ACSV -f $File -k 2 $ARGS -a >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 3 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 4 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 5 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 6 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 8 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -k 12 $ARGS >>$OTXT
	done
fi

# test kgram
ACSV="results/datUnix.N.$1.csv"
if [ -z "$2" ] || [ "$2" -eq 3 ]
 then
	for K in 2 3 4 8 16 
	do
		ARGS="-k $K -r 0.5"
		echo "$CMD $ACSV : $ARGS"
		$PROG1 $CMD -o $ACSV -f $File -n 100 $ARGS -a >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 550 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 1170 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 3364 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 6474 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 9491 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 12600 $ARGS >>$OTXT
		$PROG1 $CMD -o $ACSV -f $File -n 15700 $ARGS >>$OTXT
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
		ARGS="-k 3 -r 0.5 -n 16800"
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
		ARGS="-k 3 -r 0.5 -n 16800"
		echo "$CMD $ACSV $U : $ARGS"
		shuf $FileComp | head -n$(($U*1000)) > $FILESHUF
		$PROG1 $CMD -o $ACSV -f $FILESHUF $ARGS $AA >>$OTXT
		$SLEEPP
		AA=""
	done
fi
echo "Finished at "; date
