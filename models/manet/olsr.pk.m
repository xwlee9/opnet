MIL_3_Tfile_Hdr_ 105A 94B modeler 40 405787F1 40903331 4 sunblade79 opnet 0 0 none none 0 0 none 4CC6496F 6E8 0 0 0 0 0 0 a05 1                                                                                                                                                                                                                                                                                                                                                                                                              ����             0x0             NONE                                    Z              
   Message   
       
      -This will store data structure OlsrT_Message.       /****       typedef struct {   	OpT_uInt8			message_type;   &	OpT_uInt8			vtime;			/* Valid time */   	OpT_uInt16			message_size;   	int					originator_addr;   	OpT_uInt8			ttl;   	OpT_uInt8			hop_count;   	OpT_uInt16			message_seq_num;   @	void*				message; /* This will be either OlsrT_Hello_Message or   									OlsrT_TC_Message */   	} OlsrT_Message;       *****/   
       
   	structure   
          signed, big endian          
   0   
       ����          
   unset   
          NONE          
@�������   
       
   1   
                x              
   Packet Length   
                        integer             signed, big endian          
   16   
       ����             set             NONE          
@�������   
       
   2   
             �   x              
   Packet Sequence Number   
                        integer             signed, big endian          
   16   
       ����             set             NONE          
@a�G����   
       
   3   
          