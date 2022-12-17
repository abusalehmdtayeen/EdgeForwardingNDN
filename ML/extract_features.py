import pandas as pd
import sys
import os 
import matplotlib.pyplot as plt
import statistics
from gen_helper import *
from itertools import chain, combinations

#===============================
sim_path = os.path.expanduser('~') + '/Research/ndnSIM/scenario/'
curr_path = os.getcwd()

#============================================
def read_trace(agent_node, feature_ids, merge_ids, ft_out_id, extra=None):

	feature_names = ["IntCount", "DtCount", "RtCount", "RtRatio" ,"AvgStRatio", "FcProb", "IsrPrRatio", "AvgRtt", "NormAvgRtt", "SatRatio"]
	
	columns = ["Time"]
	ag_flow = node_flow_dict[agent_node]
	node_face_indx_dict = get_node_face_info(sim_path, net_id, ag_flow)
	ag_faces = node_face_indx_dict[agent_node][ag_flow]
	#print(ag_faces)
	for face in ag_faces:
		columns.append(ag_flow + "-" + str(face))
		for ft in range(len(feature_names)):
			columns.append("Fc"+str(face)+"-"+feature_names[ft])		
	columns.append("Throughput")
	columns.append("DiffSuccRatio")
	columns.append("DiffDelRatio")
	columns.append("Label")
	#print(columns)
	#print(len(columns))	

	df_list = []
	for k in range(len(merge_ids)):
		file_path = data_path + "IsrPr-r" + str(merge_ids[k]) + param_id + "/"
		print("Loading %s for agent %s"%(file_path, agent_node))
		df_r = pd.read_csv(file_path + agent_node + "-face_stat.csv", sep=",", header=None, index_col=False, keep_default_na=False)
		#print(df_r)
		df_list.append(df_r)				
	
	print(len(df_list))
	if len(df_list) > 1:
		df_raw = pd.concat(df_list, ignore_index=True)
	else:
		df_raw = df_list[0]

	print(df_raw.shape)
	df_raw.columns = columns
	#df_raw['Time'] = df_raw['Time'].astype(int)
	features = []
	for face in ag_faces:
		for ft_id in feature_ids:
			features.append("Fc"+str(face)+"-"+ft_id)
	if extra is not None:
		for ext in extra:
			features.append(ext)
	features.append("Label")
	print(features)
	
	for feature in features[:len(features)-1]: 
		df_raw[feature] = df_raw[feature].astype(float)
	
	df = df_raw[features] #get the necessary feature columns
	print(df)
	
	sub_df = pd.DataFrame(columns=['Feature', 'Max', 'Min'])
	for feature in features[:len(features)-1]: 
		print(feature)
		df[feature] = df[feature].fillna(0)
		max_value = df[feature].max()
		min_value = df[feature].min()
		mean_value = df[feature].mean()
		std_value = df[feature].std() 
		sub_df = sub_df.append({'Feature': feature, 'Max': max_value, 'Min': min_value, 'Mean': mean_value, 'Std': std_value}, ignore_index=True)
	
	print(sub_df)
	df.to_csv(out_path + agent_node + "-" + ft_out_id +'.csv', index=False)
	sub_df.to_csv(out_path + agent_node + "-" + ft_out_id +'-max-min.csv', index=False)

#--------------------------------
def powerset(items):
    l_items = list(items)
    return chain.from_iterable(combinations(l_items, r) for r in range(len(l_items) + 1))

#-------------------------------
#Write text files with data
def write_txt(file_name, txt_data):
	with open(out_path + file_name+'.txt', 'w') as txtfile:
		for data in txt_data:
			txtfile.write(str(data))
			txtfile.write("\n")

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	
if __name__ == "__main__":

	strategy_version = "IsrPr51" 
	train_file = strategy_version + "-3000s-r100-150-200" #"-l5000-r200" #"IsrPr51-3000s-r150-225"
	net_id = "sprint2-edge-c2-6" 
	ag_cfg_id = 1
	nodes, flows = [], []
	features = ["IsrPrRatio", "RtRatio", "NormAvgRtt"] 
	extra_f = ["DiffSuccRatio", "DiffDelRatio"]
	rates = [100, 150, 200]
	param_id = "" #"l5000-thr0.85-sr0.05-dr0.15"
	
	data_path = curr_path + "/" + net_id + "/" + strategy_version + "-training" + "/"
	out_path = curr_path + "/EdgeMLFw/data/" + net_id + "/" + train_file +"/"
	make_dir(out_path)
	
	G = get_graph(sim_path, net_id)
	p_dict, c_dict = read_config(sim_path, net_id)
	#print(c_dict)
	#print(p_dict)
	node_flow_dict = {}
	
	for c in c_dict.keys():
		cid = c[1:]
		nodes.append(c)
		node_flow_dict[c_dict[c]['node']] = c_dict[c]['flow']

	for p in p_dict.keys():
		pid = p[1:]
		flows.append("p"+str(pid))		 	
		
	node_label_dict = read_node_info(sim_path, net_id)
	all_nodes = list(node_label_dict)
	node_link_dict = get_node_link_info(G, node_label_dict)
	#print(node_link_dict)
	agent_nodes = get_agent_info(sim_path, net_id, ag_cfg_id)
	#print(agent_nodes)
	#for flow in flows:
	#	node_face_indx_dict = get_node_face_info(sim_path, net_id, flow)
	#	print(node_face_indx_dict)
	#----------------------------------
	feature_comb = list(powerset(features))
	print(feature_comb)
	extra_ft_comb = list(powerset(extra_f))
	print(extra_ft_comb)
	text_data_list = []
	all_features_list = []
	feature_id = 0
	for idx, comb_item in enumerate(feature_comb):
		if idx == 0:
			continue
		ft_item = comb_item
		for ext_f_item in extra_ft_comb:
			ext_f = ext_f_item
			final_feature = (ft_item + ext_f)
			print(feature_id)
			print(final_feature)
			#all_features_list.append(ft_item)
			print(list(ft_item))
			txt_item = str(feature_id) + "-" + str(final_feature)	
			text_data_list.append(txt_item)
			
			for ag_node in agent_nodes:
				print("Reading trace of node %s"%ag_node)
				read_trace(ag_node, list(ft_item), rates, "f"+str(feature_id), list(ext_f))	
				
			feature_id += 1
			
	write_txt("feature_names", text_data_list)
			
	#sys.exit(0)	
	#-----------------------------	
	
	
	
	
