import pandas as pd
import sys
import os 
import matplotlib.pyplot as plt
import statistics
from gen_helper import *

#===============================
sim_path = os.path.expanduser('~') + '/Research/ndnSIM/scenario/'
curr_path = os.getcwd()

#============================================
def read_trace(agent_node, feature_ids, extra=None):

	df_raw = pd.read_csv(data_path + agent_node + "-face_stat.csv", sep=",", header=None, index_col=False, keep_default_na=False)
	#print(df_raw)
	feature_names = ["IntCount", "DtCount", "RtCount", "RtRatio" ,"AvgStRatio", "FcProb", "IsrPrRatio", "AvgRtt", "SatRatio"]
	
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
	columns.append("Label")
	#print(columns)	
		
	df_raw.columns = columns
	#df_raw['Time'] = df_raw['Time'].astype(int)
	features = []
	for face in ag_faces:
		for ft_id in feature_ids:
			features.append("Fc"+str(face)+"-"+ft_id)
	if extra is not None:
		features.append(extra)
	features.append("Label")
	#print(features)
	
	df = df_raw[features] #get the necessary feature columns
	#print(df)
	
	sub_df = pd.DataFrame(columns=['Feature', 'Max', 'Min'])
	for feature in features[:len(features)-1]: 
		max_value = df[feature].max()
		min_value = df[feature].min()
		mean_value = df[feature].mean()
		std_value = df[feature].std() 
		sub_df = sub_df.append({'Feature': feature, 'Max': max_value, 'Min': min_value, 'Mean': mean_value, 'Std': std_value}, ignore_index=True)
	
	print(sub_df)
	df.to_csv(out_path + agent_node + "-" + train_file +'.csv', index=False)
	sub_df.to_csv(out_path + agent_node + "-" + train_file +'max-min.csv', index=False)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	
if __name__ == "__main__":

	train_file = "10000s-f1"
	net_id = "sprint2-edge-c2-6" 
	ag_cfg_id = 1
	nodes, flows = [], []
	#features = ["IsrPrRatio", "RtRatio"] #f2
	features = ["IsrPrRatio"] #f1
	extra_f = "DiffSuccRatio"
	
	data_path = curr_path + "/" + net_id + "/IsrPr"+train_file+"/"
	out_path = curr_path + "/" + net_id + "/training/"  

	
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
	for ag_node in agent_nodes:
		print("Reading trace of node %s"%ag_node)
		read_trace(ag_node, features, extra_f)	
	
	
	
