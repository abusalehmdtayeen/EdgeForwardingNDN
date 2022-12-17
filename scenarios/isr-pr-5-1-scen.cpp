/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-my-l3-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ptr.h"
#include "ns3/net-device.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-helper.h"
#include <stdlib.h>  
#include <boost/algorithm/string.hpp>

//#include "rnet-ndn-msg.pb.h"
#include "isr-pr-5-1-strategy.hpp"

using namespace ns3;

/**
 * This scenario simulates a random-forwarder topology (using topology reader module)
 *
 *
 * For every received interest, a random face is invoked
 * (based on a custom forwarding strategy) and the selected producer
 * replies with a data packet, containing 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=rand_strategy
 */

using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::AnnotatedTopologyReader;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
map<std::string, std::string> read_config(std::string t_id, std::string int_rate, std::string configFileName);
vector<std::string> read_agents(std::string t_id, int ag_config_id);
void send_terminal(std::string port);
std::string base_path = "";

int
main(int argc, char* argv[])
{
  int runLenArg, dotraceArg, aConfigId, rateArg, sim_id; 
  std::string t_id, agPortId, rate;
  CommandLine cmd;
  
  cmd.AddValue ("tId", "topology ID", t_id);
  cmd.AddValue ("sId", "Simulation ID", sim_id);
  cmd.AddValue("agCnfg", "agent config ID", aConfigId);  
  cmd.AddValue ("trace", " whether tracing will be done", dotraceArg);
  cmd.AddValue ("runLen", "duration of simulation", runLenArg);
  cmd.AddValue ("rate", "interest rate of simulation", rateArg);
  cmd.Parse(argc, argv);

  std::string user_name;
  user_name = getenv("USER");

  if (user_name == "tayeen")
    base_path = "/home/"+ user_name + "/Research/ndnSIM/scenario/";
  else
    base_path = "/home/"+ user_name + "/ndnSIM/scenario/";


  //----------------Simulation Parameters------------
  int dotrace = dotraceArg;
  rate = std::to_string(rateArg);
  std::string resultID = "results/IsrPr";
  std::string configFileName = t_id+"-cons-prod-config.txt";
  std::string topoFileName = "topo-" + t_id + ".txt";
  //--------------------------------------------

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName(base_path+"/topologies/"+topoFileName);
  topologyReader.Read();

  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  /*~~~~ContentStore~~~~*/
  //ndnHelper.setCsSize(1);
  ndnHelper.setCsSize(0);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");

  ndnHelper.InstallAll();
  //set link metric to delay
  ndnHelper.SetLinkDelayAsFaceMetric();

  // Installing global routing interface on all nodes
  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  map<std::string, std::string>::iterator itr;
  map<std::string, std::string> pConfig; 
  std::string producer_name, prefix;
  Ptr<Node> producer_node;

  //configure the producers and consumers
  pConfig = read_config(t_id, rate, configFileName);
     
  //get the agent ids
  vector<std::string> agent_names = read_agents(t_id, aConfigId);
     
  // Choosing forwarding strategy
  /*for (itr = pConfig.begin(); itr != pConfig.end(); ++itr) {
    prefix = itr->second;  
    for (vector<std::string>::iterator it = agent_names.begin(); it != agent_names.end(); ++it)
    {
        StrategyChoiceHelper::Install<nfd::fw::RandomStrategy>(Names::Find<Node>(*it),  prefix);
    }
  }*/
  prefix = "/data/";  
  for (vector<std::string>::iterator it = agent_names.begin(); it != agent_names.end(); ++it)
  {
      StrategyChoiceHelper::Install<nfd::fw::IsrPr51Strategy>(Names::Find<Node>(*it),  prefix);
  }
  // Add /prefix origins to ndn::GlobalRouter 
  for (itr = pConfig.begin(); itr != pConfig.end(); ++itr) {
    producer_name = itr->first;  
    prefix = itr->second;
    //std::cout << "Prefix: " << prefix << " at " << producer_name << std::endl;
    producer_node = Names::Find<Node>(producer_name);
    ndnGlobalRoutingHelper.AddOrigins(prefix, producer_node);  
  }
  
  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes(); //Only provides a single shortest path nexthop.
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes(); //Provides all possible nexthops, but many of them lead to loops.
  
  Simulator::Stop(Seconds(runLenArg));
  
  if (dotrace){
    ns3::ndn::MyL3RateTracer::InstallAll(base_path + resultID +"/" + t_id + "-"+ std::to_string(sim_id) + "-count-trace.txt", Seconds(1.0));
    ns3::ndn::AppDelayTracer::InstallAll(base_path + resultID +"/" + t_id + "-"+ std::to_string(sim_id) + "-app-delays-trace.txt");
    ns3::L2RateTracer::InstallAll (base_path + resultID +"/" + t_id + "-"+ std::to_string(sim_id) + "-drop-trace.txt", Seconds (1.0));
  }

  Simulator::Run();
  
  Simulator::Destroy();

  return 0;
}



//read the nodes for the custom forwarding strategy
vector<std::string> read_agents(std::string t_id, int ag_config_id){
    vector<std::string> agents;
    std::vector<std::vector<std::string> > dataList;

    int count = -1; 
    // Open an existing file 
    std::ifstream infile(base_path + "/config/" + t_id + "/" + t_id + "-agent-config-"+ std::to_string(ag_config_id) + ".csv");

    std::string line = "";
    // Iterate through each line and split the content using delimeter
    while (getline(infile, line))
    {
        count +=1;
        if (count == 0)
            continue; //skip the header
        std::vector<std::string> vec;
        boost::algorithm::split(vec, line, boost::is_any_of(","));
        dataList.push_back(vec);
    }
    infile.close();
    // Print the content of row by row on screen
    for(std::vector<std::string> vec : dataList)
    {
       std::cout << vec.at(0) << std::endl;
       agents.push_back(vec.at(0));
    }

    return agents;
}


map<std::string, std::string> read_config(std::string t_id, std::string int_rate, std::string configFileName){

  std::string configFilePath = base_path + "/config/" + t_id + "/" + configFileName;
  // Getting containers for the consumer/producer
  Ptr<Node> consumer_node;
  Ptr<Node> producer_node;
  //MAP to track consumer and producer already configured
  map<std::string, std::string> prodConfig; 
  std::string prefix;

  //Read the configuration file for consumer and producers  
  ifstream topgen;
  topgen.open(configFilePath.c_str());
  if (!topgen.is_open() || !topgen.good()) {
       std::cout << "Cannot open config file " << configFileName << " for reading";
       exit (EXIT_FAILURE);
  }

  while (!topgen.eof()) {
     string line;
     getline(topgen, line);
     
     if (line == "producer")
         break;
  }
  if (topgen.eof()) {
     std::cout << "Config file " << configFileName << " does not have \"producer\" section";
     exit (EXIT_FAILURE);
  }
  //read the producer section
  while (!topgen.eof()){
     string line;
     getline(topgen, line);
     if (line == "")
        continue;
     if (line[0] == '#')
        continue; // skip comments  
     if (line == "consumer")
         break;

     istringstream lineBuffer(line);
     std::string producer, node, bandwidth, metric,	delay, queue, payload;
 
     lineBuffer >> producer >> node >> bandwidth >> metric >> delay >> queue >> payload >> prefix;
     //std::cout << "Prod: " << producer << " PayloadSize:" << payload << " Prefix:" << prefix << std::endl;

     producer_node = Names::Find<Node>(producer);
     prodConfig[producer] = prefix;
     //std::cout << consumer_node << std::endl;  //consumer_node == 0 if no node is found

     //configure the producer node
     AppHelper producerHelper("ns3::ndn::Producer");
     producerHelper.SetAttribute("PayloadSize", StringValue(payload));
     producerHelper.SetPrefix(prefix);
     producerHelper.Install(producer_node);
     
  }// end of while

  AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //consumerHelper.SetAttribute("Frequency", StringValue(rate)); 
  consumerHelper.SetAttribute("Frequency", StringValue(int_rate));
  //read the consumer section
  while (!topgen.eof()){
     string line;
     getline(topgen, line);
     if (line == "")  //skip empty lines
        continue;
     if (line[0] == '#')
        continue; // skip comments  
     
     istringstream lineBuffer(line);
     std::string producer, consumer, node, bandwidth, metric, delay, queue, rate;
 
     lineBuffer >> producer >> consumer >> node >> bandwidth >> metric >> delay >> queue >> rate;
     prefix = prodConfig[producer];
     std::cout << "Cons: " << consumer << " Rate:" << rate << " Prefix:" << prefix << std::endl;

     consumer_node = Names::Find<Node>(consumer);
     //std::cout << consumer_node << std::endl;  //consumer_node == 0 if no node is found

     //configure the consumer node
     consumerHelper.SetPrefix(prefix);
     consumerHelper.Install(consumer_node);
     
  }// end of while

  return prodConfig;
}
