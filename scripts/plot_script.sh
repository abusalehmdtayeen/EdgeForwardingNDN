#!/bin/bash

#load_ids=(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29)
load_ids=(7)
for value in {0..0}
do
   echo $value
   #load_ids+=($value)
done

qsize=10
# get length of $load_ids array
len=${#load_ids[@]}

netid="sprint2-edge-c2-6" #"sprint2-edge-c2-4" #"sprint2-edge-c2-6"
trc_path="EdgeMLFw" #"sprint2-edge-c2-6" #"samples_p200_s300s" # # #"samples_p200_s300s" 
out_path="IsrPrML" # # #"FxPr" #"IsrPr-Thrs" #"IsrPr-Thrs" #"CombFixedPr"

ml_type="All" #"All" #model type
ml_id="IsPrMl10000s-f1" #model id

rate=200
d=120
add_br=1 #show only best route results
add_fp=1 #compare with fixed probability 
add_rd=0
add_ma=0
add_pr=0
add_ml=1

version="" #implementation version

ignore_per=0 #1=ignore plotting performance plots 
ignore_stats=1 #1=ignore plotting stat  plots
move_stats=0

if [ $ignore_per -eq 1 ]; then
	add_br=0 #show only best route results
	add_fp=0 #fixed probability
	add_rd=0 #random
	add_ma=0 #moving average
	add_pr=0
	add_ml=1
fi


#==============================================
for i in ${load_ids[@]}; 
	do
        echo "Loading "$i
        
		if [ "$ignore_per" = 0 ]; then
		    
			#python plot-throughput-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0 1 2 3 4 5 6 7 8"

			#python plot-delay-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0 1 2 3 4 5 6 7 8"

			#python plot-throughput-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0.35 0.40 0.45 0.5 0.55"

			#python plot-delay-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0.35 0.40 0.45 0.5 0.55"

			#python plot-throughput-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0.4-t0 0.4-t1 0.4-t2"

			#python plot-delay-2.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "0.4-t0 0.4-t1 0.4-t2"

			python plot-throughput-3.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --ml=$add_ml --mlid=$ml_id --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "LR DT SVM-L SVM-NL" --extid=$i

			python plot-delay-3.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --ml=$add_ml --mlid=$ml_id --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --list "LR DT SVM-L SVM-NL" --extid=$i


			#---------------------------------------------------
			
			#python plot-throughput.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --ml=$add_ml --mltype=$ml_type --mlid=$ml_id --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --extid=$i

			#python plot-delay.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --ml=$add_ml --mltype=$ml_type --mlid=$ml_id --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --extid=$i

			
        fi
		#---------------------------------------
		if [ "$ignore_stats" = 0 ]; then

			add_br=0 #show only best route results
			add_fp=0 #fixed probability
			add_rd=0 #random
			add_ma=0 #moving average
			add_pr=0
			add_ml=1
   			
			#python plot-data-rate.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --extid=$i

			python plot-intrst-rate.py --rate=$rate --d=$d --br=$add_br --rd=$add_rd --fp=$add_fp --ma=$add_ma --ml=$add_ml --mltype=$ml_type --mlid=$ml_id --pr=$add_pr --netid=$netid --trcid=$trc_path --outid=$out_path --extid=$i
            

			#--------------------------------------------
			
			#python tdqn-write-latex.py --rate=$rate --obid=$obj_id --lid=$i --e=$episodes --d=$d --td=$train_d --tr=$train_rate --thr=$pkt_thrs --tprfx=$train_prfx --w=$write --br=$br_only --delM=$delay_type --tdata=$train_data --plt=$plot --tlr=$test_learn_rate --rthrs=$rtt_thrs --expid=$exp_id --agcfg=$ag_cfg_id
		
   		fi

        CWD=$(pwd)
        mkdir $CWD/$trc_path/$out_path/$ml_type"-"$ml_id
        mv $CWD/$trc_path/$out_path/"plot_data" $CWD/$trc_path/$out_path/$ml_type"-"$ml_id
        mv $CWD/$trc_path/$out_path/"plots" $CWD/$trc_path/$out_path/$ml_type"-"$ml_id
        
		#python cpy_output.py --trcid=$trc_path --outid=$out_path --cgid=$i
		#python cpy_output-c2-6.py --trcid=$trc_path --outid=$out_path --cgid=$i --stats=$move_stats
		#=====================================================
		
        echo "-------------------------------------"
	done


