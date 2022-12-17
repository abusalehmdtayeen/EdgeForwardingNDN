import math
import os
import sys
import csv
import networkx as nx

#---------------------------------------
def make_dir(path):
    try:
       directory = path
       if not os.path.exists(directory):
          os.makedirs(directory)
    except OSError:
       print ('Error: Creating directory. ' +  directory)

#-------------------------------------
def read_config(base_path, net_id, cfg_id):
	producer_dict = {}
	consumer_dict = {}
	with open(base_path+"/config/"+net_id + "/"+net_id+"-cons-prod-config-" + str(cfg_id) +".txt", 'r') as reader:
		# Read and print the entire file line by line
		line = reader.readline()
		
		while line.startswith("#"):  #skip the comments 
			line = reader.readline().strip()
			#print(line)
		
		if (line == "producer"): #producer section starts
			line = reader.readline() #skip the comment
			line = reader.readline().strip()
			while len(line) != 0:
			 	line_items = line.split("\t")
				#print(line_items)
			 	if line_items[0] not in producer_dict:
			 		producer_dict[line_items[0]] = {'node': line_items[1],'bandwidth': line_items[2], 'metric': line_items[3], 'delay': line_items[4], 'queue': line_items[5]}
				
				#print(line_items[0], line_items[1], line_items[2])
			 	line = reader.readline().strip()
		else:
			print("Error: producer section missing..")
			sys.exit(0)	

		line = reader.readline().strip() 
		#print(line)
		if (line == "consumer"): #consumer section starts
			line = reader.readline() #skip the comment
			line = reader.readline().strip()
			while len(line) != 0:
			 	line_items = line.split("\t")
				#print(line_items[0], line_items[1], line_items[2])
			 	if line_items[1] not in consumer_dict:
			 		consumer_dict[line_items[1]] = {'node': line_items[2], 'flow': line_items[0].lower(), 'bandwidth': line_items[3], 'metric': line_items[4], 'delay': line_items[5], 'queue': line_items[6], 'rate': line_items[7]}
			 	line = reader.readline().strip()
		else:
			print("Error: consumer section missing..")
			sys.exit(0)	

	return producer_dict, consumer_dict

#----------------------------------------
def get_graph(base_path, net_id):
	G= nx.DiGraph() #nx.Graph()
	with open(base_path+"/config/"+net_id + "/"+net_id+"-link-faces.txt", 'r') as f:
        # Read and print the entire file line by line
		lines = f.readlines()
		for line_txt in lines:
			line = line_txt.strip()
			#print(line)
			edge = line.split(" ")[0]
			face_id = line.split(" ")[1] 
			#print(edge, face_id)
			src, dst = edge.split("->")[0], edge.split("->")[1] 
			if src not in G:
				G.add_node(src, label=src)
			if dst not in G:
				G.add_node(dst, label=dst)
			G.add_edge(src, dst, face=face_id)

	print("~~~~~~~~~~~~~~~Network~~~~~~~~~~~~~~~~~")
	#print(G.edges())
	print("Number of nodes: %d"%G.number_of_nodes())
	print("Number of edges: %d"%G.number_of_edges())
	#edge = ('Node10', 'Node9')
	#print(G.get_edge_data(*edge)['face']) #get edge data 

	return G

#-----------------------------------------
def get_node_face_info(base_path, net_id, flow):
	node_face_dict = {}
	read_line = False
	with open(base_path+"/config/"+net_id + "/"+net_id+"-node-faces.txt", 'r') as f:
        # Read and print the entire file line by line
		lines = f.readlines()
		for line_txt in lines:
			line = line_txt.strip()
			if line.startswith("C") or line.startswith("P") or line.startswith("N"):
				node_name = line
				continue #skip the node names
			#print(line, node_name)
			if line.startswith("/data/"+flow):
				read_line = True
				node_face_dict[node_name] = {}
				continue
			
			if read_line:
				faces = line.split(" ")
				#print(faces)
				node_face_dict[node_name][flow] = faces
				read_line = False			

	return node_face_dict

#--------------------------------------------
def read_node_info(base_path, net_id):
    node_label_dict = {}
    with open(base_path+"/config/"+net_id + "/" + net_id+'-node-labels.csv') as csv_file:
         csv_reader = csv.reader(csv_file, delimiter=',')
         next(csv_reader) #skip the header
         for row in csv_reader:
             node_id = row[0]
             node_label = row[1]
             node_label_dict[node_id] = node_label

    return node_label_dict

#-----------------------------------------
def get_node_link_info(G, node_label_dict):
	node_link_dict = {}
	for edge in G.edges():
		src, dst = edge[0], edge[1]
		face_id = G.get_edge_data(*edge)['face']
		#print(src, dst, face_id)
		if src not in node_label_dict:
			src_label = src
		else:
			src_label = node_label_dict[src]
		if dst not in node_label_dict:
			dst_label = dst
		else:
			dst_label = node_label_dict[dst]

		if src not in node_link_dict:
			node_link_dict[src] = {}

		node_link_dict[src][face_id] = dst_label #src_label+"->"+dst_label
		
	#print(node_link_dict)
	return node_link_dict

#----------------------------------------
def get_agent_info(base_path, net_id, ag_cfg_id):
	nodes = []
	with open(base_path+"/config/"+ net_id + "/"+net_id +"-agent-config"+"-"+str(ag_cfg_id)+".csv", 'r') as csv_file:
		csv_reader = csv.reader(csv_file, delimiter=',')
		next(csv_reader) #skip the header
		for row in csv_reader:
			node_id = row[0]
			nodes.append(node_id)

	return nodes
