import torch
from torch import nn

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
