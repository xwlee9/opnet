/* Process model C form file: aodv_rte.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char aodv_rte_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EED6 5C91EED6 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/* Includes	*/
#include <ip_addr_v4.h>
#include <ip_rte_support.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <ip_higher_layer_proto_reg_sup.h>
#include <ip_cmn_rte_table.h>
#include <oms_dist_support.h>
#include <manet.h>
#include <aodv.h>
#include <aodv_pkt_support.h>
#include <aodv_ptypes.h>
#include <oms_pr.h>


#define PACKET_ARRIVAL  		(invoke_mode == OPC_PROINV_INDIRECT)
#define HELLO_TIMER_EXPIRY		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == AODVC_HELLO_TIMER_EXPIRY))

/* Traces	*/
#define LTRACE_ACTIVE			(op_prg_odb_ltrace_active ("aodv_rte") || \
								op_prg_odb_ltrace_active ("manet"))


/* Prototypes	*/
static void						aodv_rte_sv_init (void);
static void						aodv_rte_local_stats_reg (void);
static void						aodv_rte_attributes_parse_buffers_create (void);
static void						aodv_rte_add_directly_connected_routes (void);
static void						aodv_rte_pkt_arrival_handle (void);
static void						aodv_rte_app_pkt_arrival_handle (Packet*, IpT_Dgram_Fields*, Boolean);
static void						aodv_rte_rreq_pkt_arrival_handle (Packet*, Packet*, IpT_Dgram_Fields*, 
														IpT_Rte_Ind_Ici_Fields*, AodvT_Packet_Option*);
static void						aodv_rte_rrep_pkt_arrival_handle (Packet*, Packet*, IpT_Dgram_Fields*, 
														IpT_Rte_Ind_Ici_Fields*, AodvT_Packet_Option*);
static void						aodv_rte_rrep_hello_pkt_arrival_handle (Packet*, Packet*, IpT_Dgram_Fields*, 
														IpT_Rte_Ind_Ici_Fields*, AodvT_Packet_Option*);
static void						aodv_rte_rerr_pkt_arrival_handle (Packet*, Packet*, IpT_Dgram_Fields*, 
														IpT_Rte_Ind_Ici_Fields*, AodvT_Packet_Option*);
static void						aodv_rte_data_routes_expiry_time_update (Packet* dgram_pkptr);
static void						aodv_rte_route_table_entry_update (IpT_Dgram_Fields*, IpT_Rte_Ind_Ici_Fields*, 
														AodvT_Packet_Option*);
static void						aodv_rte_route_table_entry_from_hello_update (IpT_Dgram_Fields*, AodvT_Packet_Option*);
static void						aodv_rte_route_request_send (AodvT_Route_Entry*, InetT_Address, int, double, int);
static void						aodv_rte_route_reply_send (AodvT_Rreq*, IpT_Dgram_Fields*, Boolean, AodvT_Route_Entry*, AodvT_Route_Entry*);
static void						aodv_rte_grat_route_reply_send (AodvT_Rreq*, AodvT_Route_Entry*, AodvT_Route_Entry*);
static void						aodv_rte_route_error_process (InetT_Address, AodvT_Route_Entry*, List*, AodvC_Rerr_Process, Boolean);
static void						aodv_rte_all_pkts_to_dest_send (InetT_Address, AodvT_Route_Entry*, Boolean);
static void						aodv_rte_rrep_hello_message_send (void);
static Compcode					aodv_rte_local_repair_attempt (InetT_Address, AodvT_Route_Entry*);
static Boolean					aodv_rte_local_repair_exists (InetT_Address, Boolean);
static void						aodv_rte_neighbor_connectivity_table_update (InetT_Address, Boolean);
static Packet*					aodv_rte_ip_datagram_create (Packet*, InetT_Address, AodvT_Route_Entry*, int, InetT_Address, ManetT_Nexthop_Info*);
static double					aodv_rte_max_find (double, double);
static void						aodv_rte_error (const char*, const char*, const char*);

static void						aodv_rte_ext_route_error_send (InetT_Address unreachable_node_addr, int dest_seq_num);

extern void						aodv_rte_rreq_timer_expiry_handle (void*, int);
extern void						aodv_rte_entry_expiry_handle(void*, int);
extern void						aodv_rte_forward_request_delete (void* addr_str, int req_id);	



EXTERN_C_BEGIN
static void						aodv_rte_neighbor_conn_loss_handle (void*, int);
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
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;	/* Module Data Memory for the IP module  */
	Objid	                  		own_mod_objid                                   ;	/* Objid of this module  */
	Objid	                  		own_node_objid                                  ;	/* Objid of this node  */
	Prohandle	              		own_prohandle                                   ;	/* Own process handle  */
	Prohandle	              		parent_prohandle                                ;	/* Process handle of the parent process (manet_mgr)  */
	AodvT_Route_Table*	     		route_table_ptr                                 ;	/* Pointer to the AODV route table where the routes to all known  */
	                        		                                                	/* destinations are stored.                                       */
	AodvT_Packet_Queue*	    		pkt_queue_ptr                                   ;	/* Pointer to the packet queue where the data packets are   */
	                        		                                                	/* buffered while a route to the destination is discovered  */
	int	                    		route_request_id                                ;	/* Route request message identifier  */
	int	                    		sequence_number                                 ;	/* Node's sequence number  */
	OmsT_Dist_Handle	       		hello_interval_dist_ptr                         ;	/* This attribute is used to calculate the time before which it       */
	                        		                                                	/* determines that the neighbor connectivity is lost. If a node does  */
	                        		                                                	/* not receive and packets (Hello messages or any other packet),      */
	                        		                                                	/* from the neighbor within the last                                  */
	                        		                                                	/* "Allowed Hello Loss * Hello Interval", the node assumes that the   */
	                        		                                                	/* link to the neighbor is currently lost                             */
	int	                    		net_diameter                                    ;	/* This attribute measures the maximum possible number of hops  */
	                        		                                                	/* between two nodes in the network.                            */
	                        		                                                	/*                                                              */
	                        		                                                	/* If the TTL value set in the route request reaches the TTL    */
	                        		                                                	/* Threshold, the TTL is then set to the Net Diameter for each  */
	                        		                                                	/* attempt.                                                     */
	double	                 		node_traversal_time                             ;	/* The Node Traversal Time is a conservative estimate of the       */
	                        		                                                	/* average one hop traversal time for packets and should include   */
	                        		                                                	/* queuing delays, interrupt processing times and transfer times.  */
	double	                 		last_route_error_sent_time                      ;	/* The time at which the last route error was sent. This is to  */
	                        		                                                	/* implement the rate at which the route errors are sent out.   */
	int	                    		num_route_errors_sent                           ;	/* The number of route errors that have been sent out within the  */
	                        		                                                	/* last one second.                                               */
	int	                    		route_error_rate                                ;	/* Maximum number of route errors that can be sent out in one second  */
	double	                 		last_route_request_sent_time                    ;	/* The time at which the last route request was sent. This is to  */
	                        		                                                	/* implement the rate at which the route requests are sent out.   */
	int	                    		num_route_requests_sent                         ;	/* The number of route requests that have been sent out within the  */
	                        		                                                	/* last one second.                                                 */
	int	                    		route_request_rate                              ;	/* Maximum number of route requests that can be sent out in one second  */
	int	                    		timeout_buffer                                  ;	/* This attribute provides a buffer for the timeout for receiving  */
	                        		                                                	/* a route reply for a route request sent out. If a route reply    */
	                        		                                                	/* message is delayed due to congestion, a timeout is less likely  */
	                        		                                                	/* to occur while the route reply is still en route back to the    */
	                        		                                                	/* source. To omit this buffer, set "Timeout Buffer" = 0.          */
	                        		                                                	/*                                                                 */
	                        		                                                	/* This attribute is used in calculating the ring traversal time.  */
	                        		                                                	/*                                                                 */
	                        		                                                	/* Ring Traversal Time =                                           */
	                        		                                                	/* 2 * Node Traversal Time * (TTL_Value + Timeout Buffer)          */
	int	                    		ttl_start                                       ;	/* The network-wide dissemination of route requests uses an    */
	                        		                                                	/* expanding ring search. The originating node initially uses  */
	                        		                                                	/* a TTL = "TTL Start" for the search.                         */
	int	                    		ttl_increment                                   ;	/* If the route request times out without receiving a           */
	                        		                                                	/* corresponding route reply, the originating node broadcasts   */
	                        		                                                	/* the route request again with the TTL value incremented by    */
	                        		                                                	/* "TTL Increment".                                             */
	                        		                                                	/*                                                              */
	                        		                                                	/* This increment is done upto the point the TTL value reaches  */
	                        		                                                	/* the TTL Threshold.                                           */
	int	                    		ttl_threshold                                   ;	/* The TTL value is incremented for every timeout of the route     */
	                        		                                                	/* request without receiving a valid route reply. This TTL value   */
	                        		                                                	/* is incremented upto a maximum of TTL Threshold. Beyond this     */
	                        		                                                	/* threshold, the TTL value is set to the "Net Diameter" for each  */
	                        		                                                	/* attempt.                                                        */
	int	                    		local_add_ttl                                   ;	/* When a node decides to perform a local repair, this attribute is  */
	                        		                                                	/* used to calculate the TTL value to be set on the route request    */
	                        		                                                	/* message. The value is calculated as :                             */
	                        		                                                	/*                                                                   */
	                        		                                                	/* max (MIN_REPAIR_TTL, 0.5 * #Hops) + Local_Add_TTL                 */
	                        		                                                	/*                                                                   */
	                        		                                                	/* where the "MIN_REPAIR_TTL" is the last known hop count to the     */
	                        		                                                	/* destination.                                                      */
	int	                    		max_repair_ttl                                  ;	/* Maximum TTL value to be used during route repair  */
	double	                 		my_route_timeout                                ;	/* The time for which nodes receiving the RREP consider the route  */
	                        		                                                	/* to be valid                                                     */
	double	                 		net_traversal_time                              ;	/* After a route request has been sent out, a route should be received  */
	                        		                                                	/* within the net traversal time.                                       */
	double	                 		path_discovery_time                             ;	/* The time for which a node will hold information about a route  */
	                        		                                                	/* request sent out                                               */
	AodvT_Request_Table*	   		request_table_ptr                               ;	/* Pointer to the request table  */
	AodvT_Local_Stathandles*			local_stat_handle_ptr                           ;	/* Handle to the local statistics  */
	AodvT_Global_Stathandles*			global_stat_handle_ptr                          ;	/* Handle to the global statistics  */
	Boolean	                		grat_route_reply_flag                           ;	/* Indicates whether a gratuitous route reply is required  */
	Boolean	                		dest_only_flag                                  ;	/* Indicates whether only destination nodes of route requests  */
	                        		                                                	/* can send a route reply message                              */
	Boolean	                		ack_required                                    ;	/* Indicates whether an acknowledgement is required  */
	IpT_Rte_Proc_Id	        		aodv_proto_id                                   ;	/* Unique protocol ID returned by the IP common route table  */
	int	                    		own_pro_id                                      ;	/* Own process ID  */
	int	                    		parent_pro_id                                   ;	/* Parent process ID  */
	InetT_Subnet_Mask	      		subnet_mask                                     ;	/* length 32 subnet mask for entries in the route table that are  */
	                        		                                                	/* length 32 bits                                                 */
	InetT_Subnet_Mask	      		subnet_mask_ipv6                                ;	/* length 128 subnet mask for entries in the route table that are  */
	                        		                                                	/* length 128 bits.                                                */
	double	                 		last_broadcast_sent_time                        ;	/* Time the last broadcast message was sent from this node. A       */
	                        		                                                	/* broadcast message could be eaither a hello message or a RREQ.    */
	                        		                                                	/* This is to determine whether to send out a hello message. Hello  */
	                        		                                                	/* messsages are sent out if there has been no broadcast from this  */
	                        		                                                	/* node for the last hello interval                                 */
	int	                    		allowed_hello_loss                              ;	/* Number of hello packets that can be lost before the connectivity  */
	                        		                                                	/* to the neighbor is considered to be lost.                         */
	InetT_Address_Hash_Table_Handle			neighbor_connectivity_table                     ;	/* This table maintains the neighbors to this node from which it   */
	                        		                                                	/* has received a hello packet. The table maintains the time of    */
	                        		                                                	/* the last received packet from each neighbor.                    */
	                        		                                                	/*                                                                 */
	                        		                                                	/* If no packet is received from a neighbor for more than the      */
	                        		                                                	/* ALLOWED_HELLO_LOSS * HELLO_INTERVAL, the node assumes that the  */
	                        		                                                	/* link to this neighbor is currently lost.                        */
	double	                 		delete_period                                   ;	/* Time after which a route is deleted from the route table. A  */
	                        		                                                	/* route may already be invalid before it is deleted from the   */
	                        		                                                	/* route table. This is done to keep knowledge of the old       */
	                        		                                                	/* information.                                                 */
	Boolean	                		local_repair                                    ;	/* Determines whether this node has the functionality to repair  */
	                        		                                                	/* a link break in an active route                               */
	List*	                  		local_repair_nodes_lptr                         ;	/* List of destination nodes that are currently under local repair  */
	int	                    		higher_layer_proto_id                           ;	/* Higher layer protocol ID returned by the common route table  */
	char	                   		pid_string [32]                                 ;	/* Display string  */
	InetT_Addr_Family	      		aodv_addressing_mode                            ;	/* This variable stores the address family type for this node     */
	                        		                                                	/* If node sends/receives packet that does not match the current  */
	                        		                                                	/* addressing mode (family), it will dropped. While sending out   */
	                        		                                                	/* broadcast packets, like Hello or bcast RERR, this variable    */
	                        		                                                	/* will determine the broadcast address.                    */
	Manet_Rte_Ext_Cache_Table*			external_routes_cache_table_ptr                 ;	/* This table contains the routes external hosts (non-MANET hosts). */
	} aodv_rte_state;

#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define own_mod_objid           		op_sv_ptr->own_mod_objid
#define own_node_objid          		op_sv_ptr->own_node_objid
#define own_prohandle           		op_sv_ptr->own_prohandle
#define parent_prohandle        		op_sv_ptr->parent_prohandle
#define route_table_ptr         		op_sv_ptr->route_table_ptr
#define pkt_queue_ptr           		op_sv_ptr->pkt_queue_ptr
#define route_request_id        		op_sv_ptr->route_request_id
#define sequence_number         		op_sv_ptr->sequence_number
#define hello_interval_dist_ptr 		op_sv_ptr->hello_interval_dist_ptr
#define net_diameter            		op_sv_ptr->net_diameter
#define node_traversal_time     		op_sv_ptr->node_traversal_time
#define last_route_error_sent_time		op_sv_ptr->last_route_error_sent_time
#define num_route_errors_sent   		op_sv_ptr->num_route_errors_sent
#define route_error_rate        		op_sv_ptr->route_error_rate
#define last_route_request_sent_time		op_sv_ptr->last_route_request_sent_time
#define num_route_requests_sent 		op_sv_ptr->num_route_requests_sent
#define route_request_rate      		op_sv_ptr->route_request_rate
#define timeout_buffer          		op_sv_ptr->timeout_buffer
#define ttl_start               		op_sv_ptr->ttl_start
#define ttl_increment           		op_sv_ptr->ttl_increment
#define ttl_threshold           		op_sv_ptr->ttl_threshold
#define local_add_ttl           		op_sv_ptr->local_add_ttl
#define max_repair_ttl          		op_sv_ptr->max_repair_ttl
#define my_route_timeout        		op_sv_ptr->my_route_timeout
#define net_traversal_time      		op_sv_ptr->net_traversal_time
#define path_discovery_time     		op_sv_ptr->path_discovery_time
#define request_table_ptr       		op_sv_ptr->request_table_ptr
#define local_stat_handle_ptr   		op_sv_ptr->local_stat_handle_ptr
#define global_stat_handle_ptr  		op_sv_ptr->global_stat_handle_ptr
#define grat_route_reply_flag   		op_sv_ptr->grat_route_reply_flag
#define dest_only_flag          		op_sv_ptr->dest_only_flag
#define ack_required            		op_sv_ptr->ack_required
#define aodv_proto_id           		op_sv_ptr->aodv_proto_id
#define own_pro_id              		op_sv_ptr->own_pro_id
#define parent_pro_id           		op_sv_ptr->parent_pro_id
#define subnet_mask             		op_sv_ptr->subnet_mask
#define subnet_mask_ipv6        		op_sv_ptr->subnet_mask_ipv6
#define last_broadcast_sent_time		op_sv_ptr->last_broadcast_sent_time
#define allowed_hello_loss      		op_sv_ptr->allowed_hello_loss
#define neighbor_connectivity_table		op_sv_ptr->neighbor_connectivity_table
#define delete_period           		op_sv_ptr->delete_period
#define local_repair            		op_sv_ptr->local_repair
#define local_repair_nodes_lptr 		op_sv_ptr->local_repair_nodes_lptr
#define higher_layer_proto_id   		op_sv_ptr->higher_layer_proto_id
#define pid_string              		op_sv_ptr->pid_string
#define aodv_addressing_mode    		op_sv_ptr->aodv_addressing_mode
#define external_routes_cache_table_ptr		op_sv_ptr->external_routes_cache_table_ptr

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	aodv_rte_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((aodv_rte_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
aodv_rte_sv_init (void)
	{	
	/** Initialize the state variables	**/
	FIN (aodv_rte_sv_init (void));
	
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
	sprintf (pid_string, "aodv_rte PID (%d)", own_pro_id);

	/* Initailize the identification values	*/
	route_request_id = 0;
	sequence_number = 0;
	
	/* Initailize the variables used to keep track of the rate	*/
	last_route_error_sent_time = 0.0;
	num_route_errors_sent = 0;
	last_route_request_sent_time = 0.0;
	num_route_requests_sent = 0;
	last_broadcast_sent_time = 0.0;
	
	/* Obtain a handle to the global statistics	*/
	global_stat_handle_ptr = aodv_support_global_stat_handles_obtain ();
	
	/* Assign a unique name to this AODV process.  It will include	*/
	/* the name of the routing protocol, which in this case is AODV	*/
	/* and the AS number of this process. Once the unique name is	*/
	/* obtained, this process can be registered in IPs list of all	*/
	/* routing processes.											*/
	aodv_proto_id = IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_AODV, IPC_NO_MULTIPLE_PROC);
	Ip_Cmn_Rte_Table_Install_Routing_Proc (module_data_ptr->ip_route_table, aodv_proto_id, own_prohandle);
	
	/* Create a /32 subnet mask for entries in the route table	*/
	subnet_mask = inet_smask_from_length_create (32);

	/* Create a /128 subnet mask for entries in the route table	*/
	subnet_mask_ipv6 = inet_smask_from_length_create (128);

	/* External routes are handled only by MANET gateways.		*/
	if (ip_manet_gateway_enabled(module_data_ptr))
		{
		/* Initialize the cache table used to store routes to 		*/
		/* external hosts (non-MANET hosts).						*/	
		external_routes_cache_table_ptr = manet_ext_rte_cache_table_init ();
		}

	FOUT;
	}


static void
aodv_rte_local_stats_reg (void)
	{
	/** Initializes the local statistic handles	**/
	FIN (aodv_rte_local_stats_reg (void));
	
	/* Allocate memory for the structure that stores	*/
	/* all the statistic handles						*/
	local_stat_handle_ptr = (AodvT_Local_Stathandles*) op_prg_mem_alloc (sizeof (AodvT_Local_Stathandles));
		
	/* Register each of the local statistic	*/
	local_stat_handle_ptr->route_discovery_time_shandle = op_stat_reg ("AODV.Route Discovery Time", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->pkt_queue_size_shandle = op_stat_reg ("AODV.Packet Queue Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->num_pkts_discard_shandle = op_stat_reg ("AODV.Total Packets Dropped", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->route_table_size_shandle = op_stat_reg ("AODV.Route Table Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->num_hops_shandle = op_stat_reg ("AODV.Number of Hops per Route", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_rcvd_bps_shandle = op_stat_reg ("AODV.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_rcvd_pps_shandle = op_stat_reg ("AODV.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_sent_bps_shandle = op_stat_reg ("AODV.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->rte_traf_sent_pps_shandle = op_stat_reg ("AODV.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_requests_sent_shandle = op_stat_reg ("AODV.Total Route Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_replies_sent_shandle = op_stat_reg ("AODV.Total Route Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_replies_sent_from_dest_shandle = op_stat_reg ("AODV.Total Replies Sent from Destination", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_cached_replies_sent_shandle = op_stat_reg ("AODV.Total Cached Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_route_errors_sent_shandle = op_stat_reg ("AODV.Total Route Errors Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handle_ptr->total_requests_fwd_shandle = op_stat_reg ("AODV.Total Route Requests Forwarded", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	FOUT;
	}


static void
aodv_rte_attributes_parse_buffers_create (void)
	{
	Objid				aodv_parms_id;
	Objid				aodv_parms_child_id;
	Objid				aodv_rte_disc_parms_id;
	Objid				aodv_rte_disc_parms_child_id;
	Objid				ttl_parms_id;
	Objid				ttl_parms_child_id;
	int					pkt_queue_size;
	int					route_request_retries;
	double				hello_interval;
	double				active_route_timeout;
	char				hello_interval_str [128];
	int					addressing_mode;
	
	/** This function reads in the attributes and creates	**/
	/** the buffers for the various functions.				**/
	FIN (aodv_rte_attributes_parse_buffers_create (void));
	
	/* Read the AODV Parameters	*/
	op_ima_obj_attr_get (own_mod_objid, "manet_mgr.AODV Parameters", &aodv_parms_id);
	aodv_parms_child_id = op_topo_child (aodv_parms_id, OPC_OBJTYPE_GENERIC, 0);
	
	/* Read in the route discovery parameters	*/
	op_ima_obj_attr_get (aodv_parms_child_id, "Route Discovery Parameters", &aodv_rte_disc_parms_id);
	aodv_rte_disc_parms_child_id = op_topo_child (aodv_rte_disc_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (aodv_rte_disc_parms_child_id, "Route Request Retries", &route_request_retries);
	op_ima_obj_attr_get (aodv_rte_disc_parms_child_id, "Route Request Rate Limit", &route_request_rate);
	op_ima_obj_attr_get (aodv_rte_disc_parms_child_id, "Gratuitous Route Reply Flag", &grat_route_reply_flag);
	op_ima_obj_attr_get (aodv_rte_disc_parms_child_id, "Destination Only Flag", &dest_only_flag);
	op_ima_obj_attr_get (aodv_rte_disc_parms_child_id, "Acknowledgement Required", &ack_required);
	
	/* Read in the other attributes	*/
	op_ima_obj_attr_get (aodv_parms_child_id, "Active Route Timeout", &active_route_timeout);
	op_ima_obj_attr_get (aodv_parms_child_id, "Hello Interval", &hello_interval_str);
	op_ima_obj_attr_get (aodv_parms_child_id, "Allowed Hello Loss", &allowed_hello_loss);
	op_ima_obj_attr_get (aodv_parms_child_id, "Net Diameter", &net_diameter);
	op_ima_obj_attr_get (aodv_parms_child_id, "Node Traversal Time", &node_traversal_time);
	op_ima_obj_attr_get (aodv_parms_child_id, "Route Error Rate Limit", &route_error_rate);
	op_ima_obj_attr_get (aodv_parms_child_id, "Timeout Buffer", &timeout_buffer);
	op_ima_obj_attr_get (aodv_parms_child_id, "Packet Queue Size", &pkt_queue_size);
	op_ima_obj_attr_get (aodv_parms_child_id, "Local Repair", &local_repair);
	op_ima_obj_attr_get (aodv_parms_child_id, "Addressing Mode", &addressing_mode);
	
	/* Read in the TTL parameters	*/
	op_ima_obj_attr_get (aodv_parms_child_id, "TTL Parameters", &ttl_parms_id);
	ttl_parms_child_id = op_topo_child (ttl_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (ttl_parms_child_id, "TTL Start", &ttl_start);
	op_ima_obj_attr_get (ttl_parms_child_id, "TTL Increment", &ttl_increment);
	op_ima_obj_attr_get (ttl_parms_child_id, "TTL Threshold", &ttl_threshold);
	op_ima_obj_attr_get (ttl_parms_child_id, "Local Add TTL", &local_add_ttl);
	
	/* Create the Hello interval distribution	*/
	hello_interval_dist_ptr = oms_dist_load_from_string (hello_interval_str);
	
	/* Set the addressing mode */
	aodv_addressing_mode = (addressing_mode == 1) ? InetC_Addr_Family_v4 : InetC_Addr_Family_v6;
	
	/* Calculate the rest of the state variables	*/
	max_repair_ttl =  (int) (0.3 * net_diameter);
	my_route_timeout = 2.0 * active_route_timeout;
	net_traversal_time = 2.0 * node_traversal_time * net_diameter;
	path_discovery_time = 2.0 * net_traversal_time;
	
	hello_interval = oms_dist_outcome (hello_interval_dist_ptr);
	delete_period = AODVC_DELETE_PERIOD_CONSTANT_K * aodv_rte_max_find (active_route_timeout, hello_interval);
		
	/* Schedule the first hello interrupt	*/
	op_intrpt_schedule_self (op_sim_time () + hello_interval, AODVC_HELLO_TIMER_EXPIRY);
	
	/* Initailize the various buffers	*/
	route_table_ptr = aodv_route_table_create (module_data_ptr->ip_route_table, aodv_proto_id, 
												active_route_timeout, delete_period, local_stat_handle_ptr, aodv_addressing_mode);
	
	/* Set the module memory for IP to have a handle to the	*/
	/* AODV route table. This is so that, every time a data	*/
	/* packet uses a route inserted by AODV, the expiry 	*/
	/* time of the route can be updated by IP.				*/
	module_data_ptr->manet_info_ptr->aodv_route_table_ptr = route_table_ptr;
	
	request_table_ptr = aodv_request_table_create (path_discovery_time, route_request_retries, aodv_addressing_mode);
	pkt_queue_ptr     = aodv_packet_queue_create (pkt_queue_size, local_stat_handle_ptr, aodv_addressing_mode);
	
	/* Initialize the packet queue table for local repair	*/
	if (local_repair)
		{
		/* List of destination nodes undergoing local repair	*/
		local_repair_nodes_lptr = op_prg_list_create ();
		}

	/* Create the neighbor connectivity table	*/
	if (aodv_addressing_mode == InetC_Addr_Family_v4)
		neighbor_connectivity_table = inet_addr_hash_table_create (16, 0);
	else
		neighbor_connectivity_table = inet_addr_hash_table_create (0, 16);
		
	FOUT;
	}


static void 
aodv_rte_add_directly_connected_routes (void)
	{		
	int 						i, num_interfaces = 0;
	IpT_Interface_Info* 		ith_intf_info_ptr = OPC_NIL;
	InetT_Subnet_Mask			mask;
	InetT_Address				inet_dest_addr;
	IpT_Port_Info				port_info;
	IpT_Dest_Prefix				dest_prefix;	
	
	FIN (aodv_rte_add_directly_connected_routes (void));
	
	/* Delete directly connected routes added by IP */
	/* Add its own interfaces as directly connected */
	/* routes depending upon addressing mode (v4/v6)*/
	
	if (module_data_ptr->gateway_status)
		{
		/* This is a router node */
		
		/* Get the number of valid interfaces */
		num_interfaces = inet_rte_num_interfaces_get (module_data_ptr);

		/* Clear v4/v6 route entries from IP Cmn Rte Table */	
		Inet_Cmn_Rte_Table_Clear (module_data_ptr->ip_route_table, InetC_Addr_Family_v4);
		Inet_Cmn_Rte_Table_Clear (module_data_ptr->ip_route_table, InetC_Addr_Family_v6);
				
		mask = (aodv_addressing_mode == InetC_Addr_Family_v4) ? subnet_mask : subnet_mask_ipv6;
		
		/* Loop through all intfs */
		for (i=0; i< num_interfaces; i++)
			{
			/* Get the ith intf info ptr */
			ith_intf_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, i);

			/* If intf is not shutdown */
			if (!ip_rte_intf_is_shutdown (ith_intf_info_ptr) && ip_rte_intf_manet_enabled(ith_intf_info_ptr))
				{
				/* Get the intf address */
				inet_dest_addr = inet_rte_intf_addr_get_fast (ith_intf_info_ptr, aodv_addressing_mode);
			
				if (!inet_address_equal (INETC_ADDRESS_INVALID, inet_dest_addr))
					{
					/* Create dest prefix */
					dest_prefix = ip_cmn_rte_table_dest_prefix_create (inet_dest_addr, mask);
				
					/*Get port info */
					port_info = ip_rte_port_info_from_tbl_index (module_data_ptr, i);
			
					/* Add IP Cmn Table Entry */
					Inet_Cmn_Rte_Table_Entry_Add_Options (module_data_ptr->ip_route_table, OPC_NIL, dest_prefix, 
						inet_dest_addr, port_info, 0, 
						IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IpC_Dyn_Rte_Directly_Connected, IPC_NO_MULTIPLE_PROC),
						0, IPC_CMN_RTE_TABLE_ENTRY_ADD_INDIRECT_NEXTHOP_OPTION);
					}
				}
			}
		}
	
	FOUT;
	
	}

/*********************************************************************/
/******************** PACKET ARRIVAL FUNCTIONS ***********************/
/*********************************************************************/

static void
aodv_rte_pkt_arrival_handle (void)
	{
	Packet*							ip_pkptr;
	Packet*							aodv_pkptr;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr;
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr;
	AodvT_Packet_Option*			tlv_options_ptr;
	char							addr_str [INETC_ADDR_STR_LEN];
	char							node_name [OMSC_HNAME_MAX_LEN];
	char							temp_str [256];
	ManetT_Info*					info_ptr;
	IpT_Interface_Info*				input_intf_info_ptr;		
	
	/** A packet has arrived. Handle the packet	**/
	/** appropriately based on its type			**/
	FIN (aodv_rte_pkt_arrival_handle (void));
	
	/* The process was invoked by the parent	*/
	/* MANET process indicating the arrival		*/
	/* of a packet. The packet can either be	*/
	/* 1. A higher layer application packet		*/
	/*    waiting to be transmitted when a 		*/
	/*    route is found.						*/
	/* 2. A MANET signaling/routing packet 		*/
	/*    arrival which may or may not be a		*/
	/*    broadcast packet.						*/
	
	/* Access the argument memory to get the	*/
	/* packet pointer.							*/
	
	info_ptr = (ManetT_Info*) op_pro_argmem_access();
	ip_pkptr = (Packet*) info_ptr->manet_info_type_ptr;
	

	if (ip_pkptr == OPC_NIL)
		aodv_rte_error ("Could not obtain the packet from the argument memory", OPC_NIL, OPC_NIL);

	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* First check if the aodv_addressing_mode for this node is set */
	/* if yes, then packet should only be processed if the dest 	*/
	/* address belongs to family supported at this node, else 		*/
	/* discard the packet without processing. Message will be 		*/
	/* for the same.												*/
	if (inet_address_family_get (&ip_dgram_fd_ptr->dest_addr) != aodv_addressing_mode)
		{
		/* Address family mismatch, this node does not support 	*/
		/* the address family of packet's destination address 	*/
		/* Log the message */
		ipnl_family_mismatch_pkt_drop_in_manet (op_pk_id (ip_pkptr), 
			aodv_addressing_mode, inet_address_family_get (&ip_dgram_fd_ptr->dest_addr));

		/* Destroy the received packet */
		if (ip_dgram_fd_ptr->protocol == IpC_Protocol_Aodv)
			{
			/* Destroy the AODV control option before destroying IP pkt */
			op_pk_nfd_get_pkt (ip_pkptr, "data", &aodv_pkptr);
			op_pk_destroy (aodv_pkptr);
			}
		
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Check if this is a MANET gateway.								*/	
	if (ip_manet_gateway_enabled(module_data_ptr))
		{

		/* Access the input interface information.		*/
		input_intf_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);
		}
		
	/* Check if the packet came from the application	*/
	if ((intf_ici_fdstruct_ptr->instrm == module_data_ptr->instrm_from_ip_encap) ||
		(intf_ici_fdstruct_ptr->instrm == IpC_Pk_Instrm_Child) ||
		(ip_manet_gateway_enabled(module_data_ptr) && !ip_rte_intf_manet_enabled(input_intf_info_ptr)))
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "An application packet has arrived from the higher layer to destination %s (%s).", addr_str, node_name);
			op_prg_odb_print_major (pid_string, temp_str, "No route exists to the destination.", OPC_NIL);
			}
		
		/* This is a packet that came from the	*/
		/* application (higher) layer and no 	*/
		/* route exists to the destintion of	*/
		/* this packet. Handle the arrival		*/
		aodv_rte_app_pkt_arrival_handle (ip_pkptr, ip_dgram_fd_ptr, OPC_TRUE);
		
		FOUT;
		}
	
	/* This packet did not come from the application	*/
	/* This can either be a AODV signaling packet or	*/
	/* an application packet that came from another 	*/
	/* node does not have a route from this node.		*/
	if (ip_dgram_fd_ptr->protocol != IpC_Protocol_Aodv)
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "No route exists to the destination %s (%s)", addr_str, node_name);
			op_prg_odb_print_major (pid_string, "An application packet has arrived from another node in the network.",
									temp_str, OPC_NIL);
			}
		
		/* This is an application packet from another 	*/
		/* node that does not have a route to the 	 	*/
		/* destination from this node.					*/
		aodv_rte_app_pkt_arrival_handle (ip_pkptr, ip_dgram_fd_ptr, OPC_FALSE);
		
		FOUT;
		}
	
	/* This is an AODV control packet	*/
	
	/* Get the AODV packet from the IP datagram	*/
	op_pk_nfd_get_pkt (ip_pkptr, "data", &aodv_pkptr);
	
	/* Update the statistics for the routing traffic received	*/
	aodv_support_routing_traffic_received_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, ip_pkptr);
	
	/* Get the option in the AODV packet	*/
	op_pk_nfd_access (aodv_pkptr, "Options", &tlv_options_ptr);
	
	/* This is an AODV signaling packet. Based on the	*/
	/* type of option set in the packet, handle it		*/
	switch (tlv_options_ptr->type)
		{
		case (AODVC_ROUTE_REQUEST):
			{
			/* This is a route request packet	*/
			aodv_rte_rreq_pkt_arrival_handle (ip_pkptr, aodv_pkptr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (AODVC_ROUTE_REPLY):
			{
			/* This is a route reply option	*/
			aodv_rte_rrep_pkt_arrival_handle (ip_pkptr, aodv_pkptr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (AODVC_ROUTE_ERROR):
			{
			/* This is a route error option	*/
			aodv_rte_rerr_pkt_arrival_handle (ip_pkptr, aodv_pkptr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
			
			break;
			}
			
		case (AODVC_HELLO):
			{
			/* This is a RREP Hello packet	*/
			
			aodv_rte_rrep_hello_pkt_arrival_handle (ip_pkptr, aodv_pkptr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
			
			break;
			}
			
		default:
			{
			/* Invalid option in packet	*/
			aodv_rte_error ("Invalid Option Type in AODV packet", OPC_NIL, OPC_NIL);
			}	
		}
	
	FOUT;
	}


static void
aodv_rte_app_pkt_arrival_handle (Packet* ip_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, Boolean pkt_from_app_layer)
	{
	int			ttl_value;
	int			last_known_hop_count;
	double		ring_traversal_time;
	char		addr_str [INETC_ADDR_STR_LEN];
	char		node_name [OMSC_HNAME_MAX_LEN];
	char		temp_str [256];
	Manet_Rte_Ext_Cache_entry* ext_cache_entry_ptr;
	
	AodvT_Route_Entry* route_entry_ptr = OPC_NIL;
	
	/** An application packet has arrived at this node	**/
	/** No route exists to the destination. Broadcast a	**/
	/** route request to find a route					**/
	FIN (aodv_rte_app_pkt_arrival_handle (<args>));
	
	/* Check if there exists an invalid route for this destination 	*/
	/* If yes, update lifetime = current time + Delete Period		*/
	route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, ip_dgram_fd_ptr->dest_addr);
	
	/* Update the expiry time of the route to be delete period */
	if ((route_entry_ptr != OPC_NIL) && 
		(route_entry_ptr->route_entry_state == AodvC_Invalid_Route))
		{
		aodv_route_table_entry_expiry_time_update (route_entry_ptr, ip_dgram_fd_ptr->dest_addr, delete_period, AODVC_ROUTE_ENTRY_EXPIRED);
		}
	
	if (pkt_from_app_layer == OPC_FALSE)
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
			sprintf (temp_str, "An application packet has arrived from source node %s (%s)", addr_str, node_name);
			op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
			}
		
		/* This packet came from a neighboring	*/
		/* node. Update the connectivity table	*/
		aodv_rte_neighbor_connectivity_table_update (ip_dgram_fd_ptr->src_addr, OPC_FALSE);

		/* Check if this is a MANET gateway and */
		/* the destination is a known external	*/
		/* route.								*/
		if (ip_manet_gateway_enabled(module_data_ptr) &&
			manet_ext_rte_cache_table_entry_available 
				(external_routes_cache_table_ptr, ip_dgram_fd_ptr->dest_addr, &ext_cache_entry_ptr))
			{
			/* This is a MANET gateway and the destination is 		*/
			/* "outside" the MANET cluster. The arrival of the IP	*/
			/* packet into AODV indicates that the IP forwarding 	*/
			/* table has lost the entry that corresponds to an 		*/
			/* external destination prevously detected by ADOV. 	*/
			/* Since AODV will not be able to find the destinatin	*/
			/* within the MANET cluster, and inmediate RERR shoould	*/
			/* be broadcast to notify all users of that external 	*/
			/* route about its failure.								*/
			
			/* Local repair is not performed for external routes.	*/
			/* Send a route error message and discard the packet	*/
			if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
				{
				op_prg_odb_print_major (pid_string, "Local repair is disabled for external routes on the MANET gateway", 
										"Broadcasting a route error message and discarding the application packet ", OPC_NIL);
				}
			
			/* Change the status of the external route, it is now	*/
			/* not available.										*/
			ext_cache_entry_ptr->available = OPC_FALSE;
		
			/* Send the RERR for the failed external route.			*/
			aodv_rte_ext_route_error_send (ip_dgram_fd_ptr->dest_addr, ext_cache_entry_ptr->dest_seq_num);			
			
			/* Destroy the data packet.								*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			/* Update the number of packets dropped statistic	*/
			op_stat_write (local_stat_handle_ptr->num_pkts_discard_shandle, 1.0);
			op_stat_write (global_stat_handle_ptr->num_pkts_discard_global_shandle, 1.0);
			
			FOUT;	
			}
		
		if (local_repair)
			{
			/* This node has local repair enabled and no	*/
			/* route exists to the destination. Check if	*/
			/* local repair is being performed for this 	*/
			/* destination node of the packet				*/
			if (aodv_rte_local_repair_exists (ip_dgram_fd_ptr->dest_addr, OPC_FALSE) == OPC_TRUE)
				{
				if (LTRACE_ACTIVE)
					{
					op_prg_odb_print_major (pid_string, "This destination node is under local repair", 
												"Queue the application packet", OPC_NIL);
					}
				
				/* This destination is under local repair	*/
				/* Do not send out another route request	*/
				/* Just queue the data packets				*/
				aodv_packet_queue_insert (pkt_queue_ptr, ip_pkptr, ip_dgram_fd_ptr->dest_addr);
				}
			else
				{
				/* Perform local repair for this destination	*/
				if (route_entry_ptr == OPC_NIL || aodv_rte_local_repair_attempt (ip_dgram_fd_ptr->dest_addr, route_entry_ptr) == OPC_COMPCODE_FAILURE)
					{
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major (pid_string, "Local repair cannot be performed for this destination", 
												"The destination is greater than MAX_REPAIR_TTL hops away or there is no route to it", 
												"Sending a route error message back to the source", OPC_NIL);
						}
					
					/* Local repair cannot be performed for this destination	*/
					/* as it is greater than MAX_REPAIR_TTL hops away. Send a	*/
					/* route error message and discard the data packet			*/
					aodv_rte_route_error_process (ip_dgram_fd_ptr->dest_addr, route_entry_ptr, OPC_NIL, AodvC_Data_Packet_No_Route, OPC_FALSE);
					manet_rte_ip_pkt_destroy (ip_pkptr);
					
					/* Update the number of packets dropped statistic	*/
					op_stat_write (local_stat_handle_ptr->num_pkts_discard_shandle, 1.0);
					op_stat_write (global_stat_handle_ptr->num_pkts_discard_global_shandle, 1.0);
					}
				else
					{
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major (pid_string, "Performing local repair for this destination", 
												"Queued the application packet", OPC_NIL);
						}
					
				    /* Local repair has been started for this destination	*/
					/* Queue the packet until route discovery is performed	*/
					aodv_packet_queue_insert (pkt_queue_ptr, ip_pkptr, ip_dgram_fd_ptr->dest_addr);
					}
				}
			}
		else
			{
			/* Local repair has not been enabled on this node.		*/
			/* Send a route error message and discard the packet	*/
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major (pid_string, "Local repair has not been enabled on this node", 
										"Sending a route error message and discarding the application packet ", OPC_NIL);
				}

			aodv_rte_route_error_process (ip_dgram_fd_ptr->dest_addr, route_entry_ptr, OPC_NIL, AodvC_Data_Packet_No_Route, OPC_FALSE);
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			/* Update the number of packets dropped statistic	*/
			op_stat_write (local_stat_handle_ptr->num_pkts_discard_shandle, 1.0);
			op_stat_write (global_stat_handle_ptr->num_pkts_discard_global_shandle, 1.0);
			}
		}
	else
		{
		/* This packet came from application layer of this node.*/
		
		if (aodv_request_table_orig_rreq_exists (request_table_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_FALSE)
			{
			/* The packet is from the application layer and no	*/
			/* route exists to the destination. Perform route	*/
			/* discovery to obtain a route to the destination	*/
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major (pid_string, "An application packet has arrived from the higher layer", 
					"No route exists to the destination. Perform a route discovery.", OPC_NIL);
				}	
			
			/* The hop count stored in an invalid routing table	*/
			/* entry indicates the last known hop count to that	*/
			/* destination. When a new route to the same 		*/
			/* desination is required, the TTL in the RREQ is	*/
			/* initially set to the last known hop count plus	*/
			/* the TTL_INCREMENT. Check if there is a last 		*/
			/* known hop count to the destination.				*/
			/* Check if the destination was previous known but	*/
			/* is marked invalid or whether it expired			*/
			if (aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_HOP_COUNT, &last_known_hop_count) == OPC_COMPCODE_FAILURE)
				{
				/* No route entry exists for the destination	*/
				/* Set the initial TTL to TTL_START				*/
				ttl_value = ttl_start;
				}
			else
				{
				/* An invalid routing table entry exists		*/
				ttl_value = last_known_hop_count + ttl_increment;
				}
	
			/* Calculate the ring traversal time by which a 	*/
			/* RREP message should be received					*/
			ring_traversal_time = 2.0 * node_traversal_time * (ttl_value  + timeout_buffer);
				
			/* Broadcast a route request packet to find a route	*/
			/* to the destination of the incoming data packet	*/
			aodv_rte_route_request_send (route_entry_ptr, ip_dgram_fd_ptr->dest_addr, ttl_value, ring_traversal_time, 0);
			}
	
		/* Queue the application packet in the packet queue	*/
		/* until a route is found to the destination		*/
		aodv_packet_queue_insert (pkt_queue_ptr, ip_pkptr, ip_dgram_fd_ptr->dest_addr);
		}
	
	FOUT;
	}


static void
aodv_rte_rreq_pkt_arrival_handle (Packet* ip_pkptr, Packet* aodv_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
							IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, AodvT_Packet_Option* tlv_options_ptr)
	{
	AodvT_Rreq*				rreq_option_ptr;
	InetT_Address			prev_hop_addr;
	AodvT_Route_Entry*		route_entry_ptr = OPC_NIL;
	AodvT_Route_Entry*		rev_path_entry_ptr = OPC_NIL;
	IpT_Interface_Info*		iface_elem_ptr;
	IpT_Port_Info			in_port_info;
	double					min_lifetime, lifetime;
	int						dest_seq_num, new_ttl_value;
	Packet*					ip_rreq_pkptr;
	char					src_node_name [OMSC_HNAME_MAX_LEN];
	char					src_hop_addr_str [INETC_ADDR_STR_LEN];
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					temp_str [2048];
	double 					existing_lifetime;
	AodvC_Route_Entry_State	route_entry_state;
	Boolean					dest_seq_flag = OPC_FALSE;
	InetT_Subnet_Mask		selected_subnet_mask;
	Ici*					ip_iciptr;
	int						mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
	Manet_Rte_Ext_Cache_entry* ext_cache_entry_ptr = OPC_NIL;

	/** Handles the arrival of a route request message	**/
	FIN (aodv_rte_rreq_pkt_arrival_handle (<args>));
	
	/* A route request packet has arrived at this node	*/
	/* Get the request options from the packet			*/
	rreq_option_ptr = (AodvT_Rreq*) tlv_options_ptr->value_ptr;
	
	/* Get the previous hop from which this packet arrived	*/
	prev_hop_addr = ip_dgram_fd_ptr->src_addr;
	
	/* The source address needs to be set at the first hop	*/
	/* for the route request as when sending out the RREQ	*/
	/* from the actual source node, the output interface	*/
	/* address is not known as it is broadcast.				*/
	if (rreq_option_ptr->hop_count == 0)
		rreq_option_ptr->src_addr = inet_address_copy (prev_hop_addr);
	
	if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, rreq_option_ptr->dest_addr);
		inet_address_to_hname (rreq_option_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with request ID (%d)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, rreq_option_ptr->rreq_id);
		op_prg_odb_print_major (pid_string, "Received a Route Request", temp_str, OPC_NIL);
		}
	
	/* Update the connectivity table	*/
	aodv_rte_neighbor_connectivity_table_update (prev_hop_addr, OPC_FALSE);
	
	/* Update the route to the previous hop of the	*/
	/* route request message.						*/
	aodv_rte_route_table_entry_update (ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
	
	/* If the source address of the route request	*/
	/* belongs to this node, then discard the IP	*/
	/* datagram as it has received its own packet	*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, rreq_option_ptr->src_addr) == OPC_TRUE)
		{
		
		if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Destroying the Route Request", 
				"as the source node received its own packet", OPC_NIL);
			}
		
		/* The originator of the route request has	*/
		/* received its own packet again. Discard	*/
		/* ths IP datagram							*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		op_pk_destroy (aodv_pkptr);
		
		FOUT;
		}
	
	/* Check to see if this node has received a RREQ	*/
	/* with the same request ID and originator address	*/
	if (aodv_route_request_forward_entry_exists (request_table_ptr, rreq_option_ptr->rreq_id,
												rreq_option_ptr->src_addr) == OPC_TRUE)
		{
		/* A route request with the same request ID has	*/
		/* already been received. Discard the packet	*/
		
		if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Destroying the Route Request", 
				"as same request ID has already been received", OPC_NIL);
			}
		
		manet_rte_ip_pkt_destroy (ip_pkptr);
		op_pk_destroy (aodv_pkptr);
		
		FOUT;
		}
	
	/* Insert the route request in the request table	*/
	aodv_request_table_forward_rreq_insert (request_table_ptr, rreq_option_ptr->rreq_id, rreq_option_ptr->src_addr);
	
	/* This is a new route request packet that has not 	*/
	/* been received by this node before. Increment the */
	/* hop count by one to account for the new hop		*/
	rreq_option_ptr->hop_count++;
	
	/* Drop the rreq if it has reached network diameter */
	if (rreq_option_ptr->hop_count > net_diameter )
		{
		/* Destroy the RREQ message	 */
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}

	/* Calculate the lifetime for the route entry	*/
	min_lifetime = ((2.0 * net_traversal_time) - (2.0 * rreq_option_ptr->hop_count * node_traversal_time));
	
	/* Check if there exists a reverse route to the 	*/
	/* source node of the route request.				*/
	route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, rreq_option_ptr->src_addr);
	
	/* Get the interface_info of the interface specified	*/
	iface_elem_ptr = inet_rte_intf_tbl_access (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);

	/* Create the corresponding port information accodingly	*/
	/* with the IP family address.							*/
	if (inet_address_family_get (&rreq_option_ptr->src_addr) == InetC_Addr_Family_v4)
		{
		/* Get the port information for the IPv4 case.		*/
		in_port_info = ip_rte_port_info_create (intf_ici_fdstruct_ptr->intf_recvd_index, iface_elem_ptr->full_name);

		/* Get subnet mask for the IPv4 case.				*/
		selected_subnet_mask = subnet_mask;
		}
	else
		{
		/* Get the port information for the IPv6 case.		*/		
		in_port_info = ipv6_rte_port_info_create (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);
		
		/* Get subnet mask for the IPv6 case.				*/		
		selected_subnet_mask = subnet_mask_ipv6;
		}
	
	if (route_entry_ptr == OPC_NIL)
		{
		/* No entry exists to the source of the route	*/
		/* request. Create a new route entry			*/
		route_entry_ptr = aodv_route_table_entry_create (route_table_ptr, rreq_option_ptr->src_addr, selected_subnet_mask, prev_hop_addr, 
														 in_port_info, rreq_option_ptr->hop_count, rreq_option_ptr->src_seq_num, min_lifetime);
		
		/* Send all packets queued to this destination	*/
		aodv_rte_all_pkts_to_dest_send (rreq_option_ptr->src_addr, route_entry_ptr, OPC_FALSE);
		
		}
	else if( (rreq_option_ptr->src_seq_num > route_entry_ptr->dest_seq_num) ||
		((rreq_option_ptr->src_seq_num == route_entry_ptr->dest_seq_num) && (rreq_option_ptr->hop_count < route_entry_ptr->hop_count)) ||
		((rreq_option_ptr->src_seq_num == route_entry_ptr->dest_seq_num) && (route_entry_ptr->route_entry_state == AodvC_Invalid_Route)))
		{
		/* Update the route table entry	if				*/
		/* 1. We have invalid dest seq number			*/ 
		/* 2. The new seq number is greater				*/
		/* 3. Seq numbers are same and 					*/
		/* 			new hop count is smaller or			*/
		/*			we have invalid route entry			*/
				
		if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			
			printf("Route entry already exists: Old dest_seq_num: %d, Old Hop Cnt: %d\n",
				route_entry_ptr->dest_seq_num, route_entry_ptr->hop_count);
			printf("Route entry already exists: new dest_seq_num: %d, new Hop Cnt: %d\n",
				rreq_option_ptr->src_seq_num, rreq_option_ptr->hop_count);
			op_prg_odb_print_major (pid_string, "Updating route table with new RREQ", OPC_NIL);
			}
		
		/* Update the sequence number */
		route_entry_ptr->dest_seq_num = rreq_option_ptr->src_seq_num;
		
		/* Do this before adding info to IP Cmn table in case route was invalid */
		route_entry_ptr->hop_count = rreq_option_ptr->hop_count;
		route_entry_ptr->next_hop_port_info = in_port_info;
	
		
		if (!inet_address_equal (route_entry_ptr->next_hop_addr, prev_hop_addr))
			{
			/* Update the route table entry */
			aodv_route_table_entry_next_hop_update (route_table_ptr, route_entry_ptr, 
				inet_address_copy (prev_hop_addr), rreq_option_ptr->hop_count, in_port_info);
			}
				
		if(route_entry_ptr->route_entry_state != AodvC_Invalid_Route)
			{
			existing_lifetime = route_entry_ptr->route_expiry_time - op_sim_time ();
			lifetime = aodv_rte_max_find (existing_lifetime, min_lifetime);
			}
		else
			{
			lifetime = min_lifetime;
			aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, rreq_option_ptr->src_addr, AodvC_Valid_Route);
			}
		
		/* Set the valid sequence number field to true	*/
		route_entry_ptr->valid_dest_sequence_number_flag = OPC_TRUE;
		
		aodv_route_table_entry_expiry_time_update (route_entry_ptr, rreq_option_ptr->src_addr, lifetime, AODVC_ROUTE_ENTRY_INVALID);
		}
	
	else if( !inet_address_equal (route_entry_ptr->next_hop_addr, prev_hop_addr))
		{
		/* Drop the route request, if the nexthop on reverse path */
		/* is different from the previous hop. This is to avoid   */
		/* loop formation in certain cases 						  */
		/* Destroy the RREQ message								  */
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/****************************************************/
	/* A node generates a route reply if :				*/
	/* (a) it is the destination of the RREQ			*/
	/* (b) it has an active route to the destination	*/
	/*     and the destination only flag is not set.	*/
	/****************************************************/
	
	/* Determine if this node is the destination of the	*/
	/* route request. If so, generate a route reply 	*/
	/* back to the source of the route request			*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, rreq_option_ptr->dest_addr) == OPC_TRUE)
		{
		/* This is the destination of the RREQ	*/
		/* Send a route reply message			*/
		aodv_rte_route_reply_send (rreq_option_ptr, ip_dgram_fd_ptr, OPC_TRUE, route_entry_ptr, OPC_NIL);
		
		/* Update the statistic for the number of route	*/
		/* replies sent from the destination.			*/
		aodv_support_route_reply_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, OPC_TRUE);
		
		/* Destroy the RREQ message	*/
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Check if this a MANET gateway and the destination	*/
	/* is a known-available external destination.			*/
	if (ip_manet_gateway_enabled(module_data_ptr) &&
		 manet_ext_rte_cache_table_entry_available
		 (external_routes_cache_table_ptr, rreq_option_ptr->dest_addr, &ext_cache_entry_ptr))
		{
		/* ODB trace.													*/
		if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
			{
			char	msg[256];
			char	dest_addr_str [INETC_ADDR_STR_LEN];
			
			inet_address_print (dest_addr_str, rreq_option_ptr->dest_addr);
			sprintf (msg, "Destination [%s] already exists in tthe external route cache", dest_addr_str);
			op_prg_odb_print_major ("AODV Proxy RREP:", msg, "Sening Proxy RREP", OPC_NIL);
			}
		
		/* This MGtwy has a route to the external*/
		/* destination. Send a proxy route reply */
		/* message on behalf of the actual 		 */
		/* destination.							 */
		aodv_rte_route_reply_send (rreq_option_ptr, ip_dgram_fd_ptr, OPC_TRUE, route_entry_ptr, OPC_NIL);

		/* Update the statistic for the number	 */
		/* of route replies sent from the MGtwy	 */
		aodv_support_route_reply_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, OPC_TRUE);
		
		/* Destroy the RREQ message.		 	*/
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Check if there exists an active route, ie, the	*/
	/* destination sequence number is valid and greater	*/
	/* than or equal to the destination sequence number	*/
	/* of the route request message						*/
	/* Make sure the destination is not known as 		*/
	/* external (and disabled).							*/
	if((ext_cache_entry_ptr == OPC_NIL) && 
		(rev_path_entry_ptr = aodv_route_table_entry_get (route_table_ptr, rreq_option_ptr->dest_addr)) != OPC_NIL)
		{
		dest_seq_num = rev_path_entry_ptr->dest_seq_num;
		dest_seq_flag = rev_path_entry_ptr->valid_dest_sequence_number_flag;
		route_entry_state = rev_path_entry_ptr->route_entry_state;
		
		/* Check if the destination sequence number is	*/
		/* greater than or equal to the RREQ message	*/
		if ((dest_seq_num >= rreq_option_ptr->dest_seq_num) && (route_entry_state == AodvC_Valid_Route))
			{
			/* This node has a valid route to the		*/
			/* destination. Check if the destination 	*/
			/* only flag is set in the RREQ				*/
			if (rreq_option_ptr->dest_only == OPC_FALSE)
				{
				if (LTRACE_ACTIVE)
					{
					op_prg_odb_print_major (pid_string, "A valid route exists to the destination from this node", OPC_NIL);
					}
				
				/* This node can send a route reply		*/
				aodv_rte_route_reply_send (rreq_option_ptr, ip_dgram_fd_ptr, OPC_FALSE, route_entry_ptr, rev_path_entry_ptr);
				
				/* Update the statistic for the number of	*/
				/* cached route replies sent 				*/
				aodv_support_route_reply_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, OPC_FALSE);
				
				/* Check if the gratuitous route reply	*/
				/* flag is set in the RREQ message		*/
				if (rreq_option_ptr->grat_rrep_flag == OPC_TRUE)
					{
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major (pid_string, "Sending a gratuitous route reply packet to the destination", OPC_NIL);
						}
					
					/* Send a gratuitous route reply	*/
					/* to the destination of the RREQ	*/
					aodv_rte_grat_route_reply_send (rreq_option_ptr, route_entry_ptr, rev_path_entry_ptr);
					}
				
				/* Destroy the RREQ message	*/
				op_pk_destroy (aodv_pkptr);
				manet_rte_ip_pkt_destroy (ip_pkptr);
				
				FOUT;
				}
			}
		
		/* We didn't reply to this request 		    	*/
		/* so we'll forward it. We had fwd route		*/
		/* & if our dest seq number value is bigger 	*/
		/* we'll send the max of our and rreq's dest	*/
		/* seq number value in the forwarding rreq.		*/
		if (rreq_option_ptr->dest_seq_num < dest_seq_num)
			rreq_option_ptr->dest_seq_num = dest_seq_num;
		}
	else if (ip_manet_gateway_enabled(module_data_ptr))
		{
		Compcode				route_status;
		InetT_Address			next_addr;
		IpT_Port_Info			output_port_info;
		IpT_Rte_Proc_Id			src_proto;
	
		/* AODV - Internet connectivity. A MANET gateway*/
		/* performs a lookup in the IP fwd table after	*/
		/* the lookup in the AODV table fails.			*/
		/* If the IP fwd table has an entry for the 	*/
		/* destination indicated in the RREQ, then the	*/
		/* MGtwy sends a "Proxy" RREP, indicating that 	*/
		/* it can reach the destination (which is 		*/
		/* outside the MANET cluster).					*/
		
		/* Perform a lookup in the IP common route table.*/
		route_status = inet_cmn_rte_table_recursive_lookup (module_data_ptr->ip_route_table, rreq_option_ptr->dest_addr,
			&next_addr, &output_port_info, &src_proto, OPC_NIL);

		/*	In debug mode, trace the routing action.*/
		if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
			{
			char	dest_addr_str [INETC_ADDR_STR_LEN], next_addr_str [INETC_ADDR_STR_LEN];
			char	str1[512];
			/* Create string references to the destination and next addresses. 	*/
			inet_address_print (dest_addr_str, rreq_option_ptr->dest_addr);
			inet_address_print (next_addr_str, next_addr);

			sprintf (str1, "Dest Addr(%s), Next hop (%s), Lookup(%s)", dest_addr_str, next_addr_str, 
				route_status == OPC_COMPCODE_SUCCESS ? "Success": "Failed");
			op_prg_odb_print_major ("MANET Gtwy. AODV Looking for external networks:", str1, OPC_NIL);
			}
		
		/* Check if a valid route is found.				*/
		if (route_status == OPC_COMPCODE_SUCCESS)
			{
			IpT_Interface_Info*	input_intf_info_ptr;		
				
			/* Only entries with a non-MANET (AODV) 	*/
			/* outgoing interface qualify for a proxy 	*/
			/* RREP.									*/
			
			/* Access information of the outgoing 		*/
			/* interface associated with the entry just	*/
			/* found.									*/
			input_intf_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, output_port_info.intf_tbl_index);
			
			/* Check if the outgoing interface is 		*/
			/* non-MANET.								*/
			if (!ip_rte_intf_manet_enabled(input_intf_info_ptr))
				{
				/* Outgoing interface is non-MANET.		*/
				/* The route indicates the destination 	*/
				/* is external to the MANET cluster.	*/
				
				/*	ODB trace.							*/
				if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
					{
					op_prg_odb_print_minor ("External network found. Outgoing interface is not MANET.", OPC_NIL);
					}
			
				/* For future use of this route to an 	*/
				/* external host, activate the route 	*/
				/* in the cache table. If the entry 	*/
				/* already exists it will be activated.	*/
				manet_ext_rte_cache_table_entry_activate (external_routes_cache_table_ptr,
															rreq_option_ptr->dest_addr, 
															rreq_option_ptr->dest_seq_num);
				
				/* This MGtwy has a route to the external*/
				/* destination. Send a proxy route reply */
				/* message on behalf of the actual 		 */
				/* destination.							 */
				aodv_rte_route_reply_send (rreq_option_ptr, ip_dgram_fd_ptr, OPC_TRUE, route_entry_ptr, OPC_NIL);
		
				/* Update the statistic for the number	 */
				/* of route replies sent from the MGtwy	 */
				aodv_support_route_reply_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, OPC_TRUE);
		
				/* Destroy the RREQ message.		 	*/
				op_pk_destroy (aodv_pkptr);
				manet_rte_ip_pkt_destroy (ip_pkptr);
		
				FOUT;
				}

			/* Outgoing interface is MANET.	This means	*/
			/* that the destination is not an external	*/
			/* (out if the MANET cluster) destination	*/
			/* that can be reached by this MGtwy. Assume*/
			/* the destination it is actually a MANET 	*/
			/* node. No proxy RREP is sent.				*/
			/* This RREQ will be forwarded within the 	*/
			/* MANET cluster (regular AODV behavior).	*/
				
			/*	ODB trace.								*/
			if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
				{
				op_prg_odb_print_minor ("External network not found. Outgoing interface is MANET.", OPC_NIL);
				}
			}
		else if (ext_cache_entry_ptr != OPC_NIL)
			{
			/* The external cache entry exists but is it not available	*/
			/* at this time: IP forwarding table lost the entry.		*/
			
			/* The MANET Gateway node will not forward this RREQ since	*/
			/* it is known that the destination is non-MANET (currently	*/
			/* not available). 											*/
			if (op_prg_odb_ltrace_active ("aodv_ext_rte"))
				{
				op_prg_odb_print_major (pid_string, "The destination indicated in the RREQ belongs to an external host, ", 
					"which is currently unavailable.",
					"Destroy the route request packet.", OPC_NIL);
				}
		
			/* The maximum TTL has been reached	*/
			op_pk_destroy (aodv_pkptr);
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			FOUT;			
			}
		}
	
	/* The node is neither the destination or has an	*/
	/* active route to the destintion. Check if the TTL	*/
	/* of the packet is greater than one.				*/
		
	if (ip_dgram_fd_ptr->ttl == 1)
		{
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "The maximum TTL has been reached.", 
					"Destroy the route request packet.", OPC_NIL);
			printf("Current TTL is : %d\n", ip_dgram_fd_ptr->ttl);
			}
		
		/* The maximum TTL has been reached	*/
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Rebroadcast the route request packet	*/
	/* Decrement the TTL field by one		*/
	new_ttl_value = ip_dgram_fd_ptr->ttl - 1;
		
	/* Encapsulate the RREQ packet in an IP datagram	*/
	if (inet_address_family_get (&rreq_option_ptr->dest_addr) == InetC_Addr_Family_v4)
		{
		ip_rreq_pkptr = aodv_rte_ip_datagram_create (aodv_pkptr, InetI_Broadcast_v4_Addr, OPC_NIL, new_ttl_value,
			InetI_Broadcast_v4_Addr, OPC_NIL);
		}
	else
		{
		ip_rreq_pkptr = aodv_rte_ip_datagram_create (aodv_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL,
			new_ttl_value, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
		
		/* Install the ICI for IPv6 case */
		ip_iciptr = op_ici_create ("ip_rte_req_v4");
		op_ici_attr_set_int32 (ip_iciptr, "multicast_major_port", mcast_major_port);
		op_ici_install (ip_iciptr);		
		}
	
		if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Re-broadcasting the route request packet", OPC_NIL);
			}
		
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, ip_rreq_pkptr);
	
	/* Update the statistics for the total route requests forwarded	*/
	op_stat_write (local_stat_handle_ptr->total_requests_fwd_shandle, 1.0);
	
	/* Send the packet to the CPU with jitter which will broadcast it	*/
	/* after processing the packet										*/
	manet_rte_to_cpu_pkt_send_schedule_with_jitter (module_data_ptr, parent_prohandle, parent_pro_id, ip_rreq_pkptr);
	
	/* Clear the ICI if installed */
	op_ici_install (OPC_NIL);	
	
	/* Set the last broadcast sent time	*/
	last_broadcast_sent_time = op_sim_time ();
	
	/* Destroy the old IP packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
		
	FOUT;
	}
		

static void
aodv_rte_rrep_pkt_arrival_handle (Packet* ip_pkptr, Packet* aodv_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
							IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, AodvT_Packet_Option* tlv_options_ptr)
	{
	AodvT_Rrep*					rrep_option_ptr;
	InetT_Address				prev_hop_addr;
	InetT_Address				next_hop_addr;
	IpT_Interface_Info*			iface_elem_ptr;
	IpT_Port_Info				in_port_info;
	AodvT_Route_Entry*			route_entry_ptr = OPC_NIL;
	Boolean						route_updated = OPC_FALSE;
	Boolean						reached_src = OPC_FALSE;
	int							dest_seq_num;
	int							hop_count;
	AodvC_Route_Entry_State		route_entry_state;
	Packet*						ip_rrep_pkptr;
	char						src_node_name [OMSC_HNAME_MAX_LEN];
	char						src_hop_addr_str [INETC_ADDR_STR_LEN];
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [2048], temp_str1 [2048];
	char						next_addr_str [INETC_ADDR_STR_LEN];
	char						next_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_addr_str [INETC_ADDR_STR_LEN];
	double						existing_lifetime, lifetime;
	double						route_expiry_time;
	ManetT_Nexthop_Info*		manet_nexthop_info_ptr = OPC_NIL;
	AodvT_Route_Entry*			fwd_path_entry_ptr = OPC_NIL;
	InetT_Subnet_Mask			selected_subnet_mask;
	
	/** Handles the arrival of the route reply message	**/
	FIN (aodv_rte_rrep_pkt_arrival_handle (<args>));
	
	/* A route reply packet has arrived at this node	*/
	/* Get the reply options from the packet			*/
	rrep_option_ptr = (AodvT_Rrep*) tlv_options_ptr->value_ptr;
	
	/* Get the previous hop from which this packet arrived	*/
	prev_hop_addr = ip_dgram_fd_ptr->src_addr;
	
	/* Check if node has received its own route reply */
	if(manet_rte_address_belongs_to_node (module_data_ptr, rrep_option_ptr->dest_addr))
		{
		/* Drop the route reply */
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}

	/* Update the connectivity table	*/
	aodv_rte_neighbor_connectivity_table_update (prev_hop_addr, OPC_FALSE);
	
	/* Update the route to the previous hop of the	*/
	/* route reply message.							*/
	aodv_rte_route_table_entry_update (ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, tlv_options_ptr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, rrep_option_ptr->dest_addr);
		inet_address_to_hname (rrep_option_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "from node %s (%s). Originator of this RREP: %s (%s)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major (pid_string, "Received a route reply packet", temp_str, OPC_NIL);
		}
	
	/* Increment the hop count by one to account		*/
	/* for the new hop through the intermediate node	*/
	rrep_option_ptr->hop_count++;
	
	/* Drop the rreq if it has reached network diameter */
	if (rrep_option_ptr->hop_count > net_diameter )
		{
		/* Destroy the RREQ message	 */
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Get the interface_info of the interface specified	*/
	iface_elem_ptr = inet_rte_intf_tbl_access (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);

	/* Check for the IP address family.						*/
	if (inet_address_family_get (&rrep_option_ptr->dest_addr) == InetC_Addr_Family_v4)
		{
		/* Get the port information for the IPv4 case.		*/
		in_port_info = ip_rte_port_info_create (intf_ici_fdstruct_ptr->intf_recvd_index, iface_elem_ptr->full_name);

		/* Get subnet mask for the IPv4 case.				*/
		selected_subnet_mask = subnet_mask;
		}
	else
		{
		/* Get the port information for the IPv6 case.		*/		
		in_port_info = ipv6_rte_port_info_create (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);
		
		/* Get subnet mask for the IPv6 case.				*/		
		selected_subnet_mask = subnet_mask_ipv6;
		}
	
	/* Check if this is the final destination of the RREP message	*/
	reached_src = manet_rte_address_belongs_to_node (module_data_ptr, rrep_option_ptr->src_addr);
	
	/* Check if the route exists to the destination IP	*/
	/* address in the RREP message.						*/
	route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, rrep_option_ptr->dest_addr);
	if(route_entry_ptr == OPC_NIL)
		{
		/* No route exists to this destination	*/
		/* Create a new route table entry		*/
		route_entry_ptr = aodv_route_table_entry_create (route_table_ptr, rrep_option_ptr->dest_addr, selected_subnet_mask, ip_dgram_fd_ptr->src_addr, 
										in_port_info, rrep_option_ptr->hop_count, rrep_option_ptr->dest_seq_num, 
										rrep_option_ptr->lifetime);
		
		/* Send all packets queued to this destination	*/
		aodv_rte_all_pkts_to_dest_send (rrep_option_ptr->dest_addr, route_entry_ptr, reached_src);
		
		/* Indicate that the route has been created	*/
		route_updated = OPC_TRUE;
		}
	else
		{
		/* A route does exist to this destination. Update the	*/
		/* route table entry if : 								*/
		/* (a) the sequence number is invalid in the route		*/
		/*     table entry										*/
		/* (b) the destination sequnce number in the RREP is	*/
		/*     greater than node's copy of the sequence number	*/
		/* (c) the sequence numbers are the same but the route	*/
		/*     is marked as inactive							*/
		/* (d) the sequence numbers are the same, and the new	*/
		/*     hop count is smaller than the hop count in the	*/
		/*     route table entry.								*/
		
		dest_seq_num = route_entry_ptr->dest_seq_num;
		hop_count = route_entry_ptr->hop_count;
		route_entry_state = route_entry_ptr->route_entry_state;
		
		
		if (LTRACE_ACTIVE)
			{
			printf ("Route already Exists: DestSeq: %d, HopCnt:%d \n",  dest_seq_num, hop_count);
			printf ("                     NewDestSeq: %d, NewHopCnt: %d \n", rrep_option_ptr->dest_seq_num, 
					rrep_option_ptr->hop_count);
			}
		
		if ((rrep_option_ptr->dest_seq_num > dest_seq_num) ||
			((rrep_option_ptr->dest_seq_num == dest_seq_num) && (route_entry_state == AodvC_Invalid_Route)) ||
			((rrep_option_ptr->dest_seq_num == dest_seq_num) && (rrep_option_ptr->hop_count < hop_count)))
			{
			/* Update the route table entry	if				*/
			/* We have invalid dest seq number, or new seq  */
			/* number is greater or if seq numbers are same */
			/* new hop count is greater or we have invalid  */
			/* route entry									*/
			
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major (pid_string," Updating Route Table with new RREP", OPC_NIL);
				}
				
			/* Do this before adding info to IP Cmn table in case route was invalid */
			route_entry_ptr->hop_count = rrep_option_ptr->hop_count;
			route_entry_ptr->next_hop_port_info = in_port_info;
			
			if (!inet_address_equal (route_entry_ptr->next_hop_addr, prev_hop_addr))
				{
				/* Update the next hop and the hop count to the new value	*/
				aodv_route_table_entry_next_hop_update (route_table_ptr, route_entry_ptr,
														inet_address_copy (prev_hop_addr), rrep_option_ptr->hop_count, in_port_info);
				}
			
			/* Set the route to be a valid route	*/
			if (route_entry_state != AodvC_Valid_Route)
				{
				aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, rrep_option_ptr->dest_addr, AodvC_Valid_Route);
				}
		
			/* Mark the destination sequence as valid	*/
			aodv_route_table_entry_param_set (route_entry_ptr, AODVC_ROUTE_ENTRY_VALID_SEQ_NUM_FLAG, OPC_TRUE);
			
			/* Set the destination sequence number to that in the RREP message	*/
			aodv_route_table_entry_param_set (route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, rrep_option_ptr->dest_seq_num);
			
			/* Set the expiry time to be the current time	*/
			/* plus the lifetime of the RREP message		*/
			aodv_route_table_entry_expiry_time_update (route_entry_ptr, rrep_option_ptr->dest_addr, 
								rrep_option_ptr->lifetime, AODVC_ROUTE_ENTRY_INVALID);
			
			/* Send all packets queued to this destination	*/
			aodv_rte_all_pkts_to_dest_send (rrep_option_ptr->dest_addr, route_entry_ptr, reached_src);
			
			/* Indicate that the route has been created	*/
			route_updated = OPC_TRUE;
			}
		} 
		
	if (reached_src)
		{
		/* This is the destination of the RREP message	*/
		/* Check if the destination was under local		*/
		/* repair and if so remove it from the list		*/
		if (aodv_rte_local_repair_exists (rrep_option_ptr->dest_addr, OPC_TRUE) == OPC_TRUE)
			{
			/* This destination was under local repair	*/
			/* An RREP has been received for the		*/
			/* desired destination. Compare the hop		*/
			/* count of the new route with that of the	*/
			/* invalid route table entry				*/
			aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_HOP_COUNT, &hop_count);
			
			if (rrep_option_ptr->hop_count > hop_count)
				{
				/* If new determined route has greater     */
				/* hopcount, issue a route error message   */
				/* for the destination with the N bit set. */
				
				if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major (pid_string,"This node underlocal repair and RREP \
					recevied with less hopcount so RERR", OPC_NIL);
				}
				aodv_rte_route_error_process (rrep_option_ptr->dest_addr, route_entry_ptr, OPC_NIL, 
					AodvC_Data_Packet_No_Route, OPC_TRUE);
				aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, rrep_option_ptr->dest_addr);
				}
			}
		
		/* Cancel the route request timer for this	*/
		/* request ID that we have received a RREP	*/
		aodv_route_request_orig_entry_delete (request_table_ptr, rrep_option_ptr->dest_addr);
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}

	/* We are going to forward this route reply */
	if(((fwd_path_entry_ptr = aodv_route_table_entry_get (route_table_ptr, rrep_option_ptr->src_addr)) != OPC_NIL)
		&& (fwd_path_entry_ptr->route_entry_state != AodvC_Invalid_Route))
		{
		/* Forward the route reply, if we have route entry for the				*/
		/* originator of RREQ i.e. the source address field in RREP pkt.		*/
		/* Access the next hop address from the route entry						*/
		next_hop_addr = fwd_path_entry_ptr->next_hop_addr;
		hop_count = fwd_path_entry_ptr->hop_count;
		route_expiry_time = fwd_path_entry_ptr->route_expiry_time;		
				
		/* Update the reverse path expiry time before sending out the RREP */
		existing_lifetime = route_expiry_time - op_sim_time();
		
		/* Since active_route_timeout = my_route_timeout/2, using this according to RFC */
		lifetime = aodv_rte_max_find (existing_lifetime, my_route_timeout/2);
		aodv_route_table_entry_expiry_time_update (fwd_path_entry_ptr, rrep_option_ptr->src_addr, lifetime, AODVC_ROUTE_ENTRY_INVALID);
		
		/* Set the ack requested flag for this node	*/
		rrep_option_ptr->ack_required_flag = ack_required;
	
		/* Update the precursor list */
		aodv_route_table_precursor_add (fwd_path_entry_ptr, prev_hop_addr);
		aodv_route_table_precursor_add (route_entry_ptr, next_hop_addr);
	
		/* Allocate memory for manet_nexthop_info_ptr */
		manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
		
		/* Encapsulate the AODV packet in a new IP datagram	*/
		ip_rrep_pkptr = aodv_rte_ip_datagram_create (aodv_pkptr, rrep_option_ptr->src_addr, fwd_path_entry_ptr,
			IPC_DEFAULT_TTL, next_hop_addr, manet_nexthop_info_ptr);
	
		/* Update the statistics for the routing traffic sent	*/
		aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, ip_rrep_pkptr);
	
		if (LTRACE_ACTIVE)
			{
			inet_address_print (next_addr_str, next_hop_addr);
			inet_address_to_hname (next_hop_addr, next_node_name);
			inet_address_print (dest_addr_str, rrep_option_ptr->src_addr);
			inet_address_to_hname (rrep_option_ptr->src_addr, dest_node_name);
			sprintf (temp_str, "Forwarding the route reply to next hop %s (%s)", next_addr_str, next_node_name);
			sprintf (temp_str1, " destined to %s(%s)", dest_addr_str, dest_node_name);
			op_prg_odb_print_major (pid_string, temp_str, temp_str1, OPC_NIL);
			}

		/* Install the event state 	*/
		/* This event will be processed in ip_rte_support.ex.c while receiving     */
		/* AODV control packets. manet_nexthop_info_ptr will point to structure    */
		/* containing nexthop info, so IP table lookup is not again done for them. */
		op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
		/* Send the packet to the CPU	*/
		manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rrep_pkptr);
		op_ev_state_install (OPC_NIL, OPC_NIL);
	
		/* Destory the old IP packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	else
		{
		/* We do not have route entry to forward this */
		/* route reply. Discard it.					  */
		op_pk_destroy (aodv_pkptr);
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	}


static void
aodv_rte_rrep_hello_pkt_arrival_handle (Packet* ip_pkptr, Packet* aodv_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
				IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, AodvT_Packet_Option* tlv_options_ptr)
	{
	char		addr_str [INETC_ADDR_STR_LEN];
	char		node_name [OMSC_HNAME_MAX_LEN];
	char		temp_str [256];
	
	/** Handles the arrival of the hello packet	**/
	FIN (aodv_rte_rrep_hello_pkt_arrival_handle (<args>));
	
	if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE)|| LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "Received a hello packet from node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
		}
	
	/* If we have valid route to this neighbor, update the		*/
	/* route's sequence number using hello's sequence number if	*/
	/* necessary.												*/
	aodv_rte_route_table_entry_from_hello_update (ip_dgram_fd_ptr, tlv_options_ptr);
	
	/* Update the connectivity table	*/
	if (route_table_ptr->active_route_count > 0)
		aodv_rte_neighbor_connectivity_table_update (ip_dgram_fd_ptr->src_addr, OPC_TRUE);
	
	/* Destroy the packets	*/
	op_pk_destroy (aodv_pkptr);
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}


static void
aodv_rte_rerr_pkt_arrival_handle (Packet* ip_pkptr, Packet* aodv_pkptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
					IpT_Rte_Ind_Ici_Fields* PRG_ARG_UNUSED(intf_ici_fdstruct_ptr), AodvT_Packet_Option* tlv_options_ptr)
	{
	AodvT_Rerr*		rerr_option_ptr;
	char			addr_str [INETC_ADDR_STR_LEN];
	char			node_name [OMSC_HNAME_MAX_LEN];
	char			temp_str [256];
	
	/** A route error packet has been received from a neighbor	**/
	/** Process the route error message.						**/
	FIN (aodv_rte_rerr_pkt_arrival_handle (<args>));
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, node_name);
		sprintf (temp_str, "Received a route error packet from node %s (%s)", addr_str, node_name);
		op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
		}
	
	/* Get the route error option	*/
	rerr_option_ptr = (AodvT_Rerr*) tlv_options_ptr->value_ptr;
	
	/* Process the route error message	*/
	aodv_rte_route_error_process (ip_dgram_fd_ptr->src_addr, OPC_NIL, rerr_option_ptr->unreachable_dest_lptr, 
									AodvC_Rerr_Received, rerr_option_ptr->no_delete_flag);	
	
	/* Destroy the packets	*/
	op_pk_destroy (aodv_pkptr);
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}


static void
aodv_rte_data_routes_expiry_time_update (Packet* dgram_pkptr)
	{
	IpT_Dgram_Fields*		dgram_fields_ptr;
	IpT_Rte_Ind_Ici_Fields*	rte_ind_fields_ptr;
	InetT_Address			src_addr;	
	InetT_Address			dest_addr;	
	InetT_Address			prev_hop_addr;	
	InetT_Address			next_hop_addr;	
	AodvT_Route_Entry*		src_route_entry_ptr;
	AodvT_Route_Entry*		dest_route_entry_ptr;
	AodvT_Route_Entry*		prev_hop_route_entry_ptr;
	AodvT_Route_Entry*		next_hop_route_entry_ptr;
	
	/** This function is called when we are invoked by IP	**/
	/** when it routes a data packet using one of our		**/
	/** active routes to update expiry time of the route	**/
	/** tables entries for the source, previous hop, next	**/
	/** hop and destination of the packet.					**/
	FIN (aodv_rte_data_routes_expiry_time_update (dgram_pkptr));
	
	/* Make sure that we have a packet to use.				*/
	if (dgram_pkptr == OPC_NIL)
		aodv_rte_error ("Could not obtain the IP datagram from argument memory!", OPC_NIL, OPC_NIL);
	
	/* Access the information from the IP packet.			*/
	manet_rte_ip_pkt_info_access (dgram_pkptr, &dgram_fields_ptr, &rte_ind_fields_ptr);
	src_addr  = dgram_fields_ptr->src_addr;
	dest_addr = dgram_fields_ptr->dest_addr;
	
	/* Get handles to the route entries for the source and	*/
	/* destination of the packet.							*/
	src_route_entry_ptr  = aodv_route_table_entry_get (route_table_ptr, src_addr);
	dest_route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, dest_addr);
	
	/* Update the entries for source and previous hop.		*/
	if (src_route_entry_ptr != OPC_NIL)
		{
		/* Update the expiry of the entry.					*/
		aodv_route_table_entry_expiry_time_update (src_route_entry_ptr, src_addr, route_table_ptr->route_expiry_time, AODVC_ROUTE_ENTRY_INVALID);
		
		/* Similarly update the entry for the previous hop.	*/
		aodv_route_table_entry_param_get (src_route_entry_ptr, AODVC_ROUTE_ENTRY_NEXT_HOP_ADDR, &prev_hop_addr);
		prev_hop_route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, prev_hop_addr);
		if (prev_hop_route_entry_ptr != OPC_NIL && !inet_address_equal (src_addr, prev_hop_addr))
			aodv_route_table_entry_expiry_time_update (prev_hop_route_entry_ptr, prev_hop_addr, 
													   route_table_ptr->route_expiry_time, AODVC_ROUTE_ENTRY_INVALID);
		}
	
	/* Update the entries for destination and next hop.		*/
	if (dest_route_entry_ptr != OPC_NIL)
		{
		/* Update the expiry of the entry.					*/
		aodv_route_table_entry_expiry_time_update (dest_route_entry_ptr, dest_addr, route_table_ptr->route_expiry_time, AODVC_ROUTE_ENTRY_INVALID);
		
		/* Similarly update the entry for the next hop.		*/
		aodv_route_table_entry_param_get (dest_route_entry_ptr, AODVC_ROUTE_ENTRY_NEXT_HOP_ADDR, &next_hop_addr);
		next_hop_route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, next_hop_addr);
		if (next_hop_route_entry_ptr != OPC_NIL && !inet_address_equal (dest_addr, next_hop_addr))
			aodv_route_table_entry_expiry_time_update (next_hop_route_entry_ptr, next_hop_addr, 
													   route_table_ptr->route_expiry_time, AODVC_ROUTE_ENTRY_INVALID);
		}
	
	FOUT;
	}


static void
aodv_rte_route_table_entry_update (IpT_Dgram_Fields* ip_dgram_fd_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr,
									AodvT_Packet_Option* tlv_options_ptr)
	{
	InetT_Address			prev_hop_addr;
	AodvT_Route_Entry*		route_entry_ptr;
	IpT_Interface_Info*		iface_elem_ptr;
	IpT_Port_Info			in_port_info;
	AodvT_Rreq*				rreq_option_ptr;
	AodvT_Rrep*				rrep_option_ptr;
	int						sequence_num, dest_seq_num;
	int						hop_count;
	double					lifetime;
	Boolean					reached_src = OPC_FALSE;
	AodvC_Route_Entry_State	route_entry_state;
	InetT_Subnet_Mask		selected_subnet_mask;
	Boolean					hello_msg = OPC_FALSE;
	
	/** Creates or updates the route entry to the	**/
	/** previous hop from which the control packet	**/
	/** was received.								**/
	FIN (aodv_rte_route_table_entry_update (<args>));
		
	/* Get the previous hop from which this packet arrived	*/
	prev_hop_addr = ip_dgram_fd_ptr->src_addr;
	
	switch (tlv_options_ptr->type)
		{
		case (AODVC_ROUTE_REQUEST):
			{
			rreq_option_ptr = (AodvT_Rreq*) tlv_options_ptr->value_ptr;
		
			/* Check if the source address is the previous hop	*/
			if (inet_address_equal (prev_hop_addr, rreq_option_ptr->src_addr))
				{
				/* Get the source sequence number	*/
				sequence_num = rreq_option_ptr->src_seq_num;
				}
			else
				{
				/* Set the sequence number as invalid as it	*/
				/* is not known by the RREQ message			*/
				sequence_num = AODVC_DEST_SEQ_NUM_INVALID;
				}
		
			/* Initialize the lifetime to active route timeout	*/
			lifetime = route_table_ptr->route_expiry_time;
			
			break;
			}

		case (AODVC_ROUTE_REPLY):
			{
			rrep_option_ptr = (AodvT_Rrep*) tlv_options_ptr->value_ptr;
		
			/* Check if the source address is the previous hop	*/
			if (inet_address_equal (prev_hop_addr, rrep_option_ptr->dest_addr))
				{
				/* Get the source sequence number	*/
				sequence_num = rrep_option_ptr->dest_seq_num;
				lifetime = rrep_option_ptr->lifetime;
				}
			else
				{
				/* Set the sequence number as invalid as it	*/
				/* is not known by the RREP message			*/
				sequence_num = AODVC_DEST_SEQ_NUM_INVALID;
				lifetime = route_table_ptr->route_expiry_time;
				}
			
			/* Check if this is the destination of the route reply	*/
			reached_src = manet_rte_address_belongs_to_node (module_data_ptr, rrep_option_ptr->src_addr);
			
			break;
			}
			
		case (AODVC_HELLO):
			{
			rrep_option_ptr = (AodvT_Rrep*) tlv_options_ptr->value_ptr;
			sequence_num = rrep_option_ptr->dest_seq_num;
			lifetime = rrep_option_ptr->lifetime;
			reached_src = OPC_TRUE;
			hello_msg = OPC_TRUE;
			
			break;
			}
		}
	
	/* Check if an route entry exists to the previous hop	*/
	route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, prev_hop_addr);
	
	/* Get the interface_info of the interface specified	*/
	iface_elem_ptr = inet_rte_intf_tbl_access (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);

	/* Check for IP address family.							*/
	if (inet_address_family_get (&prev_hop_addr) == InetC_Addr_Family_v4)
		{
		/* Get the port information for the IPv4 case.		*/
		in_port_info = ip_rte_port_info_create (intf_ici_fdstruct_ptr->intf_recvd_index, iface_elem_ptr->full_name);

		/* Get subnet mask for the IPv4 case.				*/
		selected_subnet_mask = subnet_mask;
		}
	else
		{
		/* Get the port information for the IPv6 case.		*/		
		in_port_info = ipv6_rte_port_info_create (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);
		
		/* Get subnet mask for the IPv6 case.				*/		
		selected_subnet_mask = subnet_mask_ipv6;
		}
	
	if (route_entry_ptr == OPC_NIL)
		{
		/* No route exists to the previous hop	*/
		/* Create a new route entry				*/
		route_entry_ptr = aodv_route_table_entry_create (route_table_ptr, prev_hop_addr, selected_subnet_mask, prev_hop_addr, in_port_info, 
														 1, sequence_num, lifetime);
		
		/* Send all packets queued to this destination	*/
		aodv_rte_all_pkts_to_dest_send (prev_hop_addr, route_entry_ptr, reached_src);
		}
	else
		{
		/* There exists an entry to this destination	*/
		/* Update the entry in the route table if the	*/
		/* new sequence number is either :				*/
		/* (a) higher than the destination sequence 	*/
		/*     number in the route table, or			*/
		/* (b) the sequence numbers are equal, but the	*/
		/*     hop count plus one of the new info is	*/
		/*     smaller than the existing hop count, or  */
		/* (c) seq number are equal but my route entry 	*/
		/*     is invalid.								*/
		/* (d) the sequence number is unknown			*/
		
		dest_seq_num = route_entry_ptr->dest_seq_num;
		hop_count = route_entry_ptr->hop_count;
		route_entry_state = route_entry_ptr->route_entry_state;
		
			
		/* Update the route entry if the received packet has higher dest seq no */
		/* or same seq no with better hop count or my route entry is invalid    */
		/* In case of hello message, always update the route entry 				*/
				
		if ((hello_msg) ||
			(sequence_num > dest_seq_num) ||
			((sequence_num == dest_seq_num) && (route_entry_state == AodvC_Invalid_Route)) ||
			((sequence_num == dest_seq_num) && (hop_count < 2)))			
			{
			
			/* Do this before adding info to IP Cmn table in case route was invalid */
			route_entry_ptr->dest_seq_num = sequence_num;
			route_entry_ptr->hop_count = 1;
			route_entry_ptr->next_hop_port_info = in_port_info;
	
			if (!inet_address_equal (route_entry_ptr->next_hop_addr, prev_hop_addr))
				{
				/* Update the next hop and the hop count to the new value	*/
				aodv_route_table_entry_next_hop_update (route_table_ptr, route_entry_ptr, 
														inet_address_copy (prev_hop_addr), 1, in_port_info);
				}
			
			if((route_entry_state == AodvC_Invalid_Route))
				{
				/* If Route was invalid, make it Valid, and add this to the IP Common table */
				aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, prev_hop_addr, AodvC_Valid_Route);
				}
			
			
			if (sequence_num == AODVC_DEST_SEQ_NUM_INVALID)
				route_entry_ptr->valid_dest_sequence_number_flag = OPC_FALSE;
			else
				route_entry_ptr->valid_dest_sequence_number_flag = OPC_TRUE;
			
			/* Update the expiry_time of the route entry */
			aodv_route_table_entry_expiry_time_update (route_entry_ptr, prev_hop_addr, lifetime, AODVC_ROUTE_ENTRY_INVALID);
			}
		}
	
	FOUT;
	}

static void
aodv_rte_route_table_entry_from_hello_update (IpT_Dgram_Fields* ip_dgram_fd_ptr, AodvT_Packet_Option* tlv_options_ptr)
	{
	InetT_Address			prev_hop_addr;
	AodvT_Route_Entry*		route_entry_ptr;
	int						sequence_num;
	
	/** This function is called when we receive a hello		**/
	/** from a neighbor to update the sequence number 		**/
	/** information for the route if we have a valid route	**/
	/** to that neighbor.									**/
	FIN (aodv_rte_route_table_entry_from_hello_update (<args>));
		
	/* Get the previous hop from which this packet arrived.	*/
	prev_hop_addr = ip_dgram_fd_ptr->src_addr;

	/* Retrieve the message's sequence number.				*/
	sequence_num = ((AodvT_Rrep*) tlv_options_ptr->value_ptr)->dest_seq_num;
		
	/* Check if an route entry exists to the previous hop.	*/
	route_entry_ptr = aodv_route_table_entry_get (route_table_ptr, prev_hop_addr);	
	if (route_entry_ptr != OPC_NIL)
		{
		/* Update the route's sequence number with			*/
		/* message's sequence number.						*/
		route_entry_ptr->dest_seq_num = sequence_num;	
		if (sequence_num == AODVC_DEST_SEQ_NUM_INVALID)
			route_entry_ptr->valid_dest_sequence_number_flag = OPC_FALSE;
		else
			route_entry_ptr->valid_dest_sequence_number_flag = OPC_TRUE;			
		}
	
	FOUT;
	}


static void
aodv_rte_route_request_send (AodvT_Route_Entry* route_entry_ptr, InetT_Address dest_addr, int ttl_value, double request_expiry_time, int rreq_retry)
	{
	int							dest_seq_num;
	AodvT_Packet_Option*		rreq_option_ptr;
	Packet*						rreq_pkptr;
	Packet*						ip_rreq_pkptr;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	Ici*						ip_iciptr;
	int							mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
		
	/** Broadcasts a route request message	**/
	FIN (aodv_rte_route_request_send (<args>));
	
	/* Check if the destination was previous known but	*/
	/* is marked invalid or whether it expired.			*/
	/* Get the last known destination sequence number  	*/
	/* to set in the route request packet.				*/
	if (aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, &dest_seq_num) == OPC_COMPCODE_FAILURE)
		{
		/* No entry exists to the destination. This		*/
		/* could be the case if the route expired or a	*/
		/* route was never discovered					*/
		dest_seq_num = AODVC_DEST_SEQ_NUM_INVALID;
		}
	
	/* Increment the originating sequence number	*/
	sequence_number++;
					
	/* Create a route request option	*/
	rreq_option_ptr = aodv_pkt_support_rreq_option_create (OPC_FALSE, OPC_FALSE,
		grat_route_reply_flag, dest_only_flag,	OPC_TRUE, 0, route_request_id, dest_addr, 
								dest_seq_num, INETC_ADDRESS_INVALID, sequence_number);
	
	if (inet_address_family_get (&dest_addr) == InetC_Addr_Family_v4)
		{
		/* This is an IPv4 destination	*/
	
		/* Insert the option in an AODV RREQ packet	*/
		rreq_pkptr = aodv_pkt_support_pkt_create (rreq_option_ptr, AODVC_RREQ_IPV4_SIZE);
	
		/* Encapsulate the AODV packet in an IP datagram	*/
		ip_rreq_pkptr = aodv_rte_ip_datagram_create (rreq_pkptr, InetI_Broadcast_v4_Addr, OPC_NIL,
			ttl_value, InetI_Broadcast_v4_Addr, OPC_NIL);
		}
	else
		{
		/* This is an IPv6 destination	*/
		
		/* Insert the option in an AODV RREQ packet	*/
		rreq_pkptr = aodv_pkt_support_pkt_create (rreq_option_ptr, AODVC_RREQ_IPV6_SIZE);
	
		/* Encapsulate the AODV packet in an IP datagram	*/
		ip_rreq_pkptr = aodv_rte_ip_datagram_create (rreq_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL, 
			ttl_value, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
		
		/* Install the ICI for IPv6 case */
		ip_iciptr = op_ici_create ("ip_rte_req_v4");
		op_ici_attr_set_int32 (ip_iciptr, "multicast_major_port", mcast_major_port);
		op_ici_install (ip_iciptr);		
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, dest_addr);
		inet_address_to_hname (dest_addr, node_name);
		sprintf (temp_str, "destined to node %s (%s) with Request ID (%d)", addr_str, node_name, route_request_id);
		op_prg_odb_print_major (pid_string, "Broadcasting a route request packet", temp_str, OPC_NIL);
		}
	
	/* Insert the route request in the request table	*/
	aodv_request_table_orig_rreq_insert (request_table_ptr, route_request_id, dest_addr, 
											ttl_value, request_expiry_time, rreq_retry);
	
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, 
																					ip_rreq_pkptr);
	
	/* Update the statistics for the total route requests sent	*/
	op_stat_write (local_stat_handle_ptr->total_requests_sent_shandle, 1.0);
	op_stat_write (global_stat_handle_ptr->total_requests_sent_global_shandle, 1.0);
	
	/* Send the packet to the CPU with jitter which will broadcast it	*/
	/* after processing the packet										*/
	manet_rte_to_cpu_pkt_send_schedule_with_jitter (module_data_ptr, parent_prohandle, 
														parent_pro_id, ip_rreq_pkptr);
	
	/* Clear the ICI if installed */
	op_ici_install (OPC_NIL);	
	
	/* Increment the request ID	*/
	route_request_id++;
	
	/* Set the last broadcast sent time	*/
	last_broadcast_sent_time = op_sim_time ();
	
	FOUT;
	}


static void
aodv_rte_route_reply_send (AodvT_Rreq* rreq_option_ptr, IpT_Dgram_Fields* ip_dgram_fd_ptr, Boolean node_is_dest, 
						   AodvT_Route_Entry* rreq_src_route_entry_ptr, AodvT_Route_Entry* rreq_dest_route_entry_ptr)
	{
	AodvT_Packet_Option*	rrep_option_ptr;
	InetT_Address			next_hop_addr;
	int						hop_count;
	int						dest_seq_num;
	double					route_expiry_time;
	double					lifetime;
	Packet*					rrep_pkptr;
	Packet*					ip_rrep_pkptr;
	char					src_hop_addr_str [INETC_ADDR_STR_LEN];
	char					src_node_name [OMSC_HNAME_MAX_LEN];
	char					temp_str [256];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					temp_str1 [256];
	ManetT_Nexthop_Info*	manet_nexthop_info_ptr = OPC_NIL;
		
	/** Sends a route reply message	**/
	FIN (aodv_rte_route_reply_send (<args>));
	
	if (node_is_dest)
		{
		/* The node is the destination of the route request	*/
		/* Update sequence number according to the rule     */
		/* New Seq num = max ( current seq, RREQ seq +1 )   */
		
		rreq_option_ptr->dest_seq_num++;
		if(rreq_option_ptr->dest_seq_num > sequence_number)
			sequence_number = rreq_option_ptr->dest_seq_num;
	
		/* Get the next hop address */
		next_hop_addr = ip_dgram_fd_ptr->src_addr;
		
		/* Create the RREP option	*/
		rrep_option_ptr = aodv_pkt_support_rrep_option_create (OPC_FALSE, ack_required, 0, 
					rreq_option_ptr->dest_addr, sequence_number, rreq_option_ptr->src_addr, 
													my_route_timeout, AODVC_ROUTE_REPLY);
		
		if ((op_prg_odb_ltrace_active ("trace_rreq") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			inet_address_print (src_hop_addr_str, rreq_option_ptr->src_addr);
			inet_address_to_hname (rreq_option_ptr->src_addr, src_node_name);
			sprintf (temp_str, "destined to the RREQ Originator %s(%s)", src_hop_addr_str, src_node_name);
			op_prg_odb_print_major (pid_string, "Destination node sending a Route Reply", temp_str, OPC_NIL);
			}
		}
	else
		{
		/* This is an intermediate node that has a route to	*/
		/* the destination of the RREQ.						*/
		dest_seq_num      = rreq_dest_route_entry_ptr->dest_seq_num;
		hop_count         = rreq_dest_route_entry_ptr->hop_count;
		next_hop_addr     = rreq_dest_route_entry_ptr->next_hop_addr;
		route_expiry_time = rreq_dest_route_entry_ptr->route_expiry_time;		
		
		/* Update the precursor list for the forward		*/
		/* direction, ie, place the node from which the		*/
		/* RREQ was received in the precursor list of the	*/
		/* destination route table entry of the RREQ		*/
		aodv_route_table_precursor_add (rreq_dest_route_entry_ptr, ip_dgram_fd_ptr->src_addr);
		
		/* Update the precursor list in the reverse 		*/
		/* direction, ie, place the next hop node to which	*/
		/* RREQ would have been sent in the precursor list	*/
		/* of the originator field of the RREQ				*/
		aodv_route_table_precursor_add (rreq_src_route_entry_ptr, next_hop_addr);
		
		/* The lifetime of the RREP is calculated by 		*/
		/* subtracting the current time from expiration 	*/
		/* time in the route table entry					*/
		lifetime = route_expiry_time - op_sim_time ();
	
		if(lifetime < 0)
			{
			printf ("NEGATIVE LIFETIME, CurTime: %f, ExpTime:%f \n", op_sim_time(), route_expiry_time);
			}
	
		/* Create the RREP option	*/
		rrep_option_ptr = aodv_pkt_support_rrep_option_create (OPC_FALSE, ack_required, hop_count, 
			rreq_option_ptr->dest_addr, dest_seq_num, rreq_option_ptr->src_addr, lifetime, AODVC_ROUTE_REPLY);
		
		if (LTRACE_ACTIVE)
			{
			inet_address_print (src_hop_addr_str, rreq_option_ptr->src_addr);
			inet_address_to_hname (rreq_option_ptr->src_addr, src_node_name);
			inet_address_print (dest_hop_addr_str, rreq_option_ptr->dest_addr);
			inet_address_to_hname (rreq_option_ptr->dest_addr, dest_node_name);
			sprintf (temp_str, "RREQ originator was %s(%s)", src_hop_addr_str, src_node_name);
			sprintf (temp_str1, "It has valid route to destination %s (%s)", dest_hop_addr_str, dest_node_name);
			op_prg_odb_print_major (pid_string, "Intermediate node sending a Reply", temp_str, temp_str1, OPC_NIL);
			}
		}
	
	/* Encapsulate the RREP option in an AODV packet	*/
	if (inet_address_family_get (&rreq_option_ptr->src_addr) == InetC_Addr_Family_v4)
		rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV4_SIZE);
	else
		rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV6_SIZE);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Encapsulate the AODV packet in an IP datagram	*/
	ip_rrep_pkptr = aodv_rte_ip_datagram_create (rrep_pkptr, rreq_option_ptr->src_addr, rreq_src_route_entry_ptr,
		IPC_DEFAULT_TTL, ip_dgram_fd_ptr->src_addr, manet_nexthop_info_ptr);
	
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr,
											global_stat_handle_ptr,ip_rrep_pkptr);
	
	/* Install event state */
	/* This event state will be processed in ip_rte_support.ex.c while receiving AODV  */
	/* control packets. manet_nexthop_info_ptr will provide nexthop info.			   */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Send the packet to the CPU 	*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rrep_pkptr);
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	FOUT;
	}


static void
aodv_rte_grat_route_reply_send (AodvT_Rreq* rreq_option_ptr, AodvT_Route_Entry* rreq_src_route_entry_ptr, 
								AodvT_Route_Entry* rreq_dest_route_entry_ptr)
	{
	InetT_Address				next_hop_address;
	int							hop_count;
	double						route_expiry_time;
	double						lifetime;
	Packet*						rrep_pkptr;
	Packet*						ip_rrep_pkptr;
	AodvT_Packet_Option*		rrep_option_ptr;
	char						src_hop_addr_str [INETC_ADDR_STR_LEN];
	char						src_node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str1 [256];
	ManetT_Nexthop_Info*		manet_nexthop_info_ptr = OPC_NIL;
	
	/** Send a gratuitous route reply	**/
	FIN (aodv_rte_grat_route_reply_send (<args>));
	
	/* The gratuitous route reply flag is set	*/
	/* Send a gratuitous RREP to the 			*/
	/* destination node of the RREP				*/
	
	hop_count         = rreq_src_route_entry_ptr->hop_count;
	route_expiry_time = rreq_src_route_entry_ptr->route_expiry_time;
	next_hop_address  = rreq_src_route_entry_ptr->next_hop_addr;
	
	/* Calculate the remaining lifetime	*/
	lifetime = route_expiry_time - op_sim_time ();
	if(lifetime < 0)
			{
			printf("NEGATIVE LIFETIME, CurTime: %f, ExpTime:%f \n", op_sim_time(), route_expiry_time);
			}
	
	/* Create the gratuitous RREP message	*/
	rrep_option_ptr = aodv_pkt_support_rrep_option_create (OPC_FALSE, ack_required, hop_count, 
			rreq_option_ptr->src_addr, rreq_option_ptr->src_seq_num, rreq_option_ptr->dest_addr, lifetime, AODVC_ROUTE_REPLY);
	
	/* Set the gratuitous RREP option in the AODV packet	*/
	if (inet_address_family_get (&rreq_option_ptr->src_addr) == InetC_Addr_Family_v4)
		rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV4_SIZE);
	else
		rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV6_SIZE);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Encapsulate the AODV packet in an IP datagram	*/
	ip_rrep_pkptr = aodv_rte_ip_datagram_create (rrep_pkptr, rreq_option_ptr->dest_addr, rreq_dest_route_entry_ptr,
		IPC_DEFAULT_TTL, rreq_option_ptr->dest_addr, manet_nexthop_info_ptr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, rreq_option_ptr->src_addr);
		inet_address_to_hname (rreq_option_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, rreq_option_ptr->dest_addr);
		inet_address_to_hname (rreq_option_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "RREQ Destination: %s (%s)", dest_hop_addr_str, dest_node_name);
		sprintf (temp_str1, "RREQ Originator: %s (%s)", src_hop_addr_str, src_node_name);
		op_prg_odb_print_major (pid_string, "Sending a gratuitous route reply", temp_str, temp_str1, OPC_NIL);
		}
	
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, global_stat_handle_ptr, ip_rrep_pkptr);
	
	/* Install event state */
	/* This event will be processed in ip_rte_support.ex.c while receiving AODV  */
	/* control pakcets. manet_nexthop_info_ptr will provide nexthop info.		 */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Send the packet to the CPU 	*/
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rrep_pkptr);
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	FOUT;
	}

static void
aodv_rte_ext_route_error_send (InetT_Address unreachable_node_addr, int dest_seq_num)
	{
	List*						unreachable_dest_lptr;
	AodvT_Packet_Option*		rerr_option_ptr;
	AodvT_Unreachable_Node*		unreachable_node_ptr;
	Packet*						rerr_pkptr;
	Packet*						ip_rerr_pkptr;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	Ici*						ip_iciptr;
	int							mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
	int							aodv_rerr_size = 0;
	
	/** Creates and sends a RERR for routes with external hosts	**/
	/** that failed. The RERR message is broadcast into the 	**/
	/** MANET cluster.											**/
	FIN (aodv_rte_ext_route_error_send ());

	/* Create a list to get the unreachable destinations	*/
	unreachable_dest_lptr = op_prg_list_create ();
	
	/* Add this destination to the unreachable dest.list	*/
	unreachable_node_ptr = aodv_pkt_support_unreachable_nodes_mem_alloc ();
	unreachable_node_ptr->unreachable_dest = inet_address_copy (unreachable_node_addr);
	unreachable_node_ptr->unreachable_dest_seq_num = dest_seq_num;
	op_prg_list_insert (unreachable_dest_lptr, unreachable_node_ptr, OPC_LISTPOS_TAIL);

	/* Create the route error option	*/
	rerr_option_ptr = aodv_pkt_support_rerr_option_create (OPC_FALSE, 1, unreachable_dest_lptr);
	
	/* Compute and set the size for RERR packet */
	aodv_rerr_size = (aodv_addressing_mode == InetC_Addr_Family_v4)? 64 : 160;
	aodv_rerr_size += 32;
	
	/* Set the route error option in the AODV packet	*/
	rerr_pkptr = aodv_pkt_support_pkt_create (rerr_option_ptr, aodv_rerr_size);

	/* Debug trace.								*/
	if (op_prg_odb_ltrace_active ("adov_conn"))
		{
		inet_address_print (addr_str, unreachable_node_addr);
		inet_address_to_hname (unreachable_node_addr, node_name);
	
		/* Destroy the Inet addresses used here */
		sprintf (temp_str, "External route to [%s](%s) is not available", addr_str, node_name);
		op_prg_odb_print_major (pid_string, "MANET Connectivity: Broadcasting a route error packet for an external route", temp_str, OPC_NIL);
		}

	/* If there is only one unique neighbor, then the	*/
	/* route error is sent unicast to that neighbor.	*/
	/* Else, the route error packet is broadcast with	*/
	/* Destination IP = 255.255.255.255 and TTL = 1		*/

	/* For an error in any external route	*/
	/* broadcast the route error message.	*/
	if (aodv_addressing_mode == InetC_Addr_Family_v4)
		{
		ip_rerr_pkptr = aodv_rte_ip_datagram_create (rerr_pkptr, InetI_Broadcast_v4_Addr, OPC_NIL, 1,
														InetI_Broadcast_v4_Addr, OPC_NIL);
		}
	else
		{
		ip_rerr_pkptr = aodv_rte_ip_datagram_create (rerr_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL,
														1, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
	
		/* Install the ICI for IPv6 case */
		ip_iciptr = op_ici_create ("ip_rte_req_v4");
		op_ici_attr_set (ip_iciptr, "multicast_major_port", mcast_major_port);
		op_ici_install (ip_iciptr);	
		}
	
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rerr_pkptr);
		
	/* Clear the ICI if installed */
	op_ici_install (OPC_NIL);
	
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr,
										global_stat_handle_ptr, ip_rerr_pkptr);
	
	/* Update the statistic for the total route errors sent	*/
	op_stat_write (local_stat_handle_ptr->total_route_errors_sent_shandle, 1.0);
	op_stat_write (global_stat_handle_ptr->total_route_errors_sent_global_shandle, 1.0);
	
	FOUT;
	}

static void
aodv_rte_route_error_process (InetT_Address unreachable_node_addr, AodvT_Route_Entry* unreach_node_route_entry_ptr,
							  List* unreachable_nodes_lptr, AodvC_Rerr_Process rerr_type, Boolean no_delete)
	{
	List*						unreachable_dest_lptr;
	List*						precursor_lptr;
	InetT_Address				unique_precursor_addr;
	int							num_unreachable_dest;
	int							dest_seq_num;
	int							count, num, size;
	int							num_precursors;
	AodvT_Packet_Option*		rerr_option_ptr;
	AodvT_Unreachable_Node*		unreachable_node_ptr;
	AodvT_Unreachable_Node*		dest_node_ptr;
	Packet*						rerr_pkptr;
	Packet*						ip_rerr_pkptr;
	InetT_Address*				precursor_addr_ptr;
	InetT_Address*				unique_addr_ptr;
	char						addr_str [INETC_ADDR_STR_LEN];
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						temp_str [256];
	int							num_routes;
	List* 						rentry_lptr = OPC_NIL;
	AodvT_Route_Entry*			route_entry_ptr;
	InetT_Address				dest_addr;
	InetT_Address*				node_addr_ptr;
	Boolean						Got_One_Precursor;
	ManetT_Nexthop_Info*		manet_nexthop_info_ptr = OPC_NIL;
	Ici*						ip_iciptr;
	int							mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
	int							aodv_rerr_size = 0;
			
	/** Process Route Error depending upon link break **/
	/** No Route or Received Route Error.			  **/
	/** Send a route error message to the precursor	  **/
	/** nodes that have the next hop as the			  **/
	/** unreachable node.							  **/
	
	FIN (aodv_rte_route_error_process (<args>));
	
	Got_One_Precursor = OPC_FALSE;
	if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, unreachable_node_addr);
			inet_address_to_hname (unreachable_node_addr, node_name);
			sprintf (temp_str, " %s (%s) as unreachable node", addr_str, node_name);
			op_prg_odb_print_major (pid_string, "Entered ROUTE ERROR function with", temp_str, OPC_NIL);
			}

	/* Create a list to get the unreachable destinations	*/
	unreachable_dest_lptr = op_prg_list_create ();
	
	if (rerr_type == AodvC_Data_Packet_No_Route)
		{
		/* A data packet has arrived at this node, but there is	*/
		/* no route to the destination of the data packet and 	*/
		/* the node is not repairing (if using local repair)	*/
		/* There is only one unreachable destination in this	*/
		/* case which is the destination of the data packet		*/
		/* Check if there exists an invalid entry for this		*/
		/* destination. If so, use that sequence number			*/
		if (aodv_route_table_entry_param_get (unreach_node_route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, &dest_seq_num) == OPC_COMPCODE_FAILURE)
			{
			/* There does not exist a route entry. Set the		*/
			/* destination sequence number as invalid			*/
			FOUT;
			}
		
		if(no_delete == OPC_FALSE)
			{
			/* Invalidate the route */
			aodv_route_table_entry_state_set (route_table_ptr, unreach_node_route_entry_ptr, unreachable_node_addr, AodvC_Invalid_Route);
			
			}
			
		if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "RERR type: No Route for Data Packet", OPC_NIL);
			
		/* Check if there are any pre-cursors to the route to	*/
		/* this destination node.								*/
		if(aodv_route_table_entry_param_get (unreach_node_route_entry_ptr, AODVC_ROUTE_ENTRY_PRECURSOR_LIST, &precursor_lptr) == OPC_COMPCODE_FAILURE)
			{
			num_precursors =0;
			}
		else
			{
			/* Get the number of precursors */
			num_precursors = op_prg_list_size (precursor_lptr);
			}
		
		if (num_precursors == 0)
			{
			/* There are no precursor node	*/
			/* Do not send the route error*/
			if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "NUM_PRECURSORS = 0, SO NO RERR", OPC_NIL);
			}
		else 
			{
			
			if (num_precursors == 1)
				{
				/* There is one precursor node	*/
				/* Send the route error unicast	*/
				precursor_addr_ptr = (InetT_Address*) op_prg_list_remove (precursor_lptr, OPC_LISTPOS_HEAD);
				unique_addr_ptr = inet_address_copy_dynamic (precursor_addr_ptr);
				inet_address_destroy_dynamic (precursor_addr_ptr);
				unique_precursor_addr = *unique_addr_ptr;
				
				if (LTRACE_ACTIVE)
					op_prg_odb_print_major (pid_string, "NUM_PRECURSORS = 1, UNICAST RERR", OPC_NIL);
				}
			else
				{
				/* There are more than one		*/
				/* precursor nodes. Broadcast	*/
				/* the route error message		*/
				unique_precursor_addr = INETC_ADDRESS_INVALID;
				
				while (op_prg_list_size (precursor_lptr) > 0)
					{
					precursor_addr_ptr = (InetT_Address*) op_prg_list_remove (precursor_lptr, OPC_LISTPOS_TAIL);
					inet_address_destroy_dynamic (precursor_addr_ptr);
					}
				
				if (LTRACE_ACTIVE)
					op_prg_odb_print_major (pid_string, "NUM_PRECURSORS = MORE THAN 1, BCAST RERR", OPC_NIL);
				}
				
			/* Add this destination to the unreachable dest.list	*/
			unreachable_node_ptr = aodv_pkt_support_unreachable_nodes_mem_alloc ();
			unreachable_node_ptr->unreachable_dest = inet_address_copy (unreachable_node_addr);
			unreachable_node_ptr->unreachable_dest_seq_num = dest_seq_num;
			op_prg_list_insert (unreachable_dest_lptr, unreachable_node_ptr, OPC_LISTPOS_TAIL);
			}
		
		}/* if(rerr_type == AodvC_Data_Packet_No_Route) */
	else
		{
		
		/* Get the number of routes in the route table	*/
		rentry_lptr = (List *) inet_addr_hash_table_item_list_get (route_table_ptr->route_table, aodv_addressing_mode);
		num_routes = op_prg_list_size (rentry_lptr);
			
		if(rerr_type == AodvC_Link_Break_Detect)
			{
				
			/* A link break has been detected. This could be due	*/
			/* to not receiving a hello packet or any other data 	*/
			/* packet from the neighbor node.						*/
			/* Get a list of all destinations in the route table	*/
			/* that have the unreachable neighbor as the next hop	*/
				
			if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "RERR TYPE: LINK BREAK \n", OPC_NIL);
			
			for (count = 0; count < num_routes; count++)
				{
				/* For each route entry check if the route is valid	*/
				route_entry_ptr = (AodvT_Route_Entry *) op_prg_list_access (rentry_lptr, count);
				
				/* Get the destination address for this route entry	*/
				dest_addr = ip_cmn_rte_table_dest_prefix_addr_get (route_entry_ptr->dest_prefix);
					
				
				/* This is for LinkBreak Detect, check if we are using this unreachable node */
				/* to reach any destination. Add them to form a unreachable dest. list 		 */
				if( (inet_address_equal (unreachable_node_addr, route_entry_ptr->next_hop_addr))
						&&(route_entry_ptr->route_entry_state == AodvC_Valid_Route))
					{						
					/* If no_delete is not set, mark route entry invalid */
					if(no_delete == OPC_FALSE)
						{
						/* Invalidate the route */
						aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, dest_addr, AodvC_Invalid_Route);						
						}
					
						/* Get the number of precursors */
						num_precursors = op_prg_list_size (route_entry_ptr->precursor_lptr);
						
						if(num_precursors > 0)
							{
							/* Add this destination to the unreachable dest. list	*/
							unreachable_node_ptr = aodv_pkt_support_unreachable_nodes_mem_alloc ();
							unreachable_node_ptr->unreachable_dest = dest_addr;
							unreachable_node_ptr->unreachable_dest_seq_num = route_entry_ptr->dest_seq_num;
							op_prg_list_insert (unreachable_dest_lptr, unreachable_node_ptr, OPC_LISTPOS_TAIL);
							
							/* Add total number of affected neigbors in the precursor list */
							if((num_precursors == 1) && (Got_One_Precursor == OPC_FALSE))
								{
								precursor_addr_ptr = (InetT_Address*) op_prg_list_remove (route_entry_ptr->precursor_lptr, OPC_LISTPOS_HEAD);
								unique_addr_ptr = inet_address_copy_dynamic (precursor_addr_ptr);
								inet_address_destroy_dynamic (precursor_addr_ptr);
								unique_precursor_addr = *unique_addr_ptr;
								Got_One_Precursor = OPC_TRUE;
								}
							else if(num_precursors > 1)
								{
								
								unique_precursor_addr = INETC_ADDRESS_INVALID;
								while (op_prg_list_size (route_entry_ptr->precursor_lptr) > 0)
									{
									node_addr_ptr = (InetT_Address*) op_prg_list_remove (route_entry_ptr->precursor_lptr, OPC_LISTPOS_TAIL);
									inet_address_destroy_dynamic (node_addr_ptr);
									}
								}
								
							} /* Num_precursor > 0 */
							
						} 
				} /* for each route in table */
				
			}/* when it was LinkBreak Detect */
		
		else if (rerr_type == AodvC_Rerr_Received)
			{			
			/* Recevied a Route Error, process it */
			if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "RERR TYPE: RECEIVED RERR \n", OPC_NIL);
			
			/* Get the number of unreachable destinations */
			size = op_prg_list_size (unreachable_nodes_lptr);
			   
			for (count = 0; count < num_routes; count++)
				{
				/* For each route entry check if the route is valid	*/
				route_entry_ptr = (AodvT_Route_Entry*) op_prg_list_access (rentry_lptr, count);
				
				/* Get the destination address for this route entry	*/
				dest_addr = ip_cmn_rte_table_dest_prefix_addr_get (route_entry_ptr->dest_prefix);
									
				for (num = 0; num < size; num ++)
					{
					dest_node_ptr = (AodvT_Unreachable_Node*) op_prg_list_access (unreachable_nodes_lptr, num);
						
					/* If i have unreachable destination and the node from which i received */
					/* route error is next hop, then mark that route invalid          		*/
					if ((inet_address_equal (dest_node_ptr->unreachable_dest, dest_addr)) 
							&& ( inet_address_equal(unreachable_node_addr, route_entry_ptr->next_hop_addr))
							&& (route_entry_ptr->route_entry_state == AodvC_Valid_Route))
						{
							
						if(no_delete == OPC_FALSE)
							{
							/* Invalidate the route */
							aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, dest_addr, AodvC_Invalid_Route);
							
							aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, dest_addr);
							}
						
						/* Copy seq no from the rerr */
						aodv_route_table_entry_param_set (route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, dest_node_ptr->unreachable_dest_seq_num);

						/* Get the number of precursors */
						num_precursors = op_prg_list_size (route_entry_ptr->precursor_lptr);
						
						if(num_precursors > 0)
							{
							/* Add it to the list				*/
							unreachable_node_ptr = aodv_pkt_support_unreachable_nodes_mem_alloc ();
							unreachable_node_ptr->unreachable_dest = dest_addr;
							unreachable_node_ptr->unreachable_dest_seq_num = route_entry_ptr->dest_seq_num;
							op_prg_list_insert (unreachable_dest_lptr, unreachable_node_ptr, OPC_LISTPOS_TAIL);
						
							if((num_precursors == 1) && (Got_One_Precursor == OPC_FALSE))
								{
								precursor_addr_ptr = (InetT_Address*) op_prg_list_remove (route_entry_ptr->precursor_lptr, OPC_LISTPOS_HEAD);
								unique_addr_ptr = inet_address_copy_dynamic (precursor_addr_ptr);
								inet_address_destroy_dynamic (precursor_addr_ptr);
								unique_precursor_addr = *unique_addr_ptr;
								Got_One_Precursor = OPC_TRUE ;
								}
							else if(num_precursors > 1)
								{
								unique_precursor_addr = INETC_ADDRESS_INVALID;
								while (op_prg_list_size (route_entry_ptr->precursor_lptr) > 0)
									{
									node_addr_ptr = (InetT_Address*) op_prg_list_remove (route_entry_ptr->precursor_lptr, OPC_LISTPOS_TAIL);
									inet_address_destroy_dynamic (node_addr_ptr);
									}
								}
							} /* num_precursors > 0 */
						}
					}
				}  /* for each route in the table */
			} /* when it was RERR recvd */
		
		/* Free the keys	*/
		prg_list_destroy (rentry_lptr, OPC_FALSE);
		
		} /* close else i.e. if (lbrk||recvd rerr) */
	
	/* Get the number of unreachable destinations	*/
	num_unreachable_dest = op_prg_list_size (unreachable_dest_lptr);
	
	/* There are no destinations in this node's route table	*/
	/* that use the neighbor lost as a part of an active 	*/
	/* route or there are no precursor nodes to that route	*/
	if (num_unreachable_dest == 0)
		{
		if (LTRACE_ACTIVE)
			op_prg_odb_print_major (pid_string, "NUM OF UNREACHABLE DESTINATIONS = 0, FREE MEM, NO RERR", OPC_NIL);
			
		/* Free the list	*/
		op_prg_mem_free (unreachable_dest_lptr);
		
		FOUT;
		}
	
	/* Create the route error option	*/
	rerr_option_ptr = aodv_pkt_support_rerr_option_create (no_delete, num_unreachable_dest,
																unreachable_dest_lptr);
	
	/* Compute and set the size for RERR packet */
	aodv_rerr_size = (aodv_addressing_mode == InetC_Addr_Family_v4)? 
							num_unreachable_dest*64 : num_unreachable_dest*160;
	aodv_rerr_size += 32;
	
	/* Set the route error option in the AODV packet	*/
	rerr_pkptr = aodv_pkt_support_pkt_create (rerr_option_ptr, aodv_rerr_size);
	
	/* If there is only one unique neighbor, then the	*/
	/* route error is sent unicast to that neighbor.	*/
	/* Else, the route error packet is broadcast with	*/
	/* Destination IP = 255.255.255.255 and TTL = 1		*/
	if (inet_address_equal (unique_precursor_addr, INETC_ADDRESS_INVALID))
		{
		/* There are a number of neighbors	*/
		/* Broadcast the route error message*/
		if (aodv_addressing_mode == InetC_Addr_Family_v4)
			{
			ip_rerr_pkptr = aodv_rte_ip_datagram_create (rerr_pkptr, InetI_Broadcast_v4_Addr, OPC_NIL, 1,
															InetI_Broadcast_v4_Addr, OPC_NIL);
			}
		else
			{
			ip_rerr_pkptr = aodv_rte_ip_datagram_create (rerr_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL,
														1, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
		
			/* Install the ICI for IPv6 case */
			ip_iciptr = op_ici_create ("ip_rte_req_v4");
			op_ici_attr_set_int32 (ip_iciptr, "multicast_major_port", mcast_major_port);
			op_ici_install (ip_iciptr);	
			}
		
		if (LTRACE_ACTIVE)
			op_prg_odb_print_major (pid_string, "Broadcasting a route error packet", OPC_NIL);
		
		manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rerr_pkptr);
		
		/* Clear the ICI if installed */
		op_ici_install (OPC_NIL);
		
		}
	else
		{
		/* There is only one neighbor node				*/
		/* Send it unicast to that node					*/
		/* Allocate memory for manet_nexthop_info_ptr 	*/
		manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
		ip_rerr_pkptr = aodv_rte_ip_datagram_create (rerr_pkptr, unique_precursor_addr, 
													 aodv_route_table_entry_get (route_table_ptr, unique_precursor_addr), 
													 1, unique_precursor_addr, manet_nexthop_info_ptr);
		
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, unique_precursor_addr);
			inet_address_to_hname (unique_precursor_addr, node_name);
			}
		
		/* Destroy the Inet addresses used here */
		inet_address_destroy_dynamic (unique_addr_ptr);
		
		if ((manet_nexthop_info_ptr->output_table_index == IPC_INTF_INDEX_INVALID) ||
			(manet_nexthop_info_ptr->interface_info_ptr == OPC_NIL))
			{
			/* Could not find route in aodv route 	*/
			/* table to send this RERR. Destroy the	*/
			/* packets.								*/
			if (LTRACE_ACTIVE)
				{
				sprintf (temp_str, "to %s (%s) to send the RERR packet", addr_str, node_name);
				op_prg_odb_print_major (pid_string, "Could NOT find route", temp_str, OPC_NIL);
				}
			
			/* Get the RERR packet from IP datagram */
			op_pk_nfd_get_pkt (ip_rerr_pkptr, "data", &rerr_pkptr);
			op_pk_destroy (rerr_pkptr);
			
			manet_rte_ip_pkt_destroy (ip_rerr_pkptr);
			FOUT;
			}
		else
			{		
			if (LTRACE_ACTIVE)
				{
				sprintf (temp_str, "destined to neighbor node %s (%s)", addr_str, node_name);
				op_prg_odb_print_major (pid_string, "Sending a route error packet", temp_str, OPC_NIL);
				}
		
			/* Install event state */
			/* This event will be processed in ip_rte_support.ex.c while receiving AODV */
			/* control packets. manet_nexthop_info_ptr will provide nexthop info. 		*/
			op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
			manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rerr_pkptr);
			op_ev_state_install (OPC_NIL, OPC_NIL);
			}
		
		}
	
	/* Update the statistics for the routing traffic sent	*/
	aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr,
		global_stat_handle_ptr, ip_rerr_pkptr);
	
	/* Update the statistic for the total route errors sent	*/
	op_stat_write (local_stat_handle_ptr->total_route_errors_sent_shandle, 1.0);
	op_stat_write (global_stat_handle_ptr->total_route_errors_sent_global_shandle, 1.0);
	
	FOUT;
	}


static void
aodv_rte_all_pkts_to_dest_send (InetT_Address dest_addr, AodvT_Route_Entry* route_entry_ptr, Boolean reached_src)
	{
	List*							pkt_lptr;
	int								num_pkts, count;
	Packet*							ip_pkptr;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr;
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr;
	InetT_Address					next_hop_addr;
	char							addr_str [INETC_ADDR_STR_LEN];
	char							node_name [OMSC_HNAME_MAX_LEN];
	char							temp_str[256];
	AodvT_Packet_Entry*				pk_entry_ptr = OPC_NIL;
	IpT_Port_Info					output_port_info;
	int								output_intf_index;
	
	/** Sends all application packets queued to	**/
	/** the specified destination.				**/
	FIN (aodv_rte_all_pkts_to_dest_send (<args>));
	
	/* Get all packets queued to the destination	*/
	pkt_lptr = aodv_packet_queue_all_pkts_to_dest_get (pkt_queue_ptr, dest_addr, OPC_TRUE, reached_src);
	
	if (pkt_lptr == OPC_NIL)
		FOUT;
	
	/* Get the number of packets	*/
	num_pkts = op_prg_list_size (pkt_lptr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (addr_str, dest_addr);
		inet_address_to_hname (dest_addr, node_name);
		sprintf (temp_str, "Sending out all buffered packets for the destination %s (%s).", addr_str, node_name);
		op_prg_odb_print_major (pid_string, temp_str, OPC_NIL);
		}
	
	for (count = 0; count < num_pkts; count++)
		{
		pk_entry_ptr = (AodvT_Packet_Entry*) op_prg_list_remove (pkt_lptr, OPC_LISTPOS_HEAD); 
		ip_pkptr = pk_entry_ptr->pkptr;
		
		/* Access the packet information	*/
		manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
		
		/* Get the next hop address and output port info */
		next_hop_addr    = inet_address_copy (route_entry_ptr->next_hop_addr);
		output_port_info = route_entry_ptr->next_hop_port_info;
		
		/* Get the output interface index */
		output_intf_index = ip_rte_intf_tbl_index_from_port_info_get (module_data_ptr, output_port_info);
		
		/* Send the packet to the MAC	*/
		manet_rte_to_mac_pkt_send_options (module_data_ptr, ip_pkptr, next_hop_addr,
			ip_dgram_fd_ptr, intf_ici_fdstruct_ptr, output_intf_index);

		op_prg_mem_free (pk_entry_ptr);		
		}
	
	/* Destroy the list	*/
	prg_list_clear (pkt_lptr, OPC_NIL);
	op_prg_mem_free (pkt_lptr);
	
	FOUT;
	}


static void
aodv_rte_rrep_hello_message_send (void)
	{
	AodvT_Packet_Option*	rrep_option_ptr;
	Packet*					rrep_pkptr;
	Packet*					ip_rrep_pkptr;
	double					hello_interval;
	Ici*					ip_iciptr;
	int						mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
	
	
	/** Checks to see if a hello message should be	**/
	/** be broadcast for determining connectivity	**/
	/** to its neighbor nodes						**/
	FIN (aodv_rte_rrep_hello_message_send (void));
	
	/* Get the hello interval	*/
	hello_interval = oms_dist_outcome (hello_interval_dist_ptr);
	
	if (((op_sim_time () - last_broadcast_sent_time) < hello_interval) || (route_table_ptr->active_route_count == 0))
		{
		/* No hello message needs to be sent as a 	*/
		/* broadcast message was sent less than the	*/
		/* last hello interval or this node is not 	*/
		/* a part of any active route.				*/		
		}
	else
		{
		
		if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE)|| LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Broadcasting a hello message", OPC_NIL);
			}
		
		/* No broadcast message has been sent for more than	*/
		/* the last hello interval. Broadcast a RREP packet	*/
		/* Last broadcast sent time is updated only when    */
		/* a non hello packet is broadcasted.				*/
		rrep_option_ptr = aodv_pkt_support_rrep_option_create (OPC_FALSE, OPC_FALSE, 0, 
				INETC_ADDRESS_INVALID, sequence_number, INETC_ADDRESS_INVALID, 
				allowed_hello_loss * hello_interval, AODVC_HELLO);
	
		if (aodv_addressing_mode == InetC_Addr_Family_v4)
			{
			/* Create the RREP hello packet	*/
			rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV4_SIZE);
	
			/* Encapsulate the AODV packet in an IP datagram	*/
			ip_rrep_pkptr = aodv_rte_ip_datagram_create (rrep_pkptr, InetI_Broadcast_v4_Addr, OPC_NIL, 1, 
															InetI_Broadcast_v4_Addr, OPC_NIL);
			}
		else
			{
			/* Create the RREP hello v6 packet	*/
			rrep_pkptr = aodv_pkt_support_pkt_create (rrep_option_ptr, AODVC_RREP_IPV6_SIZE);
	
			/* Encapsulate the AODV packet in an IP datagram	*/
			ip_rrep_pkptr = aodv_rte_ip_datagram_create (rrep_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL,
															1,InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
			
			/* Install the ICI for IPv6 case */
			ip_iciptr = op_ici_create ("ip_rte_req_v4");
			op_ici_attr_set_int32 (ip_iciptr, "multicast_major_port", mcast_major_port);
			op_ici_install (ip_iciptr);	
			}
	
		/* Update the statistics for the routing traffic sent	*/
		aodv_support_routing_traffic_sent_stats_update (local_stat_handle_ptr, 
										global_stat_handle_ptr, ip_rrep_pkptr);
	
		/* Send the packet to the CPU 	*/
		manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_rrep_pkptr);
		
		/* Clear the ICI if installed */
		op_ici_install (OPC_NIL);	
		}
		
	/* Schedule the next check to send hello messages	*/
	op_intrpt_schedule_self (op_sim_time () + hello_interval, AODVC_HELLO_TIMER_EXPIRY);
		
	FOUT;
	}


extern void
aodv_rte_entry_expiry_handle (void* route_entry_vptr, int code)
	{	
	AodvT_Route_Entry*		route_entry_ptr;
	InetT_Address			dest_addr;
	char					dest_addr_str [INETC_ADDR_STR_LEN];
	InetT_Address*			node_addr_ptr;
		
	/** Handles the interrupts when route entry timer **/
	/** expires. Depending on the code it either 	  **/
	/** invalidates the routes or delete them.		  **/
	FIN (aodv_rte_entry_expiry_handle (<args>));
	
	/* Cast the void pointer to route entry handle and retrieve the	*/
	/* route's destination address.									*/
	route_entry_ptr = (AodvT_Route_Entry *) route_entry_vptr;
	dest_addr = ip_cmn_rte_table_dest_prefix_addr_get (route_entry_ptr->dest_prefix);
	
	if(code == AODVC_ROUTE_ENTRY_INVALID)
		{		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major(OPC_NIL);
			inet_address_print (dest_addr_str, dest_addr);
			printf("Time: %f, Entry expired for dest: %s, Marking Route Invalid \n", op_sim_time(), dest_addr_str);
			}

		/* Mark the entry for this dest addr to be invalid in routing table */
		aodv_route_table_entry_state_set (route_table_ptr, route_entry_ptr, dest_addr, AodvC_Invalid_Route);		
		aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, dest_addr);
		
		/* Remove all the precusor nodes from the precursor list */
		if( op_prg_list_size (route_entry_ptr->precursor_lptr) > 0 )
			{
			while (op_prg_list_size (route_entry_ptr->precursor_lptr) > 0)
				{
				node_addr_ptr = (InetT_Address*) op_prg_list_remove (route_entry_ptr->precursor_lptr, OPC_LISTPOS_TAIL);
				inet_address_destroy_dynamic (node_addr_ptr);
				}
			}
		}
	
	else if (code == AODVC_ROUTE_ENTRY_EXPIRED)
		{
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major(OPC_NIL);
			inet_address_print (dest_addr_str, dest_addr);
			printf("Time: %f, Invalid Entry Expired for dest: %s, Deleting Route \n", op_sim_time(), dest_addr_str);
			}

		/* Delete the entry for this dest from the routing table */
		aodv_route_table_entry_delete (route_table_ptr, dest_addr);
		aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, dest_addr);
		}
	
	FOUT;
	}


extern void
aodv_rte_rreq_timer_expiry_handle (void* rreq_id_ptr1, int PRG_ARG_UNUSED(code))
	{
	AodvT_Orig_Request_Entry*		request_entry_ptr;
	int								num_pkts;
	int								new_ttl_value;
	int*							req_id_ptr;
	int								req_id;
	double							ring_traversal_time;
	
	/** The route request timer has expired. Resend	**/
	/** a new route request if possible				**/
	FIN (aodv_rte_rreq_timer_expiry_handle (<args>));
	
	/* Get the request ID of the expiry	*/
	req_id_ptr = (int*) rreq_id_ptr1;
	req_id = *req_id_ptr;
	
	/* Get the request entry that expired	*/
	request_entry_ptr = aodv_route_request_orig_entry_get (request_table_ptr, req_id, OPC_TRUE);
	
	if (LTRACE_ACTIVE)
		{
		op_prg_odb_print_major (pid_string, "The route request timer has expired.", OPC_NIL);
		printf("Rreq-Retires done so far: %d \n", request_entry_ptr->num_retries);
		}
	
	/* The route request timer has expired		*/
	/* Check if there are any packets queued	*/
	/* to the destination of the route request	*/

	/* Check the number of packets	*/
	num_pkts = aodv_packet_queue_num_pkts_get (pkt_queue_ptr, 
		request_entry_ptr->target_address);
	
	if (num_pkts == 0)
		{
		/* There are no packets queued to the destination	*/
		}
	else if (request_entry_ptr->num_retries >= request_table_ptr->max_rreq_retries)
		{
		
		/* There are packets queued to the destination.	*/
		/* No more route request retries are possible	*/
		/* as the maximum retries have been reached.	*/
		
		/* Send out a route error message to the source	*/
		/* of the route request if it is not this node	*/
		aodv_rte_route_error_process (request_entry_ptr->target_address, 
									  aodv_route_table_entry_get (route_table_ptr, request_entry_ptr->target_address),
									  OPC_NIL, AodvC_Data_Packet_No_Route, OPC_FALSE);
		
		/* If this was a local repair attempt, remove	*/
		/* it from the queue of local repair nodes		*/
		if (local_repair)
			aodv_rte_local_repair_exists (request_entry_ptr->target_address, OPC_TRUE);
		
		/* Discard all data packets queued to this		*/
		/* destination address							*/
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Request Retries Limit Reached. \
				Dropping all buffered packets.", OPC_NIL);
			}
		
		aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, request_entry_ptr->target_address);
		}
	else
		{
		/* An expanding ring search is used to find the	*/
		/* destination by incrementing the TTL value in	*/
		/* the route request for every retry. Calculate	*/ 
		/* the new TTL value to place in the request	*/
		
		new_ttl_value = request_entry_ptr->current_ttl_value + ttl_increment;
		
		/* If the TTL value is greater than the TTL 	*/
		/* threshold, set the TTL to the net diameter	*/
		if (new_ttl_value >= ttl_threshold)
			{
			new_ttl_value = net_diameter;
			
			if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "TTL Threshold reached. Assigning TTL = Net Diameter", OPC_NIL);
			
			/* Once the TTL value is greater than the	*/
			/* TTL threshold, the request expiry time 	*/
			/* is set to the net traversal time and 	*/
			/* uses a binary exponential backoff for	*/
			/* every new request sent out after this	*/
			if (request_entry_ptr->current_request_expiry_time < net_traversal_time)
				{
				ring_traversal_time = net_traversal_time;
				}
			else
				{
				/* Binary exponential backoff	*/
				ring_traversal_time = 2 * request_entry_ptr->current_request_expiry_time;
				
				if (LTRACE_ACTIVE)
					printf("Exponential backoff time: %f", ring_traversal_time);
				}
			}
		else
			{
			
			if (LTRACE_ACTIVE)
				op_prg_odb_print_major (pid_string, "Calculating new_ttl & Ring Traversal time", OPC_NIL);
						
			/* Calculate the ring traversal time to receive	*/
			/* a route reply before the request times out	*/
			ring_traversal_time = 2.0 * node_traversal_time * (new_ttl_value  + timeout_buffer);
			}
		
		/* The maximum retries have not been reached	*/
		/* Send out a new route request packet			*/
		request_entry_ptr->num_retries++;
			
		aodv_rte_route_request_send (aodv_route_table_entry_get (route_table_ptr, request_entry_ptr->target_address), 
									 request_entry_ptr->target_address, new_ttl_value, ring_traversal_time, request_entry_ptr->num_retries);
		}

	op_prg_mem_free (req_id_ptr);
	aodv_request_table_orig_entry_mem_free (request_entry_ptr);
	
	FOUT;
	}


static Compcode
aodv_rte_local_repair_attempt (InetT_Address dest_addr, AodvT_Route_Entry* route_entry_ptr)
	{
	int					dest_hop_count, src_hop_count;
	int					dest_seq_num;
	int					ttl_value;
	double				ring_traversal_time;
	InetT_Address*		copy_addr_ptr;
		
	/** Performs local repair to the destination of the	**/
	/** incoming data packet as there is no route		**/
	FIN (aodv_rte_local_repair_attempt (<args>));
	
	/* There is an incoming data packet from another	*/
	/* node in the network that has no valid route to	*/
	/* the destination. Perform a local repair			*/

	/* To repair a link break the node increments the	*/
	/* destination sequence	number for that destination	*/
	/* and then	broadcasts a route request				*/
	
	/* Get the number of hops to the destination from this node	*/
	aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_HOP_COUNT, &dest_hop_count);
	
	/* Get the number of hop to the source from this node	*/
	aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_HOP_COUNT, &src_hop_count);
	
	/* Do not perform local repair if the	*/
	/* destination is further than 			*/
	/* MAX_REPAIR_TTL hops away				*/
	if (dest_hop_count > max_repair_ttl)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* Increment the destination sequence number	*/
	aodv_route_table_entry_param_get (route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, &dest_seq_num);
	
	dest_seq_num++;
	aodv_route_table_entry_param_set (route_entry_ptr, AODVC_ROUTE_ENTRY_DEST_SEQ_NUM, dest_seq_num);

	/* Calculate the TTL value	*/
	ttl_value = (int) aodv_rte_max_find ((double) dest_hop_count, (double) 0.5 * src_hop_count) + local_add_ttl;
	
	/* Calculate the ring traversal time by which a 	*/
	/* RREP message should be received					*/
	ring_traversal_time = 2.0 * node_traversal_time * (ttl_value  + timeout_buffer);
		
	/* Broadcast a route request	*/
	aodv_rte_route_request_send (route_entry_ptr, dest_addr, ttl_value, ring_traversal_time, 0);
	
	/* Insert this destination into the list of	destinations	*/
	/* that this node is performing local repair.				*/
	copy_addr_ptr = inet_address_create_dynamic (dest_addr);
	op_prg_list_insert (local_repair_nodes_lptr, copy_addr_ptr, OPC_LISTPOS_TAIL);
	
	FRET (OPC_COMPCODE_SUCCESS);
	}


static Boolean
aodv_rte_local_repair_exists (InetT_Address dest_addr, Boolean remove)
	{
	InetT_Address*		dest_addr_ptr;
	InetT_Address*		node_addr_ptr;
	int					num_dest, count;
	
	/** Checks if the destination address is under	**/
	/** local repair. If the remove flag is set, 	**/
	/** then the destination would be removed from	**/
	/** the list of destinations undergoing repair	**/
	FIN (aodv_rte_local_repair_exists (<args>));
	
	dest_addr_ptr = inet_address_create_dynamic (dest_addr);
	num_dest = op_prg_list_size (local_repair_nodes_lptr);
	for (count = 0; count < num_dest; count++)
		{
		node_addr_ptr = (InetT_Address*) op_prg_list_access (local_repair_nodes_lptr, count);
		if (inet_address_ptr_equal (dest_addr_ptr, node_addr_ptr))
			{
			if (remove)
				{
				node_addr_ptr = (InetT_Address*) op_prg_list_remove (local_repair_nodes_lptr, count);
				inet_address_destroy_dynamic (node_addr_ptr);
				}
			
			inet_address_destroy_dynamic (dest_addr_ptr);
			FRET (OPC_TRUE);
			}
		}
	
	inet_address_destroy_dynamic (dest_addr_ptr);
	FRET (OPC_FALSE);
	}


static void
aodv_rte_neighbor_connectivity_table_update (InetT_Address neighbor_addr, Boolean is_hello_pkt)
	{
	double				current_time;
	double				conn_expiry_time;
	double				hello_interval;
	AodvT_Conn_Info*	neighbor_conn_info_ptr = OPC_NIL;
	void*				old_contents_ptr = OPC_NIL;
	
	/** Updates the neighbor connectivity table	**/
	FIN (aodv_rte_neighbor_connectivity_table_update (<args>));
	
	/* Get the current time	*/
	current_time = op_sim_time ();
	
	/* Get the hello interval	*/
	hello_interval = oms_dist_outcome (hello_interval_dist_ptr);
	
	/* Calculate the connectivity expiry time	*/
	conn_expiry_time = current_time + (allowed_hello_loss * hello_interval);
	
	/* Access the connectivity information of the neighbor	*/
	neighbor_conn_info_ptr = (AodvT_Conn_Info *) inet_addr_hash_table_item_get (neighbor_connectivity_table, &neighbor_addr);
	
	if (neighbor_conn_info_ptr == OPC_NIL)
		{
		/* This is a new neighbor. If this is not	*/
		/* a hello message, then just ignore.		*/
		/* Behavior change: A new neighbor will be created for ANY message	*/
		/* This is so, because an RREP from a new neighbor will be honored for example	*/
		/* if (is_hello_pkt != OPC_TRUE)
			{
			FOUT;
			} 
		else */
			{
			/* This is a hello message received	 from a	*/
			/* new neighbor. Allocate memory to store 	*/
			/* the last time a packet was received by 	*/
			/* this neighbor							*/
			neighbor_conn_info_ptr = (AodvT_Conn_Info *) op_prg_mem_alloc (sizeof (AodvT_Conn_Info));
			inet_addr_hash_table_item_insert (neighbor_connectivity_table, &neighbor_addr, neighbor_conn_info_ptr, &old_contents_ptr);
			neighbor_conn_info_ptr->neighbor_address = inet_address_copy (neighbor_addr);
			
			/* Behavior change: If this is a hello message, only then update the time	*/
			/* Otherwise simply initialize it to some value								*/
			if (is_hello_pkt)
				neighbor_conn_info_ptr->last_hello_received_time = current_time;
			else
				neighbor_conn_info_ptr->last_hello_received_time = 0.0;
			
			neighbor_conn_info_ptr->conn_expiry_handle = 
				op_intrpt_schedule_call (conn_expiry_time, AODVC_CONN_LOSS_TIMER, aodv_rte_neighbor_conn_loss_handle, neighbor_conn_info_ptr);
			}
		}
	else
		{
		/* This is an existing neighbor	*/
		if (is_hello_pkt)
			{
			/* A hello packet is received by this node	*/
			/* from its neighbor node.					*/
			neighbor_conn_info_ptr->last_hello_received_time = current_time;
			
			/* Cancel the old event to determine loss of	*/
			/* connectivity to the neighbor and schedule	*/
			/* a new interrupt with the new time			*/
			op_ev_cancel_if_pending (neighbor_conn_info_ptr->conn_expiry_handle);
			
			neighbor_conn_info_ptr->conn_expiry_handle = 
				op_intrpt_schedule_call (conn_expiry_time, AODVC_CONN_LOSS_TIMER, aodv_rte_neighbor_conn_loss_handle, neighbor_conn_info_ptr);
			}
		else
			{
			/* If the last hello was received is less than	*/
			/* the delete period, then reschedule the		*/
			/* interrupt that indicates the link loss.		*/
			if (current_time - neighbor_conn_info_ptr->last_hello_received_time <= delete_period)
				{
				/* Cancel the old event unless it is the	*/
				/* current one and schedule the new one.	*/
				op_ev_cancel_if_pending (neighbor_conn_info_ptr->conn_expiry_handle);		
				neighbor_conn_info_ptr->conn_expiry_handle = 
					op_intrpt_schedule_call (conn_expiry_time, AODVC_CONN_LOSS_TIMER, aodv_rte_neighbor_conn_loss_handle, neighbor_conn_info_ptr);
				}
			}
		}
	
	FOUT;		
	}


static void
aodv_rte_neighbor_conn_loss_handle (void* neighbor_conn_info_ptr1, int PRG_ARG_UNUSED (code))
	{
	AodvT_Conn_Info*			neighbor_conn_info_ptr;
	
	/** Connection to the neighbor node has been	**/
	/** lost. Handle the loss appropriately			**/
	FIN (aodv_rte_neighbor_conn_loss_handle (<args>));
	
	neighbor_conn_info_ptr = (AodvT_Conn_Info*) neighbor_conn_info_ptr1;
	
	if (LTRACE_ACTIVE)
		{
		op_prg_odb_print_major(OPC_NIL);
		printf("Time: %f, CONNECTION LOST. Last Hello received at %f\n", op_sim_time(),			
									neighbor_conn_info_ptr->last_hello_received_time);
		}
	 
	
	aodv_packet_queue_all_pkts_to_dest_delete (pkt_queue_ptr, neighbor_conn_info_ptr->neighbor_address);
	
	/* Send out route error messages	*/
	aodv_rte_route_error_process (neighbor_conn_info_ptr->neighbor_address, OPC_NIL, OPC_NIL, AodvC_Link_Break_Detect, OPC_FALSE);
	
	/* Remove the entry for this node from the neighbor connectivity hash table */
	inet_addr_hash_table_item_remove (neighbor_connectivity_table, &(neighbor_conn_info_ptr->neighbor_address));
	inet_address_destroy (neighbor_conn_info_ptr->neighbor_address);
	op_prg_mem_free (neighbor_conn_info_ptr);
		
	FOUT;
	}


static Packet*
aodv_rte_ip_datagram_create (Packet* aodv_pkptr, InetT_Address dest_addr, AodvT_Route_Entry* route_entry_ptr,
	int ttl_value, InetT_Address next_hop_address, ManetT_Nexthop_Info* nexthop_info_ptr)
	{
	Packet*					ip_pkptr = OPC_NIL;
	IpT_Dgram_Fields*		ip_dgram_fd_ptr = OPC_NIL;
	OpT_Packet_Size			aodv_pkt_size;
	InetT_Address			src_address;
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	short					output_intf_index;
	IpT_Port_Info			output_port_info;
		
	/** Creates an IP datagram and encapsulates	**/
	/** the input packet in the data field		**/
	/** Also populates the fields for nexthop   **/
	/** in ManetT_Nexthop_Info if its not a 	**/
	/** broadcast packet.						**/
	FIN (aodv_rte_ip_datagram_create (<args>));
	
	/* Get the size of the AODV packet	*/
	aodv_pkt_size = op_pk_total_size_get (aodv_pkptr);
	
	/* Create an IP datagram.	*/
	ip_pkptr = op_pk_create_fmt ("ip_dgram_v4");

	/* Set the AODV packet in the data field of the IP datagram	*/
	op_pk_nfd_set_pkt (ip_pkptr, "data", aodv_pkptr);
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data. 						*/
	op_pk_bulk_size_set (ip_pkptr, aodv_pkt_size);

	/* Create fields data structure that contains orig_len,	*/
	/* ident, frag_len, ttl, src_addr, dest_addr, frag,		*/
	/* connection class, src and dest internal addresses.	*/
	ip_dgram_fd_ptr = ip_dgram_fdstruct_create ();
	
	/* Set the destination address for this IP datagram.	*/
	ip_dgram_fd_ptr->dest_addr = inet_address_copy (dest_addr);
	ip_dgram_fd_ptr->connection_class = 0;
	
	/* Set the correct protocol in the IP datagram.	*/
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Aodv;

	/* Set the packet size-related fields of the IP datagram.	*/
	ip_dgram_fd_ptr->orig_len = aodv_pkt_size/8;
	ip_dgram_fd_ptr->frag_len = ip_dgram_fd_ptr->orig_len;
	ip_dgram_fd_ptr->original_size = 160 + aodv_pkt_size;

	/* Indicate that the packet is not yet fragmented.	*/
	ip_dgram_fd_ptr->frag = 0;

	/* Set the type of service.	*/
	ip_dgram_fd_ptr->tos = 0;
	
	/* Set the TTL value in the packet	*/
	ip_dgram_fd_ptr->ttl = ttl_value;
	
	/* Also set the compression method and original size fields	*/
	ip_dgram_fd_ptr->compression_method = IpC_No_Compression;

	/* The record route options has not been set.	*/
	ip_dgram_fd_ptr->options_field_set = OPC_FALSE;
	
	/* Set the next-hop address - will be used during wlan-mac processing */
	ip_dgram_fd_ptr->next_addr = inet_address_copy (next_hop_address);
	
	if (((inet_address_family_get (&dest_addr) == InetC_Addr_Family_v4) && 
		(inet_address_equal (dest_addr, InetI_Broadcast_v4_Addr) == OPC_FALSE)) ||
		((inet_address_family_get (&dest_addr) == InetC_Addr_Family_v6) && 
		(inet_address_equal (dest_addr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr) == OPC_FALSE)))
		{		
		/* Fill ManetT_Nexthop_Info fields if   	*/
		/* destination address is not broadcast		*/

		if (route_entry_ptr != OPC_NIL)
			{
			/* Get the output port info from the aodv route table entry */
			output_port_info = route_entry_ptr->next_hop_port_info;
			
			/* Get the interface info ptr and source address based on address family */
			output_intf_index = ip_rte_intf_tbl_index_from_port_info_get (module_data_ptr, output_port_info);
			iface_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, output_intf_index);
			src_address = inet_rte_intf_addr_get (iface_info_ptr, inet_address_family_get (&next_hop_address));
			
			/* Set the source address */
			ip_dgram_fd_ptr->src_addr = inet_address_copy (src_address);
			
			if (nexthop_info_ptr != OPC_NIL)
				{
				nexthop_info_ptr->next_hop_addr = inet_address_copy (next_hop_address);
				nexthop_info_ptr->output_table_index = (int) output_intf_index;
				nexthop_info_ptr->interface_info_ptr = iface_info_ptr;
				}
			else
				{
				printf("Error condition. aodv_rte_ip_datagram_create()\n");
				}
			}
		else
			{
			/* Unable to determine the outgoing interface	*/
			/* as there is no route present in aodv route	*/
			/* table for this destination node 				*/
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major (pid_string, 
					" aodv_rte_ip_datagram_create. Unable to determine outgoing interface",OPC_NIL);
				}
			
			if (nexthop_info_ptr != OPC_NIL)
				{
				/* Initialize the nexthop_info_ptr if the calling function	*/
				/* is expecting valud values. This will allow the function	*/
				/* to catch INVALID outgoing interface before sending pkt	*/
				nexthop_info_ptr->output_table_index = IPC_INTF_INDEX_INVALID;
				nexthop_info_ptr->interface_info_ptr = OPC_NIL;
				}
			
			}
		}
	
	/*	Set the fields structure inside the ip datagram	*/
	op_pk_nfd_set (ip_pkptr, "fields", ip_dgram_fd_ptr, 
			ip_dgram_fdstruct_copy, ip_dgram_fdstruct_destroy, sizeof (IpT_Dgram_Fields));
		
	FRET (ip_pkptr);
	}


static double
aodv_rte_max_find (double value_A, double value_B)
	{
	/** Finds the maximum value	**/
	FIN (aodv_rte_max_find (<args>));
	
	if (value_A > value_B)
		{
		FRET (value_A);
		}
	else
		{
		FRET (value_B);
		}
	}


static void
aodv_rte_error (const char* str1, const char* str2, const char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (aodv_rte_error <args>);
	
	op_sim_end ("AODV Routing Process : ", str1, str2, str3);
	
	FOUT;
	}

extern void
aodv_rte_forward_request_delete (void* inet_addr_vptr, int req_id)	
	{
	InetT_Address*					orig_addr_ptr;
	List* 							req_entry_lptr; 
	int 							i;
	AodvT_Forward_Request_Entry* 	req_entry_ptr;
	
	FIN (aodv_rte_forward_request_delete (<Args>));
	
	orig_addr_ptr = (InetT_Address *) inet_addr_vptr;
	
	/* Check if an entry for this address exists */
	req_entry_lptr = (List *) inet_addr_hash_table_item_get (request_table_ptr->forward_request_table, orig_addr_ptr);
	
	/* Error Check	*/
	if (req_entry_lptr == OPC_NIL) 
		FOUT;

	for (i = 0; i < op_prg_list_size (req_entry_lptr); i++)
		{
		req_entry_ptr = (AodvT_Forward_Request_Entry*) op_prg_list_access (req_entry_lptr, i);
		
		if (req_entry_ptr->request_id == req_id)
			{
			op_prg_list_remove (req_entry_lptr, i);
			op_prg_mem_free (req_entry_ptr);
			break;
			}
		}
			
		
	if (op_prg_list_size (req_entry_lptr) == 0)
		{
		inet_addr_hash_table_item_remove (request_table_ptr->forward_request_table, orig_addr_ptr);
		
		op_prg_mem_free (req_entry_lptr);
		}
		
	inet_address_destroy_dynamic (orig_addr_ptr);
	
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
	void aodv_rte (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_aodv_rte_init (int * init_block_ptr);
	void _op_aodv_rte_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_aodv_rte_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_aodv_rte_alloc (VosT_Obtype, int);
	void _op_aodv_rte_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
aodv_rte (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (aodv_rte ());

		{
		/* Temporary Variables */
		Prohandle			invoker_prohandle;
		int					invoke_mode;
		int					intrpt_type;
		int					intrpt_code;
		ManetT_Info*		manet_info_ptr;
		/* End of Temporary Variables */


		FSM_ENTER ("aodv_rte")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "aodv_rte [init enter execs]")
				FSM_PROFILE_SECTION_IN ("aodv_rte [init enter execs]", state0_enter_exec)
				{
				/* Initializa the state variables	*/
				aodv_rte_sv_init ();
				
				/* Register the local statistics	*/
				aodv_rte_local_stats_reg ();
				
				/* Parse the attributes and create		*/
				/* and initialize the various buffers	*/
				aodv_rte_attributes_parse_buffers_create ();
				
				/* Register the AODV Routing process as a higher layer in IP	*/
				Ip_Higher_Layer_Protocol_Register ("aodv", &higher_layer_proto_id);
				
				/* Add directly connected routes only if	*/
				/* this is not a MANET gateway.				*/
				if (!ip_manet_gateway_enabled(module_data_ptr))		
					{
					aodv_rte_add_directly_connected_routes ();
					}
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "aodv_rte [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "wait", "tr_0", "aodv_rte [init -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "aodv_rte [wait enter execs]")
				FSM_PROFILE_SECTION_IN ("aodv_rte [wait enter execs]", state1_enter_exec)
				{
				/* Wait for a packet arrival	*/
				/* This will be an invoke from	*/
				/* another process, eg : CPU	*/
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"aodv_rte")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "aodv_rte [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("aodv_rte [wait exit execs]", state1_exit_exec)
				{
				invoker_prohandle = op_pro_invoker (own_prohandle, &invoke_mode);
				
				intrpt_type = op_intrpt_type ();
				intrpt_code = op_intrpt_code ();
				
				
				if (invoke_mode == OPC_PROINV_INDIRECT)
					{	
					manet_info_ptr = (ManetT_Info *) op_pro_argmem_access();
					
					if (manet_info_ptr->code == MANETC_PACKET_ARRIVAL)
						{		
						aodv_rte_pkt_arrival_handle();
						}
					
					else if (manet_info_ptr->code == MANETC_DATA_ROUTE)
						{
						/* IP is forwarding a datagram using one of our	*/
						/* active routes. Update the route lifetime for	*/
						/* the routes of the source, previous hop, next	*/
						/* hop and destination of the routed packet.	*/
						aodv_rte_data_routes_expiry_time_update ((Packet *) manet_info_ptr->manet_info_type_ptr);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_PROFILE_SECTION_IN ("aodv_rte [wait trans conditions]", state1_trans_conds)
			FSM_INIT_COND (HELLO_TIMER_EXPIRY)
			FSM_TEST_COND (PACKET_ARRIVAL)
			FSM_TEST_LOGIC ("wait")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, aodv_rte_rrep_hello_message_send ();, "HELLO_TIMER_EXPIRY", "aodv_rte_rrep_hello_message_send ()", "wait", "wait", "tr_11", "aodv_rte [wait -> wait : HELLO_TIMER_EXPIRY / aodv_rte_rrep_hello_message_send ()]")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ;, "PACKET_ARRIVAL", "", "wait", "wait", "tr_15", "aodv_rte [wait -> wait : PACKET_ARRIVAL / ]")
				}
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"aodv_rte")
		}
	}




void
_op_aodv_rte_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_aodv_rte_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		aodv_support_route_table_print_to_string (route_table_ptr, aodv_addressing_mode);
		aodv_support_conn_table_print_to_string (neighbor_connectivity_table, aodv_addressing_mode);
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_aodv_rte_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_aodv_rte_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_aodv_rte_svar function. */
#undef module_data_ptr
#undef own_mod_objid
#undef own_node_objid
#undef own_prohandle
#undef parent_prohandle
#undef route_table_ptr
#undef pkt_queue_ptr
#undef route_request_id
#undef sequence_number
#undef hello_interval_dist_ptr
#undef net_diameter
#undef node_traversal_time
#undef last_route_error_sent_time
#undef num_route_errors_sent
#undef route_error_rate
#undef last_route_request_sent_time
#undef num_route_requests_sent
#undef route_request_rate
#undef timeout_buffer
#undef ttl_start
#undef ttl_increment
#undef ttl_threshold
#undef local_add_ttl
#undef max_repair_ttl
#undef my_route_timeout
#undef net_traversal_time
#undef path_discovery_time
#undef request_table_ptr
#undef local_stat_handle_ptr
#undef global_stat_handle_ptr
#undef grat_route_reply_flag
#undef dest_only_flag
#undef ack_required
#undef aodv_proto_id
#undef own_pro_id
#undef parent_pro_id
#undef subnet_mask
#undef subnet_mask_ipv6
#undef last_broadcast_sent_time
#undef allowed_hello_loss
#undef neighbor_connectivity_table
#undef delete_period
#undef local_repair
#undef local_repair_nodes_lptr
#undef higher_layer_proto_id
#undef pid_string
#undef aodv_addressing_mode
#undef external_routes_cache_table_ptr

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_aodv_rte_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_aodv_rte_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (aodv_rte)",
		sizeof (aodv_rte_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_aodv_rte_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	aodv_rte_state * ptr;
	FIN_MT (_op_aodv_rte_alloc (obtype))

	ptr = (aodv_rte_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "aodv_rte [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_aodv_rte_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	aodv_rte_state		*prs_ptr;

	FIN_MT (_op_aodv_rte_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (aodv_rte_state *)gen_ptr;

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
	if (strcmp ("route_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_table_ptr);
		FOUT
		}
	if (strcmp ("pkt_queue_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pkt_queue_ptr);
		FOUT
		}
	if (strcmp ("route_request_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_request_id);
		FOUT
		}
	if (strcmp ("sequence_number" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sequence_number);
		FOUT
		}
	if (strcmp ("hello_interval_dist_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_interval_dist_ptr);
		FOUT
		}
	if (strcmp ("net_diameter" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->net_diameter);
		FOUT
		}
	if (strcmp ("node_traversal_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_traversal_time);
		FOUT
		}
	if (strcmp ("last_route_error_sent_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_route_error_sent_time);
		FOUT
		}
	if (strcmp ("num_route_errors_sent" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_route_errors_sent);
		FOUT
		}
	if (strcmp ("route_error_rate" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_error_rate);
		FOUT
		}
	if (strcmp ("last_route_request_sent_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_route_request_sent_time);
		FOUT
		}
	if (strcmp ("num_route_requests_sent" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_route_requests_sent);
		FOUT
		}
	if (strcmp ("route_request_rate" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_request_rate);
		FOUT
		}
	if (strcmp ("timeout_buffer" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->timeout_buffer);
		FOUT
		}
	if (strcmp ("ttl_start" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ttl_start);
		FOUT
		}
	if (strcmp ("ttl_increment" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ttl_increment);
		FOUT
		}
	if (strcmp ("ttl_threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ttl_threshold);
		FOUT
		}
	if (strcmp ("local_add_ttl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->local_add_ttl);
		FOUT
		}
	if (strcmp ("max_repair_ttl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->max_repair_ttl);
		FOUT
		}
	if (strcmp ("my_route_timeout" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_route_timeout);
		FOUT
		}
	if (strcmp ("net_traversal_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->net_traversal_time);
		FOUT
		}
	if (strcmp ("path_discovery_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->path_discovery_time);
		FOUT
		}
	if (strcmp ("request_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->request_table_ptr);
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
	if (strcmp ("grat_route_reply_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->grat_route_reply_flag);
		FOUT
		}
	if (strcmp ("dest_only_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dest_only_flag);
		FOUT
		}
	if (strcmp ("ack_required" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ack_required);
		FOUT
		}
	if (strcmp ("aodv_proto_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->aodv_proto_id);
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
	if (strcmp ("last_broadcast_sent_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_broadcast_sent_time);
		FOUT
		}
	if (strcmp ("allowed_hello_loss" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->allowed_hello_loss);
		FOUT
		}
	if (strcmp ("neighbor_connectivity_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighbor_connectivity_table);
		FOUT
		}
	if (strcmp ("delete_period" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->delete_period);
		FOUT
		}
	if (strcmp ("local_repair" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->local_repair);
		FOUT
		}
	if (strcmp ("local_repair_nodes_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->local_repair_nodes_lptr);
		FOUT
		}
	if (strcmp ("higher_layer_proto_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->higher_layer_proto_id);
		FOUT
		}
	if (strcmp ("pid_string" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pid_string);
		FOUT
		}
	if (strcmp ("aodv_addressing_mode" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->aodv_addressing_mode);
		FOUT
		}
	if (strcmp ("external_routes_cache_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->external_routes_cache_table_ptr);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

