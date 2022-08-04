import os
import sys
import csv
import copy
import numpy as np
#import matplotlib 
#matplotlib.use('agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import getpass
import argparse
import pandas as pd
from gen_helper import *
#===============================
user_name = getpass.getuser()
#print(user_name)

if (user_name == "tayeen"):
	sim_path = '/home/'+ user_name + '/Research/ndnSIM/scenario/'
else:
	sim_path = '/home/' + user_name + '/ndnSIM/scenario/'

curr_path = os.getcwd()

#-----------------------------------------
def get_plot_df(file_name, node_id, method, flow=''):
	
	if method == 'BR':
		data_path = sim_path + "results/BestRouteApp/" + net_id + "-new-cons-q10-e1r"+str(rate)+"l"+str(duration)
	elif method == 'FP':
		#data_path = curr_path + "/" + trace_id + "/FixedPr"+ext_id
		data_path = out_path + "/FixedPr"+ext_id
	elif method == 'RD':
		data_path = curr_path + "/" + trace_id + "/RandPr"		
	elif method == 'MA':
		data_path = curr_path + "/" + trace_id + "/MvAvgPr"	
	elif method == 'PR':
		data_path = curr_path + "/" + trace_id + "/IsrPr"	

	df_raw = pd.read_csv(data_path + "/"+ file_name, sep=",", header=None, index_col=False)
	if node_id in agent_nodes:
		df_raw.columns = ["Time", "Content", "Nonce1", "Nonce2", "InFaceId", "Delay"]	
	else:
		df_raw.columns = ["Time", "Node", "Content", "Nonce", "InFaceId", "Delay"]	
	
	#print(df_raw.shape)
	df_raw['Time'] = df_raw['Time'].astype(int)
	max_time = duration #int(df_raw['Time'].max())

	df_sub = df_raw[ ( df_raw['Content'].str.contains('/data/'+flow) ) & (df_raw['Time'] < max_time) ]
	#print(df_sub)
	
	print(flow, df_sub.shape)
	if (df_sub.shape[0] == 0): #empty dataframe
		print("Empty dataframe found")
		return None, 0	
	
	df = df_sub.copy(deep=True)
	df = df_sub[["Time", "InFaceId"]] #get the necessary columns
	faces = df.InFaceId.unique() #get the list of unique faces
	print("Unique faces: %s"%faces)

	#plot_df = pd.DataFrame({'Time': pd.Series([], dtype='int'), '0': pd.Series([], dtype='float'), '1': pd.Series([], dtype='float')})
	
	face_dict = {}
	count_dict = {}
	df_dict = {'Time': pd.Series([], dtype='int')}
	for f in faces:
		next_hop = node_link_dict[node_id][str(f)]
		count_dict[next_hop] = 0.0
		face_dict[next_hop] = 0
		df_dict[next_hop] = pd.Series([], dtype='float')

	plot_df = pd.DataFrame(df_dict)
	#print(df[1:30])

	time_dict = {}
	for t in range(0,max_time):
		time_dict[t] = copy.deepcopy(count_dict)  #{'0': 0.0, '1': 0.0}
		
	for key, group_df in df.groupby('Time'):
		#print(group_df)
		total_count = group_df.shape[0]
		# iterate over rows with iterrows()
		face_count =  copy.deepcopy(face_dict) #{'0': 0, '1': 0}
		#face_count = {'0': 0, '1': 0, '2': 0, '3': 0}
		for index, row in group_df.head(n=group_df.shape[0]).iterrows():
			face_count[node_link_dict[node_id][str(row["InFaceId"])]] += 1
		
		#print(key, face_count)
		try:
			for f in faces:
				time_dict[key][node_link_dict[node_id][str(f)]] = (float(face_count[node_link_dict[node_id][str(f)]])*1024*8) / 1000000
			
		except KeyError:
			pass
		#print(face_count)
	#print(time_dict)
	for t in range(max_time):
		time_df_dict = {'Time': t}
		for f in faces:
			time_df_dict[node_link_dict[node_id][str(f)]] = time_dict[t][node_link_dict[node_id][str(f)]]
		plot_df = plot_df.append(time_df_dict, ignore_index=True) 
		#plot_df = plot_df.append({'Time': t, '0': time_dict[t]['0'], '1': time_dict[t]['1']}, ignore_index=True)
		
		
	#print(plot_df)  
	return plot_df, faces

#----------------------------------------------
def plot_data_rate(plot_df, method, node_id, faces, flow):
	#---------PLOT SETTINGS-------------------
	plt.rc('font', size=18)          # controls default text sizes
	plt.rc('legend', fontsize=18)    # legend fontsize
	plt.rc('axes', titlesize=20)     # fontsize of the axes title
	plt.rc('axes', labelsize=20)    # fontsize of the x and y labels
	plt.rc('xtick', labelsize=18)    # fontsize of the tick labels
	plt.rc('ytick', labelsize=18)    # fontsize of the tick labels


	#http://queirozf.com/entries/pandas-dataframe-plot-examples-with-matplotlib-pyplot
	colors = ['black', 'red', 'blue', 'green', '#4893d5','magenta', 'cyan', '#f4d03f']
	#styles = ['solid','dashed']
	markers = ['o', 'o', 'o', 'o', 'o', 'o','o','o']  #['o', 's', 'p', '^', 'v', 'x','>','*']
	marker_sizes = [2, 2, 2, 2, 2] #[10, 8, 6, 4, 2, 1]

	ax = plt.gca()
	for idx, f in enumerate(faces):
		try:
			plot_df.plot(kind='line', x='Time', y=node_link_dict[node_id][str(f)], color=colors[idx], lw=2, marker='o', markersize=marker_sizes[idx], label=node_link_dict[node][str(f)], ax=ax)
		except KeyError:
			continue				

	if flow != "":
		flow = "-"+flow

	thrpt_limit = math.ceil((len(agent_nodes) * rate * 1024 * 8.0) / 1000000)

	lines, labels = ax.get_legend_handles_labels()
	ax.legend(lines, labels, loc='best', framealpha=0.2)
	ax.set_ylabel("Data Throughput "+flow)
	ax.set_xlabel("Time (s)")
	ax.set_xticks(plot_df.Time)
	ax.set_ylim([0.0, thrpt_limit]) 
	ax.xaxis.set_major_locator(plt.MaxNLocator(10))
	ax.grid(True) # Turn on the grid

	plt.title(method + "-"+node_label_dict[node_id] + "-Rt"+str(rate)+flow)
	
	plt.savefig(out_path + "/plots/"+ method + "-"+node_label_dict[node_id] + flow +"-Rt"+str(rate)+'-data-thrpt.pdf', dpi=100, bbox_inches='tight')
	#plt.show()
	
	plt.close()

#------------------------------------
if __name__ == '__main__':

	parser = argparse.ArgumentParser(description='provide arguments for plotting')
    # arguments
	parser.add_argument('--rate', help='interest sending rate')
	parser.add_argument('--d', help='simulation duration')	
	parser.add_argument('--br', help='best route', default='0')	
	parser.add_argument('--rd', help='random', default='0')
	parser.add_argument('--fp', help='fixed probability', default='0')
	parser.add_argument('--ma', help='moving average', default='0')
	parser.add_argument('--pr', help='isr probabilistic', default='0')
	parser.add_argument('--netid', help='network id', default='sprint2')
	parser.add_argument('--trcid', help='trace id', default='')
	parser.add_argument('--outid', help='output dir_id', default='')
	parser.add_argument('--extid', help='extra_id', required = False, default='')	#used for fp method

	args = vars(parser.parse_args())
    
	rate = int(args['rate']) #500
	duration = int(args['d'])
	add_br = bool(int(args['br']))
	add_fp = bool(int(args['fp']))
	add_rd = bool(int(args['rd']))
	add_ma = bool(int(args['ma']))
	add_pr = bool(int(args['pr']))
	net_id = str(args['netid'])
	trace_id = str(args['trcid'])
	out_dir = str(args['outid'])
	
	if 'extid' in args:
		ext_id = str(args['extid'])
	else:
		ext_id = ''

	file_name = "data.csv" 
	ag_cfg_id = 1	

	out_path = curr_path + "/" + trace_id + "/" + out_dir
	make_dir(out_path + "/plots")
	make_dir(out_path + "/plot_data")
	
	nodes = [] 
	flows = []

	p_dict, c_dict = read_config(sim_path, net_id)
	for c in c_dict.keys():
		cid = c[1:]
		nodes.append(c)

	for p in p_dict.keys():
		pid = p[1:]
		flows.append("p"+str(pid))		 			

	methods = []
	columns = ["Time"]
	if add_br:
		methods.append('BR')
	if add_rd:
		methods.append('RD')	
	if add_fp:
		methods.append('FP')
	if add_ma:
		methods.append('MA')
	if add_pr:
		methods.append('PR')


	G = get_graph(sim_path, net_id)
	node_label_dict = read_node_info(sim_path, net_id)
	all_nodes = list(node_label_dict)
	node_link_dict = get_node_link_info(G, node_label_dict)
	#print(node_link_dict)
	agent_nodes = get_agent_info(sim_path, net_id, ag_cfg_id)
	#node_face_indx_dict = get_node_face_info(net_id)

	print("Plotting data stats...")

	for meth in methods:
		for idx, node in enumerate(all_nodes):
			print(node)
			try:
				plot_df, faces = get_plot_df(node+"-"+file_name, node, meth, flow='') 
				if plot_df is None:
					print("No data found for node: %s"%node)
					continue
			except IOError as e:
				print("File not found: %s"%str(e))
				continue		
			except KeyError as e:
				print("Face not found: %s"%str(e))
				continue			
			plot_df.to_csv(out_path + "/plot_data/"+ meth + "-"+node_label_dict[node] +"-Rt"+str(rate)+'-data-thrpt.csv')
		
			plot_data_rate(plot_df, meth, node, faces, flow='')
			for flow in flows:
				plot_df, faces = get_plot_df(node+"-"+file_name, node, meth, flow=flow) 
				if plot_df is None:
					continue
				plot_df.to_csv(out_path + "/plot_data/"+ meth + "-"+node_label_dict[node] + "-" + flow +"-Rt"+str(rate)+'-data-thrpt.csv')
				plot_data_rate(plot_df, meth, node, faces, flow=flow)
		
