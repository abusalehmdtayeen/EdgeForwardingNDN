# Edge forwarding using machine learning in NDN.


## Description

This repository contains a random strategy and its relevant files to generate training dataset for edge routers.

## Dataset

The datasets for each edge router are stored in the `traces` folder. The format of the files are below.

```
time, face1, link_capacity_face1, face2, link_capacity_face2, ..., flow_face_id, interest_count, data_count, retransmission_count, rtt_sum, avg_rtt, sat_ratio, flow_face_id, interest_count, .... 
```

 
