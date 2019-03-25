/* Process model C form file: manet_imep.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char manet_imep_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE2 5C91EEE2 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/* External header file inclusions 		*/
#include <string.h>
#include <manet_tora_imep.h>
#include <ip_support.h>
#include <ip_rte_support.h>
#include <ip_cmn_rte_table.h>
#include <manet.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <ip_higher_layer_proto_reg_sup.h>
#include <ip_mcast_support.h>
#include <ip_addr_v4.h>

/* Constant definitions 				*/
#define		BP_TIMER		is_bp_timer
#define		ULP_REG			is_ulp_reg
#define		BEACON			is_beacon
#define		ULP_DATA		is_ulp_data
#define		NEWCOLOR		is_newcolor
#define		ACK				is_ack
#define		ECHO			is_echo
#define		MRT_TIMER		is_mrt_timer
#define		OUTGOING_DATA	is_outgoing_data
#define		RP_TIMER		is_rp_timer
#define		MBT_TIMER		is_mbt_timer
#define 	LTRACE_TORA_IMEP_MGR_ACTIVE		(op_prg_odb_ltrace_active ("manet_tora_imep"))
#define		ImepS_Packet_Format		"imep_packet"
#define		IMEP_ROUTE_SUBNET_MASK (IpI_Broadcast_Addr)

#define		IMEP_MSG_HDR_SIZE			64
#define		IMEP_OBJ_HDR_SIZE			32
#define		IMEP_ECHO_OBJ_SIZE			32
#define		IMEP_NEWCOLOR_OBJ_SIZE		16
#define		IMEP_ACK_ENTRY_SIZE			48
#define		IMEP_ACK_REQ_ENTRY_SIZE		32
#define		IMEP_UDP_HDR_SIZE			64

#define		MAX_INITIAL_BEACON_TIME		100.0

/* Static variable declarations   */
static int global_router_id_index;
static double imep_exponential_backoff [10] = {1.0, 1.2, 1.4, 1.8, 2.6, 4.2, 4.2, 4.2, 4.2, 4.2};
static Boolean imep_mcast_address_built = OPC_FALSE;

/* Structure and enum definitions */
typedef enum
	{
	ImepC_Intrpt_Code_Beacon_Tx = 100,
	ImepC_Intrpt_Code_Beacon_Mbt,
	ImepC_Intrpt_Code_Packet_Retransmit,
	ImepC_Intrpt_Code_Packet_Max_Retransmit_Time
	} ImepC_Intrpt_Code;

typedef enum
	{
	ImepC_Retrans_Optimize_Status_Insert_Update,
	ImepC_Retrans_Optimize_Status_Remove,
	ImepC_Retrans_Optimize_Status_Access,
	ImepC_Retrans_Optimize_Status_Cancel
	} 
ImepC_Retrans_Optimize_Status;

typedef struct
	{
	int color;
	int sequence_number;
	int root_node;
	} ImepT_RDN_Member_Struct;

/* Structure definition to hold information about RDN on IMEP */
typedef struct
	{
	/* Maintain what the color is currently -- for the RDN rooted*/
	/* at this node*/
	int current_color;
	/* Maintain what the current sequence number for the transmissions*/
	/* from this router is at present*/
	int current_seq_number;
	/* This list maintains the router IDs that are*/
	/* part of the RDN rooted at this router*/
	List* root_rdn_members_list;
	/* Array of structures with the RDN info. The index of the array is*/
	/* the router ID for whose RDN info we are storing*/
	ImepT_RDN_Member_Struct* rdns_info;
	} ImepT_RDN_Struct;

/* Structure for storing neighbor router information. */
/* An array of these structures will be maintained    */
/* at all times. This array will be used for the      */
/* animation purposes ONLY.							  */
typedef struct
	{
	/* Last known x position of the router*/
	double x_pos;
	/* Last known y position of the rotuer*/
	double y_pos;
	/* Object ID of the router*/
	Objid node_object_id;
	Boolean	is_subnet;
	} ImepT_Neighbor_Info_Struct;

/* Structure for optimizing the Retransmission Timer Updates */
/* This structure will hold the Seq Number and the Event Handle*/
/* which is the event for retransmission of the particular seq.*/
/* If an ACK is received and an update to the event is to be done*/
/* then a partial direct access can be made instead of a sequential*/
/* search for the pending Self Retransmission Interrupts.		   */
typedef struct
	{
	int sequence_number_unacked;
	Evhandle pending_retrans_evhandle;
	} 
ImepT_Retrans_Struct;

/* Function prototypes */
static void manet_imep_sv_init ();
static void manet_imep_rdn_initialize ();
static void manet_imep_send_echo_response (int);
static void manet_imep_handle_nbr_dn (int);
static void manet_imep_send_newcolor_obj_on_intf ();
static void manet_imep_reset_mbt_timer (int, Boolean);
static Boolean manet_imep_rtr_exist_in_mbrship_list (List*);
static void manet_imep_update_rdn_info (int, int, int);
static Packet* manet_imep_ip_datagram_create (Packet* imep_packet, IpT_Address dest_addr);
static void manet_imep_remove_router_from_member_list (int);
static void manet_imep_insert_ulp_object_into_agg_buffer (Packet*);
static Packet* manet_imep_create_data_packet ();
static void	manet_imep_send_ack_response (int, int, int, int);
static void manet_imep_send_ulp_data ();
static void manet_imep_buffer_received_ulp_packet (Packet*,ImepT_Ulp_Data_Struct*, int);
static void manet_imep_support_animation_imep_init ();
static void manet_imep_support_build_animation_array ();
static void manet_imep_support_log_message (Log_Category, const char*);
static void manet_imep_route_injection_handle_newcolor_object (int, List*);
static void manet_imep_clear_ip_route_entries (int rt_id);
static Boolean manet_imep_router_exists_in_member_list (IpT_Address, List*);
static Boolean manet_imep_rt_exists_in_list (int, List*);
static void	manet_imep_send_newcolor (void);
static void	manet_imep_purge_ulp_pkts (void);
static void	manet_imep_control_pk_sent_stat_update (OpT_Packet_Size);
static ImepT_Header_Fields_Struct*	manet_imep_msg_hdr_create (ImepC_Packet_Type);
static void	manet_imep_schedule_retransmission (Packet*);
static Compcode	manet_imep_update_retransmission (int);

EXTERN_C_BEGIN
static void manet_imep_send_ip_packet (void* ip_packet, int);
static void manet_imep_inform_ulp_about_neighbor (void*, int);
static void manet_imep_support_perform_animation_updates (void* PRG_ARG_UNUSED (ptr), int PRG_ARG_UNUSED (code));
static void manet_imep_handle_received_ulp_packet (void*, int PRG_ARG_UNUSED (code));
EXTERN_C_END

/* End of Header Block */

#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */



/* State variable definitions */
typedef struct
	{
	/* Internal state tracking for FSM */
	FSM_SYS_STATE
	/* State Variables */
	int	                    		router_id                                       ;	/* router id of the local router                                   */
	                        		                                                	/* IpT_Address, IpT_Router_Id and unsigned int are all equivalent  */
	int	                    		beacon_period                                   ;	/* How often do we transmit beacon packets?  */
	int	                    		max_beacon_time                                 ;	/* wait time till you declare a link down  */
	int	                    		max_retries                                     ;	/* how many unacked messages results in link failure?  */
	Evhandle	               		beacon_tx_evhndl                                ;	/* Event handle for transmitting beacon packets  */
	Objid	                  		sv_node_objid                                   ;	/* Object ID of this node  */
	int	                    		total_num_ip_intf                               ;	/* Maintains the count of the number of IP interfaces on this node  */
	int**	                  		link_state_information                          ;	/* First of the two types of information that can be requested from IMEP by an ULP.  */
	                        		                                                	/* Note that this variable just maintains the link state information.                */
	                        		                                                	/* Its a 2 d array of integers, so link_state_information [X, Y] will                */
	                        		                                                	/* indicate the link state between Router IDs X and Y                                */
	ImepT_RDN_Struct*	      		rdn_info                                        ;	/* Structure pointer which holds the RDN information for node  */
	Objid	                  		tora_imep_compound_attr                         ;
	Objid	                  		tora_my_params_attr_id                          ;
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;
	Packet*	                		incoming_imep_packet                            ;
	List*	                  		ulp_aggregation_buffer                          ;
	List**	                 		buffer_arrived_ulp_packets                      ;
	ImepT_Neighbor_Info_Struct*			router_information                              ;
	Andid*	                 		animation_drawing_ids                           ;
	Log_Handle	             		imep_log_protocol_handle                        ;
	Log_Handle	             		imep_log_configuration_handle                   ;
	Stathandle	             		imep_control_traffic_sent_bps_stathandle        ;
	Stathandle	             		imep_control_traffic_sent_pks_stathandle        ;
	Stathandle	             		imep_control_traffic_received_bps_stathandle    ;
	Stathandle	             		imep_control_traffic_received_pks_stathandle    ;
	double	                 		aggregation_period                              ;
	double	                 		imep_pk_size                                    ;
	double	                 		retransmission_interval                         ;
	Stathandle	             		imep_number_of_neighbors_stathandle             ;
	Prohandle	              		tora_imep_mgr_prohandle                         ;
	Stathandle	             		imep_ulp_traffic_sent_bps_stathandle            ;
	Stathandle	             		imep_ulp_traffic_sent_pks_stathandle            ;
	Stathandle	             		imep_ulp_traffic_received_bps_stathandle        ;
	Stathandle	             		imep_ulp_traffic_received_pks_stathandle        ;
	Stathandle	             		imep_retransmissions_stathandle                 ;
	Boolean	                		imep_route_injection                            ;
	IpT_Address	            		my_ip_address                                   ;
	Andid*	                 		duplicate_animation_drawing_ids                 ;
	double	                 		animation_update_time                           ;
	Stathandle	             		global_imep_control_traffic_sent_bps_stathandle ;
	Stathandle	             		global_imep_control_traffic_sent_pks_stathandle ;
	Stathandle	             		global_imep_control_traffic_received_bps_stathandle;
	Stathandle	             		global_imep_control_traffic_received_pks_stathandle;
	Stathandle	             		global_imep_ulp_traffic_sent_bps_stathandle     ;
	Stathandle	             		global_imep_ulp_traffic_sent_pks_stathandle     ;
	Stathandle	             		global_imep_ulp_traffic_received_bps_stathandle ;
	Stathandle	             		global_imep_ulp_traffic_received_pks_stathandle ;
	Evhandle *	             		mbt_timer_array                                 ;
	Boolean	                		node_level_route_injection                      ;
	Evhandle	               		newcolor_transmission_evhandle                  ;
	Evhandle	               		retransmission_evhandle                         ;
	Evhandle	               		next_beacon_event                               ;
	Boolean	                		clear_to_tx                                     ;	/* Flag indicating if there is any pendng retransmission event  */
	Boolean	                		no_neighbor                                     ;	/* Flag indicating if the node has any neighbors at the time  */
	Evhandle	               		aggregation_timer_evhandle                      ;	/* Event to transmit aggregated ULP packets  */
	Boolean	                		data_to_send                                    ;	/* Flag indicating ULP packets waiting to be transmitted  */
	int	                    		num_of_retry_left                               ;	/* How many times have we retransmitted so far  */
	List*	                  		yet_to_ack_lptr                                 ;	/* List of neighbors who has not yet acked  */
	Packet *	               		retransmit_pkptr                                ;	/* Packet that needs to be retransmitted  */
	Boolean	                		newcolor_object_required                        ;	/* Flag indicating new color object needs to be transmitted  */
	Boolean	                		newcolor_transmission                           ;	/* Flag indicating we are in the middle of newcolor object transmoission  */
	char	                   		node_name [256]                                 ;	/* The name of the node to be used in animation  */
	int	                    		mcast_port_index                                ;	/* Major port on which to perform the multicast for IMEP packets  */
	int	                    		stream_from_ip_encap                            ;
	} manet_imep_state;

#define router_id               		op_sv_ptr->router_id
#define beacon_period           		op_sv_ptr->beacon_period
#define max_beacon_time         		op_sv_ptr->max_beacon_time
#define max_retries             		op_sv_ptr->max_retries
#define beacon_tx_evhndl        		op_sv_ptr->beacon_tx_evhndl
#define sv_node_objid           		op_sv_ptr->sv_node_objid
#define total_num_ip_intf       		op_sv_ptr->total_num_ip_intf
#define link_state_information  		op_sv_ptr->link_state_information
#define rdn_info                		op_sv_ptr->rdn_info
#define tora_imep_compound_attr 		op_sv_ptr->tora_imep_compound_attr
#define tora_my_params_attr_id  		op_sv_ptr->tora_my_params_attr_id
#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define incoming_imep_packet    		op_sv_ptr->incoming_imep_packet
#define ulp_aggregation_buffer  		op_sv_ptr->ulp_aggregation_buffer
#define buffer_arrived_ulp_packets		op_sv_ptr->buffer_arrived_ulp_packets
#define router_information      		op_sv_ptr->router_information
#define animation_drawing_ids   		op_sv_ptr->animation_drawing_ids
#define imep_log_protocol_handle		op_sv_ptr->imep_log_protocol_handle
#define imep_log_configuration_handle		op_sv_ptr->imep_log_configuration_handle
#define imep_control_traffic_sent_bps_stathandle		op_sv_ptr->imep_control_traffic_sent_bps_stathandle
#define imep_control_traffic_sent_pks_stathandle		op_sv_ptr->imep_control_traffic_sent_pks_stathandle
#define imep_control_traffic_received_bps_stathandle		op_sv_ptr->imep_control_traffic_received_bps_stathandle
#define imep_control_traffic_received_pks_stathandle		op_sv_ptr->imep_control_traffic_received_pks_stathandle
#define aggregation_period      		op_sv_ptr->aggregation_period
#define imep_pk_size            		op_sv_ptr->imep_pk_size
#define retransmission_interval 		op_sv_ptr->retransmission_interval
#define imep_number_of_neighbors_stathandle		op_sv_ptr->imep_number_of_neighbors_stathandle
#define tora_imep_mgr_prohandle 		op_sv_ptr->tora_imep_mgr_prohandle
#define imep_ulp_traffic_sent_bps_stathandle		op_sv_ptr->imep_ulp_traffic_sent_bps_stathandle
#define imep_ulp_traffic_sent_pks_stathandle		op_sv_ptr->imep_ulp_traffic_sent_pks_stathandle
#define imep_ulp_traffic_received_bps_stathandle		op_sv_ptr->imep_ulp_traffic_received_bps_stathandle
#define imep_ulp_traffic_received_pks_stathandle		op_sv_ptr->imep_ulp_traffic_received_pks_stathandle
#define imep_retransmissions_stathandle		op_sv_ptr->imep_retransmissions_stathandle
#define imep_route_injection    		op_sv_ptr->imep_route_injection
#define my_ip_address           		op_sv_ptr->my_ip_address
#define duplicate_animation_drawing_ids		op_sv_ptr->duplicate_animation_drawing_ids
#define animation_update_time   		op_sv_ptr->animation_update_time
#define global_imep_control_traffic_sent_bps_stathandle		op_sv_ptr->global_imep_control_traffic_sent_bps_stathandle
#define global_imep_control_traffic_sent_pks_stathandle		op_sv_ptr->global_imep_control_traffic_sent_pks_stathandle
#define global_imep_control_traffic_received_bps_stathandle		op_sv_ptr->global_imep_control_traffic_received_bps_stathandle
#define global_imep_control_traffic_received_pks_stathandle		op_sv_ptr->global_imep_control_traffic_received_pks_stathandle
#define global_imep_ulp_traffic_sent_bps_stathandle		op_sv_ptr->global_imep_ulp_traffic_sent_bps_stathandle
#define global_imep_ulp_traffic_sent_pks_stathandle		op_sv_ptr->global_imep_ulp_traffic_sent_pks_stathandle
#define global_imep_ulp_traffic_received_bps_stathandle		op_sv_ptr->global_imep_ulp_traffic_received_bps_stathandle
#define global_imep_ulp_traffic_received_pks_stathandle		op_sv_ptr->global_imep_ulp_traffic_received_pks_stathandle
#define mbt_timer_array         		op_sv_ptr->mbt_timer_array
#define node_level_route_injection		op_sv_ptr->node_level_route_injection
#define newcolor_transmission_evhandle		op_sv_ptr->newcolor_transmission_evhandle
#define retransmission_evhandle 		op_sv_ptr->retransmission_evhandle
#define next_beacon_event       		op_sv_ptr->next_beacon_event
#define clear_to_tx             		op_sv_ptr->clear_to_tx
#define no_neighbor             		op_sv_ptr->no_neighbor
#define aggregation_timer_evhandle		op_sv_ptr->aggregation_timer_evhandle
#define data_to_send            		op_sv_ptr->data_to_send
#define num_of_retry_left       		op_sv_ptr->num_of_retry_left
#define yet_to_ack_lptr         		op_sv_ptr->yet_to_ack_lptr
#define retransmit_pkptr        		op_sv_ptr->retransmit_pkptr
#define newcolor_object_required		op_sv_ptr->newcolor_object_required
#define newcolor_transmission   		op_sv_ptr->newcolor_transmission
#define node_name               		op_sv_ptr->node_name
#define mcast_port_index        		op_sv_ptr->mcast_port_index
#define stream_from_ip_encap    		op_sv_ptr->stream_from_ip_encap

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	manet_imep_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((manet_imep_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
manet_imep_sv_init ()
	{
	/* Purpose: Function will initialize the state vars */
	/* Requires: None.									*/
	/* Effects: Initializes the state vars appropriately*/
	Objid						self_objid;
	Objid 						imep_params_cattr_objid;
	Objid 						imep_params_attr_objid;
	IpT_Interface_Info*			intf_ptr;
	int							counter;
	   
	FIN (manet_imep_sv_init ())
	
	self_objid = op_id_self ();
	sv_node_objid = op_topo_parent (self_objid);
	
	op_ima_obj_attr_get (self_objid, "manet_mgr.TORA/IMEP Parameters", &tora_imep_compound_attr);
						 
	tora_my_params_attr_id = op_topo_child (tora_imep_compound_attr, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (tora_my_params_attr_id, "Router ID", &router_id);
	
	op_ima_obj_attr_get (tora_my_params_attr_id, "IMEP Parameters", &imep_params_cattr_objid);
	imep_params_attr_objid = op_topo_child (imep_params_cattr_objid, OPC_OBJTYPE_GENERIC, 0);
	
	/* Get all the IMEP related params set on the node */
	op_ima_obj_attr_get (imep_params_attr_objid, "Beacon Period", &beacon_period);
	op_ima_obj_attr_get (imep_params_attr_objid, "Max Beacon Timer", &max_beacon_time);
	op_ima_obj_attr_get (imep_params_attr_objid, "Max Retries", &max_retries);
	op_ima_obj_attr_get (imep_params_attr_objid, "Max IMEP Packet Length", &imep_pk_size);

	/* Read in the route injection attribute for the particular node */
	if (op_ima_obj_attr_exists (imep_params_attr_objid, "Route Injection"))
		{
		op_ima_obj_attr_get (imep_params_attr_objid,
			"Route Injection", &node_level_route_injection);
		}
	else
		{
		/* By default keep the individual node level route injection as false */
		node_level_route_injection = OPC_FALSE;
		}
	
	/* Read in the attribute for the Imep Route Injection - "IMEP Route Injection" */
	if (op_ima_sim_attr_exists ("IMEP Route Injection"))
		op_ima_sim_attr_get (OPC_IMA_TOGGLE, "IMEP Route Injection", &imep_route_injection);
	else
		/* By default keep it disabled */
		imep_route_injection = OPC_FALSE;
	
	/* Read in the attribute for the Animation update times */
	if (op_ima_sim_attr_exists ("TORA IMEP Animation Refresh Interval"))
		op_ima_sim_attr_get (OPC_IMA_DOUBLE, "TORA IMEP Animation Refresh Interval", &animation_update_time);
	else
		/* Unable to find the attribute, just give a reasonable default anyway */
		animation_update_time = 20;
	
	tora_imep_mgr_prohandle = op_pro_parent (op_pro_self ()); 
	
	/* Determine the number of IP interfaces on the surrounding router */
	module_data_ptr = ip_support_module_data_get (sv_node_objid);
	total_num_ip_intf = ip_rte_num_interfaces_get (module_data_ptr);
	
	/* Handle the case if the packet size has been left as "Auto Detect" */
	if (imep_pk_size == -1)
		{
		/* Try to get the size set on the IP interface */
		intf_ptr = ip_rte_intf_tbl_access (module_data_ptr, 0);
		
		/* Because we want to avoid segmentation at the lower layer		*/
		/* Max. IMEP pk size (in bytes) = Intf. MTU - IP Overhead - 	*/
		/* UDP Overhead - MAC Overhead									*/
		/* We will assume "worst" case MAC - which is WLAN having a mac */
		/* header of 281 bits. Others like Ethernet have lower values   */
		/* (20 + 64 + 281) / 8 = 46										*/
		imep_pk_size = intf_ptr->mtu - 46;
		}
	
	/* Create the simulation log handles */
	imep_log_protocol_handle = op_prg_log_handle_create (OpC_Log_Category_Protocol, "MANET", "IMEP Protocol", 100);
	imep_log_configuration_handle = op_prg_log_handle_create (OpC_Log_Category_Configuration, "MANET", "IMEP Configuration", 100);
	
	/* Register the statistic handles */
	/* First all the local stat handles */
	imep_control_traffic_sent_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	imep_control_traffic_sent_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	imep_control_traffic_received_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);	
	imep_control_traffic_received_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);  
	imep_ulp_traffic_sent_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);  
	imep_ulp_traffic_sent_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);  
	imep_ulp_traffic_received_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);  
	imep_ulp_traffic_received_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);  
	imep_number_of_neighbors_stathandle = op_stat_reg ("TORA_IMEP.IMEP Number of Neighbors", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	imep_retransmissions_stathandle = op_stat_reg ("TORA_IMEP.IMEP Retransmissions", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	/* And then the global stat handles */
	global_imep_control_traffic_received_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_control_traffic_received_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_control_traffic_sent_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_control_traffic_sent_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP Control Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	
	global_imep_ulp_traffic_sent_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_ulp_traffic_sent_pks_stathandle	= op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_ulp_traffic_received_bps_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_imep_ulp_traffic_received_pks_stathandle = op_stat_reg ("TORA_IMEP.IMEP ULP (TORA) Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		
	/* Initialize the retransmission related flags. */
	clear_to_tx = OPC_TRUE;
	no_neighbor = OPC_TRUE;
	newcolor_object_required = OPC_FALSE;
	yet_to_ack_lptr = op_prg_list_create ();
	retransmission_evhandle = op_ev_current ();
    retransmit_pkptr = OPC_NIL;
	newcolor_transmission = OPC_FALSE;
	
	/* Determine the port index to send out the multicast packets out on */
	for (counter = 0; counter < total_num_ip_intf; counter ++)
		{
		/* Get the interface pointer */
		intf_ptr = ip_rte_intf_tbl_access (module_data_ptr, counter);
		
		/* Get the list of routing protocols on the interface */
		if (ip_interface_routing_protocols_contains (ip_rte_intf_routing_prot_get (intf_ptr), IpC_Rte_Tora))
			{
			mcast_port_index = counter;		
			break;
			}
		}
	
	/* Store also the stream from IP Encap */
	stream_from_ip_encap = module_data_ptr->instrm_from_ip_encap;
	
	/* Initialize the multicast address variables for IMEP.	*/
	/* Do this only once in the simulation.					*/
	if (imep_mcast_address_built == OPC_FALSE)
		{
		IpI_Imep_Mcast_Addr 	= ip_address_create (ALL_IMEP_ROUTERS);
		InetI_Imep_Mcast_Addr	= inet_address_from_ipv4_address_create (IpI_Imep_Mcast_Addr);
		imep_mcast_address_built = OPC_TRUE;
		}
		 
	FOUT;
	}

static void
manet_imep_rdn_initialize ()
	{
	/* Purpose: Initializes the RDN structure pointer state var */
	/* Requires: None.											*/
	/* Effects: None.											*/
	int 		counter;
	
	FIN (manet_imep_rdn_initialize ())
		
	/* Initialize the RDN related variables. */	
	rdn_info = (ImepT_RDN_Struct*) op_prg_mem_alloc (sizeof (ImepT_RDN_Struct));
	
	/* The current color for the RDN rooted at this node */
	rdn_info->current_color = 1;
	
	/* And the current sequence number for the RDN rooted at this node */
	rdn_info->current_seq_number = 1;
	
	/* Maintains the list of router IDs that are part of the RDN rooted at this router */
	rdn_info->root_rdn_members_list = op_prg_list_create ();
	
	/* Maintains information about RDNs that this router is part of. */
	rdn_info->rdns_info = (ImepT_RDN_Member_Struct*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (ImepT_RDN_Member_Struct));

	/* Initialize the rdn info array */
	for (counter = 0; counter < (global_router_id_index + 1); counter ++)
		{
		rdn_info->rdns_info [counter].color = -1;
		rdn_info->rdns_info [counter].sequence_number = -1;
		rdn_info->rdns_info [counter].root_node = counter;
		}

	FOUT;	
	}

static void
manet_imep_send_beacons ()
	{
	/* Purpose: Function to create the beacon packets and send out on all interfaces 		*/
	/* 			of the router.														 		*/
	/* Requires: The previous computation of the number of interfaces present on the router */
	/* Effects: None.																		*/
	Packet* 						beacon_pkt;
	ImepT_Header_Fields_Struct* 	header_field_ptr;
	Packet* 						ip_packet;

	FIN (manet_imep_send_beacons ())

	/* For each of the interfaces create a beacon packet that has to be sent out */
	/* Note that the beacon packet is the same as the Imep packet -- with the correct information */
	/* set on the fields.*/
	beacon_pkt = op_pk_create_fmt (ImepS_Packet_Format);
		
	/* Create the header structure for setting on the packet */
	header_field_ptr = manet_imep_msg_hdr_create (ImepC_Packet_Type_Beacon);

	/* Set the size of the packet. */
	op_pk_total_size_set (beacon_pkt, IMEP_UDP_HDR_SIZE + IMEP_MSG_HDR_SIZE);
	
	/* Place the header field on to the IMEP packet */
	op_pk_nfd_set (beacon_pkt, "hdr_fields", header_field_ptr, imep_packet_header_field_copy, 
		imep_packet_header_field_destroy, sizeof (ImepT_Header_Fields_Struct));
	
	/* Update the stats. */
	manet_imep_control_pk_sent_stat_update (IMEP_MSG_HDR_SIZE);
	
	/* Before we send out the beacon packet we have to create the IP dgram to hold the beacon packet */
	ip_packet = manet_imep_ip_datagram_create (beacon_pkt, ip_address_copy (IpI_Imep_Mcast_Addr)); 
		
	/* Now that the datagram is created use the API to send out the packet */
	op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, (void*) ip_packet);
	
	FOUT;
	}

static ImepT_Header_Fields_Struct*
manet_imep_msg_hdr_create (ImepC_Packet_Type pk_type)
	{
	ImepT_Header_Fields_Struct* 	header_field_ptr;
	
	/* Helper function to create the header and fill in common fields. */
	FIN (manet_imep_msg_hdr_create (pk_type));
	
	/* Create the header structure for setting on the packet */
	header_field_ptr = (ImepT_Header_Fields_Struct*) op_prg_pmo_alloc (global_imep_header_fields_pmohandle);
		
	/* Error handle the returned value */
	if (header_field_ptr == OPC_NIL)
		{
		manet_imep_disp_message ("Unable to allocate memory for the header structure.");
		op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
		}
		
	/* And set the values on the header field */
	header_field_ptr->version = 1;
	header_field_ptr->color = rdn_info->current_color;
	header_field_ptr->seq_number = rdn_info->current_seq_number;
	/* message length is not used right now. */
	header_field_ptr->message_length = 0;
	header_field_ptr->rtr_id = router_id;
	header_field_ptr->source_ip_address = my_ip_address;
	header_field_ptr->packet_type = pk_type;
	header_field_ptr->ack_list = OPC_NIL;
	header_field_ptr->ack_info_ptr = OPC_NIL;
	 
	FRET (header_field_ptr);
	}
	
static void
manet_imep_control_pk_sent_stat_update (OpT_Packet_Size pk_size)
	{
	/* Helper function to write out stat. */
	FIN (manet_imep_control_pk_sent_stat_update (pk_size));
	
	/* Write out the statistic for the traffic sent */
	op_stat_write (imep_control_traffic_sent_bps_stathandle, pk_size);
	op_stat_write (imep_control_traffic_sent_bps_stathandle, 0);
	op_stat_write (imep_control_traffic_sent_pks_stathandle, 1);
	op_stat_write (imep_control_traffic_sent_pks_stathandle, 0);
	
	/* And then the global stat */
	op_stat_write (global_imep_control_traffic_sent_bps_stathandle, pk_size);
	op_stat_write (global_imep_control_traffic_sent_bps_stathandle, 0);
	op_stat_write (global_imep_control_traffic_sent_pks_stathandle, 1);
	op_stat_write (global_imep_control_traffic_sent_bps_stathandle, 0);
	
	FOUT;
	}

static void
manet_imep_send_echo_response (int sender_address)
	{
	Packet* 						echo_pkptr;
	ImepT_Header_Fields_Struct* 	header_fields_ptr;
	Packet* 						ip_packet;
	
	/* Purpose: Function is required to send out the echo response to the incoming beacon packet */
	/* Requires: A valid beacon packet being passed in to the function.	Packet is of the format ImepS_Packet_Format */
	/* Effects: None.																			 */
	FIN (manet_imep_send_echo_response (sender_address))
	
	/* Create the header field */
	header_fields_ptr = manet_imep_msg_hdr_create (ImepC_Packet_Type_Echo);
	
	/* Now create the echo packet */
	echo_pkptr = op_pk_create_fmt (ImepS_Packet_Format);
	
	/* Set the size of the packet. */
	op_pk_total_size_set (echo_pkptr, IMEP_UDP_HDR_SIZE + IMEP_MSG_HDR_SIZE + IMEP_OBJ_HDR_SIZE + IMEP_ECHO_OBJ_SIZE);
	
	/* And set the header and data fields on the packet */
	op_pk_nfd_set (echo_pkptr, "hdr_fields", header_fields_ptr, imep_packet_header_field_copy, imep_packet_header_field_destroy, sizeof (ImepT_Header_Fields_Struct));
	
	/* Write out the statistic for the traffic sent */
	manet_imep_control_pk_sent_stat_update (op_pk_total_size_get (echo_pkptr));

	/* Create the IP packet -- encapsulate this echo response in the IP packet */
	ip_packet = manet_imep_ip_datagram_create (echo_pkptr, sender_address);
	
	/* Send the packet out to IP */
	op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, ip_packet);
	
	FOUT;		
	} /* End manet_imep_send_echo_response (dummy, sender_address);*/

static void
manet_imep_send_newcolor_obj_on_intf ()
	{
	/* Purpose: This function is meant to send out the New Color objects on all the interfaces */
	/* Requires: None.																		   */
	/* Effects: None.																		   */
	Packet* 							newcolor_imep_packet;
	ImepT_Header_Fields_Struct* 		header_fields_ptr;
	Packet* 							ip_packet;
	char								str [256];
	
	FIN (manet_imep_send_newcolor_obj_on_intf ())

	/* Create an IMEP packet */
	newcolor_imep_packet = op_pk_create_fmt (ImepS_Packet_Format);
	
	/* Create the header field to be placed on the IMEP packet */
	header_fields_ptr = manet_imep_msg_hdr_create (ImepC_Packet_Type_Newcolor);

	/* Fill in the list for new membership list. */
	header_fields_ptr->ack_list = op_prg_list_create ();
	
	/* Just copy the current set of RDN members for this node into the list*/
	/* They are simple data types so the KP should be sufficient*/
	op_prg_list_elems_copy (rdn_info->root_rdn_members_list, header_fields_ptr->ack_list);
	
	/* And place the header fields struct on to the IMEP packet created*/
	op_pk_nfd_set (newcolor_imep_packet, "hdr_fields", header_fields_ptr, 
		imep_packet_header_field_copy, imep_packet_header_field_destroy, sizeof (ImepT_Header_Fields_Struct));
	
	/* Set the size of the packet. */
	op_pk_total_size_set (newcolor_imep_packet, IMEP_UDP_HDR_SIZE + IMEP_MSG_HDR_SIZE + IMEP_OBJ_HDR_SIZE + 
		op_prg_list_size (rdn_info->root_rdn_members_list) * IMEP_ACK_REQ_ENTRY_SIZE);
	
	/* Write out the statistic for the traffic sent */
	manet_imep_control_pk_sent_stat_update (op_pk_total_size_get (newcolor_imep_packet));
	
	/* Finally just create the IP packet and send it out */
	ip_packet = manet_imep_ip_datagram_create (newcolor_imep_packet, ip_address_copy (IpI_Imep_Mcast_Addr));
	
	/* Send the packet out to IP */
	op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, ip_packet);

	/* Schedule retransmission for this. */
	manet_imep_schedule_retransmission (ip_packet);	
	
	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		sprintf (str, "IMEP Process, sending NewColor object, color:%d, seq_num:%d", 
			rdn_info->current_color, rdn_info->current_seq_number);
		op_prg_odb_print_major (str, OPC_NIL);
		}
	
	FOUT;	
	}

static void
manet_imep_inform_ulp_about_neighbor (void* rtr_id_ptr, int stats)
	{
	/* Purpose: This function is meant to inform the ULP about a new neighbor. 								 */
	/* Requires: Neighbor information (maybe just the router ID).			   								 */
	/* Effects: (Possibly) provides an interrupt to the ULP with information regarding the neighbor router ID*/
	ToraImepMgrT_Invocation_Struct* 	argmem;
	char 								temp_node_name [256];
	int* 								rtr_id;
	LinkState 							status;
	
	FIN (manet_imep_inform_ulp_about_neighbor (int* rtr_id, int status))
		
	rtr_id = (int*) rtr_id_ptr;
	status = (LinkState) stats;
		
	/* Create the argument memory to pass back to ToraImepMgr */ 	
	argmem = (ToraImepMgrT_Invocation_Struct*) op_prg_mem_alloc (sizeof (ToraImepMgrT_Invocation_Struct));
	argmem->code = ToraImepMgrC_Invocation_Code_Link_Status_Update;
	argmem->pk_ptr = OPC_NIL;
	argmem->m_router_id = *rtr_id;
	argmem->link_state = status;

	/* Inform Tora Imep Mgr */
	op_pro_invoke (tora_imep_mgr_prohandle, argmem);
	
	/* Free up the pointer passed to the function. */
	op_prg_mem_free (rtr_id);
	
	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		op_ima_obj_attr_get (sv_node_objid, "name", temp_node_name);
		op_prg_odb_print_major ("IMEP Process, neighbor related change detected. Informing ULP.", temp_node_name, OPC_NIL);
		}

		
	FOUT;
	}

static void
manet_imep_reset_mbt_timer (int rt_id, Boolean	restart)
	{
	/* Purpose: Function is meant to start the MBT timer for neighbor rt_id. */
	/* Requires: A valid router id passed to the function.					 */
	/* Effects: Schedules a self interrupt with the ICI					     */
	Ici* intrpt_ici = OPC_NIL;
	
	FIN (manet_imep_reset_mbt_timer (int rt_id))

	/* First check if there are any pending self interrupts for this neighbor -- if there are then cancel    */
	/* and start a new MBT timer for the neighbor.															 */	

	/* Check if the event to be cancelled is valid - if it is, then go ahead and cancel it */
	/* It will be valid when the MBT has been scheduled and is now required to be "pushed" */
	/* to a later time - if an ECHO or ECHO eq. message has been received from the neighbor*/
	if (op_ev_valid ((mbt_timer_array [rt_id])))
		{
		op_ici_destroy (op_ev_ici (mbt_timer_array [rt_id]));
		op_ev_cancel ((mbt_timer_array [rt_id]));
		}

	if (restart)
		{
		/* Optimization: Just store the Evhandle so the event can be accessed directly if the MBT needs to be cancelled */
		/* We have either cleared the pending MBT interrupt for this neighbor or there was none in the first place		*/
		/* Its time now to set up a self interrupt for the MBT 													  		*/
		intrpt_ici = op_ici_create ("manet_imep_ici"); 
		op_ici_attr_set (intrpt_ici, "router_id", rt_id);
		op_ici_install (intrpt_ici);
		mbt_timer_array [rt_id] = op_intrpt_schedule_self (op_sim_time () + max_beacon_time, ImepC_Intrpt_Code_Beacon_Mbt);
		}
		
	FOUT;	
	}

static void
manet_imep_handle_nbr_dn (int rtr_id)
	{
	/* Purpose: Function is meant to handle the event when a neighbor is detected to have gone down */
	/* Requires: ICI associated with the self interrupt that caused this function invocation (from Nbr_Dn state) */
	/* Effects: Informs the registered ULP that the neighbor has gone down.							*/
	int 		temp_var_rtr_id;
	
	FIN (manet_imep_handle_nbr_dn (int))
	
	/* Inform the upper layer about change in status if it was fully bidirectional. */
	if ((link_state_information [router_id] [rtr_id] == ImepC_LinkState_Up) &&
		(link_state_information [rtr_id] [router_id] == ImepC_LinkState_Up))
		{
		temp_var_rtr_id = rtr_id;
		op_intrpt_schedule_call (op_sim_time (), ImepC_LinkState_Down, manet_imep_inform_ulp_about_neighbor, 
			op_prg_mem_copy_create (&temp_var_rtr_id, sizeof (int)));
		}

	/* Mark the router as being down in our state variable maintaining the router states */
	link_state_information [router_id] [rtr_id] = ImepC_LinkState_Down;
	link_state_information [rtr_id] [router_id] = ImepC_LinkState_Down;
	 
	/* Reset the flag to send out a newcolor object as soon as we get a chance. */
	newcolor_object_required = OPC_TRUE;		
	
	FOUT;	
	}

static Boolean
manet_imep_rtr_exist_in_mbrship_list (List* member_list)
	{
	/* Purpose: Function is meant to determine if this router ID is part of the membership list */
	/* Requires: Valid membership list got from the newcolor object or the ACK list.			*/
	/* Effects: None.																			*/
	int 				counter;
	int 				list_size;
	int* 				temp_ip_address;
	
	FIN (manet_imep_rtr_exist_in_mbrship_list (List* member_list))

	list_size = op_prg_list_size (member_list);
	for (counter = 0; counter < list_size; counter ++)
		{
		/* Access each member of the list*/
		temp_ip_address = (int*) op_prg_list_access (member_list, counter);
		
		/* Compare the two IDs - since they are ints we should be able to*/
		/* compare directly */
		if (*temp_ip_address == router_id)
			{
			FRET (OPC_TRUE);
			}
		} /* End for (...*/
		
	FRET (OPC_FALSE);
	}

static void 
manet_imep_update_rdn_info (int root_router_id, int color, int sequence_number)
	{
	char	str[256];
	
	/* Purpose: Function will update the RDN information stored on this router. This router is now */
	/* part of the RDN rooted at root_router.													   */
	/* Requires: None 																			   */
	/* Effects: Updates the RDN info struct with the information								   */

	FIN (manet_imep_update_rdn_info (IpT_Address, int, int))
	
	rdn_info->rdns_info [root_router_id].color = color;
	rdn_info->rdns_info [root_router_id].sequence_number = sequence_number;
	rdn_info->rdns_info [root_router_id].root_node = root_router_id;
			
	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		sprintf (str, "Update info for neighbor RID:%d, color=%d, seq_number=%d", root_router_id, color, sequence_number);
		op_prg_odb_print_major (str, OPC_NIL);
		}
		
	FOUT;
	} /* End manet_imep_update_rdn_info*/

static Packet*
manet_imep_ip_datagram_create (Packet* imep_packet, IpT_Address dest_addr)
	{
	/* Purpose: Function will create the IP datagram which will encapsulate the imep_packet created */
	/* This IP datagram can then be sent out using the API provided to the IP layer.				*/
	/* Requires: A valid IMEP packet to be passed along with the destination and next hop addresses.*/
	/* Effects: Returns an IP datagram.																*/
	Packet* 						ip_pkptr = OPC_NIL;
	OpT_Packet_Size	   				imep_pkt_size;
	IpT_Dgram_Fields*				ip_dgram_fd_ptr = OPC_NIL;
	
	FIN (manet_imep_ip_datagram_create (imep_packet, dest_addr));
	
	imep_pkt_size = op_pk_total_size_get (imep_packet);	
	
	/* Create an IP datagram */
	ip_pkptr = op_pk_create_fmt ("ip_dgram_v4");
	
	/* Set the ICMP packet in the data field of the IP datagram */
	op_pk_nfd_set (ip_pkptr, "data", imep_packet);
	
	/* Create fields data structure that contains orig_len,	*/
	/* ident, frag_len, ttl, src_addr, dest_addr, frag,		*/
	/* connection class, src and dest internal addresses.	*/
	ip_dgram_fd_ptr = ip_dgram_fdstruct_create ();

	/* Set the destination address for this IP datagram.	*/
	ip_dgram_fd_ptr->dest_addr = inet_address_from_ipv4_address_create (ip_address_copy (dest_addr));
	ip_dgram_fd_ptr->connection_class = 0;

	/* Set the correct protocol in the IP datagram.	*/
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Tora; /* IpC_Protocol_Unspec; */
	
	/* Set the packet size-related fields of the IP datagram.	*/
	ip_dgram_fd_ptr->orig_len = imep_pkt_size/8;
	ip_dgram_fd_ptr->frag_len = ip_dgram_fd_ptr->orig_len;
	ip_dgram_fd_ptr->original_size = 160 + imep_pkt_size;
	
	/* Indicate that the packet is not yet fragmented.	*/
	ip_dgram_fd_ptr->frag = 0;

	/* Set the type of service.	*/
	ip_dgram_fd_ptr->tos = 0;
	
	/* Also set the compression method and original size fields	*/
	ip_dgram_fd_ptr->compression_method = IpC_No_Compression;

	/* The record route options has not been set.	*/
	ip_dgram_fd_ptr->options_field_set = OPC_FALSE;

	/*	Set the fields structure inside the ip datagram	*/
	ip_dgram_fields_set (ip_pkptr, ip_dgram_fd_ptr);

	FRET (ip_pkptr);	
	}

Packet*
manet_imep_handle_received_ip_packet (Packet* ip_pkptr)
	{
	/* Purpose: Function will take in the IP packet thats received by IMEP and handle it -- take care of */
	/* the associated ICIs, decapsulate the packet and return the IMEP packet encaped in it.			 */
	/* Requires: A valid IP packet thats passed to the function.										 */
	/* Effects: None.																					 */
	Packet* 						encap_packet = OPC_NIL;
	char							packet_format [256];
	ImepT_Header_Fields_Struct* 	header_fields_ptr;
	
	FIN (manet_imep_handle_received_ip_packet (Packet*))
	
	/* The following will occur in this function: (a) Handle the ICI associated with the passed IP packet */
	/* (b) Return the encaped packet in the IP data field.												  */
	
	if (ip_pkptr == OPC_NIL)
		{
		FRET (encap_packet);
		}
	
	/* Access the data field */
	op_pk_nfd_get (ip_pkptr, "data", &encap_packet);
	
	/* And then error handle the returned value*/
	if (encap_packet == OPC_NIL)
		{
		manet_imep_disp_message ("\n NULL packet pointer got from data field in the IP datagram");
		op_sim_end ("\nTerminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
		FRET (OPC_NIL);		
		}
	
	/* At this point check if the encap packet is an IMEP packet -- if its not then just stop the sim */
	op_pk_format (encap_packet, packet_format);
	if (strcmp (packet_format, ImepS_Packet_Format) != 0)
		{
		manet_imep_disp_message ("\n Unknown packet format encapsulated in the IP datagram received by IMEP");
		op_sim_end ("\nTerminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
		FRET (OPC_NIL);
		}
	
	/* Write out the received statistics for the IMEP packet received */
	/* Remember - if the packet type is of type Data then update the  */
	/* stats for the ULP received stats, otherwise update the control */
	/* stats.														  */
	op_pk_nfd_access (encap_packet, "hdr_fields", &header_fields_ptr);
	if (header_fields_ptr->packet_type == ImepC_Packet_Type_Data)
		{
		/* Update the ULP related stats */
		op_stat_write (imep_ulp_traffic_received_bps_stathandle, op_pk_total_size_get (encap_packet));
		op_stat_write (imep_ulp_traffic_received_bps_stathandle, 0);
		op_stat_write (imep_ulp_traffic_received_pks_stathandle, 1);
		op_stat_write (imep_ulp_traffic_received_pks_stathandle, 0);
		
		/* And then the global stats */
		op_stat_write (global_imep_ulp_traffic_received_bps_stathandle, op_pk_total_size_get (encap_packet));
		op_stat_write (global_imep_ulp_traffic_received_bps_stathandle, 0);
		op_stat_write (global_imep_ulp_traffic_received_pks_stathandle, 1);
		op_stat_write (global_imep_ulp_traffic_received_pks_stathandle, 0);		
		}
	else
		{
		/* Update the Control packet stats */
		op_stat_write (imep_control_traffic_received_bps_stathandle, op_pk_total_size_get (encap_packet));
		op_stat_write (imep_control_traffic_received_bps_stathandle, 0);
		op_stat_write (imep_control_traffic_received_pks_stathandle, 1);
		op_stat_write (imep_control_traffic_received_pks_stathandle, 0);
		
		/* And then the global stats */
		op_stat_write (global_imep_control_traffic_received_bps_stathandle, op_pk_total_size_get (encap_packet));
		op_stat_write (global_imep_control_traffic_received_bps_stathandle, 0);
		op_stat_write (global_imep_control_traffic_received_pks_stathandle, 1);
		op_stat_write (global_imep_control_traffic_received_pks_stathandle, 0);
		}
	
	/* Have performed the access just for now -- we might the information at a later point when we */
	/* have to start getting information about which interface the packet came in from etc.		   */
	
	/* All seems okay -- just return the IMEP packet for further processing */		
	/* After deleting the ip packet from memory.							*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
	FRET (encap_packet);
	}

static void
manet_imep_remove_router_from_member_list (int rt_id)
	{
	/* Purpose: Function will just remove the router id from the explicit member list -- from the list of routers */
	/* seen as neighbors to this router.																		  */
	/* Requires: A valid router id. Presumably this is in the explicit member list of this router currently.	  */
	/* Effects: Removes the router ID from the list.															  */
	int 				temp_counter;
	int 				list_size;
	int* 				temp_member;
	
	FIN (manet_imep_remove_router_from_member_list (int))
	
	if (rdn_info->root_rdn_members_list == OPC_NIL)
		FOUT;

	list_size = op_prg_list_size (rdn_info->root_rdn_members_list);
	for (temp_counter = 0; temp_counter < list_size; temp_counter ++)
		{
		temp_member = (int*) op_prg_list_access (rdn_info->root_rdn_members_list, temp_counter);
		if (*temp_member == rt_id)
			{
			/* Found the one to get rid of -- the neighbor that has gone down */
			temp_member = (int*) op_prg_list_remove (rdn_info->root_rdn_members_list, temp_counter);
		    op_prg_mem_free (temp_member);	
			
			/* Update the number of neighbors stat*/
			op_stat_write (imep_number_of_neighbors_stathandle, (double) op_prg_list_size (rdn_info->root_rdn_members_list));
			
			FOUT;
			}
		}
		
	FOUT;
	}

static void
manet_imep_send_ulp_data ()
	{
	/* Purpose: Function to create an IMEP data packet and send out the ULP data from the aggregation buffer */
	/* Requires: The aggregation buffer to be created. 														 */
	/* Effects: If the aggregation buffer is NON empty then IMEP packets are created and sent out.			 */
	Packet* 			imep_packet;
	Packet* 			ip_packet;
	
	FIN (manet_imep_send_ulp_data ())

	/* 	   The following needs to happen in this function:  */
	/* a) Create the IMEP packet - aggregate the ULP packets*/
	/*     in the agg buffer.								*/
	/* b) Create the IP packet which encapsulates the IMEP  */
	/* c) Send out the IP packet.                           */
	/* d) Set the Retransmission related interrupt for the  */
	/*	   IMEP packet sent									*/
	/* e) Schedule the proc. interrupt call for this same   */
	/*     function again.									*/

	/* First check if there is anything in the ULP agg buffer to actually send out*/
	if (op_prg_list_size (ulp_aggregation_buffer) > 0)
		{
		/* Okay there are actually some ULP packets to be send out -- handle them here*/
		
		/* First do (a) - remember multiple IMEP packets may be created from the buffer - if the*/
		/* load from the ULP is pretty high then this buffer might create multiple IMEP packets.*/
		imep_packet = manet_imep_create_data_packet ();
		
		if (imep_packet != OPC_NIL)
			{
			/* Now we have to do (d) -- (c) is done below. The ICI will contain the object sequence number and the yet to ack list for the object.*/
			/* Lets first find all the objects that we are actually sending out in this IMEP packet*/
			/* op_pk_nfd_access (imep_packet, "data_fields", &unformatted_packet); */
			
			/* Write out the sent ULP statistics for the IMEP packet sent */
			op_stat_write (imep_ulp_traffic_sent_bps_stathandle, op_pk_total_size_get (imep_packet));
			op_stat_write (imep_ulp_traffic_sent_bps_stathandle, 0);
			op_stat_write (imep_ulp_traffic_sent_pks_stathandle, 1);
			op_stat_write (imep_ulp_traffic_sent_pks_stathandle, 0);
			
			/* And then the global stat */
			op_stat_write (global_imep_ulp_traffic_sent_bps_stathandle, op_pk_total_size_get (imep_packet));
			op_stat_write (global_imep_ulp_traffic_sent_bps_stathandle, 0);
			op_stat_write (global_imep_ulp_traffic_sent_pks_stathandle, 1);
			op_stat_write (global_imep_ulp_traffic_sent_pks_stathandle, 0);
		
			/* And then perform operation (b)*/
			ip_packet = manet_imep_ip_datagram_create (imep_packet, ip_address_copy (IpI_Imep_Mcast_Addr)); 
	
			/* Schedule retransmission. */
			manet_imep_schedule_retransmission (ip_packet);
			
			/* Perform operation (c)*/
			op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, ip_packet);

			}
		
		clear_to_tx = OPC_FALSE;
			
		if (LTRACE_TORA_IMEP_MGR_ACTIVE)
			{
			op_prg_odb_print_major ("Sending an ULP packet to neighbors.", OPC_NIL);
			}
		}
	else
		{
		clear_to_tx = OPC_TRUE;
		}
		
	FOUT;
	}

static void
manet_imep_send_ip_packet (void* pk_ptr, int PRG_ARG_UNUSED (code))
	{
	ToraImepMgrT_Invocation_Struct* argmem;
	Packet* 	ip_packet;

	/* Purpose: Function to create the wrapper structure and send out the packet to Tora Imep Manager */
	/* Requires: The IP packet that is to be sent out from the node.								  */
	/* Effects: None.																				  */
	FIN (manet_imep_send_ip_packet (Packet*, int))
	
	ip_packet = (Packet*) pk_ptr;
		
	argmem = (ToraImepMgrT_Invocation_Struct*) op_prg_mem_alloc (sizeof (ToraImepMgrT_Invocation_Struct));	
	argmem->code = ToraImepMgrC_Invocation_Code_Pk_From_Imep_For_IP;
	argmem->pk_ptr = ip_packet;
	argmem->m_router_id = -1;
	argmem->link_state = ImepC_LinkState_Unknown;
	
	op_pro_invoke (tora_imep_mgr_prohandle, argmem);
					
	FOUT;	
	}
	
static void
manet_imep_insert_ulp_object_into_agg_buffer (Packet* ulp_pkptr)
	{
	/* Purpose: Function inserts the provided packet pointer into the AGG Buffer -- which is a List of ULP packets */
	/* Requires: Valid ULP packet pointer.																		   */
	/* Effects: Adds the Packet to the agg buffer -- waiting to be transmitted out from this router.			   */
	FIN (manet_imep_insert_ulp_object_into_agg_buffer (Packet*))
	
	/* Just insert this packet into the agg buffer list*/
	op_prg_list_insert (ulp_aggregation_buffer, ulp_pkptr, OPC_LISTPOS_TAIL); 
		
	FOUT;
	}

static Packet*
manet_imep_create_data_packet ()
	{
	/* Purpose: Function will just create an IMEP packet by aggregating the data objects in the agg buffer. */
	/* 			The created IMEP packet will be returned back to the calling function.						*/
	/* Requires: Aggregation buffer to be created previously and the ULP objects to be stored there.		*/
	/* Effects: Returns a newly created aggregated IMEP packet.												*/
	Packet* 						retval = OPC_NIL;
	Packet* 						pkptr;
	OpT_Packet_Size 				current_size = 0;
	ImepT_Header_Fields_Struct* 	header_fields_ptr = OPC_NIL;
	OpT_Packet_Size 				tmp_size;
	Packet* 						ulp_packet;
	Packet* 						unformatted_packet;
	int 							fd_index = 0;
	
	FIN (manet_imep_create_data_packet ())

	/* Create the data field for the IMEP packet first*/
	unformatted_packet = op_pk_create (0);
		
	/* Now create the header field */
	header_fields_ptr = manet_imep_msg_hdr_create (ImepC_Packet_Type_Data);
	
	/* Fill in the list for new membership list. */
	header_fields_ptr->ack_list = op_prg_list_create ();
	
	/* Just copy the current set of RDN members for this node into the list*/
	/* They are simple data types so the KP should be sufficient*/
	op_prg_list_elems_copy (rdn_info->root_rdn_members_list, header_fields_ptr->ack_list);

	/* Set the current size so far */
	/* sending over UDP we will only model the overhead from UDP. 		 */
	current_size = IMEP_UDP_HDR_SIZE + IMEP_MSG_HDR_SIZE + IMEP_OBJ_HDR_SIZE + 
		op_prg_list_size (rdn_info->root_rdn_members_list) * IMEP_ACK_REQ_ENTRY_SIZE;

	while (op_prg_list_size (ulp_aggregation_buffer))
		{
		pkptr = (Packet*) op_prg_list_access (ulp_aggregation_buffer, OPC_LISTPOS_HEAD);
		
		/* 32 is added here because that is the size of the object header. */
		tmp_size = op_pk_total_size_get (pkptr) + IMEP_OBJ_HDR_SIZE;
		
		/* IMEP packet size is in bytes */
		if (current_size + tmp_size < imep_pk_size * 8)
			{
			/* Okay to include this packet into the IMEP packet we are creating*/
			/* Insert the packet into the unformatted packet which is the element of the IMEP packet	*/
			ulp_packet = (Packet*) op_prg_list_remove (ulp_aggregation_buffer, OPC_LISTPOS_HEAD);
			op_pk_fd_set (unformatted_packet, fd_index++, OPC_FIELD_TYPE_PACKET, ulp_packet, -1);
			
			current_size += tmp_size;
			}
		else
			{
			break;
			}
		}
	
	/* Now create the packet and set the data and header fields */
	retval = op_pk_create_fmt (ImepS_Packet_Format);
	
	/* Place the header field on to the IMEP packet */
	op_pk_nfd_set (retval, "hdr_fields", header_fields_ptr, imep_packet_header_field_copy, 
		imep_packet_header_field_destroy, sizeof (ImepT_Header_Fields_Struct));

	/* Set the packet size. */
	op_pk_total_size_set (retval, current_size);
	
	/* Place the data field on the IMEP packet */
	op_pk_nfd_set (retval, "data_fields", unformatted_packet);
	
	FRET (retval);
	}


static void
manet_imep_buffer_received_ulp_packet (Packet* ulp_packet, ImepT_Ulp_Data_Struct* temp_data_struct_element, int rt_id)
	{
	/* Purpose: Function will just buffer the received packet -- its not yet ready to pass this to the ULP */
	/* Requires: None.																					   */
	/* Effects: None.																				   	   */
	ImepT_Ulp_Data_Struct* new_data_struct_element;
	
	FIN (manet_imep_buffer_received_ulp_packet (Packet* ulp_packet, ImepT_Ulp_Data_Struct* temp_data_struct_element))
	
	/* Create the new element that we want to put into the list -- remember we want to make a copy here and */
	/* not just store the original that was passed to us*/
	new_data_struct_element = (ImepT_Ulp_Data_Struct*) op_prg_mem_alloc (sizeof (ImepT_Ulp_Data_Struct));	

	new_data_struct_element->protocol = temp_data_struct_element->protocol;
	if (temp_data_struct_element->acknowledgement_list != OPC_NIL)
		{
		new_data_struct_element->acknowledgement_list = op_prg_list_create ();
		if (op_prg_list_size (temp_data_struct_element->acknowledgement_list) > 0)
				op_prg_list_elems_copy (temp_data_struct_element->acknowledgement_list, new_data_struct_element->acknowledgement_list);			
		}
	else
		{
		new_data_struct_element->acknowledgement_list = OPC_NIL;
		}
	
	new_data_struct_element->object_length = temp_data_struct_element->object_length;
	new_data_struct_element->sequence_number = temp_data_struct_element->sequence_number;

	/* Do not make a copy of the ULP packet -- because we have stripped it out of the incoming packet anyway */
	op_prg_list_insert (buffer_arrived_ulp_packets [rt_id], ulp_packet, OPC_LISTPOS_TAIL);
	op_prg_list_insert (buffer_arrived_ulp_packets [rt_id], new_data_struct_element, OPC_LISTPOS_TAIL);
	
	FOUT;
	}

static void
manet_imep_handle_received_ulp_packet (void* pkptr, int PRG_ARG_UNUSED (code))
	{
	/* Purpose: Function handles the ULP packet thats been received - remember a copy of the received ULP packet */
	/* is made and then the packet is passed up. Also and ACK is sent back to the sender.						 */
	/* Requires: None.																							 */
	/* Effects: None.																							 */
	ToraImepMgrT_Invocation_Struct* argmem;
	Packet* 						ulp_packet;
	
	FIN (manet_imep_handle_received_ulp_packet (ImepT_Ulp_Data_Struct*))
	
	ulp_packet = (Packet*) pkptr;
		
	argmem = (ToraImepMgrT_Invocation_Struct*) op_prg_mem_alloc (sizeof (ToraImepMgrT_Invocation_Struct));
	argmem->code = ToraImepMgrC_Invocation_Code_Pk_From_Imep_For_ULP;	
	argmem->pk_ptr = ulp_packet;
	argmem->m_router_id = -1;
	argmem->link_state = ImepC_LinkState_Unknown;
	
	/* Now invoke the ToraImep Manager to process the packet*/
	op_pro_invoke (tora_imep_mgr_prohandle, argmem);
	
	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		op_prg_odb_print_major ("IMEP Process, packet meant for ULP received. Forwarding packet to ULP.", OPC_NIL);
		}

	FOUT;
	}

static void
manet_imep_send_ack_response (int rtr_id, int color, int seq_number, int sender_address)
	{
	/* Purpose: Function sends the ACK back to the sender. */
	/* Requires: The ACK list and the router ID -- of the sender */
	/* Effects: None.											 */
	Packet* 							ack_pkptr;
	ImepT_Header_Fields_Struct* 		header_fields_ptr;
	Packet* 							ip_packet;
	ImepT_Ack_Elem_Struct*				temp_ack_elem;
	
	FIN (manet_imep_send_ack_response (...))
	
	/* Create the ack element to be sent out. */
	temp_ack_elem = (ImepT_Ack_Elem_Struct*) op_prg_mem_alloc (sizeof (ImepT_Ack_Elem_Struct));
			
	/* Fill in the values -- to indicate what exactly we are ACKing. */
	temp_ack_elem->source_rt_id = rtr_id;
	temp_ack_elem->color_ack = color;
	temp_ack_elem->seq_number_ack = seq_number;
	
	/* Create the header field */
	header_fields_ptr = manet_imep_msg_hdr_create (ImepC_Packet_Type_Ack);
	header_fields_ptr->ack_info_ptr = temp_ack_elem;
	
	/* Now create the echo packet */
	ack_pkptr = op_pk_create_fmt (ImepS_Packet_Format);
	
	/* And set the header and data fields on the packet */
	op_pk_nfd_set (ack_pkptr, "hdr_fields", header_fields_ptr, imep_packet_header_field_copy, 
		imep_packet_header_field_destroy, sizeof (ImepT_Header_Fields_Struct));

	/* Set the size of the packet. */
	op_pk_total_size_set (ack_pkptr, IMEP_UDP_HDR_SIZE + IMEP_MSG_HDR_SIZE + IMEP_OBJ_HDR_SIZE + IMEP_ACK_ENTRY_SIZE);
	
	/* Write out the sent statistics for the IMEP packet sent */
	/* ACK messages are treated as the control message 		  */
	manet_imep_control_pk_sent_stat_update (op_pk_total_size_get (ack_pkptr));

	/* Create the IP packet that will be sent out*/
	ip_packet = manet_imep_ip_datagram_create (ack_pkptr, sender_address);
	
	/* Send the packet out to IP */
	op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, ip_packet);

	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		op_prg_odb_print_major ("IMEP Process, sending ACK message in response to received ULP packets", OPC_NIL);
		}
	
	FOUT;					
	}

static void
manet_imep_schedule_retransmission (Packet* pkptr)
	{
	/* Purpose: Schedule retransmission for the first time. */
	/* Requires: Packet to be retransmitted.	*/
	/* Effects: Eventhandle filled in.			*/
	FIN (manet_imep_schedule_retransmission (pkptr));
	
	/* Reset the ack list. */
	op_prg_list_free (yet_to_ack_lptr);
	op_prg_list_elems_copy (rdn_info->root_rdn_members_list, yet_to_ack_lptr);
	
	/* Store the packet for later retransmission. */
	if (retransmit_pkptr)
		{
		op_pk_destroy (retransmit_pkptr);
		}
	retransmit_pkptr = op_pk_copy (pkptr);
	
	/* Reset the retransimssion counter. */
	num_of_retry_left = max_retries;
	
	/* Schedule the self interrupt for it. */
	retransmission_evhandle = op_intrpt_schedule_self (op_sim_time () + op_dist_uniform (imep_exponential_backoff [0]), 
		ImepC_Intrpt_Code_Packet_Retransmit);
	
	FOUT;
	}
	
static void
manet_imep_retransmission (void)
	{
	Packet 							*ip_packet, *imep_pkptr;
	ImepT_Header_Fields_Struct* 	header_fields_ptr;
	
	/* Purpose: Function will handle the retransmission of the object - to the neighbors that have not ACKed as yet */
	/* Requires: An ici with the required information associated with the interrupt when this function is called.	*/
	/* Effects: Schedules a self interrupt for retransmission after processing										*/
	FIN (manet_imep_retransmission ());

	/* Retrieve the saved packet. */
	ip_packet = op_pk_copy (retransmit_pkptr);
		
	/* We need to update the ack list. */
	op_pk_nfd_get (ip_packet, "data", &imep_pkptr);
	op_pk_nfd_access (imep_pkptr, "hdr_fields", &header_fields_ptr);
	op_prg_list_free (header_fields_ptr->ack_list);
	op_prg_list_elems_copy (yet_to_ack_lptr, header_fields_ptr->ack_list);
		
	/* Write out the sent statistics for the IMEP packet sent */
	op_stat_write (imep_ulp_traffic_sent_bps_stathandle, op_pk_total_size_get (imep_pkptr));
	op_stat_write (imep_ulp_traffic_sent_bps_stathandle, 0);
	op_stat_write (imep_ulp_traffic_sent_pks_stathandle, 1);
	op_stat_write (imep_ulp_traffic_sent_pks_stathandle, 0);
	
	/* And then the global stat */
	op_stat_write (global_imep_ulp_traffic_sent_bps_stathandle, op_pk_total_size_get (imep_pkptr));
	op_stat_write (global_imep_ulp_traffic_sent_bps_stathandle, 0);
	op_stat_write (global_imep_ulp_traffic_sent_pks_stathandle, 1);
	op_stat_write (global_imep_ulp_traffic_sent_pks_stathandle, 0);
	
	/* Set the packet back to the IP packet. */
	op_pk_nfd_set (ip_packet, "data", imep_pkptr);
	
	/* Finally -- complete this retransmission */
	op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_send_ip_packet, ip_packet);

	if (LTRACE_TORA_IMEP_MGR_ACTIVE)
		{
		op_prg_odb_print_major ("IMEP Process, performing packet retransmission from node", OPC_NIL);
		}	
	
	FOUT;
	}

static void
manet_imep_purge_arrived_ulp_packet_buffer (int rt_id)
	{
	Packet* ulp_packet;
	
	/* Purpose: Function will purge the entire buffer_arrived_ulp_packets list. */
	/* Requires: None.															*/
	/* Effects: Will delete the list contents from memory -- be sure to call this*/
	/* when you are sure that the buffered packets will never acutally be used, for */
	/* instance when you detect that the RDN at rt_id has changed its color.	    */
	FIN (manet_imep_purge_arrived_ulp_packet_buffer (int rt_id))
	
	while (op_prg_list_size (buffer_arrived_ulp_packets [rt_id]))
		{		
		ulp_packet = (Packet*) op_prg_list_remove (buffer_arrived_ulp_packets [rt_id], OPC_LISTPOS_HEAD);
		op_pk_destroy (ulp_packet);
		}
		
	FOUT; 
	}

static void
manet_imep_support_print_member_list (List* list_ptr)
	{
	int 		list_size;
	int 		counter;
	int* 		member;
	
	/* Purpose: Support function to print out the member list */
	FIN (manet_imep_support_print_member_list (List* list_ptr))

	list_size = op_prg_list_size (list_ptr);
	for (counter = 0; counter < list_size; counter ++)
		{
		member = (int*) op_prg_list_access (list_ptr, counter);
		printf (" %d,", *member);		
		}
	printf ("\n");
	
	FOUT;
	}

static void 
manet_imep_support_animation_imep_init ()	
	{
	/* Purpose: Function performs the animation related initializations for the simulation */
	/* Requires: None.																	   */
	/* Effects: None.																	   */
	int counter;
	
	FIN (manet_imep_support_animation_imep_init ())
	
	/* General initialization first */
	tora_imep_support_animation_init ();	
	
	if (tora_animation_requested)
		{
		/* Find the node IDs and store in the array*/
		manet_imep_support_build_animation_array ();
		
		/* Initialize the array */
		for (counter = 0; counter < global_router_id_index; counter ++)
			animation_drawing_ids [counter] = OPC_ANIM_ID_NIL;
			
		/* To update my location. */
		op_ima_obj_attr_get (router_information [router_id].node_object_id, "name", node_name);

		/* Perform the redraw every so often -- schedule based on an attribute*/
		op_intrpt_schedule_call (op_sim_time () + animation_update_time, OPC_NIL, manet_imep_support_perform_animation_updates, OPC_NIL);
		}
		
	FOUT;
	}

static void
manet_imep_support_build_animation_array ()
	{
	/* Purpose: Function is meant to build up the array holding the animation related structures*/
	/* Requires: The router IDs to be assigned prior to invoking this function.					*/
	/* Effects: Builds up the state variable array route_information 							*/
	int 				num_manet_routers;
	int 				counter;
	Objid 				manet_router;
	double 				xpos;
	double 				ypos;
	ToraT_Topo_Struct*	tmp_topo_ptr;
	
	FIN (manet_imep_support_build_animation_array ())
	
	/* The function will assume that the only nodes in the project are all Tora enabled routers */
	num_manet_routers = op_prg_list_size (global_tora_node_lptr);	
	for (counter = 0; counter < num_manet_routers; counter ++)
		{
		tmp_topo_ptr = (ToraT_Topo_Struct*) op_prg_list_access (global_tora_node_lptr, counter);
		
		/* We will move up the ladder to find the subnet being animated now. */
		manet_router = tmp_topo_ptr->node_objid;
		router_information [tmp_topo_ptr->rid].is_subnet = OPC_FALSE;
		while (op_topo_parent (manet_router) != Imep_Subnet_Objid)
			{
			router_information [tmp_topo_ptr->rid].is_subnet = OPC_TRUE;
			manet_router = op_topo_parent (manet_router);
			}

		op_ima_obj_attr_get (manet_router, "x position", &xpos);
		op_ima_obj_attr_get (manet_router, "y position", &ypos);
		
		/* Fill this information into the state variable we have*/
		router_information [tmp_topo_ptr->rid].x_pos = xpos;
		router_information [tmp_topo_ptr->rid].y_pos = ypos;
		router_information [tmp_topo_ptr->rid].node_object_id = manet_router;
		}
		
	FOUT;
	}

static void
manet_imep_support_perform_animation_updates (void* PRG_ARG_UNUSED (ptr), int PRG_ARG_UNUSED (code))
	{
	/* Purpose: Function will draw the lines from THIS router to each of the routers in */
	/* the rdn member list being passed to it. This function will be called under 2     */
	/* situations: Periodically -- every X seconds (to support the mobility of the nodes)*/
	/* and also whenever THIS router determines that a change has been made to the known */
	/* RDN list -- like if a neighbor has come UP or gone DOWN.							 */
	int 				counter;
	int 				list_size;
	int* 				tmp_router_id;
	double 				curr_xpos;
	double 				curr_ypos;
	int 				sym_props;
	int 				srcx, srcy, destx, desty;
	double 				my_curr_xpos;
	double 				my_curr_ypos;

	double slope;
	
	FIN (manet_imep_support_perform_animation_updates ())
		
	/* First clear all the drawings we have from this router. All the drawings are stored */
	/* in a list which contains the drawing IDs.										  */
	for (counter = 0; counter < global_router_id_index + 1; counter ++)
		{
		if ((animation_drawing_ids [counter] == OPC_ANIM_ID_NIL) && 
			(duplicate_animation_drawing_ids [counter] == OPC_ANIM_ID_NIL) /* && */
			)
			continue;
		
		if (animation_drawing_ids [counter] != OPC_ANIM_ID_NIL)
			op_anim_igp_drawing_erase (Tora_ImepI_Anvid, animation_drawing_ids [counter], OPC_ANIM_ERASE_MODE_XOR); 
		
		if (duplicate_animation_drawing_ids [counter] != OPC_ANIM_ID_NIL)
			op_anim_igp_drawing_erase (Tora_ImepI_Anvid, duplicate_animation_drawing_ids [counter], OPC_ANIM_ERASE_MODE_XOR); 

		animation_drawing_ids [counter] = OPC_ANIM_ID_NIL;
		duplicate_animation_drawing_ids [counter] = OPC_ANIM_ID_NIL;
		}
	
	/* Second for each of the members in the rdn_member_list, find the x and y position   */
	/* and update the state variable route_information with the information.			  */
	/* Third for each of the members of the rdn_member_list, just go ahead and perform    */
	/* the drawing.	Store all the drawing IDs in the list.								  */
	
	/* To handle if I myself have moved*/
	op_ima_obj_attr_get (router_information [router_id].node_object_id, "x position", &my_curr_xpos);
	op_ima_obj_attr_get (router_information [router_id].node_object_id, "y position", &my_curr_ypos);		
	router_information [router_id].x_pos = my_curr_xpos;
	router_information [router_id].y_pos = my_curr_ypos;

	/* Update the node position in case it moved. */
	if (router_information [router_id].is_subnet)
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_SUBNET, node_name, 
			OPC_ANIM_OBJ_ATTR_XPOS, my_curr_xpos, OPC_ANIM_OBJ_ATTR_YPOS, my_curr_ypos, OPC_EOL);
		}
	else
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, node_name, 
			OPC_ANIM_OBJ_ATTR_XPOS, my_curr_xpos, OPC_ANIM_OBJ_ATTR_YPOS, my_curr_ypos, OPC_EOL);
		}
	
	/* Line properties */
	sym_props = OPC_ANIM_COLOR_RGB123 | OPC_ANIM_PIXOP_XOR | OPC_ANIM_RETAIN;
	
	/* Convert the information into the pixel related coordinates */
	op_anim_ime_gen_pos (Tora_ImepI_Anvid, router_information [router_id].x_pos, router_information [router_id].y_pos,
				&srcx, &srcy);
	
	/* For each of the current members in the neighbor list for the RDN rooted at this node */
	/* perform the animation operation.													    */
	list_size = op_prg_list_size (rdn_info->root_rdn_members_list);
	for (counter = 0; counter < list_size; counter ++)
		{
		tmp_router_id = (int*) op_prg_list_access (rdn_info->root_rdn_members_list, counter);
		
		/* To handle if the neighbor has moved*/
		op_ima_obj_attr_get (router_information [*tmp_router_id].node_object_id, "x position", &curr_xpos);
		op_ima_obj_attr_get (router_information [*tmp_router_id].node_object_id, "y position", &curr_ypos);
		router_information [*tmp_router_id].x_pos = curr_xpos;
		router_information [*tmp_router_id].y_pos = curr_ypos;
		op_anim_ime_gen_pos (Tora_ImepI_Anvid, curr_xpos, curr_ypos, &destx, &desty);
		
		/* Part to actually draw something - what we will do here is: 				*/
		/* If this router ID is less than the neighbor router id ie *tmp_router_id 	*/
		/* then it will be responsible for drawing. This router will ONLY draw a line*/
		/* if it sees the neighbor AND if the neighbor sees it as well.				 */
		if (router_id < *tmp_router_id)
			{
			/* Attempt to draw here - only if you see the neighbor router AND if it sees you */
			/* Obviously you see the neighbor router, thats why you are here in the first place*/
			/* Check if neighbor can hear you -- the check is done by seeing the color you have for  */
			/* the RDN rooted at neighbor. If its not -1 (means you never heard from neihbor) or 0 (means you */
			/* lost connection to him at some point) then you belong to neighbor's membership list.			  */
			if (
				(rdn_info->rdns_info [*tmp_router_id].color == -1) ||
				(rdn_info->rdns_info [*tmp_router_id].color == 0)
				)
				{
				animation_drawing_ids [*tmp_router_id] = OPC_ANIM_ID_NIL;
				duplicate_animation_drawing_ids [*tmp_router_id] = OPC_ANIM_ID_NIL;
				continue;
				}
			else
				{
				slope = abs((double)(abs (desty) - abs (srcy)) / (double) (abs (destx) - abs (srcx)));
				
				if (slope > 1)
					{
					/* Okay to draw this line */
					animation_drawing_ids [*tmp_router_id] = op_anim_igp_line_draw (Tora_ImepI_Anvid, sym_props, srcx, srcy, destx, desty);
				
					/* And then draw the duplicate line - so that the animation lines appear thicker */
					duplicate_animation_drawing_ids [*tmp_router_id] = op_anim_igp_line_draw (Tora_ImepI_Anvid, sym_props, srcx + 1, srcy, destx + 1, desty);

					}
				else
					{
					/* Okay to draw this line */
					animation_drawing_ids [*tmp_router_id] = op_anim_igp_line_draw (Tora_ImepI_Anvid, sym_props, srcx, srcy, destx, desty);
				
					/* And then draw the duplicate line - so that the animation lines appear thicker */
					duplicate_animation_drawing_ids [*tmp_router_id] = op_anim_igp_line_draw (Tora_ImepI_Anvid, sym_props, srcx, srcy + 1, destx, desty + 1);
					}				
				}
			}
		else
			{
			/* Don't bother drawing anything here - the other routers will need to handle for you */
			}
		}
	
	/* Schedule the next call for the animation updates */
	op_intrpt_schedule_call (op_sim_time () + animation_update_time, OPC_NIL, manet_imep_support_perform_animation_updates, OPC_NIL);
	
	FOUT;	
	}

static void
manet_imep_support_log_message (Log_Category log_category, const char* entry_text)
	{
	/* Function to enter the simulation log message */
	FIN (manet_imep_support_log_message (Log_Category, char*))
	
	if (log_category == OpC_Log_Category_Protocol)
		op_prg_log_entry_write (imep_log_protocol_handle, entry_text);
	else
		op_prg_log_entry_write (imep_log_configuration_handle, entry_text);
		
	FOUT;	
	}

static void 
manet_imep_route_injection_handle_newcolor_object (int nbr_rt_id, List* member_list)
	{
	/* Purpose: Function is meant to handle the new color object received on this router and then make appropriate */
	/* modifications and adjustments to the IP common route table.												   */
	/* Requires: The neighbor router ID, and the member list as seen from the neighbor router.					   */
	/* Effects: Makes appropriate modifications to the IP common route table on this node.						   */
	int 								num_entries;
	int 								entry_index;
	int 								list_size;
	int 								counter;
	IpT_Cmn_Rte_Table_Entry* 			rte_table_entry;
	IpT_Address 						neighbor_ip_address;
	IpT_Address 						remote_router_ip_address;
	IpT_Next_Hop_Entry* 				next_hop_entry;
	int* 								temp_member_elem;
	IpT_Port_Info 						port_info;
	InetT_Address 						compare_address_A;
	InetT_Address 						compare_address_B;
	InetT_Address 						compare_address_1;
	InetT_Address 						compare_address_2;
	
	FIN (manet_imep_route_injection_handle_newcolor_object (int, List*))
		
	/* The following needs to happen in this function:											*/
	/* a. For all the entries in the IP Cmn Rt Table which have Next Hop = Neighbor Router		*/
	/*		If the "Dest" is not in the Nbr's Nbr list then just make Next Hop = Invalid.		*/
	/* b. For all routers in the Nbr's Nbr list													*/
	/*		If its "this" router, then continue;												*/
	/*		If its a directly connected router to "this" router, then continue;					*/
	/*		If 																					*/	
	/*			its already in the "Dest" in the IP Cmn Rt Table, then continue;				*/
	/* 		Else																				*/
	/*			Dest = Nbr's Nbr, Next Hop = Nbr, Enter this into the Cmn Rt Table				*/
	
	/* First lets try to do (a) above */
	/* Basically we are trying to handle the change in RDN information for our neighbor here at */
	/* this point. So if some routers have gone out of the RDN for this neighbor, then the next */
	/* hop for that router on this node should be marked as Invalid.							*/
	tora_imep_sup_rid_to_ip_addr (nbr_rt_id, &neighbor_ip_address);	
	
	/* Get the number of entries in the IP common route table. Note that we do not care about	*/
	/* IPv6 entries currently.																	*/
	num_entries = ip_cmn_rte_table_num_entries_get (module_data_ptr->ip_route_table,
		InetC_Addr_Family_v4);
	for (entry_index = 0; entry_index < num_entries; entry_index ++)
		{
		/* Get each entry in the route table 				*/
		/* Access the entry_index'th routing table entry.	*/
		rte_table_entry = ip_cmn_rte_table_access (module_data_ptr->ip_route_table,
												   entry_index, InetC_Addr_Family_v4);

		/* There will be only 1 next hop so access only the first element here */
		next_hop_entry = (IpT_Next_Hop_Entry*) op_prg_list_access (rte_table_entry->next_hop_list, 0);
		
		compare_address_A = inet_address_from_ipv4_address_create (neighbor_ip_address);
		compare_address_B = next_hop_entry->next_hop;
		if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
			{
			/* There is the first condition - if the destination is also the neighbor,*/
			/* then having the next hop as the neighbor is okay. We do not want to end*/
			/* up searching in its neighbor list, obviously its not going to be there.*/
			compare_address_1 = inet_address_from_ipv4_address_create (rte_table_entry->dest_prefix.address.ipv4_addr);
			compare_address_2 = inet_address_from_ipv4_address_create (neighbor_ip_address);
			if (inet_address_equal (compare_address_1, compare_address_2) == OPC_TRUE)
				continue;
			
			/* Okay, found an entry which has the next hop as the neighbor IP address */
			/* At this point, find the destination address and see if that destination*/
			/* is in the member list that is passed in. If it is not, then mark the   */
			/* next hop address as Invalid_IP address.								  */
			if (manet_imep_router_exists_in_member_list (rte_table_entry->dest_prefix.address.ipv4_addr, member_list))
				/* Nothing to do here - its all good the route table entry is consistent with what */
				/* the RDN for this neighbor says.												   */
				continue;
			else
				{
				/* Just make the next hop for this route entry invalid 		 */
				/* So a route can be found for this from TORA, or maybe IMEP */
				/* at a later time.											 */
				/* So the RDN for the neighbor has changed and it does not 	 */
				/* have the destination router in its member list.			 */
				next_hop_entry->next_hop = inet_address_from_ipv4_address_create (IPC_ADDR_INVALID);
				}
			}
		} /* End for (entry_index ... */
	
	/* Done with (a) at this point */
	/* Now attempt to do (b) above */
	list_size = op_prg_list_size (member_list);
	for (counter = 0; counter < list_size; counter ++)
		{
		temp_member_elem = (int*) op_prg_list_access (member_list, counter);
		
		if (*temp_member_elem == router_id)
			continue;
		
		if (manet_imep_rt_exists_in_list (*temp_member_elem, rdn_info->root_rdn_members_list))
			/* Just continue here, because we have a direct connection to this router, so we do not */
			/* need to go through this neighbor router to reach the router.							*/
			continue;
		
		/* Convert the router ID to the IP address */
		tora_imep_sup_rid_to_ip_addr (*temp_member_elem, &remote_router_ip_address);
		
		/* Check if this IP address is already present in the IP common route table - i.e., if there	  */
		/* is an entry for this IP address as the destination. If yes, then we do not need this alternate */
		/* route currently.																				  */
		if (Ip_Cmn_Rte_Table_Entry_Exists (module_data_ptr->ip_route_table, remote_router_ip_address, 
			IMEP_ROUTE_SUBNET_MASK, 1)) 
			{
			/* Okay the destination exists in the route table, but we need to check if the next hop is valid*/
			/* or not. If its invalid, we need to just update the next hop address.							*/
			if (ip_cmn_rte_table_entry_exists (module_data_ptr->ip_route_table, remote_router_ip_address, 
				IMEP_ROUTE_SUBNET_MASK, &rte_table_entry) == OPC_COMPCODE_FAILURE)
				{
				manet_imep_support_log_message (OpC_Log_Category_Protocol, "Unable to update neighbor route entry in the IP common route table.");				
				}
			
			next_hop_entry = (IpT_Next_Hop_Entry*) op_prg_list_access (rte_table_entry->next_hop_list, 0);

			compare_address_A = next_hop_entry->next_hop;
			compare_address_B = inet_address_from_ipv4_address_create (IPC_ADDR_INVALID);
			if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
				{
				next_hop_entry->next_hop = inet_address_from_ipv4_address_create (neighbor_ip_address);
				}
			}
		else
			{
			/* Okay, there is a new member in the RDN of our neighbor. So we need to add an entry into the */
			/* IP common route table to reach this new member through our neighbor.						   */
			ip_rte_addr_local_network (neighbor_ip_address, module_data_ptr->ip_route_table->iprmd_ptr, OPC_NIL, 
								&port_info, OPC_NIL);
				
			if (Ip_Cmn_Rte_Table_Entry_Add (module_data_ptr->ip_route_table, (void*) OPC_NIL, remote_router_ip_address,
				IMEP_ROUTE_SUBNET_MASK, neighbor_ip_address, port_info, 1,
				IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_TORA ,IPC_NO_MULTIPLE_PROC), 1) == OPC_COMPCODE_FAILURE)
				{
				manet_imep_support_log_message (OpC_Log_Category_Protocol, "Unable to insert neighbor route entry into the IP common route table.");
				}
			} /* End else */
		}
	FOUT;
	}

static Boolean
manet_imep_rt_exists_in_list (int rt_id, List* member_list)
	{
	/* Purpose: Function checks to see if the rt_id is part of the member_list. If it is then a True is returned */
	/* otherwise a false is returned.																			 */
	int 			list_size;
	int 			counter;
	int* 			temp_elem;
	
	FIN (manet_imep_rt_exists_in_list (int rt_id, List* member_list))
	
	if (member_list == OPC_NIL)
		FRET (OPC_FALSE);
		
	list_size = op_prg_list_size (member_list);
	for (counter = 0; counter < list_size; counter ++)
		{
		temp_elem = (int*) op_prg_list_access (member_list, counter);
		if (*temp_elem == rt_id)
			FRET (OPC_TRUE);
		}
		
	FRET (OPC_FALSE);	
	}
	
static Boolean
manet_imep_router_exists_in_member_list (IpT_Address destination_address, List* member_list)
 	{
	/* Purpose: Function will check if the destination address (after converting to its router ID) is part of the member */
	/* list that is passed in. If it is, then an OPC_TRUE is returned, otherwise an OPC_FALSE is returned.				 */
	/* Requires: The global list mapping the router ID to the IpT_Address is created previously.						 */
	/* Effects: None.																									 */
	int 			destination_router_id = -1;
	int 			list_size;
	int 			counter;
	int* 			temp_mem_elem;
	
	FIN (manet_imep_router_exists_in_member_list (IpT_Address, List*))
	
	/* Find out what the router ID for this destination router is */	
	tora_imep_sup_ip_addr_to_rid (&destination_router_id, destination_address);	
	
	/* Error handle */
	if ((destination_router_id == -1) || (member_list == OPC_NIL))
		FRET (OPC_FALSE);
	
	list_size = op_prg_list_size (member_list);
	for (counter = 0; counter < list_size; counter ++)
		{
		temp_mem_elem = (int*) op_prg_list_access (member_list, counter);
		if (destination_router_id == *temp_mem_elem)
			FRET (OPC_TRUE);
		}
	
	FRET (OPC_FALSE);	
	}

static void
manet_imep_clear_ip_route_entries (int rt_id)
	{
	/* Purpose: Function will purge the IP Cmn Route table with the entries that have the next hop */
	/* as the router being passed in to this function.											   */
	/* Requires: Valid router ID.																   */
	/* Effects: The IP Cmn Rt table on this router will have the appropriate entries purged out.   */
	IpT_Address 				router_ip_address;
	int 						num_entries;
	int 						entry_index;
	IpT_Cmn_Rte_Table_Entry* 	rte_table_entry;
	IpT_Next_Hop_Entry* 		next_hop_entry;
	InetT_Address 				compare_address_A;
	InetT_Address 				compare_address_B;
	
	FIN (manet_imep_clear_ip_route_entries (int rt_id))
	
	/* This function is supposed to perform the following: 			*/
	/* a. All the entries in the IP common route table that have 	*/
	/* 	   rt_id as the next hop, need to be reset back.			*/
	
	/* Find the router IP address for this neighbor router ID */	
	tora_imep_sup_rid_to_ip_addr (rt_id, &router_ip_address);
	
	num_entries = ip_cmn_rte_table_num_entries_get (module_data_ptr->ip_route_table,
		InetC_Addr_Family_v4);
	for (entry_index = 0; entry_index < num_entries; entry_index ++)
		{
		/* Get each entry in the route table 				*/
		/* Access the entry_index'th routing table entry.	*/
		rte_table_entry = ip_cmn_rte_table_access (
					module_data_ptr->ip_route_table, entry_index, InetC_Addr_Family_v4);

		/* There will be only 1 next hop so access only the first element here */
		next_hop_entry = (IpT_Next_Hop_Entry*) op_prg_list_access (rte_table_entry->next_hop_list, 0);
		
		compare_address_A = inet_address_from_ipv4_address_create (router_ip_address);
		compare_address_B = next_hop_entry->next_hop;
		if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
			{
			/* So irrespective of what the destination really is - if the next hop is this neighbor router */
			/* then just mark the next hop as being invalid.											   */
			next_hop_entry->next_hop = inet_address_from_ipv4_address_create (IPC_ADDR_INVALID);
			}
		} /* End for (entry_index ...	*/
		
	FOUT;
	}

static void
manet_imep_send_newcolor (void)
	{
	/* Purpose: Reset a few timers and send out new color object. */
	/* Requires: None.							 */
	/* Effects: Packet is sent out and the event for retransmision is scheduled. */
	FIN (manet_imep_send_newcolor (void));
	
	/* Change the current color for the RDN rooted on this node */
	rdn_info->current_color ++;

	/* Reset the sequence number that this node will be sending out*/
	rdn_info->current_seq_number ++;
	
	/* Flag to not send out ULP data yet. */
	clear_to_tx = OPC_FALSE;
	no_neighbor = OPC_FALSE;
	newcolor_object_required = OPC_FALSE;
	newcolor_transmission = OPC_TRUE;
	
	/* Send the packet out. */
	manet_imep_send_newcolor_obj_on_intf ();
	
	FOUT;
	}
	
static void
manet_imep_purge_ulp_pkts (void)
	{
	/* Helper function to destroy pending ULP packets. */
	FIN (manet_imep_purge_ulp_pkts (void));
	
	while (op_prg_list_size (ulp_aggregation_buffer))
		{
		op_pk_destroy ((Packet*) op_prg_list_remove (ulp_aggregation_buffer, OPC_LISTPOS_HEAD));
		}
	
	FOUT;
	}

static Compcode
manet_imep_update_retransmission (int rtr_id)
	{
	int		list_size, index, *tmp_rid_ptr;
	
	/* Purpose: Remove the router from the yet to ack neighbor list. */
	/* Requires: Router id received.							 */
	/* Effects: If found the entry is removed. */
	FIN (manet_imep_update_retransmission (rtr_id));
	
	/* Are we expecting an ack at all? */
	if (op_ev_valid (retransmission_evhandle))
		{
		/* See if this neibor can be found in the yet to ack list. */
		list_size = op_prg_list_size (yet_to_ack_lptr);
		for (index=0; index < list_size; index++)
			{
			tmp_rid_ptr = (int*) op_prg_list_access (yet_to_ack_lptr, index);
			
			if (*tmp_rid_ptr == rtr_id)
				{
				/* Found! */
				op_prg_mem_free (op_prg_list_remove (yet_to_ack_lptr, index));
				
				FRET (OPC_COMPCODE_SUCCESS);
				}
			}
		}
	
	FRET (OPC_COMPCODE_FAILURE);
	}
								 

/* End of Function Block */

/* Undefine optional tracing in FIN/FOUT/FRET */
/* The FSM has its own tracing code and the other */
/* functions should not have any tracing.		  */
#undef FIN_TRACING
#define FIN_TRACING

#undef FOUTRET_TRACING
#define FOUTRET_TRACING

#if defined (__cplusplus)
extern "C" {
#endif
	void manet_imep (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_manet_imep_init (int * init_block_ptr);
	void _op_manet_imep_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_manet_imep_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_manet_imep_alloc (VosT_Obtype, int);
	void _op_manet_imep_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
manet_imep (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (manet_imep ());

		{
		/* Temporary Variables */
		Boolean 								is_bp_timer=OPC_FALSE, is_ulp_reg=OPC_FALSE,
												is_beacon=OPC_FALSE, is_ulp_data=OPC_FALSE, is_newcolor=OPC_FALSE,
												is_ack=OPC_FALSE, is_echo=OPC_FALSE, is_mrt_timer=OPC_FALSE,
												is_outgoing_data=OPC_FALSE, is_rp_timer=OPC_FALSE,
												is_mbt_timer=OPC_FALSE;
		
		/* Temporary variables for the IDLE exit execs*/
		int 									type;
		char 									packet_format [128];
		
		Packet* 								pkptr;
		int 									intrpt_code;
		ToraImepMgrT_Invocation_Struct* 		argmem;
		ImepT_Header_Fields_Struct* 			temp_hdr_fields;
		
		/* Temporary variables for INIT exit execs*/
		int 									inner_counter_index, outer_counter_index;
		
		/* Temporary variables for Echo enter execs*/
		/* argmem and pkptr which are both defined above.*/
		int* 									temp_rtr_id;
		
		/* Temporary variables for Nbr_Dn enter execs*/
		Ici* 									iciptr;
		int 									nbr_rtr_id;
		
		/* Temporary variables for Process enter execs*/
		/* argmem and pkptr which are both defined above.*/
		/* tmp_hdr_fields as well defined above*/
		List* 									tmp_membership_list;
		
		/* Temporary variables for Ack enter execs*/
		/* argmem and pkptr which are both defined above.*/
		/* tmp_hdr_fields as well defined above*/
		
		/* Temporary variables for Forward state*/
		int 									counter;
		
		int 									result;
		
		ImepT_Ack_Elem_Struct* 					temp_ack_elem;
		int 									list_size;
		
		/* Temporary variables for the Resend state*/
		Packet* 								unformatted_packet;
		int 									num_objects;
		Packet* 								ulp_packet;
		
		int* 									curr_unack_rtr_id;
		int 									var_rtr_id;
		Boolean 								potentially_starved_newrouter = OPC_FALSE;
		
		char									msg[256];
		IpT_Address								sender_address;
		double									duration, first_beacon_tx_time;
		/* End of Temporary Variables */


		FSM_ENTER ("manet_imep")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (Idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (0, "Idle", state0_enter_exec, "manet_imep [Idle enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"manet_imep")


			/** state (Idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "Idle", "manet_imep [Idle exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Idle exit execs]", state0_exit_exec)
				{
				/* Determine the appropriate transition to make 			*/
				/* based on the invocation and the current interrupt type.  */
				
				/* The flow would be as: TORA invokes TORA_IMEP_MGR which invokes 				  			  */
				/* IMEP. When the invocation to IMEP is made a wrapper structure with the following 		  */
				/* information will be passed: (a) Code indicating what invocation is being made (b) Packet   */
				/* pointer if its a stream interrupt into TORA_IMEP_MGR 									  */
				
				/* First get the argument memory */
				argmem = (ToraImepMgrT_Invocation_Struct*) op_pro_argmem_access ();
				
				/* Note that there is no ULP registration support currently since TORA is the only ULP. However*/
				/* for future if registration support is required, then the appropriate code can be sent on the*/
				/* argmem that is passsed to IMEP and the code can be processed here.						   */
				if (is_ulp_reg == OPC_FALSE)
					{
					/* Determine the type of the current interrupt */
					type = op_intrpt_type ();
				
					/* Then switch based on the interrupt type */
					switch (type)
						{
						case (OPC_INTRPT_PROCESS):
							{
							/* Code to handle all the process interrupts from the Tora_Imep Manager */
							/* First get the argument memory associated with the process invocation */
							/* The packet will be as part of one of the elements in the structure */
							argmem = (ToraImepMgrT_Invocation_Struct*) op_pro_argmem_access ();
							if (argmem == OPC_NIL)
								{
								manet_imep_disp_message ("Argument memory is NULL in IMEP invocation");
								op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);	
								}
							
							/* Access the packet pointer embedded in the wrapper structure passed during invocation */
							pkptr = argmem->pk_ptr;
							if (pkptr != OPC_NIL)
								{
								/* Get the format of the packet received, we are expecting an IP packet */
								op_pk_format (pkptr, packet_format);
								if (strcmp (packet_format, "ip_dgram_v4") == 0)
									{
									/* The argmem with the invocation is freed here. The IP packet is   */
									/* freed up in the handler function below. Only the wrapper struct  */
									/* is freed up here.												*/
									op_prg_mem_free (argmem);
										
									/* Embedded in the IP packet is the imep packet */
									/* Have a function here that will just handle the received IP packet */
									/* At the end of handling the IP packet the function should just return */
									/* the IMEP packet 													  */
										
									/* Just to be sure -- initialize the state variable here before using it */
									incoming_imep_packet = OPC_NIL;
									incoming_imep_packet = manet_imep_handle_received_ip_packet (pkptr);
										
									/* Note that the returned value from the function above need not be further */
									/* error checked -- be sure that its an IMEP packet that is non NULL 		*/
									/* Now distinguish based on the packet type field in the packet 			*/
									op_pk_nfd_access (incoming_imep_packet, "hdr_fields", &temp_hdr_fields);
									sender_address = temp_hdr_fields->source_ip_address;
									
									switch (temp_hdr_fields->packet_type)
										{
										case (ImepC_Packet_Type_Beacon):
											{
											/* Handle the beacon packet arrival here */
											is_beacon = OPC_TRUE;
											break;
											}
										case (ImepC_Packet_Type_Echo):
											{
											/* Handle the echo packet reception here */
											is_echo = OPC_TRUE;
											break;
											}	
										case (ImepC_Packet_Type_Ack):
											{
											/* Handle the ACK packet reception here */
											is_ack = OPC_TRUE;
											break;
											}
										case (ImepC_Packet_Type_Newcolor):
											{
											/* Handle the NewColor packet reception here */
											is_newcolor = OPC_TRUE;
											break;
											}
										case (ImepC_Packet_Type_Data):
											{
											/* Handle the data for ULP here */
											is_ulp_data = OPC_TRUE;
											break;
											}
										default:
											{
											manet_imep_disp_message ("Unknown packet type in the packet received, stopping simulation");
											op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
											}
										} /* End switch (packet_type) */
									}
								else 
									{
									manet_imep_disp_message ("Unknown packet format received (packet is not of ip_dgram_v4 format), stopping simulation");
									op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
									}
								}
							else
								{
								/* Free up the argmem and issue the warnings.	  */
								op_prg_mem_free (argmem);
									
								manet_imep_disp_message ("Null packet received, stopping simulation");
								op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
								}
							break;
							} /* End case (OPC_INTRPT_PROCESS) */
						
						/* We should not be expecting any procedure interrupts at the moment.	*/
						case (OPC_INTRPT_PROCEDURE):
							{
							/* The argument memory is handled in the transitioned state */
							/* This is a TORA packet meant to be sent out of the node.  */
							is_outgoing_data = OPC_TRUE;
							break;
							}
						case (OPC_INTRPT_SELF):
							{
							/* Code to handle all the possible self interrupts */
							/* Find the code associated with this self interrupt */
							intrpt_code = op_intrpt_code ();
						
							switch (intrpt_code)
								{
								case (ImepC_Intrpt_Code_Beacon_Tx):
									{
									/* Indication that its time to generate a beacon */
									is_bp_timer = OPC_TRUE;
									break;
									} 
								case (ImepC_Intrpt_Code_Beacon_Mbt):
									{
									/* Indication that a neighbor has not sent a beacon for  */
									/* some time -- the neighbor must be considered unreachable at this point */
									is_mbt_timer = OPC_TRUE;
									break;
									}
								case (ImepC_Intrpt_Code_Packet_Retransmit):
									{
									/* Indication that the packet needs to be retransmitted -- ACKs have not */
									/* been received from all the neighbors as yet. */
									if (num_of_retry_left)
										{
										is_rp_timer = OPC_TRUE; 
										}
									else
										{
										/* No more retry. */
										is_mrt_timer = OPC_TRUE;
										}
									break;
									}
								default:
									{
									manet_imep_disp_message ("Unknown code with the received self interrupt");
									op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
									}
								} /* End switch (intrpt_code) */
							break;
								
							} /* End case (OPC_INTRPT_SELF) */
						default:
							{
							/* This is an unexpected interrupt delivered to the process */
							/* Warn the user and stop the simulation. 					*/
							manet_imep_disp_message ("Unknown interrupt type received, stopping simulation");
							op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
							}
						} /* End switch (type) */
					} /* End if (ulp_is_reg ...*/
				}
				FSM_PROFILE_SECTION_OUT (state0_exit_exec)


			/** state (Idle) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_imep [Idle trans conditions]", state0_trans_conds)
			FSM_INIT_COND (OUTGOING_DATA)
			FSM_TEST_COND (BP_TIMER)
			FSM_TEST_COND (ULP_REG)
			FSM_TEST_COND (BEACON)
			FSM_TEST_COND (ACK)
			FSM_TEST_COND (NEWCOLOR)
			FSM_TEST_COND (ULP_DATA)
			FSM_TEST_COND (MBT_TIMER)
			FSM_TEST_COND (RP_TIMER)
			FSM_TEST_COND (ECHO)
			FSM_TEST_COND (MRT_TIMER)
			FSM_TEST_LOGIC ("Idle")
			FSM_PROFILE_SECTION_OUT (state0_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 9, state9_enter_exec, ;, "OUTGOING_DATA", "", "Idle", "Encap", "tr_12", "manet_imep [Idle -> Encap : OUTGOING_DATA / ]")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ;, "BP_TIMER", "", "Idle", "Snd_Bcn", "tr_-1", "manet_imep [Idle -> Snd_Bcn : BP_TIMER / ]")
				FSM_CASE_TRANSIT (2, 7, state7_enter_exec, ;, "ULP_REG", "", "Idle", "Register", "tr_7", "manet_imep [Idle -> Register : ULP_REG / ]")
				FSM_CASE_TRANSIT (3, 12, state12_enter_exec, ;, "BEACON", "", "Idle", "Send_Echo", "tr_30", "manet_imep [Idle -> Send_Echo : BEACON / ]")
				FSM_CASE_TRANSIT (4, 5, state5_enter_exec, ;, "ACK", "", "Idle", "Ack", "tr_-6", "manet_imep [Idle -> Ack : ACK / ]")
				FSM_CASE_TRANSIT (5, 6, state6_enter_exec, ;, "NEWCOLOR", "", "Idle", "Process", "tr_8", "manet_imep [Idle -> Process : NEWCOLOR / ]")
				FSM_CASE_TRANSIT (6, 10, state10_enter_exec, ;, "ULP_DATA", "", "Idle", "Forward", "tr_26", "manet_imep [Idle -> Forward : ULP_DATA / ]")
				FSM_CASE_TRANSIT (7, 2, state2_enter_exec, ;, "MBT_TIMER", "", "Idle", "Nbr_Dn", "tr_-2", "manet_imep [Idle -> Nbr_Dn : MBT_TIMER / ]")
				FSM_CASE_TRANSIT (8, 3, state3_enter_exec, ;, "RP_TIMER", "", "Idle", "Resend", "tr_-3", "manet_imep [Idle -> Resend : RP_TIMER / ]")
				FSM_CASE_TRANSIT (9, 8, state8_enter_exec, ;, "ECHO", "", "Idle", "Echo", "tr_-5", "manet_imep [Idle -> Echo : ECHO / ]")
				FSM_CASE_TRANSIT (10, 4, state4_enter_exec, ;, "MRT_TIMER", "", "Idle", "Stop_Tx", "tr_-4", "manet_imep [Idle -> Stop_Tx : MRT_TIMER / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Snd_Bcn) enter executives **/
			FSM_STATE_ENTER_FORCED (1, "Snd_Bcn", state1_enter_exec, "manet_imep [Snd_Bcn enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Snd_Bcn enter execs]", state1_enter_exec)
				{
				/* Time to send the beacons out. For each interface on the router create a beacon */
				/* packet and send it out. */
				if (clear_to_tx)
					{
					/* Only if we are not in the middle of retransmission. */
					manet_imep_send_beacons ();
											
					if (op_prg_odb_ltrace_active ("manet_tora_imep_beacon"))
						{
						op_prg_odb_print_major ("Sending a beacon",  OPC_NIL);
						}
					}
				
				/* Schedule the next interrupt for sending out beacons */
				duration = op_dist_uniform (2 * (double) beacon_period);
				next_beacon_event = op_intrpt_schedule_self (op_sim_time () + duration, ImepC_Intrpt_Code_Beacon_Tx);
							
				if (op_prg_odb_ltrace_active ("manet_tora_imep_beacon"))
					{
					sprintf (msg, "Scheduling next beacon transmission after %f", duration);
					op_prg_odb_print_major (msg,  OPC_NIL);
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** state (Snd_Bcn) exit executives **/
			FSM_STATE_EXIT_FORCED (1, "Snd_Bcn", "manet_imep [Snd_Bcn exit execs]")


			/** state (Snd_Bcn) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Snd_Bcn", "Idle", "tr_44", "manet_imep [Snd_Bcn -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Nbr_Dn) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "Nbr_Dn", state2_enter_exec, "manet_imep [Nbr_Dn enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Nbr_Dn enter execs]", state2_enter_exec)
				{
				/* The neighbor has gone down. */
				/* The following needs to be done in this state:
				   a. Set the status of the neighbor to be Down 
				   b. Inform the ULP that have registered with this updated neighbor information
				   b1. Update the RDN member list
				   c. Send out NewColor object to all IMEP neighbors
				*/
				
				/* First get the ICI associated with the interrupt. The ICI */
				/* contains the neighbor router ID.							*/
				iciptr = op_intrpt_ici ();
				
				/* Get the neighbor router ID, stored on the ICI */
				if (iciptr != OPC_NIL)
					{
					if (op_ici_attr_get (iciptr, "router_id", &nbr_rtr_id) == OPC_COMPCODE_FAILURE)
						{
						/* Inform about error */
						manet_imep_disp_message ("Unable to obtain neighbor information from ICI");
						op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);	
						}
					else
						{
						if (LTRACE_TORA_IMEP_MGR_ACTIVE)
							{
							op_prg_odb_print_major ("A neighbor lost connectivity to the router.", OPC_NIL);
							}
						
						/* Its likely that the the number of retransmissions to this neighbor ran out and we marked the neighbor */
						/* down over there already. In such a case, the MBT timer is a moot point, and we do not need to take any*/
						/* action on this over here. 																			 */
						if (link_state_information [router_id][nbr_rtr_id] == ImepC_LinkState_Down)
							{
							/* Yes, the failed retransmissions have marked this neighbor as being down already. We will just */
							/* not have to take any action right now.														 */
							/* Remember this neighbor router can not be seen as ImepC_LinkState_Unknown here because we have */
							/* seen this router at least once in the simulation (by virtue of the fact that we have MBT timer*/
							/* setup for it) and so it should be either Up or Down only.									 */
							}
						else
							{
							/* The neighbor is still being seen as UP, we need to take some action here. */
							
							/* First mark the neighbor as being down. */
							/* When the neighbor is marked as down, the interrupt for retransmission will first check if the neighbor */
							/* is marked as being down - if so, then it will not send out the retransmission. 						  */
							manet_imep_handle_nbr_dn (nbr_rtr_id);
						
							/* Then do the b1. */
							/* Remove from the RDN member list this explicit neighbor */
							manet_imep_remove_router_from_member_list (nbr_rtr_id);
							}	
						}
					
					/* Destroy the ICI at this point */
					op_ici_destroy (iciptr);
					}
				
				/* Change the status of the boolean variable so we know that a new color object has to */
				/* be sent out below.																   */
				if (op_prg_list_size (rdn_info->root_rdn_members_list))
					{
					if (clear_to_tx)
						{
						if (newcolor_object_required)
							{
							manet_imep_send_newcolor ();
							}
						else
							{
							manet_imep_send_ulp_data ();
							}
						}
					}
				else
					{
					/* We want to remove any packet while we are isolated. */
					no_neighbor = OPC_TRUE;
					manet_imep_purge_ulp_pkts ();
					}
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (Nbr_Dn) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "Nbr_Dn", "manet_imep [Nbr_Dn exit execs]")


			/** state (Nbr_Dn) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Nbr_Dn", "Idle", "tr_46", "manet_imep [Nbr_Dn -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Resend) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "Resend", state3_enter_exec, "manet_imep [Resend enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Resend enter execs]", state3_enter_exec)
				{
				/* First perform some "cleaning" up of the yet to ack list. Its possible that by the time this retransmission */
				/* event actually occured, the MBT timer for the neighbor kicked in and marked the neighbor as being down. If */
				/* that is the case, then we want to remove that router ID from the yet to ack list. No point of attempting to*/
				/* retransmit to a neighbor that we know is down currently.													  */
				for (counter = 0; counter < op_prg_list_size (yet_to_ack_lptr); counter ++)
					{
					temp_rtr_id = (int*) op_prg_list_access (yet_to_ack_lptr, counter);
					if (link_state_information [router_id] [*temp_rtr_id] == ImepC_LinkState_Down)
						{
						temp_rtr_id = (int*) op_prg_list_remove (yet_to_ack_lptr, counter);
						op_prg_mem_free (temp_rtr_id);
						counter -= 1;
						}
					}
					
				/* Now check if we really need to be making a retransmission attempt or if its not necessary */
				if (op_prg_list_size (yet_to_ack_lptr))
					{
					/* Send the queued packet out. */
					manet_imep_retransmission ();
						
					/* Schedule the self interrupt for next retranmission. */
					num_of_retry_left--;
					retransmission_evhandle = op_intrpt_schedule_self (op_sim_time () + 
						op_dist_uniform (imep_exponential_backoff [max_retries - num_of_retry_left]), ImepC_Intrpt_Code_Packet_Retransmit);
					
					/* Write out the statistic to indicate that there has been a retransmission from this node */
					/* The stat will write out a "1" to indicate that there has been a retransmission.		   */
					op_stat_write (imep_retransmissions_stathandle, 1);
					}
				else
					{
					/* We probably need to transmit updated color information. */
					if (newcolor_object_required)
						{
						manet_imep_send_newcolor ();
						}
					else
						{
						manet_imep_send_ulp_data ();
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (Resend) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "Resend", "manet_imep [Resend exit execs]")


			/** state (Resend) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Resend", "Idle", "tr_48", "manet_imep [Resend -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Stop_Tx) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "Stop_Tx", state4_enter_exec, "manet_imep [Stop_Tx enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Stop_Tx enter execs]", state4_enter_exec)
				{
				/* Check the neghibors who has yet to ack. */			
				list_size = op_prg_list_size (yet_to_ack_lptr);
				for (counter = 0; counter < list_size; counter ++)
					{
					curr_unack_rtr_id = (int*) op_prg_list_access (yet_to_ack_lptr, counter);
										
					/* Now change the status of this node to be down */
					/* Remember this function will also inform the ULP about the change of neighbor status */		
					if (link_state_information [router_id][*curr_unack_rtr_id] == ImepC_LinkState_Down)
						{
						/* The router is already seen to be down - because of the MBT timer that would have */
						/* kicked in earlier. In such a case just continue, do not do anything here.		*/
						/* Remember the neighbor router should really not be seen as ImepC_LinkState_Unknown here because */
						/* this router has at least seen it once in the sim. (by virtue of the fact that we are expecting */
						/* ACKs from it) so the state should be either Down or Up */
						continue;
						}
					else
						{
						/* The MBT timer has not kicked in as yet, so we will take the corrective action here.*/
						manet_imep_reset_mbt_timer (*curr_unack_rtr_id, OPC_FALSE);
						
						/* Mark the neighbor as being down and inform the ULP about it.						  */
						manet_imep_handle_nbr_dn (*curr_unack_rtr_id);
						
						/* Take out the neighbor from member list */
						manet_imep_remove_router_from_member_list (*curr_unack_rtr_id);
						}	
					}
				
				/* Change the status of the boolean variable so we know that a new color object has to */
				/* be sent out below.																   */
				if (op_prg_list_size (rdn_info->root_rdn_members_list))
					{
					if (newcolor_object_required)
						{
						manet_imep_send_newcolor ();
						}
					else
						{
						manet_imep_send_ulp_data ();
						}
					}
				else
					{
					/* We want to remove any packet while we are isolated. */
					no_neighbor = OPC_TRUE;
					newcolor_transmission = OPC_FALSE;
					clear_to_tx = OPC_TRUE;
				
					manet_imep_purge_ulp_pkts ();
					}
				}
				FSM_PROFILE_SECTION_OUT (state4_enter_exec)

			/** state (Stop_Tx) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "Stop_Tx", "manet_imep [Stop_Tx exit execs]")


			/** state (Stop_Tx) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Stop_Tx", "Idle", "tr_50", "manet_imep [Stop_Tx -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Ack) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "Ack", state5_enter_exec, "manet_imep [Ack enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Ack enter execs]", state5_enter_exec)
				{
				/* Get the ACK list from the packet	 */
				op_pk_nfd_access (incoming_imep_packet, "hdr_fields", &temp_hdr_fields);
				
				temp_ack_elem = temp_hdr_fields->ack_info_ptr;
				
				/* If this is an ACK that we are expecting, then process it. Otherwise, take appropriate action*/
				/* as below.																				   */
				if (temp_ack_elem->source_rt_id == router_id) 
					{
					/* If we successfully update the retrans timer, then we were still expecting the ACK. In that */
					/* case, just go ahead and handle the ACK equivalency as well.								   */
					/* Otherwise its an ACK that has come in too late for us.									   */
					if (temp_ack_elem->seq_number_ack == rdn_info->current_seq_number)
						{
						/* Remove the neigbor from the yet to ack list. */
						if (OPC_COMPCODE_SUCCESS == manet_imep_update_retransmission (temp_hdr_fields->rtr_id))
							{
							/* Echo Equivalency -- we have heard the ACK so no need of an ECHO, reschedule the MBT timer */
							manet_imep_reset_mbt_timer (temp_hdr_fields->rtr_id, OPC_TRUE);
							
							/* Now is the time to mark the neighbor as valid. */
							if (newcolor_transmission)
								{
								link_state_information [router_id] [temp_hdr_fields->rtr_id] = ImepC_LinkState_Up;
								
								if (link_state_information [temp_hdr_fields->rtr_id] [router_id] == ImepC_LinkState_Up)
									{
									/* Now the link to the neighbor is fully bidirectional. */
									
									/* Update the number of neighbors stat */
									op_stat_write (imep_number_of_neighbors_stathandle, (double) op_prg_list_size (rdn_info->root_rdn_members_list));	
							
									/* Reset the flag as well. */
									no_neighbor = OPC_FALSE;
									
									/* Next do (C) */
									var_rtr_id = temp_hdr_fields->rtr_id;
									op_intrpt_schedule_call (op_sim_time (), ImepC_LinkState_Up, manet_imep_inform_ulp_about_neighbor, 
										op_prg_mem_copy_create (&var_rtr_id, sizeof (int))); 
							
									/* Finally start the MBT timer, i.e do (D) above. */
									manet_imep_reset_mbt_timer (temp_hdr_fields->rtr_id, OPC_TRUE);
								
									if (LTRACE_TORA_IMEP_MGR_ACTIVE)
										{
										op_prg_odb_print_major ("A new neighbor was detected by router.",  OPC_NIL);
										}
									}
								}
								
							if (!op_prg_list_size (yet_to_ack_lptr))
								{
								/* We have got all the acks for the current transmission. */
								op_ev_cancel (retransmission_evhandle);
								
								if (newcolor_transmission)
									{
									newcolor_transmission = OPC_FALSE;
									}
									
								/* Increment the seq number now that we are to move on to next seq number. */
								rdn_info->current_seq_number++;
									
								if (LTRACE_TORA_IMEP_MGR_ACTIVE)
									{
									op_prg_odb_print_major ("Got all the acks for the last reliable transmission.", OPC_NIL);
									}
									
								if (newcolor_object_required)
									{
									/* Send the newcolor object first of all. */
									manet_imep_send_newcolor ();
									}
								else
									{
									manet_imep_send_ulp_data ();
									}
								}
							else
								{
								if (LTRACE_TORA_IMEP_MGR_ACTIVE)
									{
									op_prg_odb_print_major ("Got one of the acks for the last reliable transmission.", OPC_NIL);
									}
								}
							}
						}
					else
						{
						if (LTRACE_TORA_IMEP_MGR_ACTIVE)
							{
							op_prg_odb_print_major ("ACK received with wrong seq_number.", OPC_NIL);
							}
						}
					}
				
				/* Clean up. */
				op_pk_destroy (incoming_imep_packet);
				
					
				}
				FSM_PROFILE_SECTION_OUT (state5_enter_exec)

			/** state (Ack) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "Ack", "manet_imep [Ack exit execs]")


			/** state (Ack) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Ack", "Idle", "tr_54", "manet_imep [Ack -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Process) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "Process", state6_enter_exec, "manet_imep [Process enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Process enter execs]", state6_enter_exec)
				{
				/* Handle the NewColor object. Transition to this state is made when a NewColor 		*/
				/* object is received on this router. 													*/
				/* The following needs to occur when this transition happens:							*/
				/*   a. If this router is part of the explicit neighbor list on the NewColor packet		*/
				/*		Then record the color for the RDN for the router on the incoming NewColor packet*/
				/*	  Else																				*/
				/*		Record the color for the RDN for the router on the incoming NewColor packet as 0*/
				/*																						*/
				
				/* Remember the incoming_imep_packet is in the state variable */
				/* Access the explicit member list on the packet 			  */
				/* First get the header field 								  */
				op_pk_nfd_access (incoming_imep_packet, "hdr_fields", &temp_hdr_fields);
				
				/* Access the membership list */
				tmp_membership_list = (List*) temp_hdr_fields->ack_list;
					
				/* Error handle */
				if (tmp_membership_list != OPC_NIL)
					{
					/* Find if this router ID is part of the membership list */
					if (manet_imep_rtr_exist_in_mbrship_list (tmp_membership_list) == OPC_TRUE)
						{
						/* Check to see if this is retransmission of the last packet. */
						if (rdn_info->rdns_info [temp_hdr_fields->rtr_id].color == temp_hdr_fields->color)
							{
							/* Schedule to send the ACK packet. */
							manet_imep_send_ack_response (temp_hdr_fields->rtr_id, temp_hdr_fields->color, 
								temp_hdr_fields->seq_number, sender_address);
							}
						else if	(rdn_info->rdns_info [temp_hdr_fields->rtr_id].color < temp_hdr_fields->color)
							{
							/* Sure it is. Now we need to update the data structure holding the information */
							/* about the RDNs.																*/
							manet_imep_update_rdn_info (temp_hdr_fields->rtr_id, temp_hdr_fields->color, temp_hdr_fields->seq_number + 1);
				
							/* Send the ack back to the sender. */
							manet_imep_send_ack_response (temp_hdr_fields->rtr_id, temp_hdr_fields->color, 
								temp_hdr_fields->seq_number, sender_address);
						
							/* Purge the buffered ULP packets -- since the color for the RDN at the remote router */
							/* has changed anyway */
							manet_imep_purge_arrived_ulp_packet_buffer (temp_hdr_fields->rtr_id);
							
							/* Need to handle the NewColor object appropriately if the IMEP route injection is requested */
							/* Handle the route injection if this router is part of the explicit membership list as seen */
							/* by the neighbor router. If this router is not part of the member list then no change will */
							/* be performed here - instead when this router detects the neighbor to be "down", then 	 */
							/* appropriate modifications to the Common Route Table will be made.						 */
							if ((imep_route_injection) ||
								(node_level_route_injection))
								{
								manet_imep_route_injection_handle_newcolor_object (temp_hdr_fields->rtr_id, tmp_membership_list);
								}
							
							if (LTRACE_TORA_IMEP_MGR_ACTIVE)
								{
								op_prg_odb_print_major ("Updated NewColor information for neighbor RDN on node:", OPC_NIL);
								}
							
							/* This means the neighbor can see me. */
							if (link_state_information [temp_hdr_fields->rtr_id] [router_id] != ImepC_LinkState_Up)
								{
								/* This is new information regarding the neighbor.  Process it. */
								link_state_information [temp_hdr_fields->rtr_id] [router_id] = ImepC_LinkState_Up;
								
								if (link_state_information [router_id] [temp_hdr_fields->rtr_id] == ImepC_LinkState_Up)
									{
									/* Now the link to the neighbor is fully bidirectional. */
				
									/* Update the number of neighbors stat */
									op_stat_write (imep_number_of_neighbors_stathandle, (double) op_prg_list_size (rdn_info->root_rdn_members_list));	
								
									/* Reset the flag as well. */
									no_neighbor = OPC_FALSE;
									
									/* Next do (C) */
									var_rtr_id = temp_hdr_fields->rtr_id;
									op_intrpt_schedule_call (op_sim_time (), ImepC_LinkState_Up, manet_imep_inform_ulp_about_neighbor, 
										op_prg_mem_copy_create (&var_rtr_id, sizeof (int))); 
								
									/* Finally start the MBT timer, i.e do (D) above. */
									manet_imep_reset_mbt_timer (temp_hdr_fields->rtr_id, OPC_TRUE);
								
									if (LTRACE_TORA_IMEP_MGR_ACTIVE)
										{
										op_prg_odb_print_major ("A new neighbor was detected by router.",  OPC_NIL);
										}
									}
								}
							} /* End if (manet_imep ... */
						}
					else
						{
						/* Check to see if this is retransmission of the last packet. */
						if (rdn_info->rdns_info [temp_hdr_fields->rtr_id].color == temp_hdr_fields->color)
							{
							/* Do nothing. */
							}
						else
							{
							/* Its not a member -- so just put the color as 0 when updating the structure holding */
							/* the info about RDNs. */
							manet_imep_update_rdn_info (temp_hdr_fields->rtr_id, 0, temp_hdr_fields->seq_number + 1);			
							
							/* Purge the buffered ULP packets -- since the color for the RDN at the remote router */
							/* has changed anyway */
							manet_imep_purge_arrived_ulp_packet_buffer (temp_hdr_fields->rtr_id);
						
							/* Send the echo to the sender to include me. */
							manet_imep_send_echo_response (sender_address);
							
							if ((link_state_information [temp_hdr_fields->rtr_id] [router_id] == ImepC_LinkState_Up) &&
								(link_state_information [router_id] [temp_hdr_fields->rtr_id] == ImepC_LinkState_Up))
								{
								/* It is possible we experienced link failure on the other side. Let the ULP know. */
								manet_imep_remove_router_from_member_list (temp_hdr_fields->rtr_id);
								manet_imep_handle_nbr_dn (temp_hdr_fields->rtr_id);
								}
								
							if (LTRACE_TORA_IMEP_MGR_ACTIVE)
								{
								op_prg_odb_print_major ("Updated NewColor information (with 0) for neighbor RDN", OPC_NIL);
								}
							}
						}
					}
				else
					{
					/* Create log entry here - problem with a no members in the newcolor object received. */
					manet_imep_support_log_message (OpC_Log_Category_Protocol, 
						"Newcolor object received with no routers specified in the membership list.");
					}
				
				/* Destroy the incoming packet here */
				op_pk_destroy (incoming_imep_packet);
				}
				FSM_PROFILE_SECTION_OUT (state6_enter_exec)

			/** state (Process) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "Process", "manet_imep [Process exit execs]")


			/** state (Process) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Process", "Idle", "tr_53", "manet_imep [Process -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Register) enter executives **/
			FSM_STATE_ENTER_FORCED (7, "Register", state7_enter_exec, "manet_imep [Register enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Register enter execs]", state7_enter_exec)
				{
				/* State is not required at present. TORA is the only ULP and no explicit */
				/* registration from TORA is necessary at this point.					  */
				}
				FSM_PROFILE_SECTION_OUT (state7_enter_exec)

			/** state (Register) exit executives **/
			FSM_STATE_EXIT_FORCED (7, "Register", "manet_imep [Register exit execs]")


			/** state (Register) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Register", "Idle", "tr_45", "manet_imep [Register -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Echo) enter executives **/
			FSM_STATE_ENTER_FORCED (8, "Echo", state8_enter_exec, "manet_imep [Echo enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Echo enter execs]", state8_enter_exec)
				{
				/* Echo packet reception */
				op_pk_nfd_access (incoming_imep_packet, "hdr_fields", &temp_hdr_fields);
				
				/* Determine if this is the first time we are hearing from this neighbor */
				if (link_state_information [router_id] [temp_hdr_fields->rtr_id] != ImepC_LinkState_Up)
					{
					/* We will try to make it as a part of RDN from this node. */
					
					/* See if we already processing this neighbor. */
					if (!(manet_imep_rt_exists_in_list (temp_hdr_fields->rtr_id, rdn_info->root_rdn_members_list)))
						{
						/* We need to process this neighbor. */
						temp_rtr_id = (int*) op_prg_mem_alloc (sizeof (int));
						*temp_rtr_id = temp_hdr_fields->rtr_id;
						op_prg_list_insert (rdn_info->root_rdn_members_list, temp_rtr_id, OPC_LISTPOS_TAIL);
					
						/* The unidirectional link is up. */
						link_state_information [router_id] [temp_hdr_fields->rtr_id] = ImepC_LinkState_Up;
						
						/* Now do (B) */
						if (clear_to_tx)
							{
							manet_imep_send_newcolor ();
							}
						else
							{
							/* Set the flag so it can be done after the current retransmission. */
							newcolor_object_required = OPC_TRUE;
							}
				
						if (LTRACE_TORA_IMEP_MGR_ACTIVE)
							{
							op_prg_odb_print_major ("A new neighbor was detected by router.",  OPC_NIL);
							}
						}
					}
				else
					{
					/* We do not need to add to the rdn_members list at this point -- it should already be there */
					manet_imep_reset_mbt_timer (temp_hdr_fields->rtr_id, OPC_TRUE);
					}
				
				/* Destroy the incoming packet */
				op_pk_destroy (incoming_imep_packet);	
				
				}
				FSM_PROFILE_SECTION_OUT (state8_enter_exec)

			/** state (Echo) exit executives **/
			FSM_STATE_EXIT_FORCED (8, "Echo", "manet_imep [Echo exit execs]")


			/** state (Echo) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Echo", "Idle", "tr_51", "manet_imep [Echo -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Encap) enter executives **/
			FSM_STATE_ENTER_FORCED (9, "Encap", state9_enter_exec, "manet_imep [Encap enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Encap enter execs]", state9_enter_exec)
				{
				/* First access the argmem for the packet pointer got from TORA */
				/* Also handle the memory for the argmem */
				argmem = (ToraImepMgrT_Invocation_Struct*) op_pro_argmem_access ();
				pkptr = argmem->pk_ptr;
				op_prg_mem_free (argmem);
				
				if (!no_neighbor)
					{
					/* Handle the size check on the ULP object data -- if its beyond */
					/* a particular size then we should reject it.					 */
					/* We want to deduct 32 because thats the object header size.    */
					if (op_pk_total_size_get (pkptr) > ((imep_pk_size * 8) - 32))
						{
						/* Reject this packet - provide a simulation message */
						op_pk_destroy (pkptr);
						manet_imep_support_log_message (OpC_Log_Category_Configuration, "ULP (TORA) packet size exceeds Max IMEP Length. Discarding packet.");
						}
					else
						{
						/* The ULP packet can be handled -- its of acceptable size */
						/* Just insert the packet into the aggregation buffer */
						manet_imep_insert_ulp_object_into_agg_buffer (pkptr);
						
						/* Send the packet out. */
						if (clear_to_tx)
							{
							manet_imep_send_ulp_data ();
							}
						}
					}
				else
					{
					/* We do not have any neighbor to send the packet out. */
					op_pk_destroy (pkptr);
					}
				}
				FSM_PROFILE_SECTION_OUT (state9_enter_exec)

			/** state (Encap) exit executives **/
			FSM_STATE_EXIT_FORCED (9, "Encap", "manet_imep [Encap exit execs]")


			/** state (Encap) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Encap", "Idle", "tr_49", "manet_imep [Encap -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Forward) enter executives **/
			FSM_STATE_ENTER_FORCED (10, "Forward", state10_enter_exec, "manet_imep [Forward enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Forward enter execs]", state10_enter_exec)
				{
				/* The received IMEP packet is in the state variable "incoming_imep_packet" */
				op_pk_nfd_get (incoming_imep_packet, "data_fields", &unformatted_packet);
				op_pk_nfd_access (incoming_imep_packet, "hdr_fields", &temp_hdr_fields);
				
				if ((temp_hdr_fields == OPC_NIL) || (unformatted_packet == OPC_NIL))
					{
					/* Just signal an error and stop the simulation here */
					manet_imep_disp_message ("Data packet has a NULL data fields pointer");
					op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
					}
				else
					{
					/* initialize the return result */
					/* Also initialize the bool to determine if this router */
					/* is being potentially starved.						*/
					result = -1;
					potentially_starved_newrouter = OPC_FALSE;
					
					/* Get the list of packets that are present on the data fields structure */
					if (rdn_info->rdns_info [temp_hdr_fields->rtr_id].color != temp_hdr_fields->color)
						{
						/* Send an Echo to let the sender know to include me as the member. */
						manet_imep_send_echo_response (sender_address);
						
						if ((link_state_information [temp_hdr_fields->rtr_id] [router_id] == ImepC_LinkState_Up) &&
							(link_state_information [router_id] [temp_hdr_fields->rtr_id] == ImepC_LinkState_Up))
							{
							/* It is possible we experienced link failure on the other side. Let the ULP know. */
							manet_imep_remove_router_from_member_list (temp_hdr_fields->rtr_id);
							manet_imep_handle_nbr_dn (temp_hdr_fields->rtr_id);
							}
				
						if (LTRACE_TORA_IMEP_MGR_ACTIVE)
							{
							op_prg_odb_print_major ("Destroying received ULP packet for color mismatch.", OPC_NIL);
							}
						}
					else
						{
						/* Correct color. */
						
						/* See if we need to send the ack for this guy. */
						if (temp_hdr_fields->seq_number == rdn_info->rdns_info [temp_hdr_fields->rtr_id].sequence_number)
							{
							/* Correct seq number. */
				
							/* Retrieve the packets and send them up. */
							num_objects = op_pk_fd_max_index (unformatted_packet);
							for (counter = 0; counter <= num_objects; counter++)
								{
								/* access the packet and send it up. */
								op_pk_fd_get (unformatted_packet, counter, &ulp_packet);
								op_intrpt_schedule_call (op_sim_time (), OPC_NIL, manet_imep_handle_received_ulp_packet, ulp_packet);
								}
							
							/* We should send the ACK packet to the sender. */
							manet_imep_send_ack_response (temp_hdr_fields->rtr_id, temp_hdr_fields->color, 
								temp_hdr_fields->seq_number, sender_address);
											
							/* Increment to reflect the next expected sequence number; */
							(rdn_info->rdns_info [temp_hdr_fields->rtr_id].sequence_number) ++;
							
							if (LTRACE_TORA_IMEP_MGR_ACTIVE)
								{
								op_prg_odb_print_major ("Sending ACK for valid reception.", OPC_NIL);
								}
							}
						else 
							{
							if (temp_hdr_fields->seq_number + 1 == rdn_info->rdns_info [temp_hdr_fields->rtr_id].sequence_number)
								{
								if (manet_imep_rtr_exist_in_mbrship_list (temp_hdr_fields->ack_list))
									{
									/* This condition indicates the ACK was not received or delayed at the sender. */
									/* Retransmit the ACK. */
									manet_imep_send_ack_response (temp_hdr_fields->rtr_id, temp_hdr_fields->color, 
										temp_hdr_fields->seq_number, sender_address);
									
									if (LTRACE_TORA_IMEP_MGR_ACTIVE)
										{
										op_prg_odb_print_major ("Resending ACK for dup reception.", OPC_NIL);
										}
									}
								else
									{
									/* Sender is retransmitting the packet for some other neighbor. Ignore it. */
									if (LTRACE_TORA_IMEP_MGR_ACTIVE)
										{
										op_prg_odb_print_major ("Ignoring duplicate ULP reception.", OPC_NIL);
										}
									}
								}
							else
								{
								printf ("\nreceived	seq_num:%d, expecting:%d", 
									temp_hdr_fields->seq_number, rdn_info->rdns_info [temp_hdr_fields->rtr_id].sequence_number);
								}
							}
						}
					}
				
				/* Get rid of the incoming packet here */
				op_pk_destroy (unformatted_packet);
				op_pk_destroy (incoming_imep_packet);
				}
				FSM_PROFILE_SECTION_OUT (state10_enter_exec)

			/** state (Forward) exit executives **/
			FSM_STATE_EXIT_FORCED (10, "Forward", "manet_imep [Forward exit execs]")


			/** state (Forward) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Forward", "Idle", "tr_52", "manet_imep [Forward -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (11, "Init", "manet_imep [Init enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Init enter execs]", state11_enter_exec)
				{
				/* Access and parse the attribute information. */
				manet_imep_sv_init ();
				
				/* We will assume that there is only one ULP for this version. */
				global_flag_single_ulp = OPC_TRUE;
				
				/* Schedule the first beacon. */
				/* The state variable will keep track of the next beacon sending event for this process*/
				/* so that we can directly access it at a later time. */
				first_beacon_tx_time = 2 * (double) beacon_period;
				if (first_beacon_tx_time > MAX_INITIAL_BEACON_TIME)
					{
					first_beacon_tx_time = MAX_INITIAL_BEACON_TIME;
					}
					
				next_beacon_event = op_intrpt_schedule_self (op_sim_time () + 
					op_dist_uniform (first_beacon_tx_time), ImepC_Intrpt_Code_Beacon_Tx);
				
				/* Create the pooled memory structures for the IMEP header and data fields */
				if ((global_imep_header_fields_pmohandle == OPC_PMO_INVALID))
					{
					/* Not allocated the pooled memory yet, just go ahead and do so now */
					imep_allocate_pooled_memory ();
					}
				
				/* Register the Multicast address so IP knows that it needs to forward them 						*/
				/* Loop through all the interfaces and register all of them to be recipients of the mcast messages  */
				for (inner_counter_index = 0; inner_counter_index < total_num_ip_intf; inner_counter_index ++)
					{
					Ip_Address_Multicast_Register (IpI_Imep_Mcast_Addr, inner_counter_index, IP_MCAST_NO_PORT, sv_node_objid); 
					}
				
				/* Identify the router ID on this node. If it is greater than "global_router_id_index" then just */
				/* make "global_router_id_index" equal to this router_id value. The idea is to get in the global */
				/* variable the highest router ID in the network -- that will be used in the creation of the     */
				/* structure for the Link state sensing.														 */
				if (router_id > global_router_id_index)
					{
					/* Update the global value */
					global_router_id_index = router_id;
				    }
				
				/* Schedule a self interrupt to move to the next state -- this will give time for all the IMEP processes */
				/* to initialize and determinations such as the global_router_id_index would have been made.			 */
				op_intrpt_schedule_self (op_sim_time (), -1);
				
				if (LTRACE_TORA_IMEP_MGR_ACTIVE)
					{
					op_prg_odb_print_major ("IMEP Process invoked, performed initializations", OPC_NIL);
					}
				}
				FSM_PROFILE_SECTION_OUT (state11_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (23,"manet_imep")


			/** state (Init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (11, "Init", "manet_imep [Init exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Init exit execs]", state11_exit_exec)
				{
				/* The global_router_id_index has been determined. We will now create a 2-d array */
				/* of integers that will hold the link state information. This structure will be  */
				/* a state variable and the TORA ULP will be able to access it to get the link    */
				/* information.																	  */
				link_state_information = (int**) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (int*));
				
				/* Now initialize the entire 2D array. Essentially link_state_information [X] [Y] */
				/* will indicate if the link is up, down or yet unknown.						  */
				for (outer_counter_index = 0; outer_counter_index <= global_router_id_index; outer_counter_index ++)
					{
					link_state_information [outer_counter_index] = (int*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (int));
					for (inner_counter_index = 0; inner_counter_index <= global_router_id_index; inner_counter_index ++)
						{
						link_state_information [outer_counter_index] [inner_counter_index] = ImepC_LinkState_Unknown;
						}
					}
				
				/* Initialize the RDN pointer*/
				manet_imep_rdn_initialize ();
				
				/* Initialize the buffer array here. Each element of the array is a list pointer. The elements of the*/
				/* list are of ImepT_Ulp_Data_Struct*/
				buffer_arrived_ulp_packets = (List**) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (List*));
				for (counter = 0; counter < global_router_id_index + 1; counter ++)
					buffer_arrived_ulp_packets [counter] = op_prg_list_create ();
				
				/* Create the aggregation buffer here */
				ulp_aggregation_buffer = op_prg_list_create ();
				
				/* Create the state variable to hold the structures for the animation purpose */
				router_information = (ImepT_Neighbor_Info_Struct*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (ImepT_Neighbor_Info_Struct));
				for (counter = 0; counter < global_router_id_index + 1; counter ++)
					{
					/* Initialize the variables at this point*/
					router_information [counter].x_pos = -1;
					router_information [counter].y_pos = -1;
					router_information [counter].node_object_id	= OPC_OBJID_INVALID;
					}
				
				/* Find the IP address on this node */
				/* Currently we have a single IP interface on each MANET router */
				tora_imep_sup_rid_to_ip_addr (router_id, &my_ip_address);
				
				/* Animation drawing ID store array */
				animation_drawing_ids = (Andid*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (Andid));
				duplicate_animation_drawing_ids = (Andid*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (Andid));
				for (counter = 0; counter < global_router_id_index + 1; counter ++)
					{
					animation_drawing_ids [counter] = OPC_ANIM_ID_NIL;
					duplicate_animation_drawing_ids [counter] = OPC_ANIM_ID_NIL;
					}
				
				/* Create the array to store the information regarding the MBT Timer Event Handle */
				/* Each element of the array will hold the event handle of the NEXT MBT event for the particular */
				/* neighbor router ID ... the index for the array element 						*/
				mbt_timer_array = (Evhandle*) op_prg_mem_alloc ((global_router_id_index + 1) * sizeof (Evhandle));
				
				/* Call the function to perform the animations if its necessary*/
				manet_imep_support_animation_imep_init ();
				}
				FSM_PROFILE_SECTION_OUT (state11_exit_exec)


			/** state (Init) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Init", "Idle", "tr_29", "manet_imep [Init -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Send_Echo) enter executives **/
			FSM_STATE_ENTER_FORCED (12, "Send_Echo", state12_enter_exec, "manet_imep [Send_Echo enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_imep [Send_Echo enter execs]", state12_enter_exec)
				{
				/* A beacon has arrived. We need to send out the Echo packet for the incoming beacon */
				/* Remember: The IMEP packet is in the state variable -- "incoming_imep_packet"      */
				/* when we enter this state.														 */
				
				/* Schedule a procedure call at this point for the same simulation time -- */
				/* the procedure will generate and send out the echo response. We do not   */
				/* to call the proc here because of circular invocation to the Tora_Imep_Mgr*/
				/* process that might result when we try to send out the echo packet.	   */
				
				/* The incoming packet will be destroyed right away - we do not need to send  */
				/* it to the echo_response function below.									  */
				op_pk_destroy (incoming_imep_packet);
				manet_imep_send_echo_response (sender_address);
				
				
				
				}
				FSM_PROFILE_SECTION_OUT (state12_enter_exec)

			/** state (Send_Echo) exit executives **/
			FSM_STATE_EXIT_FORCED (12, "Send_Echo", "manet_imep [Send_Echo exit execs]")


			/** state (Send_Echo) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "Send_Echo", "Idle", "tr_47", "manet_imep [Send_Echo -> Idle : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (11,"manet_imep")
		}
	}




void
_op_manet_imep_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_manet_imep_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		/* Variable definitions must be at the top of the block.	*/
		char									ip_address [32];
		IpT_Address 							next_hop_address;
		int 									counter;
		int* 									temp_rtr_id;
		
		/* Print the information regarding the current neighbors seen */
		printf ("\n\tIMEP Connection Information For Router: (Router ID = %d)", router_id);
		printf ("\n\t***********************************************************");
		printf ("\n\n\tThe following routers are currently established IMEP neighbors: \n\n");
		printf ("\n\tRouter ID\tIP Address\tDirection");
		printf ("\n\t*********\t**********\t*********");
		for (counter = 0; counter < op_prg_list_size (rdn_info->root_rdn_members_list); counter ++)
			{
			printf ("\n");
			temp_rtr_id = (int*) op_prg_list_access (rdn_info->root_rdn_members_list, counter);
			printf ("\t  %d", *temp_rtr_id);
			tora_imep_sup_rid_to_ip_addr (*temp_rtr_id, &next_hop_address);
			ip_address_print (ip_address, next_hop_address);
			printf ("\t\t%s", ip_address);
			
			if ((link_state_information [router_id][*temp_rtr_id] == ImepC_LinkState_Down)&&
				(link_state_information [*temp_rtr_id][router_id] == ImepC_LinkState_Down))
				printf ("\tError!");
			
			if ((link_state_information [router_id][*temp_rtr_id] == ImepC_LinkState_Up)&&
				(link_state_information [*temp_rtr_id][router_id] == ImepC_LinkState_Up))
				printf ("\tBi");
			
			if ((link_state_information [router_id][*temp_rtr_id] == ImepC_LinkState_Up)&&
				(link_state_information [*temp_rtr_id][router_id] == ImepC_LinkState_Down))
				printf ("\tTo");
			
			if ((link_state_information [router_id][*temp_rtr_id] == ImepC_LinkState_Down)&&
				(link_state_information [*temp_rtr_id][router_id] == ImepC_LinkState_Up))
				printf ("\tFrom");
			}
		
		printf ("\n\n\tFLAG STATUS:");
		printf ("\nclear_to_tx:%d, newcolor_object_required:%d, no_neighbor:%d, newcolor_transmission:%d, num_of_retry_left:%d",
			clear_to_tx, newcolor_object_required, no_neighbor, newcolor_transmission, num_of_retry_left);
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_manet_imep_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (_op_manet_imep_terminate ())

	if (1)
		{

		/* Termination Block */

		BINIT
		{
		/* Definitions must be at the top of the block.	*/
		List*									tmp_list;
		ImepT_Ulp_Data_Struct* 					temp_data_struct_element;
		Packet* 								pkptr;
		int 									counter;
		
		/* Free up the state variables that were dynamically allocated.*/
		op_prg_mem_free (link_state_information);
		
		/* ### Freeing up of the state variable rdn_info ? */
		
		/* Free up the aggregation buffer */
		for (;;)
			{
			if (ulp_aggregation_buffer == OPC_NIL)
				break;
			
			if (op_prg_list_size (ulp_aggregation_buffer) == 0)
				{
				op_prg_mem_free (ulp_aggregation_buffer);
				break;
				}
			
			pkptr = (Packet*) op_prg_list_remove (ulp_aggregation_buffer, 0);
			op_pk_destroy (pkptr);
			}
		
		/* Free up the buffer_arrived_ulp_packets*/
		for (counter = 0; counter < (global_router_id_index + 1); counter++)
			{
			tmp_list = (List *) buffer_arrived_ulp_packets [counter];
			if (tmp_list == OPC_NIL)
				continue;
			
			/* Otherwise free up the whole list*/
			for (;;)
				{
				temp_data_struct_element = (ImepT_Ulp_Data_Struct *) op_prg_list_remove (buffer_arrived_ulp_packets [counter], OPC_LISTPOS_TAIL);
				if (temp_data_struct_element != OPC_NIL)
					{
					op_prg_list_free (temp_data_struct_element->acknowledgement_list);
					op_prg_mem_free (temp_data_struct_element->acknowledgement_list);
					op_prg_mem_free (temp_data_struct_element);			
					}
				else
					break;
				}
			
			op_prg_mem_free (tmp_list);
			}
		
		op_prg_mem_free (buffer_arrived_ulp_packets);
		
		}

		/* End of Termination Block */

		}
	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_manet_imep_svar function. */
#undef router_id
#undef beacon_period
#undef max_beacon_time
#undef max_retries
#undef beacon_tx_evhndl
#undef sv_node_objid
#undef total_num_ip_intf
#undef link_state_information
#undef rdn_info
#undef tora_imep_compound_attr
#undef tora_my_params_attr_id
#undef module_data_ptr
#undef incoming_imep_packet
#undef ulp_aggregation_buffer
#undef buffer_arrived_ulp_packets
#undef router_information
#undef animation_drawing_ids
#undef imep_log_protocol_handle
#undef imep_log_configuration_handle
#undef imep_control_traffic_sent_bps_stathandle
#undef imep_control_traffic_sent_pks_stathandle
#undef imep_control_traffic_received_bps_stathandle
#undef imep_control_traffic_received_pks_stathandle
#undef aggregation_period
#undef imep_pk_size
#undef retransmission_interval
#undef imep_number_of_neighbors_stathandle
#undef tora_imep_mgr_prohandle
#undef imep_ulp_traffic_sent_bps_stathandle
#undef imep_ulp_traffic_sent_pks_stathandle
#undef imep_ulp_traffic_received_bps_stathandle
#undef imep_ulp_traffic_received_pks_stathandle
#undef imep_retransmissions_stathandle
#undef imep_route_injection
#undef my_ip_address
#undef duplicate_animation_drawing_ids
#undef animation_update_time
#undef global_imep_control_traffic_sent_bps_stathandle
#undef global_imep_control_traffic_sent_pks_stathandle
#undef global_imep_control_traffic_received_bps_stathandle
#undef global_imep_control_traffic_received_pks_stathandle
#undef global_imep_ulp_traffic_sent_bps_stathandle
#undef global_imep_ulp_traffic_sent_pks_stathandle
#undef global_imep_ulp_traffic_received_bps_stathandle
#undef global_imep_ulp_traffic_received_pks_stathandle
#undef mbt_timer_array
#undef node_level_route_injection
#undef newcolor_transmission_evhandle
#undef retransmission_evhandle
#undef next_beacon_event
#undef clear_to_tx
#undef no_neighbor
#undef aggregation_timer_evhandle
#undef data_to_send
#undef num_of_retry_left
#undef yet_to_ack_lptr
#undef retransmit_pkptr
#undef newcolor_object_required
#undef newcolor_transmission
#undef node_name
#undef mcast_port_index
#undef stream_from_ip_encap

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_manet_imep_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_manet_imep_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (manet_imep)",
		sizeof (manet_imep_state));
	*init_block_ptr = 22;

	FRET (obtype)
	}

VosT_Address
_op_manet_imep_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	manet_imep_state * ptr;
	FIN_MT (_op_manet_imep_alloc (obtype))

	ptr = (manet_imep_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "manet_imep [Init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_manet_imep_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	manet_imep_state		*prs_ptr;

	FIN_MT (_op_manet_imep_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (manet_imep_state *)gen_ptr;

	if (strcmp ("router_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->router_id);
		FOUT
		}
	if (strcmp ("beacon_period" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->beacon_period);
		FOUT
		}
	if (strcmp ("max_beacon_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->max_beacon_time);
		FOUT
		}
	if (strcmp ("max_retries" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->max_retries);
		FOUT
		}
	if (strcmp ("beacon_tx_evhndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->beacon_tx_evhndl);
		FOUT
		}
	if (strcmp ("sv_node_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sv_node_objid);
		FOUT
		}
	if (strcmp ("total_num_ip_intf" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->total_num_ip_intf);
		FOUT
		}
	if (strcmp ("link_state_information" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->link_state_information);
		FOUT
		}
	if (strcmp ("rdn_info" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rdn_info);
		FOUT
		}
	if (strcmp ("tora_imep_compound_attr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tora_imep_compound_attr);
		FOUT
		}
	if (strcmp ("tora_my_params_attr_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tora_my_params_attr_id);
		FOUT
		}
	if (strcmp ("module_data_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->module_data_ptr);
		FOUT
		}
	if (strcmp ("incoming_imep_packet" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->incoming_imep_packet);
		FOUT
		}
	if (strcmp ("ulp_aggregation_buffer" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ulp_aggregation_buffer);
		FOUT
		}
	if (strcmp ("buffer_arrived_ulp_packets" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->buffer_arrived_ulp_packets);
		FOUT
		}
	if (strcmp ("router_information" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->router_information);
		FOUT
		}
	if (strcmp ("animation_drawing_ids" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->animation_drawing_ids);
		FOUT
		}
	if (strcmp ("imep_log_protocol_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_log_protocol_handle);
		FOUT
		}
	if (strcmp ("imep_log_configuration_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_log_configuration_handle);
		FOUT
		}
	if (strcmp ("imep_control_traffic_sent_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_control_traffic_sent_bps_stathandle);
		FOUT
		}
	if (strcmp ("imep_control_traffic_sent_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_control_traffic_sent_pks_stathandle);
		FOUT
		}
	if (strcmp ("imep_control_traffic_received_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_control_traffic_received_bps_stathandle);
		FOUT
		}
	if (strcmp ("imep_control_traffic_received_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_control_traffic_received_pks_stathandle);
		FOUT
		}
	if (strcmp ("aggregation_period" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->aggregation_period);
		FOUT
		}
	if (strcmp ("imep_pk_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_pk_size);
		FOUT
		}
	if (strcmp ("retransmission_interval" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retransmission_interval);
		FOUT
		}
	if (strcmp ("imep_number_of_neighbors_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_number_of_neighbors_stathandle);
		FOUT
		}
	if (strcmp ("tora_imep_mgr_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tora_imep_mgr_prohandle);
		FOUT
		}
	if (strcmp ("imep_ulp_traffic_sent_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_ulp_traffic_sent_bps_stathandle);
		FOUT
		}
	if (strcmp ("imep_ulp_traffic_sent_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_ulp_traffic_sent_pks_stathandle);
		FOUT
		}
	if (strcmp ("imep_ulp_traffic_received_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_ulp_traffic_received_bps_stathandle);
		FOUT
		}
	if (strcmp ("imep_ulp_traffic_received_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_ulp_traffic_received_pks_stathandle);
		FOUT
		}
	if (strcmp ("imep_retransmissions_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_retransmissions_stathandle);
		FOUT
		}
	if (strcmp ("imep_route_injection" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_route_injection);
		FOUT
		}
	if (strcmp ("my_ip_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_ip_address);
		FOUT
		}
	if (strcmp ("duplicate_animation_drawing_ids" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->duplicate_animation_drawing_ids);
		FOUT
		}
	if (strcmp ("animation_update_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->animation_update_time);
		FOUT
		}
	if (strcmp ("global_imep_control_traffic_sent_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_control_traffic_sent_bps_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_control_traffic_sent_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_control_traffic_sent_pks_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_control_traffic_received_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_control_traffic_received_bps_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_control_traffic_received_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_control_traffic_received_pks_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_ulp_traffic_sent_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_ulp_traffic_sent_bps_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_ulp_traffic_sent_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_ulp_traffic_sent_pks_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_ulp_traffic_received_bps_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_ulp_traffic_received_bps_stathandle);
		FOUT
		}
	if (strcmp ("global_imep_ulp_traffic_received_pks_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_imep_ulp_traffic_received_pks_stathandle);
		FOUT
		}
	if (strcmp ("mbt_timer_array" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mbt_timer_array);
		FOUT
		}
	if (strcmp ("node_level_route_injection" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_level_route_injection);
		FOUT
		}
	if (strcmp ("newcolor_transmission_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->newcolor_transmission_evhandle);
		FOUT
		}
	if (strcmp ("retransmission_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retransmission_evhandle);
		FOUT
		}
	if (strcmp ("next_beacon_event" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->next_beacon_event);
		FOUT
		}
	if (strcmp ("clear_to_tx" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->clear_to_tx);
		FOUT
		}
	if (strcmp ("no_neighbor" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->no_neighbor);
		FOUT
		}
	if (strcmp ("aggregation_timer_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->aggregation_timer_evhandle);
		FOUT
		}
	if (strcmp ("data_to_send" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_to_send);
		FOUT
		}
	if (strcmp ("num_of_retry_left" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_of_retry_left);
		FOUT
		}
	if (strcmp ("yet_to_ack_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->yet_to_ack_lptr);
		FOUT
		}
	if (strcmp ("retransmit_pkptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retransmit_pkptr);
		FOUT
		}
	if (strcmp ("newcolor_object_required" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->newcolor_object_required);
		FOUT
		}
	if (strcmp ("newcolor_transmission" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->newcolor_transmission);
		FOUT
		}
	if (strcmp ("node_name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->node_name);
		FOUT
		}
	if (strcmp ("mcast_port_index" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mcast_port_index);
		FOUT
		}
	if (strcmp ("stream_from_ip_encap" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->stream_from_ip_encap);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

