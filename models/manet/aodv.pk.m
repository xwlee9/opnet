MIL_3_Tfile_Hdr_ 91A 40A opnet 40 3E788CF3 3E81CEA0 A sunblade20 opnet 0 0 none none 0 0 none BB08A356 8B6 0 0 0 0                                                                                                                                                                                                                                                                                                                                                                                                                           ����             0x0             NONE       @                         (                 
   UDP Header Overhead   
    @  
      ,/* AODV control packets are sent over UDP	*/   )/* This field models the UDP overhead		*/   
          integer             signed, big endian          
   64   
       ����             set             NONE          
@�0G����   
          1                (   <              
   Options   
    @  
   ,    /* Options in the AODV packet	*/   typedef struct   	{   	int				type;   	void*			value_ptr;   	} AodvT_Packet_Option;   	   /* Route Request Option	*/   typedef struct   	{   	Boolean				join_flag;   	Boolean				repair_flag;   	Boolean				grat_rrep_flag;   	Boolean				dest_only_flag;   !	Boolean				unknown_seq_num_flag;   	int					hop_count;   	int					rreq_id;   	InetT_Address		dest_addr;   	int					dest_seq_num;   	InetT_Address		src_addr;   	int					src_seq_num;   	} AodvT_Rreq;       /* Route Reply Option	*/   typedef struct   	{   	Boolean				repair_flag;   	Boolean				ack_required_flag;   	int					prefix_size;   	int					hop_count;   	InetT_Address		dest_addr;   	int					dest_seq_num;   	InetT_Address		src_addr;   	double				lifetime;   	} AodvT_Rrep;       /* Route Error Option	*/   typedef struct   	{   	Boolean				no_delete_flag;   	int					num_unreachable_dest;   &	InetT_Address		unreachable_dest_addr;   "	int					unreachable_dest_seq_num;   	} AodvT_Rerr;   
       
   	structure   
          signed, big endian          
   0   
       ����             unset             NONE          
@�%�����   
          2                       pk_field   