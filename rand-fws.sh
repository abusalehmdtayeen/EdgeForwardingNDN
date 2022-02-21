#!/bin/bash

export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
#-----------
sim_path="/home/tayeen/Research/ndnSIM/scenario/"
sim_counter=1 #simulation counter
len=10000 #simulation duration
trace=1 #whether to do tracing
rate=200 #interet sending rate
acfg=1
sim_id="sprint2-edge-c2-2" #topology id[must match with config]

while [ $sim_counter -le 1 ]
    do
    echo $sim_counter
    mkdir $sim_path"train-traces"
    mkdir $sim_path"traces"
    rm $sim_path"/train-traces/*.*" #remove old trace files
    rm $sim_path"/traces/*.*"  #remove old trace files
	
	./waf --run="rand-train-fws-scen --tId="$sim_id" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate

    #mv $sim_path/traces/*.* $sim_path/train-traces/
    ((sim_counter++))
done
#echo "All done"

