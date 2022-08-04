import os
import sys
import csv
import argparse
from os import path
from os import listdir
from itertools import combinations
from itertools import permutations

#----------------------------------------
#check whether a file exists
def doesFileExist(file_path, file_name):
    return path.exists(path.join(file_path, file_name))	

#----remove file----------------- 
def removeFile(file_path, file_name):
	os.remove(path.join(file_path, file_name))

#------------------------------------------
def find_filenames_ext( path_to_dir, extension=".csv" ):
	filenames = listdir(path_to_dir)
	return [ filename for filename in filenames if filename.endswith( extension ) ]

#----------------------------
#write csv files with data using DictWriter
def write_csv(file_path, file_name, csv_data, header_fields):
	#print ("Writing %s file ..."%file_name)
	with open(path.join(file_path, file_name), 'w') as csvfile:
		writer = csv.DictWriter(csvfile, fieldnames=header_fields)
        #writes the field names as headers
		writer.writeheader()
		for row in csv_data:
			writer.writerow(row)

#--------------------------------
def get_combinations(sample_list):
	list_combinations = list()
	for n in range(len(sample_list) + 1):
		#list_combinations += list(combinations(sample_list, n))
		list_combinations += list(permutations(sample_list, n))
	#print(list_combinations)
	#print("~~~~~~~~2 Element Combinations~~~~~~")
	#remove the items with one element or more than one elements
	final_comb = [item for item in list_combinations if len(item) == 2]
	print(final_comb)
	print("Size: %d"%len(final_comb))
	return final_comb

#-----------------------------
def check_lists(l1, elem):
	for (c0,c1) in l1:
		if (c0 == elem[0] and c1 == elem[1]):
			print(elem)
			return True

	return False
#-----------------------------
#generate combinations of probabilities
def generate_probs(start_idx=0):
	#sample_list = [[0.3, 0.7], [0.4, 0.6], [0.55, 0.45]] #for test
	#sample_list = [[0.3, 0.7], [0.35, 0.65], [0.4, 0.6], [0.45, 0.55], [0.55, 0.45], [0.6, 0.4], [0.65, 0.35], [0.7, 0.3]]
	sample_list1 = [[0.3, 0.7], [0.35, 0.65], [0.4, 0.6], [0.6, 0.4], [0.65, 0.35], [0.7, 0.3]] #, [0.75, 0.25], [0.25, 0.75], [0.8, 0.2], [0.2, 0.8]]
	sample_list2 = [[0.75, 0.25], [0.25, 0.75], [0.8, 0.2], [0.2, 0.8], [0.35, 0.65], [0.65, 0.35], [0.3, 0.7], [0.7, 0.3]]

	comb1 = get_combinations(sample_list1)
	print("~~~~~~~~~~~")
	comb2 = get_combinations(sample_list2)
	print("~~~~~~~~~")

	final_comb = []
	for cmb in comb2:
		if check_lists(comb1, cmb):
			continue
		final_comb.append(cmb)
	print(final_comb)
	print("Size: %d"%len(final_comb))

	data_config_dict = {}

	for indx, cmb in enumerate(final_comb):
		#data_config_dict[indx] = cmb	
		data_config_dict[start_idx] = cmb
		start_idx +=1

	print(data_config_dict)

	return data_config_dict
#-------------------------------------
def generate_data(cg_dict, cgid, write=True):
	node_list = ["N0", "N1", "N2", "N3", "N4", "N5", "N6", "N7", "N8", "N9", "N10"]
	headers = ["node_id", "face_limit",	"face1_prob", "face2_prob"]
	cg_id_data = cg_dict[cgid]
	data_csv_dict_list = []
	for idx, nd in enumerate(node_list):
		if idx <= 1:
			f1_pr = cg_id_data[idx][0]
			f2_pr = cg_id_data[idx][1]
		else:
			f1_pr = 0.5
			f2_pr = 0.5

		record = {'node_id': nd, 'face_limit': 2, 'face1_prob': f1_pr, 'face2_prob': f2_pr}
		data_csv_dict_list.append(record)

	file_path = path.join(sim_path, net_id)
	file_name = net_id + '-face-pr-config.csv' 
	
	if write:
		write_csv(file_path, file_name, data_csv_dict_list, headers)

#---------------------------------------
if __name__ == '__main__':
	
	sim_path = '/home/tayeen/Research/ndnSIM/scenario/config/'
	
	parser = argparse.ArgumentParser(description='provide arguments for config')

	parser.add_argument('--cgid', type=int, help='configuration id', default=30)
	parser.add_argument('--netid', type=str, help='network id', default='sprint2-edge-c2-4')
	args = vars(parser.parse_args())
    
	cg_id = int(args['cgid']) 
	net_id = str(args['netid'])
	dt_cfg_dict = generate_probs(30)
	generate_data(dt_cfg_dict, cg_id, True)
	
