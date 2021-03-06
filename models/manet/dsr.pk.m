MIL_3_Tfile_Hdr_ 110A 94B opnet 40 3DA70AC6 4141C403 9 sunblade79 opnet 0 0 none none 0 0 none 930B758C F0B 0 0 0 0 0 0 b24 0                                                                                                                                                                                                                                                                                                                                                                                                                ����             0x0             NONE          J      NThe DSR packet format is implemented according to draft-ietf-manet-dsr-09.txt    3(April 2003) section 6 "DSR Options Header Format".   J                <   (              
   Next Header   
       
      G8-bit selector. Identifies the type of header following the DSR header.   /Uses the same value as the IPv4 Protocol field.   
          integer             signed, big endian          
   8   
       ����             set             NONE          
@������   
          1                �   (              
   Reserved   
       
      RESERVED for future work   
          integer             signed, big endian          
   8   
       ����             set             NONE          
@0�!����   
          2                �   (              
   Payload Length   
       
      IThe length of the DSR header, exlcuding the 4-octet fixed length portion.   F The value of the Payload Length field defines the total length of all   "options carried in the DSR header.   
          integer             signed, big endian          
   16   
       ����             set             NONE          
@�*@����   
          3                <   F              
   Options   
       
   <   JVariable length field; the length of the Options field is specified by the   GPayload Length field in this DSR header. Contains one or more pieces of   Noptional information (DSR options), encoded in type-length-value (TLV) format.       %/* Options field in the DSR packet	*/   typedef struct   	{   	int			option_type;   	int			option_length;   	void*		dsr_option_ptr;   	} DsrT_Packet_Option;       /* Route Request Option	*/   typedef struct   	{   	long int		identification;   	IpT_Address		target_address;   	List*			route_lptr;   	} DsrT_Route_Request_Option;       /* Route Reply Option	*/   typedef struct   	{   	Boolean			last_hop_external;   	List*			route_lptr;   	} DsrT_Route_Reply_Option;       /* Route Error Option	*/   typedef struct   	{   	int				error_type;   	int				salvage;   #	IpT_Address		error_source_address;   !	IpT_Address		error_dest_address;   '	IpT_Address		unreachable_node_address;   	} DsrT_Route_Error_Option;       $/* Acknowledgement Request Option	*/   typedef struct	   	{   	long int		identification;    	} DsrT_Acknowledgement_Request;       /* Acknowledgement Option	*/   typedef struct   	{   	long int		identification;   !	IpT_Address		ack_source_address;   	IpT_Address		ack_dest_address;   	} DsrT_Acknowledgement;       /* DSR Source Route Option	*/   typedef struct   	{   	Boolean			first_hop_external;   	Boolean			last_hop_external;   	int				salvage;   	int				segments_left;   	List*			route_lptr;   	} DsrT_Source_Route_Option;   
       
   	structure   
          signed, big endian          
   0   
       ����             unset             NONE          
@������   
          4                x   F              
   data   
       
      1The higher layer packet is inserted in this field   
       
   packet   
          signed, big endian          
   0   
       ����             unset             NONE          
@ؤ����   
          5             