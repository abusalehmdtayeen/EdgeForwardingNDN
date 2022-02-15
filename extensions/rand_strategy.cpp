#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <bits/stdc++.h>
#include <ndn-cxx/lp/tags.hpp>  //for hop count tag

#include "rand_strategy.hpp"

#include <boost/random/uniform_int_distribution.hpp>
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

#include "FnvHash.hpp"

NFD_LOG_INIT("RandStrategy");

namespace nfd {
namespace fw {

RandStrategy::RandStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
  this->start_time = ::ndn::time::steady_clock::now();

  std::string user_name;
  
  user_name = getenv("USER");
  if (user_name == "tayeen")
    this->base_path = "/home/"+ user_name + "/Research/ndnSIM/scenario/";
  else
    this->base_path = "/home/"+ user_name + "/ndnSIM/scenario/";

  this->file_path = base_path + "rand-traces/";
}

RandStrategy::~RandStrategy()
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

void RandStrategy::init( const fib::NextHopList& nexthops, std::string flow_id){
    std::cout << "Initializing.." << std::endl; 
    uint64_t sel_face_id;
    
    fib::NextHopList::const_iterator selected;
    
    //getting the node name
    ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
    std::string node_name = ns3::Names::FindName(node);
    //std::cout << node_name << std::endl;
    
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
        
        double capacity = double(bitrate /(1000)) ; //link bandwidth in kbps
        
		if (this->face_capacity.count(sel_face_id) == 0) { //the face id does not exist in map  
			this->face_capacity[sel_face_id] = capacity;
		}
         
    } //end of for

}

void RandStrategy::set_sending_time(uint32_t nonce, uint64_t sel_face_id){

  FnvHash32 intTime;	
  intTime.Update((uint8_t*)&nonce,sizeof(nonce));
  intTime.Update((uint8_t*)&sel_face_id, sizeof(sel_face_id));
  uint32_t timeHash = intTime.Digest();
  this->interestTimes[timeHash] = ndn::time::steady_clock::now();        //adding current time to calculate rtt

}

void RandStrategy::set_outgoing_stat(std::string out_flow_id){
    //set outgoing face statistics
    if (this->f_count.count(out_flow_id) == 0) { //the face id does not exist in map    
        RandStrategy::FaceCount fc;
        fc.inc_out_count();
        this->f_count[out_flow_id] = fc;
    }
    else{
        FaceCount& fc = this->f_count[out_flow_id];
        fc.inc_out_count();
    } 
}


void
RandStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_TRACE("afterReceiveInterest");

  std::string interest_name = interest.getName().toUri();
  uint32_t int_nonce = interest.getNonce();

  std::string delimiter = "/";
  std::string producerName = interest.getName().toUri();
  producerName.erase(0,6);         //remove first / from interestname /data/p1
  producerName = producerName.substr(0, producerName.find(delimiter)); //flow name

  //set incoming face statistics
  uint64_t in_face_id = inFace.getId(); //get the incoming faceID
  std::string str_in_face_id = std::to_string(in_face_id);
  std::string in_flow_id = producerName + "-" +str_in_face_id; //make an id for flows

  std::string flow_id = producerName; //to keep track which flow has only one hop

  // face counter stats
  if (this->f_count.count(in_flow_id) == 0) { //the face id does not exist in map    
     RandStrategy::FaceCount fc;
     fc.inc_in_count();
     this->f_count[in_flow_id] = fc;
  }
  else{
     FaceCount& fc = this->f_count[in_flow_id];
     fc.inc_in_count();
  }

  //getting the node name
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  std::string node_name = ns3::Names::FindName(node);
  //---------------------------------------
  std::cout << node_name << " at time " << ns3::Simulator::Now().ToDouble(ns3::Time::S) << " Interest " << interest_name << " received through " << in_face_id << std::endl;

  // get the hop count for the interest
  shared_ptr<ndn::lp::HopCountTag> hopCountTag = interest.getTag<ndn::lp::HopCountTag>();
  int hopCount = 0;
  if(hopCountTag != NULL){
       hopCount = *hopCountTag;

  }
  //std::cout << node_name << " Interest has hop count " << hopCount << std::endl;

  bool isRandom;
  if (hopCount == 1)
  	isRandom = true; //choose face randomly for this interest
  else if (hopCount > 1)
	isRandom = false; // choose the best face for this interest
	  	
 
  bool isRetransmitted = hasPendingOutRecords(*pitEntry);   

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  
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
  }
      
  if (this->pkt_snd_count >= this->pkt_snd_thrs) //needs to write face statistics and reset
  {
		write_vectors(node_name);		
		this->pkt_snd_count = 0;
  }

  
  uint64_t current_face_id, sel_face_id;
  std::string curr_flow_id;
  //--------------------------------------
  fib::NextHopList::const_iterator selected;

  if (isRandom) {
  	 //std::cout << "choose next hop randomly for the flow" << std::endl;
  	 do {
    	 boost::random::uniform_int_distribution<> dist(0, this->flow_next_hops[flow_id].size() - 1);
    	 const size_t randomIndex = dist(m_randomGenerator);
		 //std::cout << "random index " << randomIndex << " chosen out of " << this->flow_next_hops[flow_id].size() << " indices" << std::endl;
		 //find the index of a face id that matches the random index
		 size_t currentIndex = 0;
		 for (std::vector<uint64_t>::iterator it = this->flow_next_hops[flow_id].begin(); it != this->flow_next_hops[flow_id].end(); ++it, currentIndex++)
		 {  
			current_face_id = *it; 
			
         	curr_flow_id = producerName + "-" + std::to_string(current_face_id);
         	if (currentIndex == randomIndex)
            	break;
    	 }
		 
		 sel_face_id = current_face_id;
		 //std::cout << "current face id " << current_face_id <<  " inface id " <<  in_face_id << std::endl;
  	  } while(current_face_id == in_face_id);//while (!canForwardToNextHop(inFace, pitEntry, *selected));
  }
  else{
  	//std::cout << "choose the best next hop" << std::endl;
  	std::vector<uint64_t>::iterator it = this->flow_next_hops[flow_id].begin();
	sel_face_id = *it;
  }

  //std::cout << "Final selected face: " << sel_face_id << std::endl;

  //get the face id pointer for the matched index 
  for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) {
       current_face_id = selected->getFace().getId();  
       if (sel_face_id == current_face_id)
           break;  
  }

  //sel_face_id = selected->getFace().getId();
  std::string str_out_face_id = std::to_string(sel_face_id);
  std::string out_flow_id = producerName + "-" + str_out_face_id;

  if (this->m_fit.count(out_flow_id) == 0) { //the flow-face id does not exist in map    
      RandStrategy::FaceInfo fc_info;
      fc_info.inc_interest_count(); //interest count for outgoing face
	  if (isRetransmitted)
		 fc_info.inc_rt_count();

      this->m_fit[out_flow_id] = fc_info;
  }
  else{
      FaceInfo& fc_info = this->m_fit[out_flow_id];
      fc_info.inc_interest_count();
	  if (isRetransmitted)
		 fc_info.inc_rt_count();
  }     

  set_outgoing_stat(out_flow_id);

  set_sending_time(int_nonce, sel_face_id);

  this->pkt_snd_count++;


  write_interest(node_name, interest_name, int_nonce, sel_face_id, isRetransmitted);

  this->sendInterest(pitEntry, selected->getFace(), interest);

}

//write statistics for each flow-face pair(for training)
void RandStrategy::write_vectors( std::string node_id)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-face_stat.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

	
    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    outputFile << time << ",";
	
	//link capacities of the faces
	for (std::map<uint64_t, double>::iterator it = this->face_capacity.begin(); it != this->face_capacity.end(); ++it) {
        outputFile << (*it).first << "," << (*it).second << ",";
    }
	uint8_t indx = 0;
    
	for(std::map<std::string, std::vector<uint64_t>>::iterator itr = this->flow_next_hops.begin(); itr != this->flow_next_hops.end(); ++itr)
    {  
		std::string curr_flow = itr->first; 
		std::vector<uint64_t> next_hops = itr->second;
		
		for (unsigned int i = 0; i < next_hops.size(); i++){
			std::string curr_flow_face_id = curr_flow + "-" + std::to_string(next_hops[i]);

        	uint32_t data_count = this->m_fit[curr_flow_face_id].get_data_count();
			uint32_t interest_count = this->m_fit[curr_flow_face_id].get_interest_count();
			uint32_t rt_count = this->m_fit[curr_flow_face_id].get_rt_count();
			double rtt_sum = this->m_fit[curr_flow_face_id].get_delay_sum();
			double avg_rtt = this->m_fit[curr_flow_face_id].get_avg_delay();
			double sat_ratio; 
			if ( data_count >= interest_count) //min sat_ratio is always 0.0
                sat_ratio = 1.0;
             else if(data_count < interest_count)
                sat_ratio = double(data_count) / interest_count;
			
			std::cout << curr_flow_face_id << " " << data_count << " " << interest_count << " " << sat_ratio << std::endl;
        	//reset all the stats
			this->m_fit[curr_flow_face_id].reset();
        	outputFile << curr_flow_face_id << "," << interest_count << "," << data_count  << "," << rt_count << ","  << rtt_sum  << "," << avg_rtt << "," << sat_ratio;
			if (i != next_hops.size()-1)
				outputFile << ",";
			
		}
        if (indx == this->flow_next_hops.size()-1)
			outputFile << std::endl;
		else
			outputFile << ",";

		indx++;
    };

    outputFile.close();
}


void RandStrategy::write_interest(std::string node_id, std::string interest_name, uint32_t nonce, uint64_t fid, bool isRt)
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


void RandStrategy::write_data(std::string node_id, std::string data_name, uint32_t int_nonce ,uint32_t data_nonce, uint64_t fid, double delay)
{
    std::ofstream outputFile;
    std::string filename = this->file_path+node_id+"-data.csv";
    outputFile.open (filename, std::ios::out | std::ios::app);

    double time = ns3::Simulator::Now().ToDouble(ns3::Time::S);
    //outputFile << time << "," << node_id << "," << fid << "," << rtt << std::endl;

    outputFile << time << "," << data_name << "," << data_nonce << "," << int_nonce << "," << fid << "," << delay << std::endl;
 
    outputFile.close();
}

void RandStrategy::write_counts(std::string node_id)
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


/*---------------------------------*/
void RandStrategy::beforeSatisfyInterest(const shared_ptr< pit::Entry>& pitEntry, const Face& inFace,
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
         data_nonce = outRecord.getLastNonce(); //find the latest nonce if retransmitted
         break;
      }
    } //end of for

    FnvHash32 intTime, oldintTime;
    uint32_t interest_nonce = pitEntry->getInterest().getNonce();
	uint32_t nonce = data_nonce;

    intTime.Update((uint8_t*)&nonce,sizeof(nonce));
    intTime.Update((uint8_t*)&face_id, sizeof(face_id));
    uint32_t timeHash = intTime.Digest();

    //calculate latency for current interest satisfied 
    double rtt_ms = double(ndn::time::duration_cast<::ndn::time::milliseconds>(ndn::time::steady_clock::now() - interestTimes[timeHash]).count()); //(std::time_t
    this->interestTimes.erase(timeHash);    

	//remove the old nonce before retransmission
	oldintTime.Update((uint8_t*)&interest_nonce,sizeof(interest_nonce));
    oldintTime.Update((uint8_t*)&face_id, sizeof(face_id));
    uint32_t oldtimeHash = oldintTime.Digest();
	this->interestTimes.erase(oldtimeHash);

	//update the flow-face info
	if (this->m_fit.count(in_flow_id) == 0) { //the flow-face id does not exist in map    
      RandStrategy::FaceInfo fc_info;
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
        RandStrategy::FaceCount fc;
        fc.inc_sat_count();
        this->f_count[in_flow_id] = fc;
    }
    else{
        FaceCount& fc = this->f_count[in_flow_id];
        fc.inc_sat_count();
    } 
    

    write_data(node_name, data_name, interest_nonce, data_nonce, face_id, rtt_ms);
}

void
RandStrategy::beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry)
{
  //NFD_LOG_DEBUG("beforeExpirePendingInterest pitEntry=" << pitEntry->getName());
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  std::string node_name = ns3::Names::FindName(node);
  //std::cout << "Node name: " << node_name << std::endl;
  std::string delimiter = "/";
  std::string producerName = pitEntry->getInterest().getName().toUri();
  producerName.erase(0,6);         //remove first / from interestname /data/p1
  producerName = producerName.substr(0, producerName.find(delimiter));

  //std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl; 
  
  
  int out_records = 0;
  auto lastRenewed = time::steady_clock::TimePoint::min();
  uint64_t last_out_face_id;

  for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
     
     uint64_t out_face_id = outRecord.getFace().getId(); 
     //std::cout << "Packet dropped at face: " << out_face_id << std::endl;
     //time::steady_clock::TimePoint now = time::steady_clock::now();
     //std::cout << outRecord.getExpiry() << " " << now << std::endl;
     //if(outRecord.getExpiry() >= now)
     //   continue;
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

/*----------------------------------*/
const Name&
RandStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/rand-strategy/%FD%01");
  return strategyName;
}

} // namespace fw
} // namespace nfd
