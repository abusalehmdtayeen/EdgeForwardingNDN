#ifndef RANDOM_STRATEGY_HPP
#define RANDOM_STRATEGY_HPP

#include <boost/random/mersenne_twister.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"
#include <vector>
#include <numeric>
#include <map>    

namespace nfd {
namespace fw {

class RandTrainStrategy : public Strategy {
public:
  RandTrainStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  virtual ~RandTrainStrategy() override;

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;


  virtual void 
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace,
  	                    const Data&  data) override; 	

  virtual void
  beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry) override;

  static const Name&
  getStrategyName();
  //-----------------------------

  void set_sending_time(uint32_t nonce, uint64_t sel_face_id);
  void set_outgoing_stat(std::string out_flow_id);

    /*--------------------------------*/
  class FaceInfo
  {
        public:
          FaceInfo() 
          {  
                sat_ratio = 0.0; 
                avg_rtt = 0.0;
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

		  //retransmission count
		  void inc_rt_count() { 
             rt_count++;              
          }

          uint32_t get_rt_count() { 
             return rt_count;              
          }  
		  //-----------------------

          void reset() { data_count = 0; interest_count = 0; rt_count = 0, rtt.clear();}

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
          /*double get_sat_ratio()
          { 
			 double sat_ratio;
             if ( data_count >= interest_count) //min sat_ratio is always 0.0
                sat_ratio = 1.0;
             else if(data_count < interest_count)
                sat_ratio = double(data_count / interest_count);
                
             return sat_ratio;
          }*/  

        private:
          uint32_t data_count = 0, interest_count = 0, rt_count = 0; 
          std::vector<double> rtt;
          double sat_ratio, avg_rtt;
  };


  typedef std::map<std::string, FaceInfo> FaceInfoTable;

  //---------------------------------------------  
  // maps an interest's nonce to the time it was sent, for determining rtt
  std::map<uint32_t, ndn::time::steady_clock::TimePoint> interestTimes;   

  void init( const fib::NextHopList& nexthops, std::string flow_id);

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
  void write_vectors(std::string node_id);

protected:
  boost::random::mt19937 m_randomGenerator;
  std::map<std::string, std::vector<uint64_t>> flow_next_hops;

  std::map<uint64_t, double> face_capacity; //capacity of link faces
  uint64_t pkt_snd_count = 0, pkt_snd_thrs = 400;
 
  FaceInfoTable m_fit;
  ndn::time::steady_clock::TimePoint start_time;
  std::string faceConfigFile = "sprint2-edge-c2-2-face-config";
  std::string base_path = "";
  std::string file_path = "";

};

} // namespace fw
} // namespace nfd

#endif // RANDOM_STRATEGY_HPP

