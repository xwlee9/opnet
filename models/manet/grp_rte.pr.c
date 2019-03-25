/* Process model C form file: grp_rte.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char grp_rte_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EED9 5C91EED9 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/* Includes	*/
#include <math.h>
#include <ip_addr_v4.h>
#include <ip_rte_support.h>
#include <ip_support.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <ip_higher_layer_proto_reg_sup.h>
#include <manet.h>
#include <grp.h>
#include <grp_pkt_support.h>
#include <grp_ptypes.h>
#include <oms_dist_support.h>


#define PKT_ARRIVAL  			((invoke_mode == OPC_PROINV_INDIRECT) && (info_ptr->code == MANETC_PACKET_ARRIVAL))
#define HELLO_BROADCAST			((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == GRPC_HELLO_BROADCAST))
#define POSITION_UPDATE			((invoke_mode == OPC_PROINV_INDIRECT) && (info_ptr->code == MANETC_NODE_MOVEMENT))
#define POS_REQ_EXPIRY			((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == GRPC_POS_REQ_EXPIRY))
#define	FLOOD_COMPLETE			(status == GrpC_Flooding_Complete)
#define HELLO_COMPLETE			(status == GrpC_Hello_Complete)

/* Traces	*/
#define LTRACE_ACTIVE			(op_prg_odb_ltrace_active ("grp_rte") || \
								op_prg_odb_ltrace_active ("manet"))

#define LTRACE_HELLO			(op_prg_odb_ltrace_active ("grp_hello") || \
								op_prg_odb_ltrace_active ("manet"))

#define LTRACE_FLOODING			(op_prg_odb_ltrace_active ("grp_flood") || \
								op_prg_odb_ltrace_active ("manet"))

/* Function prototypes	*/
static void						grp_rte_sv_init (void);
static void						grp_rte_attributes_parse (void);
static void						grp_rte_local_stats_init (void);
static void						grp_rte_pkt_arrival_handle (ManetT_Info*);
static void						grp_rte_app_pkt_arrival_handle (Packet* ip_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr);
static void						grp_rte_hello_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr);
static void						grp_rte_flood_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr);
static void						grp_rte_data_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr);
static void						grp_rte_backtrack_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr);
static void						grp_rte_pos_req_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr);
static void						grp_rte_pos_resp_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr);
static void						grp_rte_hello_request_send (void);
static void						grp_rte_hello_response_send (IpT_Dgram_Fields* ip_dgram_fd_ptr);
static void						grp_rte_flooding_message_send (double lat_m, double long_m, double flood_low_lat_m, double flood_low_long_m, double flood_high_lat_m, double flood_high_long_m);
static void						grp_rte_position_request_message_send (List* dest_lptr, double low_lat, double low_long, double high_lat, double high_long);
static void						grp_rte_backtrack (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr);
static void						grp_rte_backtrack_table_update (void* nbr_addr_ptr, int code);
static void						grp_rte_hello_schedule (void);
static void						grp_rte_node_pos_in_metres_get (double* lat_dist_m, double* long_dist_m);
static void 					grp_rte_neighborhood_determine (double lat_dist_m, double long_dist_m, double quadrant_size, double* low_lat_m, double* low_long_m, double* high_lat_m, double* high_long_m);
static void						grp_rte_neighborhood_centre_determine (double lat_dist_m, double long_dist_m, double quadrant_size, double* quad_lat_m, double* quad_long_m);
static void						grp_rte_neighborhood_to_flood_determine (double old_lat_m, double old_long_m, double new_lat_m, double new_long_m,	double* flood_low_lat_m, double* flood_low_long_m, double* flood_high_lat_m,	double* flood_high_long_m);
static int						grp_rte_common_quadrant_determine (double old_lat_m, double old_long_m, double new_lat_m, double new_long_m);
static double					grp_rte_highest_neighbor_quadrant_determine (double dest_lat_m, double dest_long_m, double* quad_lat_m_ptr, double* quad_long_m_ptr);
static Packet*					grp_rte_ip_datagram_decapsulate (Packet* ip_pkptr, Packet* grp_pkptr);
static Packet*					grp_rte_ip_datagram_create (Packet* grp_pkptr, InetT_Address dest_addr, InetT_Address next_hop_addr);
static Packet*					grp_rte_ip_datagram_encapsulate (Packet* ip_pkptr, Packet* grp_pkptr, InetT_Address next_hop_addr);
static ManetT_Nexthop_Info*		grp_rte_next_hop_info_create (InetT_Address next_hop_addr);
static void						grp_rte_position_request_timer_schedule (void);
static void						grp_rte_address_list_clear (List* addr_lptr);
static GrpT_Backtrack_Info*		grp_rte_backtrack_info_mem_alloc (void);
static void						grp_rte_backtrack_info_mem_free (GrpT_Backtrack_Info* backtrack_ptr);
static void						grp_rte_error (char* str1, char* str2, char* str3);

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
	PrgT_String_Hash_Table*			neighbor_table                                  ;	/* This table holds the neighbors to this node indexed by the IP */
	                        		                                                	/* address                                                       */
	PrgT_String_Hash_Table*			destination_table                               ;	/* This table holds the destination nodes known to this node indexed */
	                        		                                                	/* by the IP address                                                 */
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;	/* Module Data Memory for the IP module */
	Objid	                  		own_mod_objid                                   ;	/* Objid of this module */
	Objid	                  		own_node_objid                                  ;	/* Objid of this node */
	Prohandle	              		own_prohandle                                   ;	/* Own process handle */
	Prohandle	              		parent_prohandle                                ;	/* Process handle of the parent process (manet_mgr) */
	int	                    		own_pro_id                                      ;	/* Own process ID */
	int	                    		parent_pro_id                                   ;	/* Parent process ID */
	char	                   		pid_string [32]                                 ;	/* Display string */
	IpT_Rte_Proc_Id	        		grp_proto_id                                    ;	/* Unique protocol ID returned by the IP common route table */
	InetT_Subnet_Mask	      		subnet_mask                                     ;	/* length 32 subnet mask for entries in the route table that are */
	                        		                                                	/* length 32 bits                                                */
	InetT_Subnet_Mask	      		subnet_mask_ipv6                                ;	/* length 128 subnet mask for entries in the route table that are */
	                        		                                                	/* length 128 bits.                                               */
	double	                 		nbr_expiry_time                                 ;	/* Time after which a neighbor reachability has expired */
	double	                 		dist_moved                                      ;	/* Distance moved for transmission of position update */
	double	                 		neighborhood_size                               ;	/* Size of each quadrant at the lowest level in the hierarchy */
	int	                    		higher_layer_proto_id                           ;	/* Registered ID to IP for the GRP as a higher layer protocol */
	double	                 		quad_low_lat                                    ;	/* Low latitude of the current quadrant (neighborhood) the node */
	                        		                                                	/* belongs to                                                   */
	double	                 		quad_low_long                                   ;	/* Low longitude of the current quadrant (neighborhood) the node */
	                        		                                                	/* belongs to                                                    */
	double	                 		quad_high_lat                                   ;	/* High latitude of the current quadrant (neighborhood) the node */
	                        		                                                	/* belongs to                                                    */
	double	                 		quad_high_long                                  ;	/* High longitude of the current quadrant (neighborhood) the node */
	                        		                                                	/* belongs to                                                     */
	double	                 		node_lat_m                                      ;	/* Last know position of the node's latitude in metres */
	double	                 		node_long_m                                     ;	/* Last known position of the node's longitude in metres */
	double	                 		start_lat_m                                     ;	/* Starting latitude position of the node before calculation of  */
	                        		                                                	/* whether the node has moved greater than the distance to flood */
	                        		                                                	/* its new position to all nodes in the quadrant                 */
	double	                 		start_long_m                                    ;	/* Starting longitude position of the node before calculation of */
	                        		                                                	/* whether the node has moved greater than the distance to flood */
	                        		                                                	/* its new position to all nodes in the quadrant                 */
	Evhandle	               		pos_req_timer_evhandle                          ;	/* Eventhandle for the position request timer that waits for the */
	                        		                                                	/* position response message to arrive from the neighboring node */
	List*	                  		nbr_node_attempted_lptr                         ;	/* Keep a list of neighbor nodes that this node has attempted to */
	                        		                                                	/* request the position information but has failed               */
	double	                 		pos_req_timer                                   ;	/* The position request time beyond which the node will retry a new */
	                        		                                                	/* position request                                                 */
	GrpT_Local_Stathandles*			local_stat_handle_ptr                           ;	/* Structure that contains the local statistic handles */
	GrpT_Global_Stathandles*			global_stat_handle_ptr                          ;	/* Structure that contains the global statistic handles */
	Boolean	                		global_routes_export                            ;	/* Global attribute that specifies to export routes taken by all packets */
	                        		                                                	/* to an output table.                                                   */
	Boolean	                		node_routes_export                              ;	/* Node attribute that specifies to export routes of all packets */
	                        		                                                	/* originating at this node                                      */
	OmsT_Dist_Handle	       		hello_interval_dist_ptr                         ;	/* Interval between which Hello messages need to be transmitted */
	GrpT_Process_Status	    		status                                          ;	/* Status of the process model */
	PrgT_String_Hash_Table*			backtrack_table                                 ;	/* Table of neighbors that cannot be considered when trying to find */
	                        		                                                	/* a new route to the destination due to backtracking               */
	Boolean	                		backtrack_option                                ;	/* Specifies whether this node can backtrack */
	Boolean	                		global_record_routes                            ;	/* Flag that indicates whether the record routes option has been enabled */
	double	                 		single_lat_dist_m                               ;	/* Distance in metres of 1 latitude degree */
	double	                 		single_long_dist_m                              ;	/* Distance in metres of 1 longitude degree */
	double	                 		backtrack_hold_time                             ;	/* Time to hold a next hop node in the backtrack table so that new */
	                        		                                                	/* incoming packets do not consider the node when calculating the  */
	                        		                                                	/* shortest route to the destination                               */
	int	                    		num_init_floods                                 ;	/* Number of initial floods completed */
	int	                    		num_init_flood_attempts                         ;	/* Number of initial flooding attempts configured in the attributes */
	} grp_rte_state;

#define neighbor_table          		op_sv_ptr->neighbor_table
#define destination_table       		op_sv_ptr->destination_table
#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define own_mod_objid           		op_sv_ptr->own_mod_objid
#define own_node_objid          		op_sv_ptr->own_node_objid
#define own_prohandle           		op_sv_ptr->own_prohandle
#define parent_prohandle        		op_sv_ptr->parent_prohandle
#define own_pro_id              		op_sv_ptr->own_pro_id
#define parent_pro_id           		op_sv_ptr->parent_pro_id
#define pid_string              		op_sv_ptr->pid_string
#define grp_proto_id            		op_sv_ptr->grp_proto_id
#define subnet_mask             		op_sv_ptr->subnet_mask
#define subnet_mask_ipv6        		op_sv_ptr->subnet_mask_ipv6
#define nbr_expiry_time         		op_sv_ptr->nbr_expiry_time
#define dist_moved              		op_sv_ptr->dist_moved
#define neighborhood_size       		op_sv_ptr->neighborhood_size
#define higher_layer_proto_id   		op_sv_ptr->higher_layer_proto_id
#define quad_low_lat            		op_sv_ptr->quad_low_lat
#define quad_low_long           		op_sv_ptr->quad_low_long
#define quad_high_lat           		op_sv_ptr->quad_high_lat
#define quad_high_long          		op_sv_ptr->quad_high_long
#define node_lat_m              		op_sv_ptr->node_lat_m
#define node_long_m             		op_sv_ptr->node_long_m
#define start_lat_m             		op_sv_ptr->start_lat_m
#define start_long_m            		op_sv_ptr->start_long_m
#define pos_req_timer_evhandle  		op_sv_ptr->pos_req_timer_evhandle
#define nbr_node_attempted_lptr 		op_sv_ptr->nbr_node_attempted_lptr
#define pos_req_timer           		op_sv_ptr->pos_req_timer
#define local_stat_handle_ptr   		op_sv_ptr->local_stat_handle_ptr
#define global_stat_handle_ptr  		op_sv_ptr->global_stat_handle_ptr
#define global_routes_export    		op_sv_ptr->global_routes_export
#define node_routes_export      		op_sv_ptr->node_routes_export
#define hello_interval_dist_ptr 		op_sv_ptr->hello_interval_dist_ptr
#define status                  		op_sv_ptr->status
#define backtrack_table         		op_sv_ptr->backtrack_table
#define backtrack_option        		op_sv_ptr->backtrack_option
#define global_record_routes    		op_sv_ptr->global_record_routes
#define single_lat_dist_m       		op_sv_ptr->single_lat_dist_m
#define single_long_dist_m      		op_sv_ptr->single_long_dist_m
#define backtrack_hold_time     		op_sv_ptr->backtrack_hold_time
#define num_init_floods         		op_sv_ptr->num_init_floods
#define num_init_flood_attempts 		op_sv_ptr->num_init_flood_attempts

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	grp_rte_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((grp_rte_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
grp_rte_sv_init (void)
	{
	/** Initialize the state variables	**/
	FIN (grp_rte_sv_init (void));
	
	/* Access the module data memory	*/
	module_data_ptr = (IpT_Rte_Module_Data*) op_pro_modmem_access ();
	
	/* Obtain own module ID	*/
	own_mod_objid = op_id_self ();

	/* Obtain the node's objid. */
	own_node_objid = op_topo_parent (own_mod_objid);

	/* Obtain own process handle. */
	own_prohandle = op_pro_self ();
	
	/* Obtain parent process handle	*/
	parent_prohandle = op_pro_parent (own_prohandle);
	
	/* Own process ID	*/
	own_pro_id = op_pro_id (own_prohandle);
	
	/* Parent process ID	*/
	parent_pro_id = op_pro_id (parent_prohandle);
	
	/* Set up the display string. */
	sprintf (pid_string, "grp_rte PID (%d)", own_pro_id);
	
	/* Create the neighbor table that holds the list of neighbor nodes	*/
	neighbor_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);

	/* Create the destination table that holds the list of destinations	*/
	/* known to this node.												*/
	destination_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	
	/* Create the backtrack table that holds the neighbor nodes that	*/
	/* cannot be considered during route computation.					*/
	backtrack_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	
	/* Create a /32 subnet mask for entries in the route table	*/
	subnet_mask = inet_smask_from_length_create (32);

	/* Create a /128 subnet mask for entries in the route table	*/
	subnet_mask_ipv6 = inet_smask_from_length_create (128);
	
	/* Get the global attribute neighborhood size	*/
	neighborhood_size = grp_support_nbrhood_size_sim_attr_get ();
	
	/* Get the global attribute routes export	*/
	global_routes_export = grp_support_routes_export_sim_attr_get ();
	
	/* Get the global record routes attribute	*/
	global_record_routes = grp_support_record_routes_sim_attr_get ();
	
	/* Keep a list of neighbor nodes that this	*/
	/* node has attempted to request the 		*/
	/* position information but has failed      */
	nbr_node_attempted_lptr = op_prg_list_create ();
	
	/* Initialize the number of initial floods completed	*/
	num_init_floods = 0;
	
	/* Initialize the global statistic handles	*/
	global_stat_handle_ptr = grp_support_global_stat_handles_obtain ();
	
	/* Get the distance of one degree in metres for lat and long	*/
	single_lat_dist_m = prg_geo_lat_long_distance_get (-90.0, 0.0, 0.0, -89.0, 0.0, 0.0);
	single_long_dist_m = prg_geo_lat_long_distance_get (0.0, -180.0, 0.0, 0.0, -179.0, 0.0);
	
	FOUT;
	}

static void
grp_rte_attributes_parse (void)
	{
	Objid					grp_parms_id;
	Objid					grp_parms_child_id;
	Objid					pos_update_parms_id;
	Objid					pos_update_parms_child_id;
	char					hello_interval_str [128];
	char					nbr_interval_str [128];
	OmsT_Dist_Handle		nbr_expiry_dist_ptr;
	
	/** This function reads in the attributes **/
	FIN (grp_rte_attributes_parse (void));
	
	/* Read the GRP Parameters	*/
	op_ima_obj_attr_get (own_mod_objid, "manet_mgr.GRP Parameters", &grp_parms_id);
	grp_parms_child_id = op_topo_child (grp_parms_id, OPC_OBJTYPE_GENERIC, 0);
	
	/* Read in the Hello Interval	*/
	op_ima_obj_attr_get (grp_parms_child_id, "Hello Interval", &hello_interval_str);
	
	/* Read in the neighbor expiry time	*/
	op_ima_obj_attr_get (grp_parms_child_id, "Neighbor Expiry Time", &nbr_interval_str);
	
	/* Read in the routes export variable	*/
	op_ima_obj_attr_get (grp_parms_child_id, "Routes Export", &node_routes_export);
	
	/* Read if the backtrack option is enabled	*/
	op_ima_obj_attr_get (grp_parms_child_id, "Backtrack Option", &backtrack_option);
	
	/* Read in the number of initial flooding attempts */
	op_ima_obj_attr_get (grp_parms_child_id, "Number of Initial Floods", &num_init_flood_attempts);
	
	/* Read in the position update parameters	*/
	op_ima_obj_attr_get (grp_parms_child_id, "Position Update Parameters", &pos_update_parms_id);
	pos_update_parms_child_id = op_topo_child (pos_update_parms_id, OPC_OBJTYPE_GENERIC, 0);
	
	op_ima_obj_attr_get (pos_update_parms_child_id, "Distance moved", &dist_moved);
	op_ima_obj_attr_get (pos_update_parms_child_id, "Position Request Timer", &pos_req_timer);
	
	/* Create the Hello interval distribution	*/
	hello_interval_dist_ptr = oms_dist_load_from_string (hello_interval_str);
	backtrack_hold_time = oms_dist_outcome (hello_interval_dist_ptr);
	
	/* Create the neighbor expiry interval distribution	*/
	nbr_expiry_dist_ptr = oms_dist_load_from_string (nbr_interval_str);
	nbr_expiry_time = oms_dist_outcome (nbr_expiry_dist_ptr);
	
	if ((node_routes_export == OPC_TRUE) || (global_routes_export == OPC_TRUE))
		{
		/* Initialize the OT package	*/
		grp_support_route_print_to_ot_initialize ();
	
		/* Schedule a call interrupt to close the file	*/
		op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, 0, grp_support_route_print_to_ot_close, OPC_NIL);
		}
	
	FOUT;
	}


static void
grp_rte_local_stats_init (void)
	{
	/** Initializes the local statistic handles	**/
	FIN (grp_rte_local_stats_init (void));
	
	local_stat_handle_ptr = (GrpT_Local_Stathandles*) op_prg_mem_alloc (sizeof (GrpT_Local_Stathandles));
	local_stat_handle_ptr->pkts_discard_total_bps_shandle = op_stat_reg ("GRP.Packets Dropped (Total) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->pkts_discard_ttl_bps_shandle = op_stat_reg ("GRP.Packets Dropped (TTL expiry) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->pkts_discard_no_nbr_bps_shandle = op_stat_reg ("GRP.Packets Dropped (No available neighbor) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->pkts_discard_no_dest_bps_shandle = op_stat_reg ("GRP.Packets Dropped (Destination unknown) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_rcvd_bps_shandle = op_stat_reg ("GRP.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_rcvd_pps_shandle = op_stat_reg ("GRP.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_sent_bps_shandle = op_stat_reg ("GRP.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_sent_pps_shandle = op_stat_reg ("GRP.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_traf_rcvd_bps_shandle = op_stat_reg ("GRP.Total Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_traf_rcvd_pps_shandle = op_stat_reg ("GRP.Total Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_traf_sent_bps_shandle = op_stat_reg ("GRP.Total Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_traf_sent_pps_shandle = op_stat_reg ("GRP.Total Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_num_backtracks_shandle = op_stat_reg ("GRP.Total Number of Backtracks", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_num_quadrant_change_shandle = op_stat_reg ("GRP.Total Number of Quadrant Changes", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	FOUT;
	}


static void
grp_rte_pkt_arrival_handle (ManetT_Info* manet_ptr)
	{
	Packet*						ip_pkptr;
	Packet*						grp_pkptr;
	IpT_Rte_Ind_Ici_Fields*		intf_ici_fdstruct_ptr;
	IpT_Dgram_Fields* 			ip_dgram_fd_ptr;
	GrpT_Packet_Option*			tlv_options_ptr;
	OpT_Packet_Size				total_size;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];

	/** A packet has arrived. Handle the packet	**/
	/** appropriately based on its type			**/
	FIN (grp_rte_pkt_arrival_handle (void));
	
	/* The process was invoked by the parent	*/
	/* MANET process indicating the arrival		*/
	/* of a packet. The packet can either be	*/
	/* 1. A higher layer application packet		*/
	/*    waiting to be transmitted when a 		*/
	/*    route is found.						*/
	/* 2. A MANET signaling/routing packet 		*/
	/*    arrival which may or may not be a		*/
	/*    broadcast packet.						*/
	
	/* Get a handle to the IP packet	*/
	ip_pkptr = (Packet*) manet_ptr->manet_info_type_ptr;
	
	if (ip_pkptr == OPC_NIL)
		grp_rte_error ("Could not obtain the packet from the argument memory", OPC_NIL, OPC_NIL);
	
	/* Get the total size of the packet	*/
	total_size = op_pk_total_size_get (ip_pkptr);

	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* Check if the packet came from the application	*/
	if ((intf_ici_fdstruct_ptr->instrm == module_data_ptr->instrm_from_ip_encap) ||
		(intf_ici_fdstruct_ptr->instrm == IpC_Pk_Instrm_Child))
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "An application packet has arrived from the higher layer to destination %s (%s).", addr_str, node_name);
			op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
			}
		
		/* This is a packet that came from the	*/
		/* application (higher) layer and no 	*/
		/* route exists to the destintion of	*/
		/* this packet. Handle the arrival		*/
		grp_rte_app_pkt_arrival_handle (ip_pkptr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
		
		FOUT;
		}
	
	/* Get the GRP packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &grp_pkptr);
	
	/* Get the option in the GRP packet	*/
	op_pk_nfd_access (grp_pkptr, "Options", &tlv_options_ptr);
	
	/* This is either an GRP control packet or a data 		*/
	/* packet from the lower layer. This node should not	*/
	/* receive the packet it transmits except backtrack 	*/
	/* packets 												*/
	if ((tlv_options_ptr->option_type != GRPC_BACKTRACK_OPTION) && 
		(manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->src_addr) == OPC_TRUE))
		{
		/* This node received its own packet. Destroy it	*/
		op_pk_destroy (grp_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* This is a GRP signaling packet. Based on the		*/
	/* type of option set in the packet, handle it		*/
	switch (tlv_options_ptr->option_type)
		{
		case (GRPC_HELLO_OPTION):
			{
			/* This is a GRP Hello packet	*/
			
			/* Update the received routing traffic statistic	*/
			grp_support_routing_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			/* Handle the arrival of the Hello packet	*/
			grp_rte_hello_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (GRPC_FLOODING_OPTION):
			{
			/* This is a flooding packet	*/
			
			/* Update the received routing traffic statistic	*/
			grp_support_routing_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			/* Handle the arrival of the Flooding packet	*/
			grp_rte_flood_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (GRPC_DATA_OPTION):
			{
			/* This is a data packet	*/
			
			/* Update the total traffic received statistic	*/
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			/* Handle the arrival of the Data packet	*/
			grp_rte_data_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			
			break;
			}
			
		case (GRPC_POS_REQ_OPTION):
			{
			/* This is a position request option	*/
			
			/* Update the received routing traffic statistic	*/
			grp_support_routing_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			/* Handle the arrival of the Position Request packet	*/
			grp_rte_pos_req_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (GRPC_POS_RESP_OPTION):
			{
			/* This is a position response option	*/
			
			/* Update the received routing traffic statistic	*/
			grp_support_routing_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			/* Handle the arrival of the Position Response packet	*/
			grp_rte_pos_resp_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (GRPC_BACKTRACK_OPTION):
			{
			/* This is the backtrack option received	*/
			
			/* Update the total traffic received statistic	*/
			grp_support_total_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, total_size);
			
			grp_rte_backtrack_pkt_arrival_handle (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			
			break;
			}
			
		default:
			{
			/* Invalid option in packet	*/
			grp_rte_error ("Invalid Option Type in GRP packet", OPC_NIL, OPC_NIL);
			}
		}
	
	FOUT;
	}


static void
grp_rte_app_pkt_arrival_handle (Packet* ip_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	InetT_Address			next_hop_addr;
	InetT_Address			src_address;
	GrpT_Dest_Info*			dest_ptr;
	GrpT_Packet_Option*		grp_tlv_ptr;
	Packet*					grp_pkptr;
	Packet*					app_pkptr;
	OpT_Packet_Size			pkt_size;
	short					output_intf_index;
	IpT_Interface_Info*		iface_info_ptr;
	int						outstrm;
	InetT_Address_Range* 	inet_addr_range_ptr;
	double					new_dist_to_dest;
	char					addr_str [INETC_ADDR_STR_LEN];
	char					node_name [OMSC_HNAME_MAX_LEN];
	char					temp_str [256];
	char					temp_str1 [256];
	
	/** This function handles the arrival of an	**/
	/** data application packet from the higher	**/
	/** layer to be transmitted to the dest		**/
	FIN (grp_rte_app_pkt_arrival_handle (<args>));
	
	/* Check if the destination exists in the destination table	*/
	dest_ptr = grp_dest_table_entry_exists (destination_table, ip_dgram_fd_ptr->dest_addr);
	
	if ((dest_ptr == OPC_NIL) || (dest_ptr->destination_state == GrpC_Destination_Invalid))
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "No route entry exists for the destination %s (%s)", addr_str, node_name);
			op_prg_odb_print_major (pid_string, temp_str, "The application packet will be dropped.", OPC_NIL);
			}
		
		/* The destination is not known to this node	*/
		/* Destroy the application packet.				*/
		op_pk_nfd_get (ip_pkptr, "data", &app_pkptr);
		pkt_size = op_pk_total_size_get (app_pkptr);
		grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Dest_Drop);
		op_pk_nfd_set (ip_pkptr, "data", app_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* The destination is known to this node. Find	*/
	/* the neighbor node that is closest in 		*/
	/* distance to the destination node.			*/
	next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, OPC_NIL, dest_ptr, -1.0, &new_dist_to_dest);
	
	if (inet_address_equal (next_hop_addr, InetI_Invalid_Addr))
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "No neighbor node found for destination %s (%s)", addr_str, node_name);
			op_prg_odb_print_major (pid_string, temp_str, "The application packet will be dropped.", OPC_NIL);
			}
		
		/* No next hop has been found. Destroy the	*/
		/* application packet.						*/
		op_pk_nfd_get (ip_pkptr, "data", &app_pkptr);
		pkt_size = op_pk_total_size_get (app_pkptr);
		grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Nbr_Drop);
		op_pk_nfd_set (ip_pkptr, "data", app_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &output_intf_index,
			&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
		{
		src_address = inet_address_range_addr_get (inet_addr_range_ptr);
		}
	
	/* Create the GRP TLV options	*/
	grp_tlv_ptr = grp_pkt_support_data_tlv_create (ip_dgram_fd_ptr->dest_addr, new_dist_to_dest, dest_ptr->destination_type, 
													dest_ptr->dest_lat, dest_ptr->dest_long, dest_ptr->quadrant_size, 
													dest_ptr->timestamp, src_address);
	
	/* Create the GRP packet	*/
	grp_pkptr = grp_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
	/* Set the data TLV option in the GRP packet	*/
	grp_pkt_support_option_set (grp_pkptr, grp_tlv_ptr);
	
	/* Set the export route status in the packet	*/
	op_pk_nfd_set (grp_pkptr, "route_export", node_routes_export);
	
	/* Encapsulate the IP datagram in a GRP packet	*/
	ip_pkptr = grp_rte_ip_datagram_encapsulate (ip_pkptr, grp_pkptr, next_hop_addr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
		sprintf (temp_str, "A route entry exists for the destination %s (%s)", addr_str, node_name);
		inet_address_print (addr_str, next_hop_addr);
		inet_address_to_hname (next_hop_addr, node_name);
		sprintf (temp_str1, "Sending the application packet to the next hop %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, temp_str, temp_str1, OPC_NIL);
		}
	
	/* Update the statistic for total traffic sent	*/
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, op_pk_total_size_get (ip_pkptr));
	
	/* Send the packet to the next hop towards the destination	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (next_hop_addr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}


static void
grp_rte_hello_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
										GrpT_Packet_Option* tlv_options_ptr)
	{
	GrpT_Hello_Option*			hello_option_ptr;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	
	/** Handles the arrival of a Hello packet	**/
	FIN (grp_rte_hello_pkt_arrival_handle (<args>));
	
	if (LTRACE_HELLO)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "Received a Hello packet from node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
		}
	
	/* Get the Hello option	*/
	hello_option_ptr = (GrpT_Hello_Option*) tlv_options_ptr->grp_option_ptr;
	
	/* Add the neighbor to the neighbor table	*/
	grp_nbr_table_update (neighbor_table, ip_dgram_fd_ptr->src_addr, hello_option_ptr->nbr_lat, 
							hello_option_ptr->nbr_long, hello_option_ptr->timestamp, nbr_expiry_time);
	
	/* Destroy the GRP packet	*/
	op_pk_destroy (grp_pkptr);
	
	/* Destroy the Hello packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}


static void
grp_rte_flood_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr,
										GrpT_Packet_Option* tlv_options_ptr)
	{
	GrpT_Flooding_Option*		flooding_option_ptr;
	GrpT_Dest_Info*				dest_ptr;
	Objid						source_node_id;
	Objid						module_objid;
	double						low_lat_m, low_long_m, high_lat_m, high_long_m;
	double						quad_size;
	double						quad_lat_m, quad_long_m;
	OpT_Packet_Size				pkt_size;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	
	/** Handles the arrival of a flooding packet	**/
	FIN (grp_rte_flood_pkt_arrival_handle (<args>));
	
	/* Get the flooding option	*/
	flooding_option_ptr = (GrpT_Flooding_Option*) tlv_options_ptr->grp_option_ptr;
	
	if (LTRACE_FLOODING)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "from source node %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Received a Flooding packet", temp_str, OPC_NIL);
		}
	
	/* Check if the received flooding packet's source address	*/
	/* belongs to this node. If so, destroy the packet			*/
	module_objid = op_pk_creation_mod_get (ip_pkptr);
	source_node_id = op_topo_parent (module_objid);
	if (source_node_id == own_node_objid)
		{
		/* Simply destroy the packet	*/
		op_pk_destroy (grp_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Check if the node has received the flooding message	*/
	/* inside the boundary which the message is supposed	*/
	/* to be broadcast.										*/
	if ((flooding_option_ptr->low_quad_lat <= node_lat_m) && (node_lat_m < flooding_option_ptr->high_quad_lat) &&
		(flooding_option_ptr->low_quad_long <= node_long_m) && (node_long_m < flooding_option_ptr->high_quad_long))
		{
		/* The flooding message is intended for this node 	*/
		/* as it is within the boundary of the flood		*/
		
		/* Check if we have already received this flooding packet	*/
		dest_ptr = grp_dest_table_entry_exists (destination_table, ip_dgram_fd_ptr->src_addr);
	
		if ((dest_ptr != OPC_NIL) && (flooding_option_ptr->timestamp <= dest_ptr->timestamp))
			{
			if (LTRACE_FLOODING)
				{
				op_prg_odb_print_major ("We have already received this flooding packet. Discarding packet.", OPC_NIL);
				}
			
			/* We have already received this flooding packet	*/
			/* Simply discard this packet.						*/
			op_pk_destroy (grp_pkptr);
			manet_rte_ip_pkt_destroy (ip_pkptr);
		
			FOUT;
			}
		
		/* We have not received this flooding packet	*/
		/* Update the destination table with the info	*/
		
		/* Determine if the node is in the same	*/
		/* quadrant as the destination			*/
		grp_support_neighborhood_determine (flooding_option_ptr->node_lat, flooding_option_ptr->node_long, neighborhood_size, 
										&low_lat_m, &low_long_m, &high_lat_m, &high_long_m);
		
		if ((low_lat_m <= node_lat_m) && (node_lat_m < high_lat_m) && (low_long_m <= node_long_m) && (node_long_m < high_long_m))
			{
			/* The nodes are in the same quadrant	*/
			/* Insert the actual position of the	*/
			/* destination node into the table		*/
			grp_dest_table_update (destination_table, ip_dgram_fd_ptr->src_addr, flooding_option_ptr->node_lat,
							flooding_option_ptr->node_long, flooding_option_ptr->timestamp, neighborhood_size, 
							GrpC_Destination_Node);
			}
		else
			{
			/* The nodes are not in the same quadrant	*/
			/* Insert the position of the centre of the */
			/* highest level neighbor quadrant into the	*/
			/* destination table.						*/
			quad_size = grp_support_highest_neighbor_quadrant_determine (node_lat_m, node_long_m, flooding_option_ptr->node_lat, flooding_option_ptr->node_long, 
																			neighborhood_size, &quad_lat_m, &quad_long_m);
			
			grp_dest_table_update (destination_table, ip_dgram_fd_ptr->src_addr, quad_lat_m,
								quad_long_m, flooding_option_ptr->timestamp, quad_size, GrpC_Destination_Quad);
			}
	
		/* Insert the GRP packet in the IP datagram	*/
		op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
		
		/* Update the statistic for total traffic sent	*/
		pkt_size = op_pk_total_size_get (ip_pkptr);
		grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
		grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
		
		if (LTRACE_FLOODING)
			{
			op_prg_odb_print_major ("Reflooding the packet to other neighbor nodes.", temp_str, OPC_NIL);
			}
		
		/* Reflood the packet to all neighboring nodes	*/
		manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
		}
	else
		{
		/* The node is outside the boundary. Discard the packet	*/
		if (LTRACE_FLOODING)
			{
			op_prg_odb_print_major ("This node is outside the boundary for which the flooding packet is meant.", 
										"Simply discarding the packet.", OPC_NIL);
			}
		
		op_pk_destroy (grp_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		}
	
	FOUT;
	}


static void
grp_rte_data_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
								GrpT_Packet_Option* tlv_options_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	GrpT_Data_Option*			data_option_ptr;
	Packet*						return_pkptr;
	Packet*						app_pkptr;
	OpT_Packet_Size				pkt_size;
	GrpT_Dest_Info*				dest_ptr;
	InetT_Address				next_hop_addr;
	InetT_Address*				node_traversed_ptr;
	InetT_Address*				prev_node_addr_ptr;
	double						new_dist_to_dest;
	int							num_nodes_traversed, num_hops;
	Boolean						route_export;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	char						temp_str1 [256];
	double						creation_time;
	
	/** Handles the arrival of the data packet	**/
	FIN (grp_rte_data_pkt_arrival_handle (<args>));
	
	/* Get the data option	*/
	data_option_ptr = (GrpT_Data_Option*) tlv_options_ptr->grp_option_ptr;
	
	/* Get the previous node address that sent this packet	*/
	prev_node_addr_ptr = (InetT_Address*) op_prg_list_access (data_option_ptr->nodes_traversed_lptr, OPC_LISTPOS_TAIL);
	
	/* Get the received interface address and insert this 	*/
	/* received interface address on the list of nodes 		*/
	/* traversed to backtrack if needed						*/
	node_traversed_ptr = inet_address_create_dynamic (intf_ici_fdstruct_ptr->interface_received);
	op_prg_list_insert (data_option_ptr->nodes_traversed_lptr, node_traversed_ptr, OPC_LISTPOS_TAIL);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "from source node %s (%s).", addr_str, node_name);
		inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
		sprintf (temp_str1, "to destination node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Received a Data packet", temp_str, temp_str1, OPC_NIL);
		}
	
	/* Check if this is the destination node	*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_TRUE)
		{
		/* This is the destination node. Send	*/
		/* the packet to the application layer	*/
		/* Decapsulate the GRP packet			*/
		return_pkptr = grp_rte_ip_datagram_decapsulate (ip_pkptr, grp_pkptr);
		
		/* Check if the routes export flag is set	*/
		if (op_pk_nfd_is_set (grp_pkptr, "route_export"))
			{
			op_pk_nfd_get (grp_pkptr, "route_export", &route_export);
			}
	
		if (return_pkptr != OPC_NIL)
			{
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("This node is the destination of the packet.", 
									"Sending the packet to the application layer.", OPC_NIL);
				}
			
			if (global_routes_export || route_export)
				{
				/* Get the creation time of the grp packet	*/
				creation_time = op_pk_creation_time_get (grp_pkptr);
				
				/* Print the route to the output table	*/
				grp_support_route_print_to_ot (data_option_ptr->nodes_traversed_lptr, creation_time);
				}
			
			if (global_record_routes)
				{
				/* Record the route taken for display	*/
				manet_rte_path_display_dump (data_option_ptr->nodes_traversed_lptr);
				}
			
			/* Get the number of nodes traversed	*/
			/* Also include this destination node	*/
			num_nodes_traversed = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
			
			/* Number of hops	*/
			num_hops = num_nodes_traversed - 1;
			
			/* Update the statistic	*/
			op_stat_write (global_stat_handle_ptr->num_hops_global_shandle, (double) num_hops);
			
			/* Send the IP packet to the higher layer	*/
			manet_rte_to_higher_layer_pkt_send_schedule (module_data_ptr, parent_prohandle, return_pkptr);
			
			/* Destory the GRP packet	*/
			op_pk_destroy (grp_pkptr);
			}
		else
			{
			/* Destory the GRP packet	*/
			op_pk_destroy (grp_pkptr);
			
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		
		FOUT;
		}
	
	/* If the TTL is zero, destroy the packet	*/
	if (ip_dgram_fd_ptr->ttl == 0)
		{
		/* Record the end of the route	*/
		manet_rte_dgram_discard_record (module_data_ptr, grp_pkptr, "TTL expired");
		
		/* Update the statistic for data dropped	*/
		op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
		pkt_size = op_pk_total_size_get (app_pkptr);
		grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_TTL_Drop);
		op_pk_nfd_set (grp_pkptr, "data", app_pkptr);
		
		/* Destroy the data packet	*/
		op_pk_destroy (grp_pkptr);
			
		/* Destroy the packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
			
		FOUT;
		}
	
	/* Get the destination information in this node's destination table	*/
	dest_ptr = grp_dest_table_entry_exists (destination_table, ip_dgram_fd_ptr->dest_addr);
	
	if ((dest_ptr == OPC_NIL) || (dest_ptr->destination_state == GrpC_Destination_Invalid))
		{
		/* Check if the backtracking option is enabled	*/
		if (backtrack_option == OPC_TRUE)
			{
			/* Backtracking needs to be done as the destination	*/
			/* is not known to this node.						*/
			grp_rte_backtrack (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			}
		else
			{
			/* Record the end of the route	*/
			manet_rte_dgram_discard_record (module_data_ptr, grp_pkptr, "Destination node does not exist in the route table");
			
			/* Update the statistic for data dropped	*/
			op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
			pkt_size = op_pk_total_size_get (app_pkptr);
			grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Dest_Drop);
			op_pk_nfd_set (grp_pkptr, "data", app_pkptr);
			
			/* Destroy the data packet	*/
			op_pk_destroy (grp_pkptr);
			
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		
		FOUT;
		}
	
	/* This node is an intermediate node to the destination	*/
	/* Find the neighbor node that is closest to the		*/
	/* destination node to send the packet to.				*/
	if (dest_ptr->destination_type == GrpC_Destination_Node)
		{
		/* This node is in the same quadrant as the destination node	*/
		if (data_option_ptr->dest_type == GrpC_Destination_Node)
			{
			/* The previous node was also in the same quadrant as the destination node	*/
			/* Set the previous distance to destination as the distance in metres of	*/
			/* this node to the destination node.										*/
			next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, 
																	data_option_ptr->nodes_traversed_lptr, dest_ptr, 
																	data_option_ptr->prev_dist_to_dest, &new_dist_to_dest);
			}
		else
			{
			/* The previous node was in a different quadrant to the destination node	*/
			/* Recalculate the previous distance to destination to find the closest		*/
			/* neighbor node in the quadrant to the destination node.					*/
			next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, 
																	data_option_ptr->nodes_traversed_lptr, dest_ptr, -1.0, &new_dist_to_dest);
			}
		}
	else
		{
		/* This node is in a different quadrant to the destination node.	*/
		if (data_option_ptr->dest_type == GrpC_Destination_Node)
			{
			/* The previous node was in the same quadrant as the destination node	*/
			/* Recalculate the previous distance to destination to find the closest	*/
			/* neighbor node to the destination quadrant.							*/
			next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, 
																	data_option_ptr->nodes_traversed_lptr, dest_ptr, -1.0, &new_dist_to_dest);
			}
		else
			{
			/* The previous node was also in a different quadrant to the destination node	*/
			/* Set the previous distance to destination as the distance in metres of		*/
			/* this node to the destination node.											*/
			next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, 
																	data_option_ptr->nodes_traversed_lptr, dest_ptr, 
																	data_option_ptr->prev_dist_to_dest, &new_dist_to_dest);
			}
		}
	
	if (inet_address_equal (next_hop_addr, InetI_Invalid_Addr))
		{
		/* No next hop neighbor has been found. This could	*/
		/* be due to :										*/
		/* 1. There is no neighboring nodes to this node	*/
		/* 2. There is no closer neighbor node than the 	*/
		/*    previous distance to destination calculation	*/
		/* Backtracking needs to be done					*/
		
		/* Check if the backtracking option is enabled	*/
		if (backtrack_option == OPC_TRUE)
			{
			grp_rte_backtrack (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			}
		else
			{
			/* Record the end of the route	*/
			manet_rte_dgram_discard_record (module_data_ptr, grp_pkptr, "No valid next hop neighbor found");
			
			/* Update the statistic for data dropped	*/
			op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
			pkt_size = op_pk_total_size_get (app_pkptr);
			grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Nbr_Drop);
			op_pk_nfd_set (grp_pkptr, "data", app_pkptr);
			
			/* Destroy the data packet	*/
			op_pk_destroy (grp_pkptr);
			
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		
		FOUT;
		}
	
	/* Set the new destination information that this	*/
	/* node carries about the destination node.	This 	*/
	/* will either be a new highest neighbor quardant	*/
	/* or specific destination location information		*/
	data_option_ptr->dest_lat = dest_ptr->dest_lat;
	data_option_ptr->dest_long = dest_ptr->dest_long;
	data_option_ptr->dest_type = dest_ptr->destination_type;
	data_option_ptr->quadrant_size = dest_ptr->quadrant_size;
	data_option_ptr->prev_dist_to_dest = new_dist_to_dest;
	
	/* Insert the GRP packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
	
	/* Decrement the TTL	*/
	ip_dgram_fd_ptr->ttl -= 1;
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, next_hop_addr);
		inet_address_to_hname (next_hop_addr, node_name);
		sprintf (temp_str1, "to next hop node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major ("Sending the data packet", temp_str1, OPC_NIL);
		}
		
	/* Update the statistic for total traffic sent	*/
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, op_pk_total_size_get (ip_pkptr));
	
	/* Send the packet to the neighboring node closest to the destination	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (next_hop_addr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}


static void
grp_rte_backtrack_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
										GrpT_Packet_Option* tlv_options_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	GrpT_Data_Option*			data_option_ptr;
	InetT_Address*				node_addr_ptr;
	InetT_Address*				nbr_ptr;
	GrpT_Backtrack_Info*		backtrack_ptr;
	Packet*						app_pkptr;
	OpT_Packet_Size				pkt_size;
	char						nbr_addr_str [128];
	GrpT_Dest_Info*				dest_ptr;
	InetT_Address				next_hop_addr;
	void*						ignore;
	double						new_dist_to_dest;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	char						temp_str1 [256];
	
	/** Handles the arrival of a backtrack packet	**/
	FIN (grp_rte_backtrack_pkt_arrival_handle (<args>));
	
	/* Get the data option	*/
	data_option_ptr = (GrpT_Data_Option*) tlv_options_ptr->grp_option_ptr;
	
	/* Get the node from which the backtrack packet was received	*/
	node_addr_ptr = (InetT_Address*) op_prg_list_remove (data_option_ptr->nodes_traversed_lptr, OPC_LISTPOS_TAIL);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_ptr_print (addr_str, node_addr_ptr);
		inet_address_to_hname (*node_addr_ptr, node_name);
		sprintf (temp_str, "from node %s (%s).", addr_str, node_name);
		inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
		sprintf (temp_str1, "to destination node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Received a Backtrack packet", temp_str, temp_str1, OPC_NIL);
		}
	
	/* Insert this node address in the do not consider	*/
	/* table for next hop neighbors when tying to find	*/
	/* another route to the destination					*/
	backtrack_ptr = grp_rte_backtrack_info_mem_alloc ();
	inet_address_ptr_print (nbr_addr_str, node_addr_ptr);
	
	nbr_ptr = inet_address_copy_dynamic (node_addr_ptr);
	backtrack_ptr->nbr_addr_ptr = node_addr_ptr;
	backtrack_ptr->insert_time = op_sim_time ();
	prg_string_hash_table_item_insert (backtrack_table, nbr_addr_str, backtrack_ptr, &ignore);
	op_intrpt_schedule_call (op_sim_time () + backtrack_hold_time, 0, grp_rte_backtrack_table_update, nbr_ptr);
	
	/* Get the destination information in this node's destination table	*/
	dest_ptr = grp_dest_table_entry_exists (destination_table, ip_dgram_fd_ptr->dest_addr);
	
	if ((dest_ptr == OPC_NIL) || (dest_ptr->destination_state == GrpC_Destination_Invalid))
		{
		if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->src_addr) == OPC_FALSE)
			{
			/* Backtracking needs to be done as the destination	*/
			/* is not known to this node.						*/
			grp_rte_backtrack (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			}
		else
			{
			/* This is the source of the data packet	*/
			/* There is no more backtracking that can	*/
			/* be done. Destroy the packet.				*/
			
			/* Update the statistic for data dropped	*/
			op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
			pkt_size = op_pk_total_size_get (app_pkptr);
			grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Dest_Drop);
			op_pk_nfd_set (grp_pkptr, "data", app_pkptr);
			
			/* Destroy the GRP packet	*/
			op_pk_destroy (grp_pkptr);
			
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		
		FOUT;
		}
	
	/* Find the neighbor node that is closest to the		*/
	/* destination node to send the packet to.				*/
	next_hop_addr = grp_nbr_table_closest_nbr_to_dest_find (neighbor_table, backtrack_table, destination_table, data_option_ptr->nodes_traversed_lptr, 
															dest_ptr, -1.0, &new_dist_to_dest);
	
	if (inet_address_equal (next_hop_addr, InetI_Invalid_Addr))
		{
		/* No next hop neighbor has been found. This could	*/
		/* be due to :										*/
		/* 1. There is no neighboring nodes to this node	*/
		/* 2. There is no closer neighbor node than the 	*/
		/*    previous distance to destination calculation	*/
		/* Backtracking needs to be done					*/
		if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->src_addr) == OPC_FALSE)
			{
			/* Backtracking needs to be done as there is no	*/
			/* valid next hop for this node.				*/
			grp_rte_backtrack (ip_pkptr, grp_pkptr, ip_dgram_fd_ptr, tlv_options_ptr, intf_ici_fdstruct_ptr);
			}
		else
			{
			/* This is the source of the data packet	*/
			/* There is no more backtracking that can	*/
			/* be done. Destroy the packet.				*/
			
			/* Update the statistic for data dropped	*/
			op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
			pkt_size = op_pk_total_size_get (app_pkptr);
			grp_support_traffic_dropped_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size, GrpC_No_Nbr_Drop);
			op_pk_nfd_set (grp_pkptr, "data", app_pkptr);
			
			/* Destroy the GRP packet	*/
			op_pk_destroy (grp_pkptr);
			
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		
		FOUT;
		}
	
	/* Set the distance of the previous node to the destination	*/
	/* Update the destination information						*/
	data_option_ptr->dest_lat = dest_ptr->dest_lat;
	data_option_ptr->dest_long = dest_ptr->dest_long;
	data_option_ptr->dest_type = dest_ptr->destination_type;
	data_option_ptr->quadrant_size = dest_ptr->quadrant_size;
	data_option_ptr->prev_dist_to_dest = new_dist_to_dest;
	
	/* Set the option type as data	*/
	tlv_options_ptr->option_type = GRPC_DATA_OPTION;
	
	/* Insert the GRP packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, next_hop_addr);
		inet_address_to_hname (next_hop_addr, node_name);
		sprintf (temp_str1, "to next hop node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major ("Sending the data packet", temp_str1, OPC_NIL);
		}
	
	/* Update the statistic for total traffic sent	*/
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, op_pk_total_size_get (ip_pkptr));
	
	/* Send the packet to the neighboring node closest to the destination	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (next_hop_addr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
		
	FOUT;
	}
	
	


static void
grp_rte_pos_req_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
										GrpT_Packet_Option* tlv_options_ptr)
	{
	GrpT_Dest_Pos_Req_Option*		pos_req_option_ptr;
	int								num_elem, count;
	InetT_Address*					addr_ptr;
	GrpT_Dest_Info*					dest_ptr;
	GrpT_Dest_Pos_Resp_Option*		pos_resp_option_ptr = OPC_NIL;
	GrpT_Packet_Option*				tlv_ptr;
	Packet*							grp_resp_pkptr;
	Packet*							ip_resp_pkptr;
	ManetT_Nexthop_Info*			manet_nexthop_info_ptr;
	OpT_Packet_Size					pkt_size;
	char							addr_str [INETC_ADDR_STR_LEN];
	char							node_name [OMSC_HNAME_MAX_LEN];
	char							temp_str [256];
	
	/** Handles the arrival of a position request packet	**/
	FIN (grp_rte_pos_req_pkt_arrival_handle (<args>));
	
	/* Get the position request option	*/
	pos_req_option_ptr = (GrpT_Dest_Pos_Req_Option*) tlv_options_ptr->grp_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "from source node %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Received a Position Request packet", temp_str, OPC_NIL);
		}
	
	/* Get the number of destination infomation required	*/
	num_elem = op_prg_list_size (pos_req_option_ptr->dest_req_lptr);
	
	for (count = 0; count < num_elem; count++)
		{
		addr_ptr = (InetT_Address*) op_prg_list_remove (pos_req_option_ptr->dest_req_lptr, OPC_LISTPOS_HEAD);
		
		/* Check if the request is for this node. This node's	*/
		/* information will not exist in the destination table	*/
		if (manet_rte_address_belongs_to_node (module_data_ptr, *addr_ptr) == OPC_TRUE)
			{
			/* The address does belong to this node	*/
			if (pos_resp_option_ptr == OPC_NIL)
				{
				/* Create a position response option	*/
				pos_resp_option_ptr = grp_pkt_support_pos_resp_option_create (ip_dgram_fd_ptr->src_addr);
				}
			
			/* Add this destination to the position reponse option	*/
			grp_pkt_support_pos_resp_option_dest_add (pos_resp_option_ptr, *addr_ptr, node_lat_m, node_long_m,
														GrpC_Destination_Node, neighborhood_size, op_sim_time ());
			}
		else
			{		
			/* Get the destination information for this destination	*/
			dest_ptr = grp_dest_table_entry_exists_ptr (destination_table, addr_ptr);
		
			if (dest_ptr != OPC_NIL)
				{
				/* This is a valid destination location	*/
				/* Create a position response option if	*/
				/* one has not yet been created.		*/
				if (pos_resp_option_ptr == OPC_NIL)
					{
					/* Create a position response option	*/
					pos_resp_option_ptr = grp_pkt_support_pos_resp_option_create (ip_dgram_fd_ptr->src_addr);
					}
				
				/* Add this destination to the position reponse option	*/
				grp_pkt_support_pos_resp_option_dest_add (pos_resp_option_ptr, dest_ptr->dest_addr, dest_ptr->dest_lat, dest_ptr->dest_long,
															dest_ptr->destination_type, dest_ptr->quadrant_size, dest_ptr->timestamp);
				}
			}
		
		/* Destroy the memory allocated for the address	*/
		inet_address_destroy_dynamic (addr_ptr);
		}
	
	if (pos_resp_option_ptr == OPC_NIL)
		{
		/* There were no requested destinations found	*/
		/* in this node's destination table. Do not		*/
		/* send a position response packet back			*/
		op_pk_destroy (grp_pkptr);
	
		/* Destroy the incoming IP packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	
	/* Create the GRP TLV	*/
	tlv_ptr = grp_pkt_support_pos_resp_tlv_create (pos_resp_option_ptr);
	
	/* Create the Flooding packet	*/
	grp_resp_pkptr = grp_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the TLV option in the packet	*/
	grp_pkt_support_option_set (grp_resp_pkptr, tlv_ptr);
	
	/* Create the IP datagram	*/
	ip_resp_pkptr = grp_rte_ip_datagram_create (grp_resp_pkptr, ip_dgram_fd_ptr->src_addr, ip_dgram_fd_ptr->src_addr);
	
	if (LTRACE_ACTIVE)
		{
		op_prg_odb_print_major ("Sending a Position Response packet.", OPC_NIL);
		}
	
	/* Create the next hop infomation for association with the event	*/
	manet_nexthop_info_ptr = grp_rte_next_hop_info_create (ip_dgram_fd_ptr->src_addr);
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* GRP control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Update the statistic for total traffic sent	*/
	pkt_size = op_pk_total_size_get (ip_resp_pkptr);
	grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	
	/* Send the packet to the CPU	*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_resp_pkptr);
	
	/* Uninstall the event state 	*/
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	/* Destroy the incoming GRP packet	*/
	op_pk_destroy (grp_pkptr);
	
	/* Destroy the incoming IP packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
		
	FOUT;
	}


static void
grp_rte_pos_resp_pkt_arrival_handle (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr)
	{
	GrpT_Dest_Pos_Resp_Option*		pos_resp_option_ptr;
	int								num_elem, count;
	GrpT_Dest_Response*				dest_info_ptr;
	char							addr_str [INETC_ADDR_STR_LEN];
	char							node_name [OMSC_HNAME_MAX_LEN];
	char							temp_str [256];
	
	/** Handles the arrival of a position response packet	**/
	FIN (grp_rte_pos_resp_pkt_arrival_handle (<args>));
	
	/* A position response packet has arrived	*/
	/* Cancel the position request timer.		*/
	if (op_ev_valid (pos_req_timer_evhandle) && op_ev_pending (pos_req_timer_evhandle))
		{
		/* Cancel the timer	*/
		op_ev_cancel (pos_req_timer_evhandle);
		}
	
	/* Get the position response option	*/
	pos_resp_option_ptr = (GrpT_Dest_Pos_Resp_Option*) tlv_options_ptr->grp_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "from node %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Received a Position Response packet", temp_str, OPC_NIL);
		}
	
	/* Get the number of elements	*/
	num_elem = op_prg_list_size (pos_resp_option_ptr->desp_resp_lptr);
	
	if (num_elem == 0)
		{
		/* There are no elements in the list	*/
		op_pk_destroy (grp_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	for (count = 0; count < num_elem; count++)
		{
		/* Access each destination information	*/
		dest_info_ptr = (GrpT_Dest_Response*) op_prg_list_access (pos_resp_option_ptr->desp_resp_lptr, count);
		
		/* Update the destination table with the information	*/
		grp_dest_table_update (destination_table, dest_info_ptr->dest_addr, dest_info_ptr->dest_lat, dest_info_ptr->dest_long,
								dest_info_ptr->timestamp, dest_info_ptr->quadrant_size, dest_info_ptr->dest_type);
		}
	
	/* All the destinations have been validated	*/
	/* Remove all the neighbors from the list	*/
	grp_rte_address_list_clear (nbr_node_attempted_lptr);
	
	/* Destroy the packets	*/
	op_pk_destroy (grp_pkptr);
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}


static void
grp_rte_hello_request_send (void)
	{
	GrpT_Packet_Option*		tlv_ptr;
	Packet*					grp_pkptr;
	Packet*					ip_pkptr;
	OpT_Packet_Size			pkt_size;
	
	/** Broadcasts a Hello request message	**/
	FIN (grp_rte_hello_request_send (<args>));
	
	/* Create a Hello TLV	*/
	tlv_ptr = grp_pkt_support_hello_tlv_create (InetI_Invalid_Addr, node_lat_m, node_long_m, GRPC_HELLO_REQUEST);
	
	/* Create the Hello packet	*/
	grp_pkptr = grp_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the TLV option in the packet	*/
	grp_pkt_support_option_set (grp_pkptr, tlv_ptr);

	/* Create the IP datagram	*/
	ip_pkptr = grp_rte_ip_datagram_create (grp_pkptr, InetI_Broadcast_v4_Addr, InetI_Broadcast_v4_Addr);
	
	/* Update the statistic for total traffic sent	*/
	pkt_size = op_pk_total_size_get (ip_pkptr);
	grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	
	if (LTRACE_HELLO)
		{
		op_prg_odb_print_major (pid_string, "Broadcasting a Hello request message.", OPC_NIL);
		}
	
	/* Send the packet to the CPU which will broadcast it	*/
	/* after processing the packet							*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	FOUT;
	}


static void
grp_rte_hello_response_send (IpT_Dgram_Fields* ip_dgram_fd_ptr)
	{
	short					output_intf_index;
	IpT_Interface_Info*		iface_info_ptr;
	int						outstrm;
	InetT_Address_Range* 	inet_addr_range_ptr;
	GrpT_Packet_Option*		tlv_ptr;
	Packet*					grp_pkptr;
	Packet*					ip_pkptr;
	InetT_Address			src_address;
	ManetT_Nexthop_Info*	manet_nexthop_info_ptr;
	OpT_Packet_Size			pkt_size;
	char					addr_str [INETC_ADDR_STR_LEN];
	char					node_name [OMSC_HNAME_MAX_LEN];
	char					temp_str [256];
	
	/** Sends a Hello response packet on	**/
	/** reception of a Hello request packet	**/
	FIN (grp_rte_hello_response_send (<args>));
	
	/* Get the interface address on which	*/
	/* this packet is going to be sent out	*/
	if (ip_rte_destination_local_network (module_data_ptr, ip_dgram_fd_ptr->src_addr, &output_intf_index,
			&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
		{
		src_address = inet_address_range_addr_get (inet_addr_range_ptr);
		}
	
	/* Create the Hello TLV	*/
	tlv_ptr = grp_pkt_support_hello_tlv_create (src_address, node_lat_m, node_long_m, GRPC_HELLO_RESPONSE);
	
	/* Create the Hello packet	*/
	grp_pkptr = grp_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
	/* Set the TLV option in the packet	*/
	grp_pkt_support_option_set (grp_pkptr, tlv_ptr);
	
	/* Create the next hop infomation for association with the event	*/
	manet_nexthop_info_ptr = grp_rte_next_hop_info_create (ip_dgram_fd_ptr->src_addr);
	
	/* Create the IP datagram	*/
	ip_pkptr = grp_rte_ip_datagram_create (grp_pkptr, ip_dgram_fd_ptr->src_addr, ip_dgram_fd_ptr->src_addr);
	
	if (LTRACE_HELLO)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "to node %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Sending a Hello response packet", temp_str, OPC_NIL);
		}
	
	/* Update the statistic for total traffic sent	*/
	pkt_size = op_pk_total_size_get (ip_pkptr);
	grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* GRP control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Send the packet to the CPU	*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	/* Uninstall the event state 	*/
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	FOUT;
	}


static void
grp_rte_flooding_message_send (double lat_m, double long_m, double flood_low_lat_m, double flood_low_long_m,
								double flood_high_lat_m, double flood_high_long_m)
	{
	GrpT_Packet_Option*			tlv_ptr;
	Packet*						grp_pkptr;
	Packet*						ip_pkptr;
	OpT_Packet_Size				pkt_size;
	
	/** Broadcasts a flooding packet	**/
	FIN (grp_rte_flooding_message_send (<args>));
	
	/* Create a Flooding TLV	*/
	tlv_ptr = grp_pkt_support_flooding_tlv_create (InetI_Invalid_Addr, lat_m, long_m, flood_low_lat_m, flood_low_long_m,
													flood_high_lat_m, flood_high_long_m);
	
	/* Create the Flooding packet	*/
	grp_pkptr = grp_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the TLV option in the packet	*/
	grp_pkt_support_option_set (grp_pkptr, tlv_ptr);
	
	/* Create the IP datagram	*/
	ip_pkptr = grp_rte_ip_datagram_create (grp_pkptr, InetI_Broadcast_v4_Addr, InetI_Broadcast_v4_Addr);
	
	/* Update the statistic for total traffic sent	*/
	pkt_size = op_pk_total_size_get (ip_pkptr);
	grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	
	if (LTRACE_FLOODING)
		{
		op_prg_odb_print_major (pid_string, "Broadcasting a Flooding message.", OPC_NIL);
		}
	
	/* Send the packet to the CPU which will broadcast it	*/
	/* after processing the packet							*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	FOUT;
	}


static void
grp_rte_position_request_message_send (List* dest_lptr, double low_lat, double low_long, double high_lat, double high_long)
	{
	InetT_Address			nbr_node_addr;
	GrpT_Packet_Option*		tlv_ptr;
	Packet*					grp_pkptr;
	Packet*					ip_pkptr;
	ManetT_Nexthop_Info*	manet_nexthop_info_ptr;
	InetT_Address*			nbr_node_addr_ptr;
	OpT_Packet_Size			pkt_size;
	char					addr_str [INETC_ADDR_STR_LEN];
	char					node_name [OMSC_HNAME_MAX_LEN];
	char					temp_str [256];
	
	/** Sends a position request message to the	**/
	/** neighbor node in the same quadrant.		**/
	FIN (grp_rte_position_request_message_send (<args>));
	
	/* Find the neighbor node that is in the same quadrant	*/
	/* to send the position request packet to.				*/
	nbr_node_addr = grp_nbr_table_pos_req_nbr_find (neighbor_table, nbr_node_attempted_lptr, low_lat, low_long, high_lat, high_long);
	
	if (inet_address_equal (nbr_node_addr, InetI_Invalid_Addr))
		{
		/* No valid neighbor node that can be	*/
		/* attempted exists. Schedule a timer	*/
		/* to try again	later					*/
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "No valid neighbor node exists to send a position request message to.", 
										"Schedule the position request timer to try again", OPC_NIL);
			}
		
		FOUT;
		}
	
	/* Create the position request TLV	*/
	tlv_ptr = grp_pkt_support_pos_req_tlv_create (nbr_node_addr, dest_lptr);
	
	/* Create the Flooding packet	*/
	grp_pkptr = grp_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the TLV option in the packet	*/
	grp_pkt_support_option_set (grp_pkptr, tlv_ptr);
	
	/* Create the IP datagram	*/
	ip_pkptr = grp_rte_ip_datagram_create (grp_pkptr, nbr_node_addr, nbr_node_addr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, nbr_node_addr);
		inet_address_to_hname (nbr_node_addr, node_name);
		sprintf (temp_str, "to neighbor node %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "Sending a Position request message", temp_str, OPC_NIL);
		}
	
	/* Create the next hop infomation for association with the event	*/
	manet_nexthop_info_ptr = grp_rte_next_hop_info_create (nbr_node_addr);
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* GRP control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Update the statistic for total traffic sent	*/
	pkt_size = op_pk_total_size_get (ip_pkptr);
	grp_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, pkt_size);
	
	/* Send the packet to the CPU	*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	/* Uninstall the event state 	*/
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	/* Insert this neighbor node into the attempted list	*/
	nbr_node_addr_ptr = inet_address_create_dynamic (nbr_node_addr);
	op_prg_list_insert (nbr_node_attempted_lptr, nbr_node_addr_ptr, OPC_LISTPOS_TAIL);
	
	FOUT;
	}


static void
grp_rte_backtrack (Packet* ip_pkptr, Packet* grp_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, GrpT_Packet_Option* tlv_options_ptr, 
					IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	GrpT_Data_Option*		data_option_ptr;
	InetT_Address*			prev_node_addr_ptr;
	int						list_size, total_size;
	int						num_hops;
	
	/** Backtracks the data packet to the previous node	**/
	FIN (grp_rte_backtrack (<args>));
	
	/* Get the data option	*/
	data_option_ptr = (GrpT_Data_Option*) tlv_options_ptr->grp_option_ptr;
	
	/* Update the backtrack statistic	*/
	op_stat_write (local_stat_handle_ptr->total_num_backtracks_shandle, 1.0);
	op_stat_write (global_stat_handle_ptr->total_num_backtracks_global_shandle, 1.0);
	
	/* Get the previous node to send the packet to	*/
	num_hops = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
	
	/* The last element in the list is this node. So the previous	*/
	/* hop is the node before this node which is n-2				*/
	prev_node_addr_ptr = (InetT_Address*) op_prg_list_access (data_option_ptr->nodes_traversed_lptr, num_hops - 2);
	
	if (LTRACE_ACTIVE)
		{
		op_prg_odb_print_major (pid_string, "Cannot route data packet from this node", 
									"Backtrack the packet to the previous node", OPC_NIL);
		}
	
	/* Set the option type as backtrack	*/
	tlv_options_ptr->option_type = GRPC_BACKTRACK_OPTION;
	
	/* Get the length of the option	*/
	list_size = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
	total_size = GRP_DATA_HEADER_SIZE + list_size * IPC_V4_ADDR_LEN;
	tlv_options_ptr->option_length = total_size + GRP_TLV_SIZE;
	
	/* Insert the GRP packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
	
	/* Update the statistic for total traffic sent	*/
	grp_support_total_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, op_pk_total_size_get (ip_pkptr));
	
	/* Send the packet to the neighboring node closest to the destination	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (*prev_node_addr_ptr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}


static void
grp_rte_backtrack_table_update (void* addr_ptr, int code)
	{
	GrpT_Backtrack_Info*		backtrack_ptr;
	InetT_Address*				nbr_addr_ptr;
	char						nbr_addr_str [256];
	
	/** Removes all expired neighbors from the	**/
	/** backtrack table.						**/
	FIN (grp_rte_backtrack_table_update (<args>));
	
	nbr_addr_ptr = (InetT_Address*) addr_ptr;
	
	inet_address_ptr_print (nbr_addr_str, nbr_addr_ptr);
	backtrack_ptr = (GrpT_Backtrack_Info*) prg_string_hash_table_item_remove (backtrack_table, nbr_addr_str);
	
	if (backtrack_ptr != OPC_NIL)
		{
		grp_rte_backtrack_info_mem_free (backtrack_ptr);
		}
	
	inet_address_destroy_dynamic (nbr_addr_ptr);
	
	FOUT;
	}
		


static void
grp_rte_hello_schedule (void)
	{
	double			grp_hello_interval;
	
	/** Schedules a self interrupt for	**/
	/** periodic Hello broadcast.		**/
	FIN (grp_rte_hello_schedule (void));
	
	grp_hello_interval = oms_dist_outcome (hello_interval_dist_ptr);
	
	/* Schedule a self interrupt to send out the next Hello broadcast	*/
	op_intrpt_schedule_self (op_sim_time () + grp_hello_interval, GRPC_HELLO_BROADCAST);
	
	FOUT;
	}


static void
grp_rte_node_pos_in_metres_get (double* lat_dist_m, double* long_dist_m)
	{
	double			lat, longe, alt, x, y, z;
	double			lat_dist_deg, long_dist_deg;
	
	/** Get the current position of the node in metres	**/
	FIN (grp_rte_node_pos_in_metres_get (<args>));
	
	/* Get the current position of the node in degrees	*/
	op_ima_obj_pos_get (own_node_objid, &lat, &longe, &alt, &x, &y, &z);
	
	/* Consider (-90.0, -180.0) as the starting point 	*/
	/* in the graph. Get the distance in degrees of the	*/
	/* current position of the node from the starting 	*/
	/* point in the graph along the x anf y axes		*/
	lat_dist_deg = lat + 90.0;
	long_dist_deg = longe + 180.0;
	
	/* Convert the lat and long values from degrees to metres	*/
	/* Consider that the starting point in the graph to be 		*/
	/* (-90, -180) and calculate the distance from this starting*/
	/* point to the current position of the node in metres		*/
	*lat_dist_m = single_lat_dist_m * lat_dist_deg;
	*long_dist_m = single_long_dist_m * long_dist_deg;
	
	FOUT;
	}


static void
grp_rte_neighborhood_determine (double lat_dist_m, double long_dist_m, double quadrant_size, double* low_lat_m, 
								double* low_long_m, double* high_lat_m, double* high_long_m)
	{
	/** Determines the node's current neighborhood	**/
	FIN (grp_rte_neighborhood_determine (<args>));
	
	/* Calculate the low co-ordinates of the neighborhood in metres	*/
	*low_lat_m = (floor (lat_dist_m/quadrant_size)) * quadrant_size;
	*low_long_m = (floor (long_dist_m/quadrant_size)) * quadrant_size;
	
	/* Calculate the high co-ordinates of the neighborhood in metres*/
	*high_lat_m = *low_lat_m + quadrant_size;
	*high_long_m = *low_long_m + quadrant_size;
	
	FOUT;
	}


static void
grp_rte_neighborhood_centre_determine (double lat_dist_m, double long_dist_m, double quadrant_size, 
												double* quad_lat_m, double* quad_long_m)
	{
	double		low_lat_m;
	double		low_long_m;
	
	/** Determines the centre of the quadrant	**/
	FIN (grp_rte_neighborhood_centre_determine (<args>));
	
	/* Calculate the low co-ordinates of the neighborhood in metres	*/
	low_lat_m = (floor (lat_dist_m/quadrant_size)) * quadrant_size;
	low_long_m = (floor (long_dist_m/quadrant_size)) * quadrant_size;
	
	/* Calculate the centre coordinates of the quadrant	*/
	*quad_lat_m = low_lat_m + (quadrant_size/2.0);
	*quad_long_m = low_long_m + (quadrant_size/2.0);
	
	FOUT;
	}


static void
grp_rte_neighborhood_to_flood_determine (double old_lat_m, double old_long_m, double new_lat_m, 
										double new_long_m,	double* flood_low_lat_m, double* flood_low_long_m, 
										double* flood_high_lat_m,	double* flood_high_long_m)
	{
	double		common_size;
	
	/** Determines the block of neighborhood to flood based	**/
	/** on the old and new co-ordinates of the node. It		**/
	/** returns the lat, long values of the quadrant 2 flood**/
	FIN (grp_rte_neighborhood_to_flood_determine (<args>));

	/* First determine the common level where the two	*/
	/* positions fall into the same quadrant.			*/
	common_size = grp_rte_common_quadrant_determine (old_lat_m, old_long_m, new_lat_m, new_long_m);
	
	grp_rte_neighborhood_determine (old_lat_m, old_long_m, common_size, flood_low_lat_m, flood_low_long_m,
										flood_high_lat_m, flood_high_long_m);
	
	FOUT;
	}


static int
grp_rte_common_quadrant_determine (double old_lat_m, double old_long_m, double new_lat_m, double new_long_m)
	{
	double		common_size;
	int			level = 0;
	double		x1, y1, x2, y2;
	
	/** Determines the common quadrant for both	**/
	/** input positions on the node				**/
	FIN (grp_rte_common_quadrant_determine (<args>));
	
	/* Initialize the common neighborhood size	*/
	common_size = neighborhood_size;
	
	/* First determine the common level where the two	*/
	/* positions fall into the same quadrant.			*/
	
	do {
	/* Increment the level for each loop	*/
	/* to find the common level.			*/
	level = level + 1;
	
	/* Double the neighborhood size for		*/
	/* each higher level.					*/
	if (level > 1)
		common_size = common_size * 2.0;
	
	x1 = floor (old_lat_m/common_size);
	y1 = floor (old_long_m/common_size);
	
	x2 = floor (new_lat_m/common_size);
	y2 = floor (new_long_m/common_size);
	
	} while ((x1 != x2) || (y1 != y2));
	
	FRET (common_size);
	}


static double
grp_rte_highest_neighbor_quadrant_determine (double dest_lat_m, double dest_long_m, double* quad_lat_m_ptr, 
												double* quad_long_m_ptr)
	{
	double			common_level_quad_size;
	double			highest_nbr_quad_size;
	
	/** Determines the highest neighborhood quadrant	**/
	/** that a destination node lies in.				**/
	FIN (grp_rte_highest_neighbor_quadrant_determine (<args>));
	
	/* Determine the highest common level	*/
	common_level_quad_size = grp_rte_common_quadrant_determine (node_lat_m, node_long_m, dest_lat_m, dest_long_m);
	
	/* If the node is in the same lowest level quadrant	*/
	/* then this function is not required to be called	*/
	if (common_level_quad_size == neighborhood_size)
		grp_rte_error ("Nodes are in the same quadrant", OPC_NIL, OPC_NIL);
	
	/* The highest neighbor quadrant would	*/
	/* one less than the common quadrant	*/
	highest_nbr_quad_size = common_level_quad_size/2.0;
	
	/* Determine the centre coordinates of the quadrant	*/
	grp_rte_neighborhood_centre_determine (dest_lat_m, dest_long_m, highest_nbr_quad_size, quad_lat_m_ptr, quad_long_m_ptr);
	
	FRET (highest_nbr_quad_size);
	}


static Packet*
grp_rte_ip_datagram_decapsulate (Packet* ip_pkptr, Packet* grp_pkptr)
	{
	Packet*					app_pkptr = OPC_NIL;
	OpT_Packet_Size			app_pkt_size = 0;
	IpT_Dgram_Fields* 		ip_dgram_fd_ptr = OPC_NIL;
	int						next_header;
	
	/** Decapsulates the DSR packet from the 	**/
	/** IP datagram and returns the DSR packet	**/
	FIN (grp_rte_ip_datagram_decapsulate (<args>));
	
	/* Remove the GRP packet from the IP packet	*/
	/* op_pk_nfd_get (ip_pkptr, "data", &grp_pkptr); */
	
	/* Remove the application packet from the GRP packet	*/
	/* if it is set in the packet							*/
	if (op_pk_nfd_is_set (grp_pkptr, "data") == OPC_TRUE)
		{
		op_pk_nfd_get (grp_pkptr, "data", &app_pkptr);
		}
	else
		{
		FRET (OPC_NIL);
		}
		
	if (app_pkptr == OPC_NIL)
		{
		app_pkt_size = 0;
		}
	else
		{
		/* Get the size of the application packet	*/
		app_pkt_size = op_pk_total_size_get (app_pkptr);
	
		/* Set the application packet in the IP datagram	*/
		op_pk_nfd_set (ip_pkptr, "data", app_pkptr);
		}
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data.						*/
	op_pk_bulk_size_set (ip_pkptr, app_pkt_size);
	
	/* Get the protocol type from the GRP packet	*/
	op_pk_nfd_access (grp_pkptr, "Next Header", &next_header);
	
	/* Access the fields of the IP datagram	*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* Set the protocol type in the IP fields	*/
	ip_dgram_fd_ptr->protocol = next_header;
	
	FRET (ip_pkptr);
	}


static Packet*
grp_rte_ip_datagram_create (Packet* grp_pkptr, InetT_Address dest_addr, InetT_Address next_hop_addr)
	{
	Packet*					ip_pkptr = OPC_NIL;
	IpT_Dgram_Fields*		ip_dgram_fd_ptr = OPC_NIL;
	InetT_Address			src_address;
	OpT_Packet_Size			grp_pkt_size;
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	int						outstrm;
	short					output_intf_index;
	InetT_Address_Range* 	inet_addr_range_ptr;
	
	/** Encapsulates the GRP packet in an 	**/
	/** IP datagram and sets the fields		**/
	FIN (grp_rte_ip_datagram_create (<args>));
	
	/* Get the size of the GRP packet	*/
	grp_pkt_size = op_pk_total_size_get (grp_pkptr);
	
	/* Create an IP datagram.	*/
	ip_pkptr = op_pk_create_fmt ("ip_dgram_v4");

	/* Set the GRP packet in the data field of the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data.						*/
	op_pk_bulk_size_set (ip_pkptr, grp_pkt_size);

	/* Create fields data structure that contains orig_len,	*/
	/* ident, frag_len, ttl, src_addr, dest_addr, frag,		*/
	/* connection class, src and dest internal addresses.	*/
	ip_dgram_fd_ptr = ip_dgram_fdstruct_create ();

	/* Set the destination address for this IP datagram.	*/
	ip_dgram_fd_ptr->dest_addr = inet_address_copy (dest_addr);
	ip_dgram_fd_ptr->connection_class = 0;
	
	/* Set the correct protocol in the IP datagram.	*/
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Grp;

	/* Set the packet size-related fields of the IP datagram.	*/
	ip_dgram_fd_ptr->orig_len = grp_pkt_size/8;
	ip_dgram_fd_ptr->frag_len = ip_dgram_fd_ptr->orig_len;
	ip_dgram_fd_ptr->original_size = 160 + grp_pkt_size;

	/* Indicate that the packet is not yet fragmented.	*/
	ip_dgram_fd_ptr->frag = 0;

	/* Set the type of service.	*/
	ip_dgram_fd_ptr->tos = 0;
	
	/* Also set the compression method and original size fields	*/
	ip_dgram_fd_ptr->compression_method = IpC_No_Compression;

	/* The record route options has not been set.	*/
	ip_dgram_fd_ptr->options_field_set = OPC_FALSE;
	
	if (((inet_address_family_get (&dest_addr) == InetC_Addr_Family_v4) && 
		(inet_address_equal (dest_addr, InetI_Broadcast_v4_Addr) == OPC_FALSE)) ||
		((inet_address_family_get (&dest_addr) == InetC_Addr_Family_v6) && 
		(inet_address_equal (dest_addr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr) == OPC_FALSE)))
		{
		if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &output_intf_index,
			&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
			{
			
			/* Get the outgoing interface address	*/
			src_address = inet_address_range_addr_get (inet_addr_range_ptr);
			
			/* Set the source address */
			ip_dgram_fd_ptr->src_addr = inet_address_copy (src_address);
			}
		}
	
	/*	Set the fields structure inside the ip datagram	*/
	ip_dgram_fields_set (ip_pkptr, ip_dgram_fd_ptr);

	FRET (ip_pkptr);
	}


static Packet*
grp_rte_ip_datagram_encapsulate (Packet* ip_pkptr, Packet* grp_pkptr, InetT_Address next_hop_addr)
	{
	Packet*					app_pkptr = OPC_NIL;
	OpT_Packet_Size			app_pkt_size;
	int						fd_data_index;
	OpT_Packet_Size			grp_pkt_size;
	IpT_Dgram_Fields* 		ip_dgram_fd_ptr = OPC_NIL;
	OpT_Packet_Size			grp_header_size;
	
	/** Encapsultes the GRP packet an the existing	**/
	/** IP packet that is passed into the function	**/
	/** The encapsulated IP datagram is returned	**/
	FIN (grp_rte_ip_datagram_encapsulate (<args>));
	
	/* Access the fields of the IP datagram	*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* Remove the application packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &app_pkptr);
	
	/* Get GRP header size. This represents increment in original IP packet's payload	*/
	grp_header_size = op_pk_total_size_get (grp_pkptr);

	if (app_pkptr != OPC_NIL)
		{
		/* Get the size of the application packet	*/
		app_pkt_size = op_pk_total_size_get (app_pkptr);
	
		/* Get the field index for the data in the GRP packet	*/
		fd_data_index = op_pk_nfd_name_to_index (grp_pkptr, "data");
	
		/* Set the application packet in the GRP packet	*/
		op_pk_fd_set (grp_pkptr, fd_data_index, OPC_FIELD_TYPE_PACKET, app_pkptr, app_pkt_size);
		}
	
	/* Get the size of the GRP packet	*/
	grp_pkt_size = op_pk_total_size_get (grp_pkptr);
	
	/* Set the GRP packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", grp_pkptr);
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data.						*/
	op_pk_bulk_size_set (ip_pkptr, grp_pkt_size);
	
	/* Set the protocol type and next hop of the IP datagram 	*/
	ip_dgram_fd_ptr->next_addr = inet_address_copy (next_hop_addr);
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Grp;
	
	/* IP fragment and original sizes have increased due to added GRP header as IP payload	*/
	ip_dgram_fd_ptr->frag_len = (OpT_Packet_Size) (grp_pkt_size/8);
	ip_dgram_fd_ptr->orig_len += (OpT_Packet_Size) (grp_header_size/8);
	ip_dgram_fd_ptr->original_size += (OpT_Packet_Size) (grp_header_size/8);
	
	FRET (ip_pkptr);
	}


static ManetT_Nexthop_Info*
grp_rte_next_hop_info_create (InetT_Address next_hop_addr)
	{
	ManetT_Nexthop_Info*	nexthop_info_ptr;
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	int						outstrm;
	short					output_intf_index;
	InetT_Address_Range* 	inet_addr_range_ptr;
	
	/** Creates and populates the next hop	**/
	/** information structure used in IP	**/
	FIN (grp_rte_next_hop_info_create (<args>));
	
	/* Allocate memory for manet_nexthop_info_ptr */
	nexthop_info_ptr = (ManetT_Nexthop_Info*) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &output_intf_index,
			&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
			{
			nexthop_info_ptr->next_hop_addr = inet_address_copy (next_hop_addr);
			nexthop_info_ptr->output_table_index = (int) output_intf_index;
			nexthop_info_ptr->interface_info_ptr = iface_info_ptr;
			}
	
	FRET (nexthop_info_ptr);
	}


static void
grp_rte_position_request_timer_schedule (void)
	{
	/** Schedules the position request timer	**/
	FIN (grp_rte_position_request_timer_schedule (void));
	
	/* Schedule a self interrupt for the position request timer	*/
	op_intrpt_schedule_self (op_sim_time () + pos_req_timer, GRPC_POS_REQ_EXPIRY);
	
	FOUT;
	}


static void
grp_rte_address_list_clear (List* addr_lptr)
	{
	int					count = 0;
	int					size;
	InetT_Address*		addr_ptr;
	
	/** Frees the list of IP address pointers	**/
	FIN (grp_rte_address_list_clear (<args>));
	
	size = op_prg_list_size (addr_lptr);
	while (count < size)
		{
		addr_ptr = (InetT_Address*) op_prg_list_remove (addr_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (addr_ptr);
		count++;
		}
	
	FOUT;
	}


static GrpT_Backtrack_Info*
grp_rte_backtrack_info_mem_alloc (void)
	{
	static Pmohandle		backtrack_pmh;
	GrpT_Backtrack_Info*	backtrack_ptr = OPC_NIL;
	static Boolean			backtrack_pmh_defined = OPC_FALSE;
	
	/** Allocates the backtrack info	**/
	FIN (grp_rte_backtrack_info_mem_alloc (<args>));
	
	if (backtrack_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for backtrack	*/
		/* table entries if not already defined			*/
		backtrack_pmh = op_prg_pmo_define ("GRP Backtrack Info", sizeof (GrpT_Backtrack_Info), 32);
		backtrack_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the flooding options from the pooled memory	*/
	backtrack_ptr = (GrpT_Backtrack_Info*) op_prg_pmo_alloc (backtrack_pmh);
	
	FRET (backtrack_ptr);
	}

static void
grp_rte_backtrack_info_mem_free (GrpT_Backtrack_Info* backtrack_ptr)
	{
	/** Frees the backtrack info	**/
	FIN (grp_rte_backtrack_info_mem_free (<args>));
	
	inet_address_destroy_dynamic (backtrack_ptr->nbr_addr_ptr);
	op_prg_mem_free (backtrack_ptr);
	
	FOUT;
	}


static void
grp_rte_error (char* str1, char* str2, char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (grp_rte_error (<args>));
	
	op_sim_end ("GRP Routing : ", str1, str2, str3);
	
	FOUT;
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
	void grp_rte (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_grp_rte_init (int * init_block_ptr);
	void _op_grp_rte_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_grp_rte_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_grp_rte_alloc (VosT_Obtype, int);
	void _op_grp_rte_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
grp_rte (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (grp_rte ());

		{
		/* Temporary Variables */
		double			max_lat_dist, max_long_dist;
		int				intrpt_type = -1;
		int				intrpt_code = -1;
		int				invoke_mode = -1;
		Prohandle		invoker_prohandle;
		List*			dest_lptr;
		double			lat_dist, long_dist;
		double			dist_moved_from_start;
		double			flood_low_lat, flood_low_long;
		double			flood_high_lat, flood_high_long;
		double			low_lat, low_long;
		double			high_lat, high_long;
		ManetT_Info*	info_ptr;
		double			jitter;
		/* End of Temporary Variables */


		FSM_ENTER ("grp_rte")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "grp_rte [init enter execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [init enter execs]", state0_enter_exec)
				{
				/* Initialize the state variables	*/
				grp_rte_sv_init ();
				
				/* Parse the attributes	*/
				grp_rte_attributes_parse ();
				
				/* Initailize the local statistics	*/
				grp_rte_local_stats_init ();
				
				/* Register the GRP Routing process as a higher layer in IP	*/
				Ip_Higher_Layer_Protocol_Register ("grp", &higher_layer_proto_id);
				
				/* Determine the starting point of the node in metres	*/
				grp_rte_node_pos_in_metres_get (&node_lat_m, &node_long_m);
				
				start_lat_m = node_lat_m;
				start_long_m = node_long_m;
				
				/* Determine the starting neighborhood the node belongs to	*/
				grp_rte_neighborhood_determine (node_lat_m, node_long_m, neighborhood_size, &quad_low_lat, &quad_low_long, &quad_high_lat, &quad_high_long);
				
				/* Enable the process model to be notified whenever the site moves	*/
				op_ima_obj_pos_notification_register (own_node_objid, GRPC_POSITION_UPDATE, OPC_IMA_NOTIFY_ONLY_WHEN_CHANGED);
				
				/* Schedule a self interrupt to move to the next state. Adding		*/
				/* jitter between 0 and 5 secs before sending out flooding packets 	*/
				jitter = op_dist_uniform ((double) (5.0));
				op_intrpt_schedule_self (op_sim_time () + jitter, GRPC_INIT_FLOOD);
				
				/* Set the status to be incomplete	*/
				status = GrpC_Initialization_Incomplete;
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "grp_rte [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (4, state4_enter_exec, ;, "default", "", "init", "wait_for_flood", "tr_22", "grp_rte [init -> wait_for_flood : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "grp_rte [wait enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"grp_rte")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "grp_rte [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [wait exit execs]", state1_exit_exec)
				{
				invoker_prohandle = op_pro_invoker (own_prohandle, &invoke_mode);
				if (invoke_mode == OPC_PROINV_DIRECT)
					{
					/* The process was invoked directly by the kernel.	*/
					intrpt_type = op_intrpt_type ();
					intrpt_code = op_intrpt_code ();
					}
				else
					{
					/* This process was invoked by the parent process	*/
					info_ptr = (ManetT_Info*) op_pro_argmem_access();
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_PROFILE_SECTION_IN ("grp_rte [wait trans conditions]", state1_trans_conds)
			FSM_INIT_COND (PKT_ARRIVAL)
			FSM_TEST_COND (POSITION_UPDATE)
			FSM_TEST_COND (HELLO_BROADCAST)
			FSM_TEST_COND (POS_REQ_EXPIRY)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("wait")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "PKT_ARRIVAL", "", "wait", "pkt_arrival", "tr_13", "grp_rte [wait -> pkt_arrival : PKT_ARRIVAL / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "POSITION_UPDATE", "", "wait", "position_update", "tr_17", "grp_rte [wait -> position_update : POSITION_UPDATE / ]")
				FSM_CASE_TRANSIT (2, 5, state5_enter_exec, ;, "HELLO_BROADCAST", "", "wait", "hello_broadcast", "tr_26", "grp_rte [wait -> hello_broadcast : HELLO_BROADCAST / ]")
				FSM_CASE_TRANSIT (3, 6, state6_enter_exec, ;, "POS_REQ_EXPIRY", "", "wait", "pos_req_expiry", "tr_30", "grp_rte [wait -> pos_req_expiry : POS_REQ_EXPIRY / ]")
				FSM_CASE_TRANSIT (4, 1, state1_enter_exec, ;, "default", "", "wait", "wait", "tr_46", "grp_rte [wait -> wait : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (pkt_arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "pkt_arrival", state2_enter_exec, "grp_rte [pkt_arrival enter execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [pkt_arrival enter execs]", state2_enter_exec)
				{
				/* A packet has arrived at this node.	*/
				/* Handle the packet appropriately based*/
				/* on the type of packet it is.			*/
				grp_rte_pkt_arrival_handle (info_ptr);
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (pkt_arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "pkt_arrival", "grp_rte [pkt_arrival exit execs]")


			/** state (pkt_arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "pkt_arrival", "wait", "tr_14", "grp_rte [pkt_arrival -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (position_update) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "position_update", state3_enter_exec, "grp_rte [position_update enter execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [position_update enter execs]", state3_enter_exec)
				{
				/* Get the position of the node in metres	*/
				grp_rte_node_pos_in_metres_get (&lat_dist, &long_dist);
				
				/* Check if the node is still in the	*/
				/* same neighborhood or has moved to a	*/
				/* new neighborhood						*/
				if ((quad_low_lat <= lat_dist) && (lat_dist < quad_high_lat) && 
					(quad_low_long <= long_dist) && (long_dist < quad_high_long))
					{
					/* Node is still in the same quadrant	*/
					/* Check if it has moved greater than	*/
					/* the distance beyond which the node	*/
					/* needs to flood its current position	*/
					
					/* Calculate the distance moved from start	*/
					dist_moved_from_start =  sqrt (((lat_dist - start_lat_m) * (lat_dist - start_lat_m)) + 
											((long_dist - start_long_m) * (long_dist - start_long_m)));
					
					if (dist_moved_from_start >dist_moved)
						{
						if (LTRACE_ACTIVE || LTRACE_FLOODING)
							{
							op_prg_odb_print_major ("A position update message has been received due to node movement.", 
													"The node has moved greater than the flooding distance",
													"Sending out a flooding message", OPC_NIL);
							}
						
						/* The node has moved greater than	*/
						/* the flooding distance. Send out 	*/
						/* a flood of the new position of	*/
						/* the node.						*/
						grp_rte_flooding_message_send (lat_dist, long_dist, quad_low_lat, quad_low_long, 
														quad_high_lat, quad_high_long);
						
						/* Set the new starting position of the node	*/
						start_lat_m = lat_dist;
						start_long_m = long_dist;
						}
					}
				else
					{
					/* The node has moved to a new quadrant	*/
					/* Check if the old position request	*/
					/* timer still exists. If so, cancel it	*/
					if (op_ev_valid (pos_req_timer_evhandle) && op_ev_pending (pos_req_timer_evhandle))
						{
						/* Cancel the timer	*/
						op_ev_cancel (pos_req_timer_evhandle);
						}
					
					if (LTRACE_ACTIVE || LTRACE_FLOODING)
						{
						op_prg_odb_print_major ("A position update message has been received due to node movement.",
												"The node has moved to a new quadrant.", "Sending out a flooding message.", OPC_NIL);
						}
					
					/* Update the backtrack statistic	*/
					op_stat_write (local_stat_handle_ptr->total_num_quadrant_change_shandle, 1.0);
					op_stat_write (global_stat_handle_ptr->total_num_quadrant_change_global_shandle, 1.0);
					
					/* Clear the old list of neighbors	*/
					grp_rte_address_list_clear (nbr_node_attempted_lptr);
					
					/* Flood the new position of the node	*/
					/* to all nodes in the same higher		*/
					/* level neighborhood (quadrant).		*/
					grp_support_neighborhood_to_flood_determine (start_lat_m, start_long_m, lat_dist, long_dist, neighborhood_size, &flood_low_lat, 
																&flood_low_long, &flood_high_lat, &flood_high_long);
					
					/* Send out a flood of the new position	*/
					/* of the node to all the concerned		*/
					/* neighborhoods that need to know		*/
					grp_rte_flooding_message_send (lat_dist, long_dist, flood_low_lat, flood_low_long, 
													flood_high_lat, flood_high_long);
					
					/* Determine the coordinates of the new	*/
					/* quadrant that the node has moved to	*/
					grp_support_neighborhood_determine (lat_dist, long_dist, neighborhood_size, &low_lat, &low_long, &high_lat, &high_long);
					
					/* Set the new quadrant of the node	*/
					quad_low_lat = low_lat;
					quad_low_long = low_long;
					quad_high_lat = high_lat;
					quad_high_long = high_long;
					
					/* Set the starting position of the		*/
					/* in this new quadrant.				*/
					start_lat_m = lat_dist;
					start_long_m = long_dist;
					
					/* Since the node has moved quadrants,	*/
					/* it needs to figure out the new 		*/
					/* destination quadrant information in	*/
					/* the destination table. Get the list	*/
					/* of nodes that need the new info		*/
					dest_lptr = grp_dest_table_dest_info_request_list_get (destination_table, lat_dist, long_dist, neighborhood_size);
					
					/* Send out a position request message	*/
					/* to the neighbor node that is in the	*/
					/* same quadrant as this node.			*/
					grp_rte_position_request_message_send (dest_lptr, low_lat, low_long, high_lat, high_long);
					
					/* Set a timer to make sure that we		*/
					/* receive a position response message	*/
					/* or schedule a reattempt later if no 	*/
					/* valid neighbor node exists.			*/
					grp_rte_position_request_timer_schedule ();
					}
				
				/* Set the new current position of the node	*/
				node_lat_m = lat_dist;
				node_long_m = long_dist;
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (position_update) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "position_update", "grp_rte [position_update exit execs]")


			/** state (position_update) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "position_update", "wait", "tr_18", "grp_rte [position_update -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait_for_flood) enter executives **/
			FSM_STATE_ENTER_UNFORCED (4, "wait_for_flood", state4_enter_exec, "grp_rte [wait_for_flood enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (9,"grp_rte")


			/** state (wait_for_flood) exit executives **/
			FSM_STATE_EXIT_UNFORCED (4, "wait_for_flood", "grp_rte [wait_for_flood exit execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [wait_for_flood exit execs]", state4_exit_exec)
				{
				invoker_prohandle = op_pro_invoker (own_prohandle, &invoke_mode);
				if (invoke_mode == OPC_PROINV_DIRECT)
					{
					/* The process was invoked directly by the kernel.	*/
					intrpt_type = op_intrpt_type ();
					intrpt_code = op_intrpt_code ();
					
					if ((intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == GRPC_INIT_FLOOD))
						{
						/* Perform initial flooding so that it	*/
						/* bootstraps to reach steady state.	*/
				
						/* Calculate the maximum distance	*/
						max_lat_dist = prg_geo_lat_long_distance_get (-90, 0.0, 0.0, 90, 0.0, 0.0);
						max_long_dist = 2.0 * prg_geo_lat_long_distance_get (0.0, -180.0, 0.0, 0.0, 0.0, 0.0);
				
						/* Broadcast a flooding message to all nodes in the network	*/
						grp_rte_flooding_message_send (node_lat_m, node_long_m, 0.0, 0.0, max_lat_dist, max_long_dist);
						
						/* Increment the number of completed floods	*/
						num_init_floods += 1;
						
						/* Get the jitter value to schedule the next hello or flood	*/
						jitter = op_dist_uniform (5.0);
						
						if (num_init_floods == num_init_flood_attempts)
							{
							/* Schedule a self interrupt to move to the next state. Adding	*/
							/* jitter between 0 and 5 before sending out hello packets 		*/
							op_intrpt_schedule_self ((num_init_floods * 5.0) + jitter, GRPC_INIT_HELLO);
							status = GrpC_Flooding_Complete;
							}
						else
							{
							/* Schedule another round of initial flooding	*/
							op_intrpt_schedule_self ((num_init_floods * 5.0) + jitter, GRPC_INIT_FLOOD);
							}
						}
					else
						{
						/* Received an invalid interrupt	*/
						grp_rte_error ("Received an invalid interrupt at state wait_for_flood", OPC_NIL, OPC_NIL);
						}
					}
				else
					{
					/* This process was invoked by another process	*/
					info_ptr = (ManetT_Info*) op_pro_argmem_access();
				
					if (info_ptr->code == MANETC_PACKET_ARRIVAL)
						{
						/* A packet has arrived at this GRP process	*/
						grp_rte_pkt_arrival_handle (info_ptr);
						}
					else if (info_ptr->code == MANETC_NODE_MOVEMENT)
						{
						/* This is an indication of node movement	*/
						/* Update the position of the node			*/
						grp_rte_node_pos_in_metres_get (&node_lat_m, &node_long_m);
				
						/* Check if the node is still in the	*/
						/* same neighborhood or has moved to a	*/
						/* new neighborhood						*/
						if ((quad_low_lat <= node_lat_m) && (node_lat_m < quad_high_lat) && 
							(quad_low_long <= node_long_m) && (node_long_m < quad_high_long))
							{
							/* Node is still in the same quadrant	*/
							/* Do nothing till initial flooding		*/
							}
						else
							{
							/* The node has moved to a new quadrant	*/
							/* Determine the coordinates of the new	*/
							/* quadrant that the node has moved to	*/
							grp_support_neighborhood_determine (node_lat_m, node_long_m, neighborhood_size, &low_lat, &low_long, &high_lat, &high_long);
				
							/* Set the new quadrant of the node	*/
							quad_low_lat = low_lat;
							quad_low_long = low_long;
							quad_high_lat = high_lat;
							quad_high_long = high_long;
							}
						}
					else
						{
						/* Received an invalid interrupt	*/
						grp_rte_error ("Received an invalid interrupt at state wait_for_flood", OPC_NIL, OPC_NIL);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state4_exit_exec)


			/** state (wait_for_flood) transition processing **/
			FSM_PROFILE_SECTION_IN ("grp_rte [wait_for_flood trans conditions]", state4_trans_conds)
			FSM_INIT_COND (FLOOD_COMPLETE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("wait_for_flood")
			FSM_PROFILE_SECTION_OUT (state4_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 7, state7_enter_exec, ;, "FLOOD_COMPLETE", "", "wait_for_flood", "wait_for_hello", "tr_44", "grp_rte [wait_for_flood -> wait_for_hello : FLOOD_COMPLETE / ]")
				FSM_CASE_TRANSIT (1, 4, state4_enter_exec, ;, "default", "", "wait_for_flood", "wait_for_flood", "tr_42", "grp_rte [wait_for_flood -> wait_for_flood : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (hello_broadcast) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "hello_broadcast", state5_enter_exec, "grp_rte [hello_broadcast enter execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [hello_broadcast enter execs]", state5_enter_exec)
				{
				/* Broadcast a Hello request to all neighbor nodes	*/
				grp_rte_hello_request_send ();
				
				/* Schedule the next periodic Hello	*/
				grp_rte_hello_schedule ();
				}
				FSM_PROFILE_SECTION_OUT (state5_enter_exec)

			/** state (hello_broadcast) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "hello_broadcast", "grp_rte [hello_broadcast exit execs]")


			/** state (hello_broadcast) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "hello_broadcast", "wait", "tr_27", "grp_rte [hello_broadcast -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (pos_req_expiry) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "pos_req_expiry", state6_enter_exec, "grp_rte [pos_req_expiry enter execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [pos_req_expiry enter execs]", state6_enter_exec)
				{
				/* The position request timer has expired	*/
				/* Resend the position request message		*/
				
				if (LTRACE_ACTIVE)
					{
					op_prg_odb_print_major (pid_string, "The position request timer has expired.", OPC_NIL);
					}
				
				/* Find the list of destinations that we	*/
				/* are still waiting on the position info	*/
				dest_lptr = grp_dest_table_invalid_dest_list_get (destination_table);
				
				if (op_prg_list_size (dest_lptr) > 0)
					{
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major ("There still exists invalid destinations.", 
												"Sending out a new position request message.", OPC_NIL);
						}
					
					/* Send out a position request message to 	*/
					/* the neighbor node that is in the same	*/
					/* quadrant as this node.					*/
					grp_rte_position_request_message_send (dest_lptr, quad_low_lat, quad_low_long, quad_high_lat, quad_high_long);
				
					/* Schedule the position request timer for	*/
					/* receiving a position response message	*/
					/* or schedule a reattempt later if no 		*/
					/* valid neighbor node exists.				*/
					grp_rte_position_request_timer_schedule ();
					}
				else
					{
					/* Free the list	*/
					op_prg_mem_free (dest_lptr);
					}
				}
				FSM_PROFILE_SECTION_OUT (state6_enter_exec)

			/** state (pos_req_expiry) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "pos_req_expiry", "grp_rte [pos_req_expiry exit execs]")


			/** state (pos_req_expiry) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "pos_req_expiry", "wait", "tr_31", "grp_rte [pos_req_expiry -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait_for_hello) enter executives **/
			FSM_STATE_ENTER_UNFORCED (7, "wait_for_hello", state7_enter_exec, "grp_rte [wait_for_hello enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (15,"grp_rte")


			/** state (wait_for_hello) exit executives **/
			FSM_STATE_EXIT_UNFORCED (7, "wait_for_hello", "grp_rte [wait_for_hello exit execs]")
				FSM_PROFILE_SECTION_IN ("grp_rte [wait_for_hello exit execs]", state7_exit_exec)
				{
				invoker_prohandle = op_pro_invoker (own_prohandle, &invoke_mode);
				if (invoke_mode == OPC_PROINV_DIRECT)
					{
					/* The process was invoked directly by the kernel.	*/
					intrpt_type = op_intrpt_type ();
					intrpt_code = op_intrpt_code ();
					
					if ((intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == GRPC_INIT_HELLO))
						{
						/* Initialization of the Hello process			*/
						/* so that it bootstraps to reach steady state.	*/
				
						/* Broadcast a Hello request to all neighbor nodes	*/
						grp_rte_hello_request_send ();
				
						/* Schedule the next periodic Hello	*/
						grp_rte_hello_schedule ();
						
						status = GrpC_Hello_Complete;
						}
					else
						{
						/* Received an invalid interrupt	*/
						grp_rte_error ("Received an invalid interrupt at state wait_for_flood", OPC_NIL, OPC_NIL);
						}
					}
				else
					{
					/* This process was invoked by another process	*/
					info_ptr = (ManetT_Info*) op_pro_argmem_access();
				
					if (info_ptr->code == MANETC_PACKET_ARRIVAL)
						{
						/* A packet has arrived at this GRP process	*/
						grp_rte_pkt_arrival_handle (info_ptr);
						}
					else if (info_ptr->code == MANETC_NODE_MOVEMENT)
						{
						/* This is an indication of node movement	*/
						/* Update the position of the node			*/
						grp_rte_node_pos_in_metres_get (&node_lat_m, &node_long_m);
				
						/* Check if the node is still in the	*/
						/* same neighborhood or has moved to a	*/
						/* new neighborhood						*/
						if ((quad_low_lat <= node_lat_m) && (node_lat_m < quad_high_lat) && 
							(quad_low_long <= node_long_m) && (node_long_m < quad_high_long))
							{
							/* Node is still in the same quadrant	*/
							/* Do nothing till initial flooding		*/
							}
						else
							{
							/* The node has moved to a new quadrant	*/
							/* Determine the coordinates of the new	*/
							/* quadrant that the node has moved to	*/
							grp_support_neighborhood_determine (node_lat_m, node_long_m, neighborhood_size, &low_lat, &low_long, &high_lat, &high_long);
				
							/* Set the new quadrant of the node	*/
							quad_low_lat = low_lat;
							quad_low_long = low_long;
							quad_high_lat = high_lat;
							quad_high_long = high_long;
							}
						}
					else
						{
						/* Received an invalid interrupt	*/
						grp_rte_error ("Received an invalid interrupt at state wait_for_flood", OPC_NIL, OPC_NIL);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state7_exit_exec)


			/** state (wait_for_hello) transition processing **/
			FSM_PROFILE_SECTION_IN ("grp_rte [wait_for_hello trans conditions]", state7_trans_conds)
			FSM_INIT_COND (HELLO_COMPLETE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("wait_for_hello")
			FSM_PROFILE_SECTION_OUT (state7_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ;, "HELLO_COMPLETE", "", "wait_for_hello", "wait", "tr_45", "grp_rte [wait_for_hello -> wait : HELLO_COMPLETE / ]")
				FSM_CASE_TRANSIT (1, 7, state7_enter_exec, ;, "default", "", "wait_for_hello", "wait_for_hello", "tr_43", "grp_rte [wait_for_hello -> wait_for_hello : default / ]")
				}
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"grp_rte")
		}
	}




void
_op_grp_rte_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_grp_rte_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		/* Print the destination table	*/
		grp_dest_table_print (destination_table);
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_grp_rte_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_grp_rte_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_grp_rte_svar function. */
#undef neighbor_table
#undef destination_table
#undef module_data_ptr
#undef own_mod_objid
#undef own_node_objid
#undef own_prohandle
#undef parent_prohandle
#undef own_pro_id
#undef parent_pro_id
#undef pid_string
#undef grp_proto_id
#undef subnet_mask
#undef subnet_mask_ipv6
#undef nbr_expiry_time
#undef dist_moved
#undef neighborhood_size
#undef higher_layer_proto_id
#undef quad_low_lat
#undef quad_low_long
#undef quad_high_lat
#undef quad_high_long
#undef node_lat_m
#undef node_long_m
#undef start_lat_m
#undef start_long_m
#undef pos_req_timer_evhandle
#undef nbr_node_attempted_lptr
#undef pos_req_timer
#undef local_stat_handle_ptr
#undef global_stat_handle_ptr
#undef global_routes_export
#undef node_routes_export
#undef hello_interval_dist_ptr
#undef status
#undef backtrack_table
#undef backtrack_option
#undef global_record_routes
#undef single_lat_dist_m
#undef single_long_dist_m
#undef backtrack_hold_time
#undef num_init_floods
#undef num_init_flood_attempts

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_grp_rte_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_grp_rte_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (grp_rte)",
		sizeof (grp_rte_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_grp_rte_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	grp_rte_state * ptr;
	FIN_MT (_op_grp_rte_alloc (obtype))

	ptr = (grp_rte_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "grp_rte [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_grp_rte_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	grp_rte_state		*prs_ptr;

	FIN_MT (_op_grp_rte_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (grp_rte_state *)gen_ptr;

	if (strcmp ("neighbor_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighbor_table);
		FOUT
		}
	if (strcmp ("destination_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->destination_table);
		FOUT
		}
	if (strcmp ("module_data_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->module_data_ptr);
		FOUT
		}
	if (strcmp ("own_mod_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_mod_objid);
		FOUT
		}
	if (strcmp ("own_node_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_node_objid);
		FOUT
		}
	if (strcmp ("own_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_prohandle);
		FOUT
		}
	if (strcmp ("parent_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->parent_prohandle);
		FOUT
		}
	if (strcmp ("own_pro_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_pro_id);
		FOUT
		}
	if (strcmp ("parent_pro_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->parent_pro_id);
		FOUT
		}
	if (strcmp ("pid_string" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pid_string);
		FOUT
		}
	if (strcmp ("grp_proto_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->grp_proto_id);
		FOUT
		}
	if (strcmp ("subnet_mask" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->subnet_mask);
		FOUT
		}
	if (strcmp ("subnet_mask_ipv6" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->subnet_mask_ipv6);
		FOUT
		}
	if (strcmp ("nbr_expiry_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->nbr_expiry_time);
		FOUT
		}
	if (strcmp ("dist_moved" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dist_moved);
		FOUT
		}
	if (strcmp ("neighborhood_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighborhood_size);
		FOUT
		}
	if (strcmp ("higher_layer_proto_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->higher_layer_proto_id);
		FOUT
		}
	if (strcmp ("quad_low_lat" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->quad_low_lat);
		FOUT
		}
	if (strcmp ("quad_low_long" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->quad_low_long);
		FOUT
		}
	if (strcmp ("quad_high_lat" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->quad_high_lat);
		FOUT
		}
	if (strcmp ("quad_high_long" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->quad_high_long);
		FOUT
		}
	if (strcmp ("node_lat_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_lat_m);
		FOUT
		}
	if (strcmp ("node_long_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_long_m);
		FOUT
		}
	if (strcmp ("start_lat_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->start_lat_m);
		FOUT
		}
	if (strcmp ("start_long_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->start_long_m);
		FOUT
		}
	if (strcmp ("pos_req_timer_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pos_req_timer_evhandle);
		FOUT
		}
	if (strcmp ("nbr_node_attempted_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->nbr_node_attempted_lptr);
		FOUT
		}
	if (strcmp ("pos_req_timer" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pos_req_timer);
		FOUT
		}
	if (strcmp ("local_stat_handle_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->local_stat_handle_ptr);
		FOUT
		}
	if (strcmp ("global_stat_handle_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_stat_handle_ptr);
		FOUT
		}
	if (strcmp ("global_routes_export" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_routes_export);
		FOUT
		}
	if (strcmp ("node_routes_export" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_routes_export);
		FOUT
		}
	if (strcmp ("hello_interval_dist_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_interval_dist_ptr);
		FOUT
		}
	if (strcmp ("status" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->status);
		FOUT
		}
	if (strcmp ("backtrack_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backtrack_table);
		FOUT
		}
	if (strcmp ("backtrack_option" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backtrack_option);
		FOUT
		}
	if (strcmp ("global_record_routes" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_record_routes);
		FOUT
		}
	if (strcmp ("single_lat_dist_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->single_lat_dist_m);
		FOUT
		}
	if (strcmp ("single_long_dist_m" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->single_long_dist_m);
		FOUT
		}
	if (strcmp ("backtrack_hold_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backtrack_hold_time);
		FOUT
		}
	if (strcmp ("num_init_floods" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_init_floods);
		FOUT
		}
	if (strcmp ("num_init_flood_attempts" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_init_flood_attempts);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

