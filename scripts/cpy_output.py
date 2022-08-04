import os
import sys
import csv
import argparse
from os import path
from gen_config import *
from gen_helper import *
import shutil

curr_path = os.getcwd()
#-------------------------------------
def mv_output(cg_dict, cgid):
	prob_comb = cg_dict[cgid]
	dest_dir_name = "FixedPr-" + str(cgid) + "-C0-" + str(prob_comb[0][0]).replace(".","") + "-" + str(prob_comb[0][1]).replace(".","") + "-C1-" + str(prob_comb[1][0]).replace(".","") + "-" + str(prob_comb[1][1]).replace(".","") 
	make_dir(curr_path + "/" + trace_id + "/" + out_dir + "/" + dest_dir_name)
	src_dir_name = curr_path + "/" + trace_id + "/" + out_dir + "/plots"

	plot_files = find_filenames_ext(src_dir_name, extension=".pdf" )	

	for fl in plot_files:
		shutil.move(src_dir_name + "/" + fl, curr_path + "/" + trace_id + "/" + out_dir + "/" + dest_dir_name + "/" + fl)

	src_dir_name = curr_path + "/" + trace_id + "/" + out_dir + "/plot_data"

	plot_data_files = find_filenames_ext(src_dir_name, extension=".csv" )	

	for fl in plot_data_files:
		shutil.move(src_dir_name + "/" + fl, curr_path + "/" + trace_id + "/" + out_dir + "/" + "FixedPr"+str(cgid) + "/" + fl)


#---------------------------------------
if __name__ == '__main__':
	
	parser = argparse.ArgumentParser(description='provide arguments for config')

	parser.add_argument('--cgid', type=int, help='configuration id', default=0)
	parser.add_argument('--trcid', help='trace dir_id', default='')
	parser.add_argument('--outid', help='output dir_id', default='')
	
	args = vars(parser.parse_args())
    
	cg_id = int(args['cgid']) 
	trace_id = str(args['trcid'])
	out_dir = str(args['outid'])
	
	dt_cfg_dict = generate_probs()

	mv_output(dt_cfg_dict, cg_id)
	
