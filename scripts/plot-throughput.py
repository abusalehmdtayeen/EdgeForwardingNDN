import pandas as pd
import sys
#import matplotlib 
#matplotlib.use('agg')
import matplotlib.pyplot as plt
import math
import argparse
import os
import getpass
from gen_helper import *
#===============================
user_name = getpass.getuser()
print(user_name)

if (user_name == "tayeen"):
	sim_path = '/home/'+ user_name + '/Research/ndnSIM/scenario/'
else:
	sim_path = '/home/' + user_name + '/ndnSIM/scenario/'

curr_path = os.getcwd()

time_step = 1  #time_step
time_unit = "s" #sec or min
packet_type = "InData" #"OutSatisfiedInterests" #


#============================================
def get_plot_df(file_name, node_id, method):
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
	elif method == 'ML':
		data_path = curr_path + "/" + trace_id + "/EdgeMLFlow_results/" + model_type + "/" + model_id
	
	df_raw = pd.read_csv(data_path + "/"+ file_name, sep="\t", index_col=False)

	df = df_raw[['Time', 'Node', 'FaceId', 'FaceDescr', 'Type', 'PacketRaw']] #get the necessary columns
	df_raw['Time'] = df_raw['Time'].astype(int)

	total_time = duration #int(df_raw['Time'].max())
	max_time = total_time

	# filtering data 
	subset_df = df[ (df['Node'] == node_id) & (df['Type'] == packet_type) & (df['FaceDescr'].str.match('netdev')) & (df['Time'] < max_time) ]
	num_pkts = 0

	#print subset_df

	#https://www.shanelynn.ie/summarising-aggregation-and-grouping-data-in-python-pandas/
	plt_df = subset_df.groupby('Time', as_index=False).agg({'PacketRaw':sum})
	#print plt_df
	#print plot_df.shape
	
	plot_df = pd.DataFrame(columns=['Time', 'Throughput'])

	#https://cmdlinetips.com/2018/12/how-to-loop-through-pandas-rows-or-how-to-iterate-over-pandas-rows/
	prev_num_pkts = 0
	total_pkts = 0

	# iterate over rows with iterrows()
	for index, row in plt_df.head(n=plt_df.shape[0]).iterrows():
		# access data using column names
		time = float(row['Time'])
		#print(index, row)
		#https://kanoki.org/2019/04/12/pandas-how-to-get-a-cell-value-and-update-it/ 
		if index == 0:
			num_pkts = row['PacketRaw']
		else:
			num_pkts = row['PacketRaw'] - prev_num_pkts

		prev_num_pkts = row['PacketRaw']
		total_pkts += num_pkts

		pkt_data_bytes = num_pkts * 1024 * 8 #data packet size is 1024 bytes 
		throughput = float(pkt_data_bytes) / 1000000 #Mbps

		plot_df = plot_df.append({'Time': int(time), 'Throughput': throughput}, ignore_index=True)			

	#print len(throughput)	
	#print plot_df
	#https://www.geeksforgeeks.org/adding-new-column-to-existing-dataframe-in-pandas/
	total_transfer = float(total_pkts * 1024 * 8) / 1000000
	total_throughput = total_transfer / total_time
	print("Total data packets: %d"%total_pkts)
	print("Total Throughput: %f Mbps"%total_throughput)
	
	return plot_df, total_throughput

#------------------------------------
def plot_data(plot_df, node_id='', plot_type='indv'):
	#---------PLOT SETTINGS-------------------
	plt.rc('font', size=18)          # controls default text sizes
	plt.rc('legend', fontsize=18)    # legend fontsize
	plt.rc('axes', titlesize=20)     # fontsize of the axes title
	plt.rc('axes', labelsize=20)    # fontsize of the x and y labels
	plt.rc('xtick', labelsize=18)    # fontsize of the tick labels
	plt.rc('ytick', labelsize=18)    # fontsize of the tick labels


	#http://queirozf.com/entries/pandas-dataframe-plot-examples-with-matplotlib-pyplot
	ax = plt.gca()
	colors = ['black', 'red', 'blue', 'green', '#4893d5','magenta', 'cyan', '#f4d03f']
	#styles = ['solid','dashed']
	markers = ['o', 'o', 'o', 'o', 'o', 'o','o','o']  #['o', 's', 'p', '^', 'v', 'x','>','*']
	marker_sizes = [2, 2, 2, 2] #[10, 8, 6, 4, 2, 1]

	if plot_type == 'indv':
		for idx, meth in enumerate(methods):
			plot_df.plot(kind='line', x='Time', y=node_id + "-" + meth + "-" + "Throughput", color=colors[idx], lw=2, marker=markers[idx], markersize=marker_sizes[idx], label=meth, ax=ax)

		plot_title = node_id+ "-" + "Rt"+ str(rate) + "-throughput"
		drate = rate * 1024 * 8
		thrpt_min = 1.0
		thrpt_max = (float(drate)/ 1000000)+0.08
		thrpt_limit = (float(drate)/ 1000000)
		#plt.axhline(y = thrpt_limit, color = 'b', label = 'Max Throughput') 

	elif plot_type == 'aggr':
		for idx, meth in enumerate(methods):
			plot_df.plot(kind='line', x='Time', y=meth + "-" + "TotalThroughput", color=colors[idx], lw=2, marker=markers[idx], markersize=marker_sizes[idx], label=meth, ax=ax)
	
		plot_title = "All-" + "Rt"+str(rate)+"-throughput"
		drate = len(nodes) * rate * 1024 * 8
		thrpt_min = 2.0 #1.75
		#thrpt_max = math.floor((float(drate)/ 1000000))
		thrpt_max = 3.5#3.5

	lines, labels = ax.get_legend_handles_labels()
	ax.legend(lines, labels, loc=0)
	ax.set_ylabel("Throughput (Mbps)")
	ax.set_xlabel("Time ("+time_unit+")")
	ax.set_xticks(plot_df.Time)
	ax.xaxis.set_major_locator(plt.MaxNLocator(10))#https://jakevdp.github.io/PythonDataScienceHandbook/04.10-customizing-ticks.html

	
	#print("Total throughput limit: %f"%thrpt_limit)
	
	#ax.set_ylim([0, math.ceil(thrpt_limit)]) 

	ax.set_ylim([thrpt_min, thrpt_max]) 
	ax.grid(True) # Turn on the grid

	plt.title(plot_title)
	
	plt.savefig(out_path + "/plots/"+plot_title + '.pdf',  bbox_inches = "tight")
	#plt.show()
	
	plt.close()


#~~~~~~~~~~~~~~~~~~~~~~~~~~
if __name__ == "__main__":

	parser = argparse.ArgumentParser(description='provide arguments for plotting')
    # arguments
	parser.add_argument('--rate', help='interest sending rate')
	parser.add_argument('--d', help='simulation duration')	
	parser.add_argument('--br', help='best route', default='0')	
	parser.add_argument('--rd', help='random', default='0')
	parser.add_argument('--fp', help='fixed probability', default='0')
	parser.add_argument('--ma', help='moving average', default='0')
	parser.add_argument('--ml', help='machine learning', type=int , default=0)
	parser.add_argument('--mltype', help='model type', type=str , default='LR')
	parser.add_argument('--mlid', help='model id', type=str , default='')
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
	add_ml = bool(int(args['ml']))
	
	model_type = str(args['mltype'])
	model_id = str(args['mlid'])
	
	net_id = str(args['netid'])
	trace_id = str(args['trcid'])
	out_dir = str(args['outid'])
	
	if 'extid' in args:
		ext_id = str(args['extid'])
	else:
		ext_id = ''

	file_name = net_id + "-1-count-trace.txt" 
	
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
	if add_ml:
		methods.append('ML')
	
	for node_id in nodes:
		for meth in methods:
			columns.append(node_id + "-" + meth + "-" + "Throughput")
	
	
	comp_df = pd.DataFrame(columns=columns)

	print("~~~~~~THROUGHPUT~~~~~~~")
	col_added = False
	for idx, node_id in enumerate(nodes):
		for meth in methods:
			print(node_id, meth)
			
			plot_df, thrpt = get_plot_df(file_name, node_id, meth)
			if not col_added:	
				comp_df[['Time']] = plot_df[['Time']]
				col_added = True

			comp_df[[node_id + "-" + meth + "-" + "Throughput"]] = plot_df[['Throughput']]
	
	
	#compute total and avg throughput for each method
	for meth in methods:
		aggr_cols = []
		for idx, node_id in enumerate(nodes):
			aggr_cols.append(node_id + "-" + meth + "-" + "Throughput") 

		comp_df[meth + "-" + "TotalThroughput"] = comp_df.loc[:, aggr_cols].sum(axis=1)	
		comp_df[meth + "-" + "AvgThroughput"] = comp_df.loc[:, aggr_cols].mean(axis=1)	


	#plot throughput for each method
	plot_data(comp_df, node_id='', plot_type='aggr')
	for idx, node in enumerate(nodes):
		plot_data(comp_df, node_id=node, plot_type='indv')		

	comp_df.loc['Mean'] = comp_df.mean(axis=0)
	comp_df.to_csv(out_path + "/plot_data/"+ "total-throughput.csv", index=False)
	#-------------------------------------------------------------------		
	

	

