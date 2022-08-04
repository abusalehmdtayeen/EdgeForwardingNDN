import pandas as pd
import sys
import os
import matplotlib as mpl
#matplotlib.use('agg')
import matplotlib.pyplot as plt
import argparse
import math
import getpass
from gen_helper import *
#===============================
curr_path = os.getcwd()
user_name = getpass.getuser()
#print(user_name)

if (user_name == "tayeen"):
	sim_path = '/home/'+ user_name + '/Research/ndnSIM/scenario/'
else:
	sim_path = '/home/' + user_name + '/ndnSIM/scenario/'

time_unit = "s"

#============================================

def get_plot_df(file_name, node_id, method, metric_type):
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
	
	df_raw['Time'] = df_raw['Time'].astype(int)
	df = df_raw[["Time", "Node", "AppId", "Type", "DelayS"]] #get the necessary columns
	
	max_time = duration #int(df_raw['Time'].max())
	#print(max_time)
	
	# filtering data 
	subset_df = df[ (df['Node'] == node_id) & (df['Type'].str.match(metric_type)) & (df['Time'] < max_time) ]
	#print(subset_df)
	
	dl_df = subset_df.copy(deep=True)
	dl_df['DelayMS'] = subset_df['DelayS']*1000
	#print(dl_df)
	w_df = dl_df[["Time", "DelayMS"]] #get the necessary columns
	w_df.to_csv(out_path + "/plot_data/"+ method + "-" + node_id + "-" + metric_type + ".csv", index=False)
		
	avg_delay = dl_df['DelayMS'].mean()
	print("Avg delay:%f"%avg_delay)
	
	dl_cdf_df_temp = dl_df.copy(deep=True)
	dl_cdf_df = dl_cdf_df_temp[["DelayMS"]] #get the column for cdf
	
	#https://queirozf.com/entries/pandas-dataframe-groupby-examples
	agg_df = dl_df.groupby(['Time'], as_index=False).mean()
	#print(agg_df)

	#-------------------------------------------
	#plot_df = pd.DataFrame(columns=['Time', 'AvgDelay'])
	plot_df = pd.DataFrame({'Time': pd.Series([], dtype='int'),'AvgDelay': pd.Series([], dtype='float')})
	
	# iterate over rows with iterrows()
	for index, row in agg_df.head(n=agg_df.shape[0]).iterrows():
		t = row['Time'] + 1
		plot_df = plot_df.append({'Time': t, 'AvgDelay': row['DelayMS']}, ignore_index=True)		

	return plot_df, dl_cdf_df
	
#----------------------------------------------------
def plot_data(plot_df, node_id, delay_type):
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

	max_delays = []
	for idx, meth in enumerate(methods):
		plot_df.plot(kind='line', x='Time', y=node_id + "-" + meth + "-" + "Delay", color=colors[idx], lw=2, marker=markers[idx], markersize=marker_sizes[idx], label=meth, ax=ax)
		max_delays.append(plot_df[node_id + "-" + meth + "-" + "Delay"].max()/1000.0)

	lines, labels = ax.get_legend_handles_labels()
	ax.legend(lines, labels, loc=0)
	ax.set_ylabel("Avg Delay (ms)")
	ax.set_xlabel("Time ("+time_unit+")")
	ax.set_xticks(plot_df.Time)
	ax.xaxis.set_major_locator(plt.MaxNLocator(10))
	ax.grid(True) # Turn on the grid


	plt.title(node_id+"-Rt"+str(rate)+"-"+delay_type+'-delay')
	#max_y_limit = math.ceil(max(max_delays))*1000
	max_y_limit = max(max_delays)*1000 + 100	
	ax.set_ylim([0, max_y_limit])  

	
	plt.savefig(out_path + "/plots/" + node_id + "-Rt"+str(rate) + "-" + delay_type +'-delay.pdf',  bbox_inches = "tight")
	#plt.show()
	
	plt.close()

#--------------------------------------------
def fix_hist_step_vertical_line_at_end(ax):
    axpolygons = [poly for poly in ax.get_children() if isinstance(poly, mpl.patches.Polygon)]
    for poly in axpolygons:
        poly.set_xy(poly.get_xy()[:-1])

#-----------------------------------
def plot_cdf_data(node_id, cdf_dict, delay_type):
	colors = ['black', 'red', 'blue', 'green', '#4893d5','magenta', 'cyan', '#f4d03f']

	n_bins = 300

	#---------PLOT SETTINGS-------------------
	plt.rc('font', size=18)          # controls default text sizes
	plt.rc('legend', fontsize=18)    # legend fontsize
	plt.rc('axes', titlesize=20)     # fontsize of the axes title
	plt.rc('axes', labelsize=20)    # fontsize of the x and y labels
	plt.rc('xtick', labelsize=18)    # fontsize of the tick labels
	plt.rc('ytick', labelsize=18)    # fontsize of the tick labels

	#fig, ax = plt.subplots(figsize=(12, 6))
	ax = plt.gca()
	for idx, meth in enumerate(methods): 
		plot_df = cdf_dict[meth]
		col_name = meth
		
		#https://stackoverflow.com/questions/39728723/vertical-line-at-the-end-of-a-cdf-histogram-using-matplotlib/52921726
		#print(plot_df)
		plot_df = plot_df.rename(columns={'DelayMS': col_name})

		#df_arr = plot_df["DelayMS"].to_numpy()

		# plot the cumulative histogram
		#n, bins, patches = ax.hist(df_arr, n_bins, density=True, histtype='step', cumulative=True, label=meth, alpha=1.0, color=colors[idx], linewidth=2)
		ax = plot_df.plot.hist(ax=ax, bins=n_bins, cumulative=True, density=True, histtype='step', label=meth, alpha=1.0, color=colors[idx], linewidth=2, legend=False)
		fix_hist_step_vertical_line_at_end(ax)

	# tidy up the figure
	lines, labels = ax.get_legend_handles_labels()
	ax.legend(lines, labels, loc='best')

	ax.grid(True)
	
	ax.set_title(node_id + "-Rt"+str(rate)+"-"+delay_type+ '-delay CDF')
	ax.set_xlabel('Delay (ms)')
	ax.set_ylabel('F(x)')
	
	plt.savefig(out_path + "/plots/" + node_id + "-Rt"+str(rate) + "-" + delay_type +'-delay-cdf.pdf', bbox_inches = "tight")
	#plt.show()
	
	plt.close()


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	
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
	parser.add_argument('--trcid', help='trace dir_id', default='')
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
	
		
	delay_types = ["LastDelay", "FullDelay"]
	delay_labels = ["net", "app"]

	file_name = net_id + "-1-app-delays-trace.txt" #application delay
	
	for ix, dl_type in enumerate(delay_types):
		columns = ["Time"]
		for node_id in nodes:
			for meth in methods:
				columns.append(node_id + "-" + meth + "-" + "Delay")

		comp_df = pd.DataFrame(columns=columns)
	
		col_added = False
		print("~~~~~~DELAY: %s~~~~~~~"%dl_type)
		for idx, node_id in enumerate(nodes):
			cdf_dict = {}
			for meth in methods:
				print(node_id, meth)
			
				plot_df, dl_cdf = get_plot_df(file_name, node_id, meth, dl_type)
				cdf_dict[meth] = dl_cdf
				if not col_added:
					comp_df[['Time']] = plot_df[['Time']]
					col_added = True

				comp_df[[node_id + "-" + meth + "-" + "Delay"]] = plot_df[['AvgDelay']]

			plot_cdf_data(node_id, cdf_dict, delay_labels[ix])	
			#print(comp_df)
			plot_data(comp_df, node_id, delay_labels[ix])

		comp_df.to_csv(out_path + "/plot_data/"+ "avg-" + delay_labels[ix] + "-delay.csv", index=False)	
	
	#---------------------------------------				
	#avg_df['BR-Delay'] = avg_df.iloc[:, [1,3]].mean(axis=1)
	#avg_df[extra_id+"-Delay"] = avg_df.iloc[:, [2,4]].mean(axis=1)

	
	
