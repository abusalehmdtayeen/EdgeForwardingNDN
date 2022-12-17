import numpy as np
import torch
import os
import glob
import csv
import sys
import random
import time
import math
import statistics 
import zmq
import google.protobuf.text_format
#from edg_ml_fw2_msg_pb2 import *
from edg_ml_fw3_msg_pb2 import *
from gen_helper import *
import signal
import subprocess
import getpass
import pickle
import shutil
import argparse

from os import listdir

from models import Net

#======================================
curr_path = os.getcwd()
user_name = getpass.getuser()
print(user_name)

if (user_name == "tayeen"):
	sim_path = '/home/'+ user_name + '/Research/ndnSIM/scenario/'
else:
	sim_path = '/home/' + user_name + '/ndnSIM/scenario'

br_trace_dir = "traces"

#-----------------------------------------
def remove_traces():
    files = glob.glob(sim_path + '/'+br_trace_dir+'/*.*', recursive=True)
    for f in files:
       try:
          os.remove(f)
       except OSError as e:
          print("Error: %s : %s" % (f, e.strerror))
          
#------------------------------------
def find_filenames_ext( path_to_dir, extension=".csv" ):
    filenames = listdir(path_to_dir)
    return [ filename for filename in filenames if filename.endswith( extension ) ]

#-------------------------------------------
def setup_conn():
    context = zmq.Context()
    global socket
    socket = context.socket(zmq.REP)
    socket.bind("tcp://127.0.0.1:"+port)

    print('Binding to port '+port)

#------------------------------------------
def get_state():
    print ("Waiting for client..")
    try:
      message = socket.recv()
    except zmq.error.ZMQError:
      print("Error occured, setting up connection again")
      socket.close()
      setup_conn()
      message = socket.recv()
    
    node_obj = Node()
    node_obj.ParseFromString(message)
    node_id = node_obj.id    
    print ("Received message from client: %s"%node_id) 

    if node_obj.HasField('done'):
       done = bool(node_obj.done)
    else:
       done = False 

    if done:
       return node_id, None, None, done
	
    state_features = []
    f_ids = []
    for face in node_obj.faces:
        if face.HasField('face_id'):
           face_id = face.face_id 
           f_ids.append(face_id)
        isrpr = face.feature1
        state_features.append(isrpr)
        if face.HasField('feature2'):
           delay = face.feature2
           #if feature_id in ['f2', 'f3']:
           state_features.append(delay)
	
    ext_ft1 = node_obj.ext_ft1
    ext_ft2 = node_obj.ext_ft2
    print(f_ids)
    #if extra_features == 1:
    state_features.append(ext_ft1)
    state_features.append(ext_ft2)
    
    print ("Received state: ")
    print (state_features)
    #print (done)
    
    return node_id, f_ids, state_features, done

#---------------------------------
def send_action(action_id):
    agent_obj = Agent()
    agent_obj.status = action_id
    ag_msg = agent_obj.SerializeToString()
    print("Action sent: %d"%action_id)
    socket.send(ag_msg)

#---------------------------------------------
def clean_up(agent_obj_dict):
    print ('Cleaning up resources...')
    for agent_id in agent_obj_dict.keys():	
        write_csv(str(agent_id)+"-traces" + ".csv", traces[str(agent_id)])
        
    socket.close()

#--------------------------------------------
#https://pythonadventures.wordpress.com/2012/11/21/handle-ctrlc-in-your-script/
def sigint_handler(signum, frame):
    clean_up(agent_obj_dict)
    sys.exit(0)
 
signal.signal(signal.SIGINT, sigint_handler)

#------------------------------------------
#runs a new simulation similar to resetting the environment  
def run_episode(episode_num, rate, sim_len, a_config_id, dotrace):
    sim_working_dir = sim_path #'/home/tayeen/Research/ndnSIM/scenario'
    if dotrace:
       trace = 1
       make_dir(sim_path+"/results/IsrPrML")
    else:
       trace = 0
    print ("Running episode %d"%episode_num)
    command = './waf --run="isr-pr-51-ml-scen --tId='+net_id+ " --sId="+str(episode_num) +" --agCnfg="+ str(a_config_id) + " --agPort="+ port +" --trace="+str(trace)+ " --runLen="+str(sim_len)+ " --rate="+str(rate) + " --cfgId=" + str(cons_prod_config_id) + '"'
    p = subprocess.Popen(command, universal_newlines=True, shell=True, stdout=None, stderr=None, cwd=sim_working_dir)
    
#------------------------------------------------
def write_csv(file_path, data):

    with open(file_path, 'w', newline='') as csvfile:
        cwriter = csv.writer(csvfile, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        for d in data:    
            cwriter.writerow(d)

#-----------------------------------------
def create_agents(model_type):
    agent_dict = {}
    for agent in agent_nodes:
        ag_flow = node_flow_dict[agent]
        node_face_indx_dict = get_node_face_info(sim_path, net_id, ag_flow)
        ag_faces = node_face_indx_dict[agent][ag_flow]     
        input_dim = len(ag_faces) * num_features + extra_features
        if model_type == 'MLP':     
            model = Net(input_dim, hid_dim)         
            model_path = model_root_dir + "/" + agent + "-" + train_file + "-MLP-h"+str(hid_dim)+".pth"	
            model.load_state_dict(torch.load(model_path))
        else:
            print("load model %s from disk"%model_type)
            model_path = model_root_dir + "/" + agent + "-" + feature_id + '-' + model_type + '.sav'
            model = pickle.load(open(model_path, 'rb'))	
        	
        agent_dict[agent] = model

    print("%d agents created"%len(agent_dict))	
    return agent_dict
    
#-----------------------------------------------
def get_scaling_info():
    ag_scale_info = {}
    for agent in agent_nodes:
       file_path = data_path + "/" + agent + "-" + feature_id + "-max-min.csv"	    
       csv_data = []
       #print ("Reading %s ...."%file_name)
       with open(file_path, 'r') as csvfile:
            csvreader = csv.DictReader(csvfile)
		    #get fieldnames from DictReader object and store in list
            fieldnames = csvreader.fieldnames 
		
            for row in csvreader:
               record = {}
               for field in fieldnames:
                  record[field] = row[field]
               csv_data.append(record)    
       ag_scale_info[agent] = csv_data        
    
    return ag_scale_info 
#-----------------------------------
def get_action(ag_id, model_type, state):
    agent_model = agent_obj_dict[ag_id] #get agent model object
    
    print("Model type: %s"%model_type)
    
    if model_type == 'MLP':   
       agent_input = torch.from_numpy(state)
       agent_output = agent_model(agent_input.float())        
       agent_out = agent_output.detach().numpy().round().flatten()[0]
    else:
       agent_out = agent_model.predict(state)
    
    print("Output: %f"%agent_out)
    return int(agent_out)	

#--------------------------------------------
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description='provide arguments plotting')
    parser.add_argument('--rate', help='interest sending rate')
    parser.add_argument('--dur', help='simulation duration')
    parser.add_argument('--mtype', help='model type')
    parser.add_argument('--fid', help='feature id')
    #parser.add_argument('--agcfg', help='agent config id')
    #parser.add_argument('--tid', help='topology id')
    
    args = vars(parser.parse_args())

    model_type = str(args['mtype']) #model type
    feature_id = str(args['fid'])
    rate = int(args['rate'])
    duration = int(args['dur'])
    
    #-------Network Parameters------------
    net_id = "sprint2-edge-c4-6" #"sprint2-edge-c4-6" #"sprint2-bttnck-1" 
    agent_config_id = 1
    cons_prod_config_id = 1 #consumer/producer configuration file
    port = "5555"
    out_dir_id = "EdgeMLFlow_results"
    train_file = "IsrPr51-3000s-r100-150-200" #"IsrPr52-l5000-r200" #"IsrPr51-3000s-r150-225" #"IsrPr51-3000s-r200" #"IsrPr51-3000s-r150-225"
    #------------SIMULATION PARAMETERS-----------
    EPISODES = 1	
    #rate = 200 #interest sending rate
    #duration = 120 #seconds of simulation   
    pkt_thrs = 200
    num_features = 2 #number of features per face
    extra_features = 2
    Trace = True
    #-------------Others---------------------
    hid_dim = 8
    data_path = curr_path + "/data/" + net_id + "/" + train_file +"/" #curr_path+"/data/"+net_id+"/"
    model_root_dir = curr_path + "/saved_models/" + net_id + "/" + train_file + "/"   #curr_path+"/saved_models/"+net_id+"/"
    
    #model_type = 'RF' #['RF', 'LR', 'DT', 'SVM-L', 'SVM-NL', 'MLP']
    #feature_id = "f2"
    
    
    if feature_id == 'f27':
       num_features = 1 #number of features per face
       extra_features = 1
    elif feature_id == 'f2':
       num_features = 2 #number of features per face
       extra_features = 1
    elif feature_id == 'f3':
       num_features = 2 #number of features per face
       extra_features = 0
    elif feature_id == 'f0':
       num_features = 1 #number of features per face
       extra_features = 0
    
    
    #--------------------------
    
    G = get_graph(sim_path, net_id)
    p_dict, c_dict = read_config(sim_path, net_id, cons_prod_config_id)
	#print(c_dict)
	#print(p_dict)
    node_flow_dict = {}
    nodes, flows = [], []
    for c in c_dict.keys():
        cid = c[1:]
        nodes.append(c)
        node_flow_dict[c_dict[c]['node']] = c_dict[c]['flow']

    for p in p_dict.keys():
        pid = p[1:]
        flows.append("p"+str(pid))		 	
		
	#---------------------------------	
    agent_nodes = get_agent_info(sim_path, net_id, agent_config_id)
    agent_obj_dict = create_agents(model_type)
    agent_scale_info = get_scaling_info()
    
    socket = None
    setup_conn()

    traces = {}
    for agent_id in agent_obj_dict.keys():
        traces[agent_id] = []
        
    start_time = time.time()

    out_path = curr_path+"/"+out_dir_id+'/'+ net_id + "/" + train_file + "/" + model_type
    if(Trace):
      make_dir(out_path)

    
    episodes = []

    for e in range(1, EPISODES+1):
        done = False
        
        run_episode(e, rate, duration, agent_config_id, Trace)
        
        while not done:   
            #get the state
            node_id, f_ids, state_features, done = get_state()

            if done:
                print("End of episode %d"%e)
                break
            scale_info = agent_scale_info[str(node_id)]
            print(scale_info)    
            scaled_features = []
            for idx, ft in enumerate(state_features):
                mean_val = float(scale_info[idx]['Mean'])
                std_val = float(scale_info[idx]['Std'])
                scale_val = (ft - mean_val) / std_val
                scaled_features.append(scale_val)
                
            state_size = len(f_ids)*num_features + extra_features
            print(state_size)
            state = np.reshape(np.array(scaled_features), [1, state_size])
       
            # get action for the current state and go one step in environment 
            action = get_action(str(node_id), model_type, state)
            send_action(action) #perform the action in the environment
            traces[str(node_id)].append(state_features+[action])
            
    
    #------------------find best route nodes--------------------------------
    file_names = find_filenames_ext(sim_path + '/'+br_trace_dir+'/')
	#filter node names from file names
    logged_nodes = [ fname[:fname.rfind("-")] for fname in file_names if 'count' in fname] #get the nodes that has been logged
    br_nodes = [nd for nd in logged_nodes if nd not in agent_obj_dict.keys()] #get the names of best route nodes
    print("Best route nodes")
    print(br_nodes)
    #~~~~~~~~~~~~~~~copy trace files~~~~~~~~~~~~~~~~~~~
    dst = sim_path+"/results/IsrPrML/"
    for nd in br_nodes:
	    #print(dst)
        try:
           shutil.copy(sim_path+"/"+br_trace_dir+"/"+nd+"-count.csv", dst+nd+"-count.csv")
           shutil.copy(sim_path+"/"+br_trace_dir+"/"+nd+"-delay.csv", dst+nd+"-delay.csv") 
           shutil.copy(sim_path+"/"+br_trace_dir+"/"+nd+"-interest.csv", dst+nd+"-interest.csv")
           shutil.copy(sim_path+"/"+br_trace_dir+"/"+nd+"-data.csv", dst+nd+"-data.csv")       
        except FileNotFoundError: 
           pass
    #https://linuxize.com/post/python-delete-files-and-directories/
    remove_traces() #remove files from traces directory     
         
    #~~~~~~~~~~~~ 
    if (Trace):
       # renaming output directory
       src_dir = sim_path+"/results/IsrPrML"
       dst_dir = "IsPrMl5"+"-"+feature_id + "-r"+str(rate)
       dst_path = sim_path+"/results/"+ dst_dir 
       os.rename(src_dir, dst_path)
       shutil.move(dst_path, out_path+"/")
       #save the test traces
       for agent_id in agent_obj_dict.keys():
           write_csv(out_path + "/"+dst_dir + "/" + train_file + "-" + str(agent_id)+"-traces" + ".csv", traces[agent_id])
    #clean_up(agent_obj_dict)
    
    end_time = time.time()
    print("Total run time: %s seconds"%(end_time - start_time))
