#!/bin/bash

export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
#-----------
sim_path="/home/tayeen/Research/ndnSIM/scenario"
mv_path="/home/tayeen/Research/NDN-RL/NDN_ML/IsrPr/"
trace_path="sprint2-edge-c2-6" #"sprint2-edge-c2-6"
sim_counter=1
len=50000
trace=1
rate=200
acfg=1
sim_id="sprint2-edge-c2-6" #"sprint2-edge-c2-6" #"1router2path-1" #"1router4path-1" #"sprint2-bttnck-2" #"sprint2-bttnck-4" #"sprint2-multipath-1" #"sprint2-bttnck-1"  #"sprint2"


while [ $sim_counter -le 1 ]
    do
    echo $sim_counter
	mkdir $sim_path/results/IsrPr/
	mkdir $mv_path
    rm $sim_path/traces/*.*
    rm $sim_path/results/IsrPr/*.*
    rm $mv_path*.*
	
	#./waf --run="isr-pr-3-scen --tId="$sim_id" --sId="$sim_counter" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate
    #./waf --run="isr-pr-2-scen --tId="$sim_id" --sId="$sim_counter" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate
	#./waf --run="isr-pr-22-scen --tId="$sim_id" --sId="$sim_counter" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate
	#./waf --run="isr-pr-23-scen --tId="$sim_id" --sId="$sim_counter" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate
	./waf --run="isr-pr-4-scen --tId="$sim_id" --sId="$sim_counter" --agCnfg="$acfg" --trace="$trace" --runLen="$len" --rate="$rate

    #NS_LOG=nfd.Forwarder ./waf --run="single-best-fws-lf-scen --tId="$sim_id" --trace="$trace" --runLen="$len
    mv $sim_path/traces/*.* $mv_path
    mv $sim_path/results/IsrPr/*.* $mv_path
    mv $mv_path "/home/tayeen/Research/NDN-RL/NDN_ML/"$trace_path/
     ((sim_counter++))
done
#echo "All done"

