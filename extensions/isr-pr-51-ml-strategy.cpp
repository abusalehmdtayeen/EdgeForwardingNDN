/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <zmq.hpp>
#include "edg-ml-fw3-msg.pb.h"

#include <vector>
#include <map>
#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <bits/stdc++.h> 

#include "isr-pr-51-ml-strategy.hpp"
#include "FnvHash.hpp"
#include <ndn-cxx/lp/tags.hpp> //for hop count
//#include <boost/random/uniform_int_distribution.hpp>
#include <boost/algorithm/string.hpp> //https://thispointer.com/how-to-read-data-from-a-csv-file-in-c/
#include <ndn-cxx/util/random.hpp>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/channel.h"
#include "ns3/simulator.h"
#include "model/ndn-net-device-transport.hpp"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

#include "core/logger.hpp"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/node-list.h"


NFD_LOG_INIT("IsrPr51MLStrategy");

namespace nfd {
namespace fw {

IsrPr51MLStrategy::IsrPr51MLStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
  //setting the start time and initialize a learning time
  this->start_time = ndn::time::steady_clock::now();
  //this->decision_time = ns3::Simulator::Now().ToDouble(ns3::Time::S);//ndn::time::steady_clock::now();
  std::string user_name;

  this->pktSize = (8 * 1024);

  user_name = getenv("USER");
  if (user_name == "tayeen")
    this->base_path = "/home/"+ user_name + "/Research/ndnSIM/scenario/";
  else
    this->base_path = "/home/"+ user_name + "/ndnSIM/scenario/";

  this->file_path = base_path + "/results/IsrPrML/";
}

IsrPr51MLStrategy::~IsrPr51MLStrategy()
{
    
}

static bool
canForwardToNextHop(const Face& inFace, shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return !wouldViolateScope(inFace, pitEntry->getInterest(), nexthop.getFace()) &&
    canForwardToLegacy(*pitEntry, nexthop.getFace());
}

static bool
hasFaceForForwarding(const Face& inFace, const fib::NextHopList& nexthops, const shared_ptr<pit::Entry>& pitEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop, cref(inFace), pitEntry, _1))
         != nexthops.end();
}



void IsrPr51MLStrategy::init( const fib::NextHopList& nexthops, std::string flow_id){
    std::cout << "Initializing.." << std::endl; 
    uint64_t sel_face_id;
    uint16_t hop_count = 0;
    fib::NextHopList::const_iterator selected;
    
    //getting the node name
    ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
    std::string node_name = ns3::Names::FindName(node);
    //std::cout << node_name << std::endl;
    //-------------------Get Face restrictions------------------------------
    map<std::string, int> faceConfig; 
    std::vector<std::vector<std::string> > dataList;
    //std::cout << "Opening face config file: " << this->faceConfigFile << std::endl;
    uint64_t face_limit;
    int count = -1; 
    std::string net_id;

    size_t found = this->faceConfigFile.find("-face-config"); 
    if (found != string::npos){ 
        net_id = this->faceConfigFile.substr(0,0+found); //get the network id from file name
    }

    // Open an existing file 
    std::ifstream infile(this->base_path + "/config/"+net_id+"/"+ this->faceConfigFile + ".csv");

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
    // Print the content row by row 
    for(std::vector<std::string> vec : dataList)
    {
       //std::cout << vec.at(0) << std::endl;
       std::string n_name = vec.at(0); //get the node name
       face_limit = std::stoi(vec.at(1));
       faceConfig[n_name] = face_limit;
    }
    //--------------------------------------
    //get the face limit for the current node
    face_limit = faceConfig[node_name];

    //---------------------------------
    for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) { 
        Face& outFace = selected->getFace();
        //get the id of the face 
        sel_face_id = outFace.getId();
		//store the next hop for this flow
        this->flow_next_hops[flow_id].push_back(sel_face_id);
        uint64_t bitrate = 0;
        
        //get link data rate(bandwidth) and propagation delay for the face
        auto transport = dynamic_cast<ns3::ndn::NetDeviceTransport*>(outFace.getTransport()); //Get Transport for outFace
        if (transport != nullptr) {
            ns3::Ptr<ns3::NetDevice> nd = transport->GetNetDevice(); //Get point-to-point network device associated with the face
            if (nd != nullptr) {
                ns3::DataRateValue res;
                nd->GetAttribute("DataRate", res);
                bitrate = res.Get().GetBitRate();
                ns3::Ptr<ns3::Channel> channel = nd->GetChannel(); //Get the channel associated with the network device
                ns3::TimeValue delay;
                channel->GetAttribute("Delay", delay);
                double hopDelay = delay.Get().ToDouble(ns3::Time::S);
                //std::cout << " BW: " << bitrate << " Delay: " << hopDelay << std::endl;
            }
        }
        
        //uint32_t capacity = uint32_t(bitrate / this->pktSize) ; //link bandwidth in terms of pkts (maximum pkts)
        uint32_t capacity = floor((bitrate * bw_mulp)/ this->pktSize) ; //link bandwidth in terms of pkts (maximum pkts)  //ceil
        //std::cout << sel_face_id << " Capc: " << capacity << std::endl;
		if (this->face_capacity.count(sel_face_id) == 0) { //the face id does not exist in map  
			this->face_capacity[sel_face_id] = capacity;
		}
        hop_count++;
        if (face_limit == hop_count) //limit the number of hops
            break;
            
    } //end of for
    //std::cout << "Exiting init..." << std::endl;
}

uint64_t IsrPr51MLStrategy::findLatestSendFaceId(const shared_ptr<pit::Entry>& pitEntry)
{
  auto lastRenewed = time::steady_clock::TimePoint::min();
  uint64_t last_out_face_id;

  for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
     
     uint64_t out_face_id = outRecord.getFace().getId(); 
     if (outRecord.getLastRenewed() > lastRenewed) {
         last_out_face_id = out_face_id;
         lastRenewed = outRecord.getLastRenewed();
     }
  } //end of for

  return last_out_face_id;
}


uint64_t IsrPr51MLStrategy::get_num_hops_prefix(const fib::NextHopList& nexthops)
{
    uint64_t count_next_hops = 0;
    fib::NextHopList::const_iterator selected;
    for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) {     
        count_next_hops++;
    }//end of for
    return count_next_hops;
}


void IsrPr51MLStrategy::set_sending_time(uint32_t nonce, uint64_t sel_face_id){

  FnvHash32 intTime;	
  intTime.Update((uint8_t*)&nonce,sizeof(nonce));
  intTime.Update((uint8_t*)&sel_face_id, sizeof(sel_face_id));
  uint32_t timeHash = intTime.Digest();
  this->interestTimes[timeHash] = ndn::time::steady_clock::now();        //adding current time to calculate rtt

}

void IsrPr51MLStrategy::set_outgoing_stat(std::string out_flow_id){
    //set outgoing face statistics
    if (this->f_count.count(out_flow_id) == 0) { //the face id does not exist in map    
        IsrPr51MLStrategy::FaceCount fc;
        fc.inc_out_count();
        this->f_count[out_flow_id] = fc;
    }
    else{
        FaceCount& fc = this->f_count[out_flow_id];
        fc.inc_out_count();
    } 
}

// Comparator function to sort pairs according to second value 
static bool cmp(std::pair<FaceId, double>& a, std::pair<FaceId, double>& b) { 
    return a.second < b.second; 
} 
  
// Function to sort the map according to value in a (key-value) pairs 
std::vector<std::pair<FaceId, double> > IsrPr51MLStrategy::sort_map(std::map<FaceId, double>& M) { 
	 // Declare vector of pairs 
	 std::vector<std::pair<FaceId, double> > A; 

	 // Copy key-value pair from Map 
	 // to vector of pairs 
	 for (auto& it : M) { 
		A.push_back(it); 
	 } 

	 // Sort using comparator function 
	 sort(A.begin(), A.end(), cmp); 

	 return A;
}  


void
IsrPr51MLStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{
  //NFD_LOG_TRACE("afterReceiveInterest");
  NFD_LOG_DEBUG("afterReceiveInterest");

  std::string delimiter = "/";
  std::string producerName = interest.getName().toUri();
  producerName.erase(0,6);         //remove first / from interestname /data/p1
  producerName = producerName.substr(0, producerName.find(delimiter)); // "p0" or "p1"
  std::string flow_id = producerName;

  //set incoming face statistics
  uint64_t in_face_id = inFace.getId(); //get the incoming faceID
  std::string str_in_face_id = std::to_string(in_face_id);

  std::string in_flow_id = producerName + "-" +str_in_face_id; //make an id for flows  'p1-256'

  if (this->f_count.count(in_flow_id) == 0) { //the flow id does not exist in map    
     IsrPr51MLStrategy::FaceCount fc;
     fc.inc_in_count();
     this->f_count[in_flow_id] = fc;
  }
  else{
     FaceCount& fc = this->f_count[in_flow_id];
     fc.inc_in_count();
  }

  // get the hop count for the interest
  shared_ptr<ndn::lp::HopCountTag> hopCountTag = interest.getTag<ndn::lp::HopCountTag>();
  int hopCount = 0;
  if(hopCountTag != NULL){
       hopCount = *hopCountTag;

  }
  

  bool isRandom;
  if (hopCount == 1){
  	isRandom = true; //choose face randomly for this interest
    this->src_flow = flow_id;
  }
  else if (hopCount > 1)
	isRandom = false; // choose the best face for this interest


  fib::NextHopList::const_iterator selected;  
  uint32_t face_indx, curr_indx, curr_alloted_pkts, curr_interest_pkts;
  uint64_t sel_face_id, curr_face_id;
  std::vector<uint64_t> face_ids;
  std::vector<double> isr_values, delay_values, avg_satr_values, pr_avg_satr_values, norm_delay_values;  

  
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  //getting the node name
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  std::string node_name = ns3::Names::FindName(node);

  uint32_t int_nonce = interest.getNonce(); //pitEntry->getInterest().getNonce();
  std::string interest_name = pitEntry->getName().toUri();

  //std::cout << node_name << " Interest has hop count " << hopCount << std::endl;
  //std::cout << node_name << " at time " << ns3::Simulator::Now().ToDouble(ns3::Time::S) << " received Interest " << interest_name << " through " << in_face_id << std::endl;

  // consider retransmission as new packet
  bool isRetransmitted = hasPendingOutRecords(*pitEntry);   
  uint64_t last_out_face_id = 0;
  if (isRetransmitted) {
     last_out_face_id = findLatestSendFaceId(pitEntry);
     std::string face_flow_id = flow_id + "-" + std::to_string(last_out_face_id);
  }
  
  //write the statistics every 1000ms(1s)
  std::time_t period = ndn::time::duration_cast<::ndn::time::milliseconds>(ndn::time::steady_clock::now() - this->start_time).count();
  if (period >= 1000)
  {
     //std::cout << "Period: " << period << std::endl;
     write_counts(node_name);
     this->start_time = ndn::time::steady_clock::now();
  }
  
  if (this->flow_next_hops.count(flow_id) == 0) { //the flow id does not exist in map    
     init(nexthops, flow_id); //initialize the face info object
     if (isRandom) { 
        uint32_t hop_count = this->flow_next_hops[flow_id].size(); 
        //std::cout << "IsRandom " << hop_count << std::endl;
        for (std::vector<uint64_t>::iterator it = this->flow_next_hops[flow_id].begin(); it != this->flow_next_hops[flow_id].end(); ++it)
        {  
		    sel_face_id = *it;
            std::string face_flow_id = flow_id + "-" + std::to_string(sel_face_id); 
            //std::cout << "FaceFlow " << face_flow_id << std::endl;
            if (this->m_fit.count(face_flow_id) == 0) { //the flow-face id does not exist in map    
                IsrPr51MLStrategy::FaceInfo fc_info;
                double initial_prob = 1.0 / hop_count;
                fc_info.set_prob(initial_prob); //interest probability for outgoing face
	            this->m_fit[face_flow_id] = fc_info;
                //std::cout << "FaceFlow " << face_flow_id << " " << initial_prob << std::endl; 
            }
            this->last_prob_faces.push_back(sel_face_id);
        }
     } //end of inner if 
  }//end of outer if
  
  uint32_t hop_count = this->flow_next_hops[flow_id].size(); 
  this->max_face_probability = 1.0 - (this->min_face_probability * hop_count);
      
  bool isOneHop = false;
  if (get_num_hops_prefix(nexthops) == 1)
  {
     isOneHop = true;   
  }       

  //std::cout << "Check for pkt send count ..." << std::endl;
  uint32_t total_interest_count = 0, total_data_count = 0;
  vector<uint64_t> prob_sorted_faces;
  double delay_sum = 0.0;
  //get output from ML model and set allocated pkts for face-flow pairs
  if (this->pkt_snd_count >= this->pkt_snd_thrs) //needs to write face statistics and reset
  {
        //std::cout << "write the stats after pkt count "<< this->pkt_snd_count << std::endl;
        write_vectors(node_name);
        
        for (std::vector<uint64_t>::iterator it = this->flow_next_hops[this->src_flow].begin(); it != this->flow_next_hops[this->src_flow].end(); ++it)
        {  
		    sel_face_id = *it;
            std::string face_flow_id = src_flow + "-" + std::to_string(sel_face_id); 
            FaceInfo& fc_info = this->m_fit[face_flow_id];
            double sat_ratio, avg_satratio;
 
            uint32_t data_count = fc_info.get_data_count();
            total_data_count += data_count;

			uint32_t interest_count = fc_info.get_interest_count();
            total_interest_count += interest_count;

			uint32_t rt_count = fc_info.get_rt_count();
			double rtt_sum = fc_info.get_delay_sum();
			double avg_rtt = fc_info.get_avg_delay();

            sat_ratio = fc_info.get_last_satratio();
            avg_satratio = fc_info.get_last_avg_satratio(); 

			delay_values.push_back(avg_rtt);
			//delay_values.push_back(double(rt_count)/interest_count);
			delay_sum += avg_rtt;
			
            isr_values.push_back(sat_ratio);
            avg_satr_values.push_back(avg_satratio);
            face_ids.push_back(sel_face_id);

            double face_prb = fc_info.get_prob();
            double isr_pr_ratio = sat_ratio / face_prb;
            pr_avg_satr_values.push_back(isr_pr_ratio);

            std::cout << face_flow_id << " " << interest_count << " " << data_count << " " << sat_ratio << " " << face_prb << " " << isr_pr_ratio << std::endl;
        	//reset all the stats
			fc_info.reset();
        }
        
        //std::cout << "MaxSatR" << " " << max_sat_ratio << " " << face_indx << " " << std::endl;
        std::map<uint64_t, double> face_isr_values, face_del_values, face_prob_values;
        for (uint8_t k = 0; k < face_ids.size(); k++){
            curr_face_id = face_ids.at(k);
            face_isr_values[curr_face_id] = pr_avg_satr_values.at(k);
            face_del_values[curr_face_id] = delay_values.at(k) / delay_sum;
            norm_delay_values.push_back(delay_values.at(k) / delay_sum);
            //std::cout << curr_face_id << " " << face_isr_values[curr_face_id] << std::endl;
        }

        uint8_t j, k;
        vector<pair<uint64_t, double>> f_isr_vec, f_del_vec, f_prob_vec; 
        //sort faces based on ISR values
        f_isr_vec = sort_map(face_isr_values);  

        f_del_vec = sort_map(face_del_values);
        //std::cout << "Sorted the map" << std::endl;

	    vector<uint64_t> isr_sorted_faces, del_sorted_faces, prob_sorted_faces;
        
        //put them in ascending order
        for (j = 0; j < face_isr_values.size(); j++){  
            //std::cout << "Sorted: " <<  f_isr_vec[j].first << " " << f_isr_vec[j].second << std::endl;  
		    isr_sorted_faces.push_back(f_isr_vec[j].first);
		}

        //put them in ascending order (for delay)
        for (j = 0; j < face_isr_values.size(); j++){  
            //std::cout << "Sorted: " <<  f_isr_vec[j].first << " " << f_isr_vec[j].second << std::endl;  
		    del_sorted_faces.push_back(f_del_vec[j].first);
		  
		}
        //std::cout << "Size of isr_vec: "<< isr_sorted_faces.size()  << " " << isr_sorted_faces.at(0) << " " << isr_sorted_faces.at(1) << std::endl;
        
        //------------------------------
        double max_prob = 0.0, min_prob = 0.0, new_prob = 0.0, res_prob = 1.0, total_prob = 0.0, sat_diff = 0.0;
        
        //set the alloted pkts for the face with highest ISR(interest satisfaction ratio)
        //for (std::vector<uint64_t>::iterator it = this->flow_next_hops[this->src_flow].begin(); it != this->flow_next_hops[this->src_flow].end(); ++it)
        double curr_thrpt = double(total_data_count) / total_interest_count;
        
        double max_ratio = *max_element(pr_avg_satr_values.begin(), pr_avg_satr_values.end());
        double min_ratio = *min_element(pr_avg_satr_values.begin(), pr_avg_satr_values.end()); 
    
        double succ_diff = max_ratio - min_ratio;
    
        double max_del_ratio = *max_element(norm_delay_values.begin(), norm_delay_values.end());
        double min_del_ratio = *min_element(norm_delay_values.begin(), norm_delay_values.end()); 
        
        double nrm_del_diff = max_del_ratio - min_del_ratio;
        
        uint16_t cong_status = send_msg(node_name, face_ids, pr_avg_satr_values, norm_delay_values, succ_diff, nrm_del_diff);
        
        if (cong_status == 1)
        {       
            std::cout << "======="<< total_data_count << " " << curr_thrpt << " " << total_interest_count << "===============" << std::endl;
            
            //need to change the probabilities to reach the throughput
            for (k = 0, j =  isr_sorted_faces.size()-1; k < j; )        
            {  
	            sel_face_id = isr_sorted_faces.at(j); //face with max ratio
                curr_face_id = isr_sorted_faces.at(k); //face with min ratio

                std::string max_face_flow_id = this->src_flow + "-" + std::to_string(sel_face_id); 
                FaceInfo& max_fc_info = this->m_fit[max_face_flow_id];
                max_prob = max_fc_info.get_prob();

                std::string min_face_flow_id = this->src_flow + "-" + std::to_string(curr_face_id); 
                FaceInfo& min_fc_info = this->m_fit[min_face_flow_id];
                min_prob = min_fc_info.get_prob();
                if (min_prob <= this->min_face_probability){
                    k++;
                    std::cout << "Continue: " << curr_face_id << " Min: " << min_prob << sel_face_id << std::endl; 
                    continue;
                }
                else{
                   std::cout << "Not Continue: " << curr_face_id << " Min: " << min_prob << " " << this->min_face_probability << std::endl;
                }
                
                
                sat_diff = abs(face_isr_values[curr_face_id] - face_isr_values[sel_face_id]);

                std::cout << "Max: " << max_face_flow_id << " " << max_prob << " " << "Min: " << min_face_flow_id << " " << min_prob << " " << sat_diff << std::endl;
                    
                bool change_prob = can_change(sel_face_id, curr_face_id);
                   
                //if (sat_diff > sucr_diff_thrs && change_prob){ 
                if (change_prob){ 
                     new_prob = max_prob + this->pth;
                     new_prob = std::min(res_prob, new_prob);
                     max_fc_info.set_prob(new_prob); 

                     total_prob += new_prob;
                     res_prob = (1.0-total_prob);

                     new_prob = min_prob - this->pth;
                     new_prob = std::min(res_prob, new_prob);
                     min_fc_info.set_prob(new_prob); 

                     total_prob += new_prob;
                     res_prob = (1.0-total_prob);
                }
                else {
                     new_prob = std::min(res_prob, max_prob);
                     max_fc_info.set_prob(new_prob);
                     
                     total_prob += new_prob;
                     res_prob = (1.0-total_prob);

                     new_prob = std::min(res_prob, min_prob);
                     min_fc_info.set_prob(new_prob); 

                     total_prob += new_prob;
                     res_prob = (1.0-total_prob);
                }
               
                //prob_sorted_faces.push_back(sel_face_id);
                //prob_sorted_faces.push_back(curr_face_id);
                
                k++;
                j--;
                
            }//end of for

            /*if ((isr_sorted_faces.size()%2 != 0))
               prob_sorted_faces.push_back(isr_sorted_faces.at(j));*/
        }
        else if (cong_status == 2) //check for delay
        {       
            for (k = 0, j = del_sorted_faces.size()-1; k < j; )        
            {     
                sel_face_id = del_sorted_faces.at(j); //face with max delay
                curr_face_id = del_sorted_faces.at(k); //face with min delay

                std::string max_face_flow_id = this->src_flow + "-" + std::to_string(sel_face_id); 
                FaceInfo& max_fc_info = this->m_fit[max_face_flow_id];
                max_prob = max_fc_info.get_prob();
                
                if (max_prob <= this->min_face_probability){
                    j--;
                    std::cout << "Continue: " << sel_face_id << " Max: " << max_prob << curr_face_id << std::endl; 
                    continue;
                }
                else{
                   std::cout << "Not Continue: " << sel_face_id << " Min: " << max_prob << " " << this->min_face_probability << std::endl;
                }


                std::string min_face_flow_id = this->src_flow + "-" + std::to_string(curr_face_id); 
                FaceInfo& min_fc_info = this->m_fit[min_face_flow_id];
                min_prob = min_fc_info.get_prob();
                
                double del_ratio_diff = abs(face_del_values[curr_face_id] - face_del_values[sel_face_id]);
                bool change_prob = can_change(curr_face_id, sel_face_id);
                
                if (change_prob){ 
                   new_prob = min_prob + this->pth;
                   new_prob = std::min(res_prob, new_prob);
                   min_fc_info.set_prob(new_prob); 

                   total_prob += new_prob;
                   res_prob = (1.0-total_prob);

                   new_prob = max_prob - this->pth;
                   new_prob = std::min(res_prob, new_prob);
                   max_fc_info.set_prob(new_prob); 

                   total_prob += new_prob;
                   res_prob = (1.0-total_prob);
                    
                   //std::cout << "Setting alloc for " << face_flow_id << " Curr:" << curr_alloted_pkts << " New:" << new_alloted_pkts << std::endl;
                }
                else
                {
                    new_prob = std::min(res_prob, min_prob);
                    min_fc_info.set_prob(new_prob);
                     
                    total_prob += new_prob;
                    res_prob = (1.0-total_prob);

                    new_prob = std::min(res_prob, max_prob);
                    max_fc_info.set_prob(new_prob); 

                    total_prob += new_prob;
                    res_prob = (1.0-total_prob);
                }   
                //prob_sorted_faces.push_back(curr_face_id);
                //prob_sorted_faces.push_back(sel_face_id);
                k++;
                j--;
                      
            } //end of for
            /*if ((del_sorted_faces.size()%2 != 0))
                prob_sorted_faces.push_back(del_sorted_faces.at(j)); */
            
        } //end of else if thrpt condition
        /*else{
            prob_sorted_faces.clear();
            for (uint8_t k = 0; k < isr_sorted_faces.size(); k++)
            {       
                prob_sorted_faces.push_back(isr_sorted_faces.at(k));
            }
        } //end of congestion
        */    
       
        //update the changed faces
        /*this->last_prob_faces.clear();
        std::cout << "Update the last faces" << std::endl;
        for (uint8_t k = 0; k < prob_sorted_faces.size(); k++)
        {       
            std::cout << prob_sorted_faces.at(k) << std::endl;
            this->last_prob_faces.push_back(prob_sorted_faces.at(k));
        } 
        */      
        std::cout << "~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
        write_allocations(node_name);
        
		this->pkt_snd_count = 0;
  }

  //https://stackoverflow.com/questions/9330394/how-to-pick-an-item-by-its-probability
  //https://en.wikipedia.org/wiki/Fitness_proportionate_selection
  //find the face to forward the interest
  bool found_face = false;
  if (isRandom) {
     //std::cout << "Finding the face for flow " << flow_id << std::endl;
     vector<pair<uint64_t, double>> f_cost_vec; 
     map<uint64_t, double> face_probs; //map a face to its forwarding probability value 
     //get the current allocations
     //for (uint8_t k = 0; k < this->last_prob_faces.size(); k++) 
     for (std::vector<uint64_t>::iterator it = this->flow_next_hops[this->src_flow].begin(); it != this->flow_next_hops[this->src_flow].end(); ++it)
     {  
	    //curr_face_id = this->last_prob_faces.at(k);
	    curr_face_id = *it;
        std::string face_flow_id = src_flow + "-" + std::to_string(curr_face_id);
        FaceInfo& fc_info = this->m_fit[face_flow_id];
        face_probs[curr_face_id] = fc_info.get_prob();
        //cout << face_flow_id << " " << fc_info.get_prob() << endl;
     }

     //cout << "Sorting face probabilities..." << std::endl;
     sel_face_id = 0;
     f_cost_vec = sort_map(face_probs);  
	 vector<double> s_prob;
	 for (uint16_t j = 0; j < face_probs.size(); j++){     
		s_prob.push_back(f_cost_vec[j].second);
		//cout << f_cost_vec[j].first << " " << f_cost_vec[j].second << endl;
	 }

     //get the face id to forward the interest packet based on the forwarding probability
     std::uniform_real_distribution<double> distribution(0.0,1.0);

     double randomNum, limit = 0.0; 
	 randomNum = distribution(generator);
	 //std::cout << "Random: " << randomNum << std::endl;
     for(uint16_t k = 0; k < s_prob.size(); k++ ){
        limit = limit + s_prob.at(k);
	    //std::cout << "Limit: " << limit << std::endl;
        if (randomNum <= limit){
            sel_face_id = f_cost_vec[k].first;
			//std::cout << "Selected: " << sel_face_id << std::endl;
            found_face = true;
			break;
		}
     }

  } //end of if random
  else{
  	//std::cout << "choose the best next hop" << std::endl;
  	std::vector<uint64_t>::iterator it = this->flow_next_hops[flow_id].begin();
	sel_face_id = *it;
    found_face = true;
  }

  /*if (not found_face){
     sel_face_id = nexthops.begin()->getFace().getId(); //when no valid faces were found, choose first face
  } */
  if (not found_face){
    std::cout << this->src_flow << " Face not found. Dropping." << std::endl;
    return; //drop interest
  }  
 
  
  //find the face pointer for the selected face id
  for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) {
        curr_face_id = selected->getFace().getId();  
        if (curr_face_id == sel_face_id)
            break;
  }

  std::string str_out_face_id = std::to_string(sel_face_id);
  std::string out_flow_id = producerName + "-" + str_out_face_id;

  if (this->m_fit.count(out_flow_id) == 0) { //the flow-face id does not exist in map    
      IsrPr51MLStrategy::FaceInfo fc_info;
      fc_info.inc_interest_count(); //interest count for outgoing face
      if (isRetransmitted && sel_face_id == last_out_face_id)
		 fc_info.inc_rt_count();
      this->m_fit[out_flow_id] = fc_info;
  }
  else{
      FaceInfo& fc_info = this->m_fit[out_flow_id];
      fc_info.inc_interest_count();
      if (isRetransmitted && sel_face_id == last_out_face_id)
		 fc_info.inc_rt_count();
  }     


  set_outgoing_stat(out_flow_id);

  set_sending_time(int_nonce, sel_face_id);

  if (isRandom){
  	this->pkt_snd_count++; 
  }

  write_interest(node_name, interest_name, int_nonce, sel_face_id, isRetransmitted);

  this->sendInterest(pitEntry, selected->getFace(), interest);

}

//can probabilities be changed to mainitain the totol prob constant
bool IsrPr51MLStrategy::can_change(uint64_t max_face_id, uint64_t min_face_id)
{
      std::map<uint64_t, double> face_prob_values;
      double prob;
      for (std::vector<uint64_t>::iterator it = this->flow_next_hops[this->src_flow].begin(); it != this->flow_next_hops[this->src_flow].end(); ++it)
      {  
            uint64_t sel_face_id = *it;
            std::string face_flow_id = this->src_flow + "-" + std::to_string(sel_face_id); 
            FaceInfo& fc_info = this->m_fit[face_flow_id];
            prob = fc_info.get_prob();
            face_prob_values[sel_face_id] = prob;
      }
      
      face_prob_values[max_face_id] = face_prob_values[max_face_id] + this->pth;
      face_prob_values[min_face_id] = face_prob_values[min_face_id] - this->pth;
      
      if ( (face_prob_values[max_face_id] > this->max_face_probability) || (face_prob_values[min_face_id] < this->min_face_probability) )
        return false;
      
      double total_prob = 0.0;
      for (std::map<uint64_t, double>::iterator it=face_prob_values.begin(); it!=face_prob_values.end(); ++it)
      {  
          prob = it->second;
          total_prob += prob; 
      } 
      
      double prob_limit = 1.01;
      if (total_prob <= prob_limit){
         std::cout << "Total Prob does not exceed.." << std::endl;
         return true;
      }
      else{
         std::cout << "Total Prob exceeded.." << std::endl;
         return false;  
      }
      
}

//write model output
void IsrPr51MLStrategy::write_vectors(std::string node_id)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-face_stat.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

	
    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    outputFile << time << ",";
	
	//link capacities of the faces
	/*for (std::map<uint64_t, double>::iterator it = this->face_capacity.begin(); it != this->face_capacity.end(); ++it) {
        outputFile << (*it).first << "," << (*it).second << ",";
    }*/
    double delay_sum = 0.0;
    for(std::vector<uint64_t>::iterator itr = this->flow_next_hops[this->src_flow].begin(); itr != this->flow_next_hops[this->src_flow].end(); ++itr)
    {  
        uint64_t curr_face_id = *itr; 
		
		std::string curr_flow_face_id = this->src_flow + "-" + std::to_string(curr_face_id);
        FaceInfo& fc_info = this->m_fit[curr_flow_face_id];
        double avg_rtt = fc_info.get_avg_delay();
		delay_sum += avg_rtt;
		
    }
    
	uint8_t indx = 0;
	uint32_t total_interest_count = 0, total_data_count = 0;
    double sat_ratio, avg_satratio, fc_prob, pr_avg_satr;
    
    vector<double> succ_ratios, nrdel_ratios, sat_ratios;
    
	for(std::vector<uint64_t>::iterator itr = this->flow_next_hops[this->src_flow].begin(); itr != this->flow_next_hops[this->src_flow].end(); ++itr, indx++)
    {  
		uint64_t curr_face_id = *itr; 
		
		std::string curr_flow_face_id = this->src_flow + "-" + std::to_string(curr_face_id);
        FaceInfo& fc_info = this->m_fit[curr_flow_face_id];
        fc_prob = fc_info.get_prob();
        
        uint32_t data_count = fc_info.get_data_count();
        total_data_count += data_count;
		uint32_t interest_count = fc_info.get_interest_count();
		total_interest_count += interest_count;
		uint32_t rt_count = fc_info.get_rt_count();
		//double rtt_sum = this->m_fit[curr_flow_face_id].get_delay_sum();
        //double sat_ratio = this->m_fit[curr_flow_face_id].get_sat_ratio(); 
        //double avg_satratio = this->m_fit[curr_flow_face_id].get_last_avg_satratio();
        
        if (interest_count == 0){
             sat_ratio = fc_info.get_last_satratio();
             avg_satratio = fc_info.get_last_avg_satratio();
        }
        else{
             fc_info.comp_sat_ratio();
             sat_ratio = fc_info.get_last_satratio(); 
             fc_info.set_last_satratio(sat_ratio);
             fc_info.add_in_satratio(sat_ratio);
             avg_satratio = fc_info.get_avg_satratio();
             fc_info.set_last_avg_satratio(avg_satratio);
		}

        double avg_rtt = fc_info.get_avg_delay();
		sat_ratios.push_back(sat_ratio);
		
		double norm_rtt = avg_rtt /delay_sum;  //normalized delay 
		nrdel_ratios.push_back(norm_rtt);

		pr_avg_satr = sat_ratio / fc_prob;
		succ_ratios.push_back(pr_avg_satr);
		double rt_ratio = double(rt_count) / interest_count;
		
        //outputFile << curr_flow_face_id << "," << interest_count << "," << data_count  << "," << rt_count << "," << rt_ratio << "," << avg_satratio << "," << fc_prob << "," << pr_avg_satr << "," << avg_rtt << "," << sat_ratio;
		outputFile << curr_flow_face_id << "," << interest_count << "," << data_count  << "," << rt_count << ","  <<  rt_ratio << "," << avg_satratio << "," << fc_prob << "," << pr_avg_satr << "," << avg_rtt << "," << norm_rtt << "," << sat_ratio;
		
       
		outputFile << ",";	
    }
    double max_ratio = *max_element(succ_ratios.begin(), succ_ratios.end());
    double min_ratio = *min_element(succ_ratios.begin(), succ_ratios.end()); 
    
    double succ_diff = max_ratio - min_ratio;
    
    double max_nrdel_ratio = *max_element(nrdel_ratios.begin(), nrdel_ratios.end());
    double min_nrdel_ratio = *min_element(nrdel_ratios.begin(), nrdel_ratios.end()); 
    
    double del_diff = max_nrdel_ratio - min_nrdel_ratio;
    
    
    double curr_thrpt = double(total_data_count) / total_interest_count;
    outputFile << curr_thrpt << "," << succ_diff << "," << del_diff << ",";
    
    std::string congestion;
    /*if (curr_thrpt < this->thrpt_thrs && succ_diff > sucr_min_diff_thrs)
        congestion = "1"; //congestion: change probability
    else if (curr_thrpt < this->thrpt_thrs && succ_diff <= sucr_min_diff_thrs)
        congestion = "0"; //stable: keep current probability
    else if (curr_thrpt >= this->thrpt_thrs && succ_diff > sucr_min_diff_thrs)
        congestion = "0";
    else if (curr_thrpt >= this->thrpt_thrs && succ_diff <= sucr_min_diff_thrs){
         if (del_diff > delr_diff_thrs) 
            congestion = "2"; //stable condition: keep same probability
         else 
            congestion = "0"; //congestion: change probability
    }*/
    if (curr_thrpt >= this->thrpt_thrs && del_diff <= delr_diff_thrs) 
        congestion = "0"; //stable condition: keep same probability
    else if (curr_thrpt >= this->thrpt_thrs && del_diff > delr_diff_thrs)
        congestion = "2"; //congestion: change probability
    else if (curr_thrpt < this->thrpt_thrs && succ_diff > sucr_diff_thrs)
        congestion = "1"; //congestion: change probability
    else if (curr_thrpt < this->thrpt_thrs && succ_diff <= sucr_diff_thrs)
        congestion = "0"; //stable: keep current probability 

    outputFile << congestion << std::endl;
    outputFile.close();
}

//write allocations for faces
void IsrPr51MLStrategy::write_allocations(std::string node_id)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-face_alloc.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

    //std::cout << "In writing: " << this->flow_next_hops[flow].size() << " " << next_face_indx << std::endl;
    
    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    outputFile << time << ",";

    uint64_t sel_face_id;
    uint8_t indx = 0;
    for (std::vector<uint64_t>::iterator it = this->flow_next_hops[this->src_flow].begin(); it != this->flow_next_hops[this->src_flow].end(); ++it, indx++)
    {  
		sel_face_id = *it;
        std::string face_flow_id = this->src_flow + "-" + std::to_string(sel_face_id); 
       
        FaceInfo& fc_info = this->m_fit[face_flow_id];
        double alloted_prob = fc_info.get_prob(); //current prob for outgoing face
        outputFile << face_flow_id << "," << alloted_prob;
        if (indx == this->flow_next_hops[this->src_flow].size()-1)
			outputFile << std::endl;
		else
			outputFile << ",";
    }
    
    outputFile.close();
}

void IsrPr51MLStrategy::write_interest(std::string node_id, std::string interest_name, uint32_t nonce, uint64_t fid, bool isRt)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-interest.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    //outputFile << time << "," << node_id << "," << fid << "," << rtt << std::endl;
    std::string interest_type;
    if (isRt)
        interest_type = "retransmit";
    else
        interest_type = "new";

    outputFile << time << "," << node_id << "," << interest_name << "," << nonce << "," << interest_type << "," << fid << std::endl;
 
    outputFile.close();
}


void IsrPr51MLStrategy::write_data(std::string node_id, std::string data_name, uint32_t int_nonce ,uint32_t data_nonce, uint64_t fid, double delay)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-data.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    //outputFile << time << "," << node_id << "," << fid << "," << rtt << std::endl;

    outputFile << time << "," << data_name << "," << data_nonce << "," << int_nonce << "," << fid << "," << delay << std::endl;
 
    outputFile.close();
}

void IsrPr51MLStrategy::write_counts(std::string node_id)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-count.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    for (std::map<std::string, FaceCount>::iterator it=this->f_count.begin(); it!=this->f_count.end(); ++it)
    {  
        std::string flowfaceId = it->first; 
        FaceCount& fc = it->second;
        outputFile << time << "," << flowfaceId << "," << fc.get_in_counts() << "," << fc.get_out_counts() << "," << fc.get_sat_counts() << "," << fc.get_drop_counts() << std::endl;
    } 
    outputFile.close();
}


//--------------------------------------

void IsrPr51MLStrategy::beforeSatisfyInterest(const shared_ptr< pit::Entry>& pitEntry, const Face& inFace,
  	                    const Data&  data)
{
    //----getting the node name-----
    ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
    std::string node_name = ns3::Names::FindName(node);
	//----get the flow id----- 
	std::string delimiter = "/";
  	std::string producerName = pitEntry->getInterest().getName().toUri();
 	producerName.erase(0,6);         //remove first / from interestname /data/p1
  	producerName = producerName.substr(0, producerName.find(delimiter));
    /*--------------------------*/ 

    NFD_LOG_DEBUG("beforeSatisfyInterest");
    if (pitEntry->getInRecords().empty()) { // already satisfied by another upstream
        NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() << " not-fastest");
        return;
    }
   
    pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);
    if (outRecord == pitEntry->getOutRecords().end()) { // no OutRecord
          NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() << " no-out-record");
          return;
    }
   
    uint64_t face_id = inFace.getId(); //get the incoming faceID
    std::string str_in_face_id = std::to_string(face_id);
    std::string in_flow_id = producerName + "-" +str_in_face_id; //make an id for flows : "p0-256"
    //-------------------------------------
    std::string data_name = data.getName().toUri();
	uint32_t data_nonce = 0;
    for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
      uint64_t out_face_id = outRecord.getFace().getId(); 
      if (out_face_id == face_id) {
         data_nonce = outRecord.getLastNonce(); //find the latest nonce when transmitted
         break;
      }
    } //end of for

    FnvHash32 intTime;
    uint32_t interest_nonce = pitEntry->getInterest().getNonce();
	uint32_t nonce = data_nonce;

    intTime.Update((uint8_t*)&nonce,sizeof(nonce));
    intTime.Update((uint8_t*)&face_id, sizeof(face_id));
    uint32_t timeHash = intTime.Digest();

    //calculate latency for current interest satisfied 
    double rtt_ms = double(ndn::time::duration_cast<::ndn::time::milliseconds>(ndn::time::steady_clock::now() - interestTimes[timeHash]).count()); //(std::time_t
    this->interestTimes.erase(timeHash);    

    //remove all the old nonces
    for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
       FnvHash32 oldintTime;
       uint64_t out_face_id = outRecord.getFace().getId(); 
       uint32_t old_nonce = outRecord.getLastNonce(); //find the latest nonce when transmitted
       oldintTime.Update((uint8_t*)&old_nonce,sizeof(old_nonce));
       oldintTime.Update((uint8_t*)&out_face_id, sizeof(out_face_id));
       uint32_t oldtimeHash = oldintTime.Digest();
	   this->interestTimes.erase(oldtimeHash);
      
    } //end of for

	
	//update the flow-face info
	if (this->m_fit.count(in_flow_id) == 0) { //the flow-face id does not exist in map    
      IsrPr51MLStrategy::FaceInfo fc_info;
      fc_info.inc_data_count(); //data count for incoming face
	  fc_info.add_delay(rtt_ms);
      this->m_fit[in_flow_id] = fc_info;
  	}
  	else{
      FaceInfo& fc_info = this->m_fit[in_flow_id];
      fc_info.inc_data_count();
	  fc_info.add_delay(rtt_ms);
  	}     

    //set incoming data face statistics
    if (this->f_count.count(in_flow_id) == 0) { //the face id does not exist in map    
        IsrPr51MLStrategy::FaceCount fc;
        fc.inc_sat_count();
        this->f_count[in_flow_id] = fc;
    }
    else{
        FaceCount& fc = this->f_count[in_flow_id];
        fc.inc_sat_count();
    } 
    
    write_data(node_name, data_name, interest_nonce, data_nonce, face_id, rtt_ms);

}


//-------------------------------------------------------
void
IsrPr51MLStrategy::beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry)
{
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  std::string node_name = ns3::Names::FindName(node);
  //std::cout << "Node name: " << node_name << std::endl;
  std::string delimiter = "/";
  std::string producerName = pitEntry->getInterest().getName().toUri();
  producerName.erase(0,6);         //remove first / from interestname /data/p1
  producerName = producerName.substr(0, producerName.find(delimiter));

  
  int out_records = 0;
  auto lastRenewed = time::steady_clock::TimePoint::min();
  uint64_t last_out_face_id;

  for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
     
     uint64_t out_face_id = outRecord.getFace().getId(); 
     
     std::string str_out_face_id = std::to_string(out_face_id);
     std::string flow_id = producerName + "-" +str_out_face_id; //make an id for flows
     if (this->f_count.count(flow_id) == 0) { //the face id does not exist in map    
        FaceCount fc;
        fc.inc_alt_drop_count();
        this->f_count[flow_id] = fc;
     }
     else{
        FaceCount& fc = this->f_count[flow_id];
        fc.inc_alt_drop_count();
     }   
     out_records++;
     if (outRecord.getLastRenewed() > lastRenewed) {
         last_out_face_id = out_face_id;
         lastRenewed = outRecord.getLastRenewed();
     }
  } //end of for

  std::string str_out_face_id = std::to_string(last_out_face_id);
  std::string out_flow_id = producerName + "-" +str_out_face_id; //make an id for flows

  //set packet drop statistics
  if (this->f_count.count(out_flow_id) == 0) { //the face id does not exist in map    
        FaceCount fc;
        fc.inc_drop_count();
        this->f_count[out_flow_id] = fc;
  }
  else{
        FaceCount& fc = this->f_count[out_flow_id];
        fc.inc_drop_count();
  }

}
//===========================================
uint16_t IsrPr51MLStrategy::send_msg(std::string node_name, std::vector<uint64_t> face_ids, std::vector<double> isr_pr_values, std::vector<double> delay_values, double ext_ft1, double ext_ft2)
{
   
    //setup connection : Prepare context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    //std::cout << "Connecting to agent …" << std::endl;
    socket.connect ("tcp://127.0.0.1:"+this->port);
 
    
    edgmlfw3msg::Node node;
    node.set_id(node_name);
	edgmlfw3msg::Node::Face* face;
 
    
    node.set_done(false);    
    uint64_t f_id;
    
    for (uint8_t i = 0; i < face_ids.size(); i++)
	{ 
        f_id = face_ids.at(i);
        //std::cout << f_id << std::endl;
        //std::cout << s_ratios.at(i) << " ";
        //std::cout << interest_delays.at(i) << " ";
		face = node.add_faces();
        face->set_face_id(f_id); //set the face 
		face->set_feature1(isr_pr_values.at(i));
        face->set_feature2(delay_values.at(i));
    }
    
    //node.set_ext_feature(ext_ft);
    node.set_ext_ft1(ext_ft1);
    node.set_ext_ft2(ext_ft2);

	std::string msg_str;
    node.SerializeToString(&msg_str);
	
	// create a zmq message from the serialized string
    zmq::message_t request (msg_str.size());
    memcpy ((void *) request.data (), msg_str.c_str(), msg_str.size());
    std::cout << "Sending ndnSIM data ..." << std::endl;
    socket.send (request);

	//  Get the reply.
    zmq::message_t reply;
    socket.recv (&reply);
    //std::cout << "Received reply from agent " << std::endl;

	edgmlfw3msg::Agent agent;
	//converting the zmq msg to string
	std::string msg_rpl(static_cast<char*>(reply.data()), reply.size());

	agent.ParseFromString(msg_rpl);
	//std::string text_str;
    //google::protobuf::TextFormat::PrintToString(agent, &text_str);
    //std::cout << text_str << std::endl;
    uint16_t cong_status; 
	cong_status = agent.status(); 
	//std::cout << act_id << std::endl;
    return cong_status; 
}


  

//-----------------------------------


const Name&
IsrPr51MLStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/isr-pr-51-ml-strategy/%FD%01");
  return strategyName;
}

} // namespace fw
} // namespace nfd
