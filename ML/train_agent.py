import os
import sys
import torch
import numpy as np
import pandas as pd
import pickle
#import matplotlib.pyplot as plt

from gen_helper import *
from torch.utils.data import Dataset, DataLoader
from torch import nn
from torch.nn import functional as F

from imblearn.over_sampling import SMOTE

from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.linear_model import LogisticRegression
from sklearn import svm


#======================================
sim_path = os.path.expanduser('~') + '/Research/ndnSIM/scenario/'
curr_path = os.getcwd()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class dataset(Dataset):
  def __init__(self,x,y):
    self.x = torch.tensor(x,dtype=torch.float32)
    self.y = torch.tensor(y,dtype=torch.float32)
    self.length = self.x.shape[0]
 
  def __getitem__(self,idx):
    return self.x[idx],self.y[idx]  

  def __len__(self):
    return self.length

#---------------------------------
class Net(nn.Module):
  def __init__(self,input_dim, hidden_dim):
    super(Net,self).__init__()
    self.fc1 = nn.Linear(input_dim, hidden_dim)
    self.fc2 = nn.Linear(hidden_dim, hidden_dim//2)
    self.fc3 = nn.Linear(hidden_dim//2, 1)  

  def forward(self,x):
    x = torch.relu(self.fc1(x))
    x = torch.relu(self.fc2(x))
    x = torch.sigmoid(self.fc3(x))
    return x

#------------------------------
def load_data(agent_node):
	df = pd.read_csv(data_path + agent_node + "-"+ train_file +".csv", sep=",", index_col=False, keep_default_na=False)
	features = df.columns
	#print(features)
	#x = df[features]
	y = df['Label']
	x = df.drop('Label', axis=1)
	
	#print(x)
	#print(y)
	print("shape of x: {}\nshape of y: {}".format(x.shape,y.shape))
	# define resampling
	#over_sample = SMOTE(sampling_strategy='minority')
	over_sample = SMOTE(sampling_strategy={1: (x.shape[0]//2)})
	x, y = over_sample.fit_resample(x, y)
	print("shape of x: {}\nshape of y: {}".format(x.shape,y.shape))
	#print( (y == 1).sum() )
	#print(x)

	#perform standard normalization
	sc = StandardScaler()
	x = sc.fit_transform(x)
	#print(x)
	
	return x, y
#--------------------------------
def train(ag_node, x, trainloader, in_dim, hid_dim):
	model = Net(in_dim, hid_dim)
	optimizer = torch.optim.SGD(model.parameters(),lr=learning_rate)
	loss_fn = nn.BCELoss()

	losses = []
	accur = []
	for i in range(epochs):
		for j, (x_train,y_train) in enumerate(trainloader):
    		#calculate output
			output = model(x_train)
			#print(output)
			#calculate loss
			loss = loss_fn(output,y_train.reshape(-1,1))
 			#accuracy
			predicted = model(torch.tensor(x,dtype=torch.float32))
			#print(predicted)
			acc = (predicted.reshape(-1).detach().numpy().round() == y).mean()    #backprop
			optimizer.zero_grad()
			loss.backward()
			optimizer.step()
			
		if i%2 == 0:
			losses.append(loss)
			accur.append(acc)
			print("epoch {}\tloss : {}\t accuracy : {}".format(i,loss,acc))

	#saving the model	
	torch.save(model.state_dict(), out_path + ag_node + "-" + train_file + "model.pth")
	
#---------------------------------------	
def train_model(ag_node, model_type, X_train, y_train):
	if model_type == 'RF':
		# creating a RF classifier
		clf = RandomForestClassifier(n_estimators = 50)
	elif model_type == 'LR':
		clf = LogisticRegression()
	elif model_type == 'DT':
		# Create Decision Tree classifer object
		clf = DecisionTreeClassifier(criterion="entropy", max_depth=5) 
	elif model_type == 'SVM-L':
		#Create a svm Classifier
		clf = svm.SVC(kernel='linear') # Linear Kernel
	elif model_type == 'SVM-NL':
		#Create a svm Classifier
		clf = svm.SVC(kernel='rbf') # Gaussian Kernel 
 
	# Training the model on the training dataset
	# fit function is used to train the model using the training sets as parameters
	clf.fit(X_train, y_train)
	
	# save the classifier to disk (https://machinelearningmastery.com/save-load-machine-learning-models-python-scikit-learn/)
	clf_file_name = train_file + '-' + model_type + '.sav'
	pickle.dump(clf, open(out_path + ag_node + "-" + clf_file_name, 'wb'))
	
	
#-----------------------------------
if __name__ == '__main__':

	#---hyper-parameters-----
	learning_rate = 0.01
	epochs = 10
	hidden_dim = 16
	#------------------------
	train_file = "10000s-f1"
	net_id = "sprint2-edge-c2-6" 
	ag_cfg_id = 1
	nodes, flows = [], []
	data_path = curr_path + "/" + net_id + "/training/"
	out_path = curr_path + "/" + net_id + "/training/models/"  
	model_types = ['RF', 'LR', 'DT', 'SVM-L', 'SVM-NL']
	
	
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

	for agent in agent_nodes:
		X, y = load_data(agent)
		input_dim = X.shape[1]
		for ml_type in model_types:
			print("Training model: %s for node %s"%(ml_type, agent))
			train_model(agent, ml_type, X, y)
		
		#trainset = dataset(X, y) #create custom dataset
		#trainloader = DataLoader(trainset, batch_size=64, shuffle=False)
		#print("Training agent: %s"%agent)
		#train(agent, X, trainloader, input_dim, hidden_dim)







