# Edge forwarding using machine learning in NDN.

## Installation


1. Follow the steps from the link below:

[Installing ndnSIM-2.7](https://www.fatalerrors.org/a/installing-ndn-sim2.7-in-ubuntu18.html)

2. Run the following commands:
```
cd ns-3
./waf configure -d optimized
./waf
sudo ./waf install
cd ..
git clone https://github.com/named-data-ndnSIM/scenario-template.git scenario
cd scenario
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

./waf configure

./waf --run <scenario>
```

## Description

This repository contains a random strategy and its relevant files to generate training dataset for edge routers.

## Dataset

The datasets for each edge router are stored in the `traces` folder. The format of the files are below.

```
time, face1, link_capacity_face1, face2, link_capacity_face2, ..., flow_face_id, interest_count, data_count, retransmission_count, rtt_sum, avg_rtt, sat_ratio, flow_face_id, interest_count, .... 
```

 
