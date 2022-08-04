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

#ifndef PER_FACE_TDQN_SW_HPP
#define PER_FACE_TDQN_SW_HPP

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"
#include <vector>
#include <random>
#include <numeric>
#include <map>     

namespace nfd {
namespace fw {

//define the action types
//enum action_type { incr=1, decr = -1};
//-------------------------

class IsrPr4MLStrategy : public Strategy {
public:
  IsrPr4MLStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  virtual ~IsrPr4MLStrategy() override;

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  virtual void 
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace,
  	                    const Data&  data) override; 	

  
  virtual void
  beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry) override;


  //virtual void
  //afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,const Face& inFace, const Data& data) override;

  static const Name&
  getStrategyName();

  uint16_t send_msg(std::string node_name, std::vector<uint64_t> face_ids, std::vector<double> isr_pr_values, std::vector<double> delay_values, double ext_ft);
  
  void init(const fib::NextHopList& nexthops, std::string flow_id);

  uint64_t get_num_hops_prefix(const fib::NextHopList& nexthops);
  //void update_face_info(uint64_t sel_face_id);
  void set_sending_time(uint32_t nonce, uint64_t sel_face_id);
  void set_outgoing_stat(std::string out_flow_id);
  
  uint64_t findLatestSendFaceId(const shared_ptr<pit::Entry>& pitEntry);

  std::vector<std::pair<FaceId, double> > sort_map(std::map<FaceId, double>& M);

  void write_vectors(std::string node_id);

  void write_allocations(std::string node_id);
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  class MyQueue { 
    public:
    // Initialize front and rear 
    int rear, front; 
   
    // Circular Queue 
    int size; 
    double *circular_queue; 
   
    MyQueue(int sz) { 
       front = rear = -1; 
       size = sz; 
       circular_queue = new double[sz]; 
    } 
    int enQueue(double elem){
        if ((front == 0 && rear == size-1) || (rear == (front-1)%(size-1)))  { 
        //cout<<"\nQueue is Full"; 
        return -1; 
        } 
        else if (front == -1) {     /* Insert First Element */
            front = rear = 0; 
            circular_queue[rear] = elem; 
        } 
        else if (rear == size-1 && front != 0) { 
            rear = 0; 
            circular_queue[rear] = elem; 
        } 
        else {  
            rear++; 
            circular_queue[rear] = elem; 
        }
	    return 0; //insertion successful 
    
    }
	int deQueue(){
        if (front == -1)  { 
            //cout<<"\nQueue is Empty"; 
            return -1; 
        } 
   
        double data = circular_queue[front]; 
        circular_queue[front] = -1; 
        if (front == rear)  { 
            front = -1; 
            rear = -1; 
        } 
        else if (front == size-1) 
            front = 0; 
        else
            front++; 
   
        return 0; // delete successful 

    }  
    double avgQueue(){
        double sum = 0.0, avg;
	    int q_len = 0;

        if (rear >= front) { 
            for (int i = front; i <= rear; i++){
 			    q_len += 1;
                sum += circular_queue[i];
 		    } 
        } 
        else  { 
            for (int i = front; i < size; i++){
		    	q_len += 1; 
                sum += circular_queue[i]; 
   		    }
            for (int i = 0; i <= rear; i++){
			    q_len += 1; 
                sum += circular_queue[i];
		    } 
        } 

	    avg = sum / q_len;

	    return avg;

    }

    
	int statusQueue(){
        if (front == -1) { 
            //cout<<"\nQueue is Empty"<<endl; 
            return -1; 
        }
	    else
		    return 0;

    }
  }; //end of class queue 


  /*--------------------------------*/
  class FaceInfo
  {
        public:
          FaceInfo() 
          {  
                last_avg_sat_ratio = 0.0; 
                last_sat_ratio = 0.0;
                avg_rtt = 0.0;
                last_satratios = new MyQueue(5);
                t = 0; //time step
          }
          
          void inc_data_count() { 
             data_count++;              
          }

          uint32_t get_data_count() { 
             return data_count;              
          }  

          void inc_interest_count() { 
             interest_count++;              
          }

          uint32_t get_interest_count() { 
             return interest_count;              
          }  
          //-----------------------
          void set_alloted_pkts(uint32_t pkt_count){
            alloted_count = pkt_count;
          }

          uint32_t get_alloted_pkts(){
                return alloted_count;
          } 


		  //retransmission count
		  void inc_rt_count() { 
             rt_count++;              
          }

          uint32_t get_rt_count() { 
             return rt_count;              
          }  
		  //-----------------------

          void reset() { data_count = 0; interest_count = 0; rt_count = 0;}
      
          double comp_sat_ratio() {
             double sat_ratio;
             
             if (data_count >= interest_count) //max sat_ratio is always 1.0
                sat_ratio = 1.0;
             else if (data_count < interest_count)
                sat_ratio = double(data_count) / interest_count;
			
             set_last_satratio(sat_ratio);
          }
          
          void add_delay(double delay) { 
             rtt.push_back(delay);
          }

          //sum of rtt values
          double get_delay_sum() { 
             double delay_sum = 0.0;
             for(std::vector<double>::iterator iter=rtt.begin(); iter != rtt.end(); iter++)  
                   delay_sum += *iter; 
			 return delay_sum;
          }
          
          double get_avg_delay()
          { 
              double delay_sum;
              if (rtt.empty())
                  avg_rtt = 0.0;
              else {
                  delay_sum = get_delay_sum();
                  avg_rtt = delay_sum / rtt.size(); 
              }
              return avg_rtt;
          } 
          //--------------------------------------
          void reset_t() { t = 0;}
          uint32_t get_t() { return t; }  

          void inc_t(){
              t++;
          }
          void dec_t(){ 
              if (t > 0) 
                 t--;
              else
                 t = 0;
          }

		  //-----------------------
          void add_in_satratio(double satratio) 
          { 
                int status = last_satratios->enQueue(satratio);
                if (status == -1){ //queue is full
                    last_satratios->deQueue();
                    last_satratios->enQueue(satratio);
                }
          }

         
          double get_avg_satratio(){
              double average; 
              if (last_satratios->statusQueue())
                 average = 1.0;
              else
                 average = last_satratios->avgQueue();                      
               
              //last_avg_sat_ratio = average;
              return average;
          }  
          
          void set_last_satratio(double satratio){ last_sat_ratio = satratio; }
          double get_last_satratio(){ return last_sat_ratio; }

          void set_last_avg_satratio(double sat_ratio) { last_avg_sat_ratio = sat_ratio;} 
          double get_last_avg_satratio(){ return last_avg_sat_ratio;}   
          //-------------------------
          void set_prob(double pr) { prob = pr; }
          double get_prob() { return prob; } //probability

          
        private:
          MyQueue *last_satratios;
          uint32_t data_count = 0, interest_count = 0, rt_count = 0, alloted_count = 0, t; 
          std::vector<double> rtt;
          double last_avg_sat_ratio, avg_rtt, last_sat_ratio, prob;
  };


  typedef std::map<std::string, FaceInfo> FaceInfoTable;

  
  // maps an interest's nonce to the time it was sent, for determining rtt
  std::map<uint32_t, ndn::time::steady_clock::TimePoint> interestTimes;    
  
  //std::unordered_map <std::string, ndn::time::steady_clock::TimePoint> interestTimes;

  //void removeFaceInfo(shared_ptr<Face> face);

  /*~~~~~~~~~~~~~~~~~~~*/
  class FaceCount
  {
        public:
          FaceCount() { num_incoming_interests = 0; num_outgoing_interests = 0; num_satisfied_interests = 0; num_dropped_interests=0; num_alt_dropped_interests=0; }
          void inc_in_count(){ num_incoming_interests++; }
          void inc_out_count(){ num_outgoing_interests++; }
          void inc_sat_count(){ num_satisfied_interests++; }
          void inc_drop_count(){ num_dropped_interests++; }
          void inc_alt_drop_count(){ num_alt_dropped_interests++; }
          
          uint64_t get_in_counts() { return num_incoming_interests;}
          uint64_t get_out_counts() { return num_outgoing_interests;}
          uint64_t get_sat_counts() { return num_satisfied_interests;}
          uint64_t get_drop_counts() { return num_dropped_interests;}
          uint64_t get_alt_drop_counts() { return num_alt_dropped_interests;}
          
        private:
          uint64_t num_incoming_interests, num_outgoing_interests, num_satisfied_interests, num_dropped_interests, num_alt_dropped_interests;
  };
  
  
  typedef std::map <std::string, FaceCount> FACEcount;
  FACEcount f_count;  

  void write_counts(std::string node_id);
  void write_interest(std::string node_id, std::string interest_name, uint32_t nonce, uint64_t fid, bool isRt);
  void write_data(std::string node_id, std::string data_name, uint32_t int_nonce ,uint32_t data_nonce, uint64_t fid, double delay);

 

protected:
  FaceInfoTable m_fit;
  //boost::random::mt19937 m_randomGenerator;  
  std::default_random_engine generator;

  ndn::time::steady_clock::TimePoint start_time;
  std::map<uint64_t, double> face_capacity; //capacity of link faces
  std::map<std::string, std::vector<uint64_t>> flow_next_hops;
  bool start = false;
  uint64_t pkt_snd_count = 0, pkt_snd_thrs = 200;
  std::string src_flow = "";

  uint64_t pktSize;
  double pth = 0.05; //5% increase or decrease
  double thrpt_thrs = 0.85;
  double satr_diff_thrs = 0.0; //satratio diff threshold

  
  double bw_mulp = 1.0;
  std::string faceConfigFile = "sprint2-edge-c2-6-face-config";//"sprint2-edge-c2-6-face-config"; 
  std::string port = "5555";
  std::string base_path = "";
  std::string file_path = "";
  
  
};

} // namespace fw
} // namespace nfd

#endif // PER_FACE_TDQN_SW_HPP
