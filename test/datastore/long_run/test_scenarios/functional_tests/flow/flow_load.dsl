flow bridge01 add in_port=0 in_phy_port=1 write_metadata=3 apply_actions=dl_type:2,nw_proto:3,output:controller

flow bridge01 add in_port=0 in_phy_port=2 write_metadata=4 apply_actions=dl_type:5,nw_proto:6,output:local
