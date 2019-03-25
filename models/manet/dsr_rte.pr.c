/* Process model C form file: dsr_rte.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char dsr_rte_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EED8 5C91EED8 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/* Includes	*/
#include <ip_rte_support.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <ip_higher_layer_proto_reg_sup.h>
#include <manet.h>
#include <dsr.h>
#include <dsr_pkt_support.h>
#include <dsr_ptypes.h>

/* Transitions	*/
#define PACKET_ARRIVAL			(invoke_mode == OPC_PROINV_INDIRECT)
#define TIMER_EXPIRY			((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_PROCEDURE) && \
								((intrpt_code == DSRC_ROUTE_REQUEST_TIMER) || (intrpt_code == DSRC_ROUTE_MAINTENANCE_TIMER)))
#define ROUTE_CACHE_DUMP		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_PROCEDURE) && \
								(intrpt_code == DSRC_ROUTE_CACHE_EXPORT))

/* Traces	*/
#define LTRACE_ACTIVE			(op_prg_odb_ltrace_active ("dsr_rte") || \
								op_prg_odb_ltrace_active ("manet"))

/* Prototypes	*/
static void						dsr_rte_sv_init (void);
static void						dsr_rte_attributes_parse_buffers_create (void);
static void						dsr_rte_received_pkt_handle (void);
static void						dsr_rte_app_pkt_arrival_handle (Packet*, IpT_Rte_Ind_Ici_Fields*, IpT_Dgram_Fields*, Boolean);
static void						dsr_rte_received_route_request_process (Packet*, DsrT_Packet_Option*);
static void						dsr_rte_received_route_reply_process (Packet*, DsrT_Packet_Option*);
static void						dsr_rte_received_route_error_process (Packet*, DsrT_Packet_Option*);
static Compcode					dsr_rte_received_ack_request_process (Packet*);
static void						dsr_rte_received_acknowledgement_option_process (Packet*, DsrT_Packet_Option*);
static void						dsr_rte_received_dsr_source_route_option_process (Packet*);
static void						dsr_rte_route_request_send (InetT_Address, Boolean);
static void						dsr_rte_route_reply_send (Packet*, DsrT_Packet_Option*);
static void						dsr_rte_maintenance_send (InetT_Address, InetT_Address, int);
static void						dsr_rte_route_error_send (Packet*, InetT_Address, IpT_Dgram_Fields*, IpT_Rte_Ind_Ici_Fields*);
static void						dsr_rte_cached_route_reply_send (DsrT_Packet_Option*, DsrT_Path_Info*, IpT_Dgram_Fields*);
static void						dsr_rte_send_buffer_check (List*);
static Compcode					dsr_rte_automatic_route_shortening_check (IpT_Dgram_Fields*, 
														DsrT_Source_Route_Option*, IpT_Rte_Ind_Ici_Fields*);
static void						dsr_rte_route_cache_update (DsrT_Packet_Option*, IpT_Dgram_Fields*);
static Boolean					dsr_rte_packet_salvage (Packet*, InetT_Address, IpT_Dgram_Fields*, IpT_Rte_Ind_Ici_Fields*);
static Packet*					dsr_rte_ip_datagram_encapsulate (Packet*, Packet*, InetT_Address);
static Packet*					dsr_rte_ip_datagram_decapsulate (Packet*);
static Packet*					dsr_rte_ip_datagram_create (Packet*, InetT_Address, InetT_Address, ManetT_Nexthop_Info*);
static DsrT_Packet_Type			dsr_rte_packet_type_determine (IpT_Dgram_Fields*, IpT_Rte_Ind_Ici_Fields*);
static DsrT_Packet_Option*		dsr_rte_packet_option_get (Packet*, int);
static void						dsr_rte_jitter_schedule (Packet*, int);
static void						dsr_rte_error (const char*, const char*, const char*);
static void						dsr_temp_list_clear (List*);


/* Extern Functions	*/
EXTERN_C_BEGIN
void						dsr_rte_route_request_expiry_handle (InetT_Address*, int);
void						dsr_rte_maintenance_expiry_handle (void*, int);
static void					dsr_rte_jittered_pkt_send (void*, int);

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
	Objid	                  		own_mod_objid                                   ;	/* Objid of thie module  */
	Objid	                  		own_node_objid                                  ;	/* Objid of this node  */
	Prohandle	              		own_prohandle                                   ;	/* Own process handle  */
	Prohandle	              		parent_prohandle                                ;	/* Process handle of the parent process (manet_mgr)  */
	char	                   		proc_model_name [10]                            ;	/* Process Model Name  */
	OmsT_Dist_Handle	       		broadcast_jitter_dist_ptr                       ;	/* A small delay is applied to broadcasts from a node that is     */
	                        		                                                	/* uniformly distributed between 0 and the Broadcast Jitter in    */
	                        		                                                	/* the following cases :                                          */
	                        		                                                	/*                                                                */
	                        		                                                	/* 1) When a node creates and sends a route reply (not for nodes  */
	                        		                                                	/* forwarding the route reply)                                    */
	                        		                                                	/* 2) When a node re-broadcasts a route request (not for nodes   */
	                        		                                                	/* that originate the route request).                     */
	DsrT_Route_Cache*	      		route_cache_ptr                                 ;	/* Pointer to the Route Cache  */
	DsrT_Send_Buffer*	      		send_buffer_ptr                                 ;	/* Pointer to the send buffer  */
	DsrT_Maintenance_Buffer*			maint_buffer_ptr                                ;	/* Pointer to the maintenance buffer  */
	int	                    		higher_layer_proto_id                           ;	/* Registered ID to IP for the DSR as a higher layer protocol  */
	double	                 		maint_ack_req_timer                             ;	/* The duration before the maintenace buffer will timeout if  */
	                        		                                                	/* no acknowledgement is received for the acknowledgement     */
	                        		                                                	/* request that was sent out                                  */
	int	                    		ack_request_identifier                          ;	/* Acknowledgement request identifier  */
	int	                    		own_pro_id                                      ;	/* Own process ID  */
	int	                    		parent_pro_id                                   ;	/* Parent process ID  */
	int	                    		route_request_identifier                        ;	/* Route Request Identifier  */
	DsrT_Route_Reply_Table*			grat_reply_table_ptr                            ;	/* Pointer to the "Gratuitous" route reply table  */
	DsrT_Route_Request_Table*			route_request_table_ptr                         ;	/* Pointer to the route request table  */
	char	                   		pid_string [32]                                 ;	/* Display string  */
	DsrT_Stathandles*	      		stat_handle_ptr                                 ;	/* Pointer to the statistic handles  */
	Boolean	                		routes_export                                   ;	/* Flag to indicate whether to export the routes to a file  */
	DsrT_Global_Stathandles*			global_stathandle_ptr                           ;	/* Pointer to the structure holding the global stathandles  */
	Boolean	                		routes_dump                                     ;	/* Indicates whether to dump the routes  */
	Boolean	                		packet_salvaging_function                       ;	/* Flag to indicate whether packet salvaging has been enabled  */
	                        		                                                	/* on this node                                                */
	Boolean	                		cached_route_replies_function                   ;	/* Flag to indicate whether route replies using cached routes  */
	                        		                                                	/* is enabled or not                                           */
	Boolean	                		non_propagating_request_function                ;	/* Flag to indicate whether the non propagating request function  */
	                        		                                                	/* has been enabled on this node                                  */
	} dsr_rte_state;

#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define own_mod_objid           		op_sv_ptr->own_mod_objid
#define own_node_objid          		op_sv_ptr->own_node_objid
#define own_prohandle           		op_sv_ptr->own_prohandle
#define parent_prohandle        		op_sv_ptr->parent_prohandle
#define proc_model_name         		op_sv_ptr->proc_model_name
#define broadcast_jitter_dist_ptr		op_sv_ptr->broadcast_jitter_dist_ptr
#define route_cache_ptr         		op_sv_ptr->route_cache_ptr
#define send_buffer_ptr         		op_sv_ptr->send_buffer_ptr
#define maint_buffer_ptr        		op_sv_ptr->maint_buffer_ptr
#define higher_layer_proto_id   		op_sv_ptr->higher_layer_proto_id
#define maint_ack_req_timer     		op_sv_ptr->maint_ack_req_timer
#define ack_request_identifier  		op_sv_ptr->ack_request_identifier
#define own_pro_id              		op_sv_ptr->own_pro_id
#define parent_pro_id           		op_sv_ptr->parent_pro_id
#define route_request_identifier		op_sv_ptr->route_request_identifier
#define grat_reply_table_ptr    		op_sv_ptr->grat_reply_table_ptr
#define route_request_table_ptr 		op_sv_ptr->route_request_table_ptr
#define pid_string              		op_sv_ptr->pid_string
#define stat_handle_ptr         		op_sv_ptr->stat_handle_ptr
#define routes_export           		op_sv_ptr->routes_export
#define global_stathandle_ptr   		op_sv_ptr->global_stathandle_ptr
#define routes_dump             		op_sv_ptr->routes_dump
#define packet_salvaging_function		op_sv_ptr->packet_salvaging_function
#define cached_route_replies_function		op_sv_ptr->cached_route_replies_function
#define non_propagating_request_function		op_sv_ptr->non_propagating_request_function

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	dsr_rte_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((dsr_rte_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
dsr_rte_sv_init (void)
	{
	/** Initialize the state variables	**/
	FIN (dsr_rte_sv_init (void));
	
	/* Access the module data memory	*/
	module_data_ptr = (IpT_Rte_Module_Data*) op_pro_modmem_access ();
	
	/* Initialize variables used for process registry.	*/
	/* Obtain the tcp2 module's objid. */
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
	
	/* Obtain the name of the process. It is the "process model" attribute of the node. */
	op_ima_obj_attr_get (own_mod_objid, "process model", proc_model_name);
	
	/* Initialize the unique identifiers	*/
	ack_request_identifier = 0;
	route_request_identifier = 0;
	
	/* Set up the display string. */
	sprintf (pid_string, "dsr_rte PID (%d)", own_pro_id);
	
	/* Determine whether to export the routes to a file	*/
	routes_export = dsr_support_route_export_sim_attr_get ();
	
	if (routes_export == OPC_TRUE)
		{
		/* Initialize the OT package	*/
		dsr_support_route_print_to_ot_initialize ();
		
		/* Schedule a call interrupt to close the file	*/
		op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, 0, dsr_support_route_print_to_ot_close, OPC_NIL);
		}
	
	/* Determine whether to dump the routes for display	*/
	routes_dump = dsr_support_route_record_sim_attr_get ();
	
	/* Get a handle to the global statistics	*/
	global_stathandle_ptr = dsr_support_global_stat_handles_obtain ();
	
	/* Initialize the print procedure for the options field in the DSR packet	*/ 
	op_pk_format_print_proc_set ("dsr", "Options", dsr_pkt_support_options_print);
	
	FOUT;
	}


static void
dsr_rte_stats_reg (void)
	{
	FIN (dsr_rte_stats_reg (void));
	
	/* Allocate memory for the structure that stores	*/
	/* all the statistic handles						*/
	stat_handle_ptr = (DsrT_Stathandles*) op_prg_mem_alloc (sizeof (DsrT_Stathandles));
		
	/* Register each of the local statistic	*/
	stat_handle_ptr->route_cache_size_shandle = op_stat_reg ("DSR.Route Cache Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->route_cache_hit_success_shandle = op_stat_reg ("DSR.Route Cache Access Success", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->route_cache_hit_failure_shandle = op_stat_reg ("DSR.Route Cache Access Failure", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->route_discovery_time_shandle = op_stat_reg ("DSR.Route Discovery Time", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->maintenance_buffer_size_shandle = op_stat_reg ("DSR.Maintenance Buffer Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->request_table_size_shandle = op_stat_reg ("DSR.Request Table Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->num_hops_shandle = op_stat_reg ("DSR.Number of Hops per Route", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->send_buffer_size_shandle = op_stat_reg ("DSR.Send Buffer Size", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->num_pkts_discard_shandle = op_stat_reg ("DSR.Total Packets Dropped", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->rte_traf_rcvd_bps_shandle = op_stat_reg ("DSR.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->rte_traf_rcvd_pps_shandle = op_stat_reg ("DSR.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->rte_traf_sent_bps_shandle = op_stat_reg ("DSR.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->rte_traf_sent_pps_shandle = op_stat_reg ("DSR.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_traf_rcvd_bps_shandle = op_stat_reg ("DSR.Total Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_traf_rcvd_pps_shandle = op_stat_reg ("DSR.Total Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_traf_sent_bps_shandle = op_stat_reg ("DSR.Total Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_traf_sent_pps_shandle = op_stat_reg ("DSR.Total Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	stat_handle_ptr->total_requests_sent_shandle = op_stat_reg ("DSR.Total Route Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_non_prop_requests_sent_shandle = op_stat_reg ("DSR.Total Non Propagating Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_prop_requests_sent_shandle = op_stat_reg ("DSR.Total Propagating Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_replies_sent_shandle = op_stat_reg ("DSR.Total Route Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_replies_sent_from_dest_shandle = op_stat_reg ("DSR.Total Replies Sent from Destination", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_cached_replies_sent_shandle = op_stat_reg ("DSR.Total Cached Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_ack_requests_sent_shandle = op_stat_reg ("DSR.Total Acknowledgement Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_acks_sent_shandle = op_stat_reg ("DSR.Total Acknowledgements Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_route_errors_sent_shandle = op_stat_reg ("DSR.Total Route Errors Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	stat_handle_ptr->total_pkts_salvaged_shandle = op_stat_reg ("DSR.Total Packets Salvaged", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	FOUT;
	}
	


static void
dsr_rte_attributes_parse_buffers_create (void)
	{
	int				route_cache_size;
	double			route_cache_timer;
	int				send_buffer_size;
	double			send_buffer_timer;
	int				request_table_size;
	int				max_request_table_id;
	int				max_request_retrans;
	double			max_request_period;
	double			init_request_period;
	double			non_propagating_request_timer;
	int				retrans_buffer_size;
	double			grat_route_reply_timer;
	double			maint_holdoff_time;
	int				max_maint_retrans;
	double			maint_ack_timer;
	Boolean			route_cache_export_status = OPC_FALSE;
	int				num_rows, count;
	double			export_time;
	char			broadcast_jitter_str [128];
	Objid			dsr_parms_id, dsr_parms_child_id;
	Objid			route_cache_parms_id, route_cache_parms_child_id;
	Objid			cache_export_parms_id, cache_export_parms_child_id;
	Objid			cache_time_export_id, cache_time_export_child_id;
	Objid			send_buffer_parms_id, send_buffer_parms_child_id;
	Objid			rte_dis_parms_id, rte_dis_parms_child_id;
	Objid			rte_maint_parms_id, rte_maint_parms_child_id;
		
	/** Parses the DSR attributes and initializes	**/
	/** the various buffers that are needed			**/
	FIN (dsr_rte_attributes_parse_buffers_create (void));
	
	/* Read the DSR parameters compound attribute	*/
	op_ima_obj_attr_get (own_mod_objid, "manet_mgr.DSR Parameters", &dsr_parms_id);
	dsr_parms_child_id = op_topo_child (dsr_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (dsr_parms_child_id, "Broadcast Jitter", &broadcast_jitter_str);
	broadcast_jitter_dist_ptr = oms_dist_load_from_string (broadcast_jitter_str);
	
	/* Check whether packet salvaging is enabled	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Packet Salvaging", &packet_salvaging_function);
	
	/* Check whether route replies using cached routes is enabled	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Route Replies using Cached Routes", &cached_route_replies_function);
	
	/* Check if the non-propagating route request function is enabled	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Non Propagating Request", &non_propagating_request_function);
	
	/* If the simulation attribute "DSR Routes Export is not	*/
	/* been set, then read the per node attribute to check		*/
	/* as to whether to export the routes 						*/
	if (routes_export == OPC_FALSE)
		{
		op_ima_obj_attr_get (dsr_parms_child_id, "DSR Routes Export", &routes_export);
		
		if (routes_export == OPC_TRUE)
			{
			/* Initialize the OT package	*/
			dsr_support_route_print_to_ot_initialize ();
		
			/* Schedule a call interrupt to close the file	*/
			op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, 0, dsr_support_route_print_to_ot_close, OPC_NIL);
			}
		}
	
	/* Read the Route Cache parameters	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Route Cache Parameters", &route_cache_parms_id);
	route_cache_parms_child_id = op_topo_child (route_cache_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (route_cache_parms_child_id, "Max Cached Routes", &route_cache_size);
	op_ima_obj_attr_get (route_cache_parms_child_id, "Route Expiry Timer", &route_cache_timer);
	
	/* Read the Send Buffer Parameters	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Send Buffer Parameters", &send_buffer_parms_id);
	send_buffer_parms_child_id = op_topo_child (send_buffer_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (send_buffer_parms_child_id, "Max Buffer Size", &send_buffer_size);
	op_ima_obj_attr_get (send_buffer_parms_child_id, "Expiry Timer", &send_buffer_timer);
	
	/* Read the Route Discovery Parameters	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Route Discovery Parameters", &rte_dis_parms_id);
	rte_dis_parms_child_id = op_topo_child (rte_dis_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Request Table Size", &request_table_size);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Maximum Request Table Identifiers", &max_request_table_id);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Maximum Request Retransmissions", &max_request_retrans);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Maximum Request Period", &max_request_period);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Initial Request Period", &init_request_period);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Non Propagating Request Timer", &non_propagating_request_timer);
	op_ima_obj_attr_get (rte_dis_parms_child_id, "Gratuitous Route Reply Timer", &grat_route_reply_timer);
	
	/* Read the Route Maintenance Parameters	*/
	op_ima_obj_attr_get (dsr_parms_child_id, "Route Maintenance Parameters", &rte_maint_parms_id);
	rte_maint_parms_child_id = op_topo_child (rte_maint_parms_id, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (rte_maint_parms_child_id, "Maximum Buffer Size", &retrans_buffer_size);
	op_ima_obj_attr_get (rte_maint_parms_child_id, "Maintenance Holdoff Time", &maint_holdoff_time);
	op_ima_obj_attr_get (rte_maint_parms_child_id, "Maximum Maintenance Retransmissions", &max_maint_retrans);
	op_ima_obj_attr_get (rte_maint_parms_child_id, "Maintenance Acknowledgement Timer", &maint_ack_timer);
	
	/** Create and initialize the various	**/
	/** caches/buffers that are present		**/
	
	/* Create and initialize the route cache	*/
	route_cache_ptr = dsr_route_cache_create (route_cache_size, route_cache_timer, stat_handle_ptr);
	
	/* Create and initialize the send buffer	*/
	send_buffer_ptr = dsr_send_buffer_create (send_buffer_size, send_buffer_timer, stat_handle_ptr, global_stathandle_ptr);
	
	/* Create and initialize the maintenance buffer	*/
	maint_buffer_ptr = dsr_maintenance_buffer_create (retrans_buffer_size, maint_holdoff_time, 
														maint_ack_timer, max_maint_retrans, stat_handle_ptr);
	
	/* Create and initialize the route request table	*/
	route_request_table_ptr = dsr_route_request_tables_create (request_table_size, max_request_table_id, 
								max_request_retrans, init_request_period, max_request_period, 
								non_propagating_request_timer, stat_handle_ptr);
	
	/* Create and initialize the gratuitous route reply table	*/
	grat_reply_table_ptr = dsr_grat_reply_table_create (grat_route_reply_timer);
	
	/* Read the route cache export parameters	*/
	op_ima_obj_attr_get (route_cache_parms_child_id, "Route Cache Export", &cache_export_parms_id);
	cache_export_parms_child_id = op_topo_child (cache_export_parms_id, OPC_OBJTYPE_GENERIC, 0);
	
	op_ima_obj_attr_get (cache_export_parms_child_id, "Status", &route_cache_export_status);
	
	if (route_cache_export_status == OPC_FALSE)
		{
		/* The status to export the	*/
		/* route cache is disabled	*/
		FOUT;
		}
	
	op_ima_obj_attr_get (cache_export_parms_child_id, "Export Time(s) Specification", &cache_time_export_id);
	num_rows = op_topo_child_count (cache_time_export_id, OPC_OBJTYPE_GENERIC);
	
	for (count = 0; count < num_rows; count++)
		{
		cache_time_export_child_id = op_topo_child (cache_time_export_id, OPC_OBJTYPE_GENERIC, count);
		op_ima_obj_attr_get (cache_time_export_child_id, "Time", &export_time);
		
		if (export_time != -1)
			{
			/* Schedule a self interrupt to export the route cache	*/
			/* at the specified time to dump to a log message		*/
			op_intrpt_schedule_call (export_time, DSRC_ROUTE_CACHE_EXPORT, dsr_route_cache_export_to_ot, route_cache_ptr);
			}
		else
			{
			/* The route cache needs to be dumped at end of simulation	*/
			op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, DSRC_ROUTE_CACHE_EXPORT, 
										dsr_route_cache_export_to_ot, route_cache_ptr);
			}
		}
	
	if (num_rows > 0)
		{
		/* Open the OT file to write to	*/
		dsr_route_cache_export_to_ot_initialize ();
		
		/* Schedule to close the OT file at the end of simulation	*/
		op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, DSRC_ROUTE_CACHE_EXPORT, 
										dsr_route_cache_export_to_ot_close, route_cache_ptr);
		}
	
	FOUT;
	}


/*********************************************************************/
/******************** PACKET ARRIVAL FUNCTIONS ***********************/
/*********************************************************************/

static void
dsr_rte_received_pkt_handle (void)
	{
	Packet* 						ip_pkptr = OPC_NIL;
	Packet*							copy_pkptr = OPC_NIL;
	Packet*							return_pkptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr = OPC_NIL;
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr = OPC_NIL;
	Packet*							dsr_pkptr = OPC_NIL;
	List*							tlv_options_lptr = OPC_NIL;
	int								num_options, count;
	DsrT_Packet_Option*				dsr_tlv_ptr = OPC_NIL;
	DsrT_Packet_Type				packet_type = DsrC_Undef_Packet;
	char							addr_str [INETC_ADDR_STR_LEN];
	char							node_name [OMSC_HNAME_MAX_LEN];
	char							temp_str [256];
	Compcode						status;
	Boolean							app_pkt_set = OPC_FALSE;
	
	/** A packet has arrived. Handle the packet	**/
	/** appropriately based on its various TLV	**/
	/** options set in the DSR header			**/
	FIN (dsr_rte_received_pkt_handle (void));
	
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
	ip_pkptr = (Packet*) op_pro_argmem_access ();

	if (ip_pkptr == OPC_NIL)
		dsr_rte_error ("Could not obtain the packet from the argument memory", OPC_NIL, OPC_NIL);

	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* Determine the packet type	*/
	packet_type = dsr_rte_packet_type_determine (ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);

	/* Check if this IP packet is carrying a DSR header	*/
	if (packet_type == DsrC_Higher_Layer_Packet)
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (addr_str, ip_dgram_fd_ptr->dest_addr);
			inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, node_name);
			sprintf (temp_str, "to destination %s (%s)", addr_str, node_name);
			op_prg_odb_print_major (pid_string, "An application packet has arrived at this node", temp_str, OPC_NIL);
			}
		
		/* This IP datagram does not have a DSR header	*/
		/* It should be a higher layer packet			*/
		dsr_rte_app_pkt_arrival_handle (ip_pkptr, intf_ici_fdstruct_ptr, ip_dgram_fd_ptr, OPC_FALSE);
		
		FOUT;
		}
	
	/* This packet is received from the MAC layer	*/
	/* Update the statistic for the total traffic	*/
	dsr_support_total_traffic_received_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Get the DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Check if this is an application packet	*/
	/* or just a DSR routing packet				*/
	app_pkt_set = op_pk_nfd_is_set (dsr_pkptr, "data");
		
	/* Get the list of options	*/
	op_pk_nfd_access (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Set the DSR packet into the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	if (app_pkt_set == OPC_FALSE)
		{
		/* This is a DSR routing packet. Update	*/
		/* the statistic for routing traffic	*/
		dsr_support_routing_traffic_received_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
		}
	else
		{
		/* This is an application packet. Decrease the TTL 	*/
		/* if i am not the destination node 				*/
		if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_FALSE)
			ip_dgram_fd_ptr->ttl--;
		}
	
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Make a copy of the incoming packet for each option	*/
		copy_pkptr = manet_rte_ip_pkt_copy (ip_pkptr);
		
		/* Get the DSR packet from the IP datagram	*/
		op_pk_nfd_get (copy_pkptr, "data", &dsr_pkptr);
	
		/* Get the list of options	*/
		op_pk_nfd_access (dsr_pkptr, "Options", &tlv_options_lptr);
	
		/* Set the DSR packet into the IP datagram	*/
		op_pk_nfd_set (copy_pkptr, "data", dsr_pkptr);
		
		/* Get each option	*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		/* Process the option based on the type	*/
		switch (dsr_tlv_ptr->option_type)
			{
			case (DSRC_ROUTE_REQUEST):
				{
				/* The packet contains a route request option	*/
				/* Insert this node into the route request list	*/
				dsr_pkt_support_route_request_hop_insert (copy_pkptr, intf_ici_fdstruct_ptr->interface_received);

				/* Insert the route in the route cache based on			*/
				/* the requirement for caching overheard information	*/
				dsr_rte_route_cache_update (dsr_tlv_ptr, ip_dgram_fd_ptr);
				
				/* After possibly inserting the route in the route cache	*/
				/* process the received route request option				*/
				dsr_rte_received_route_request_process (copy_pkptr, dsr_tlv_ptr);
				
				break;
				}
			
			case (DSRC_ROUTE_REPLY):
				{
				/* The packet contains a route reply option	*/
				
				/* Insert the route in the route cache based on			*/
				/* the requirement for caching overheard information	*/
				dsr_rte_route_cache_update (dsr_tlv_ptr, ip_dgram_fd_ptr);
				
				/* After possibly inserting the route in the route cache	*/
				/* process the received route reply option					*/
				dsr_rte_received_route_reply_process (copy_pkptr, dsr_tlv_ptr);
				
				break;
				}
				
			case (DSRC_ROUTE_ERROR):
				{
				/* The packet contains a route error option	*/
				/* Process the received route error			*/
				dsr_rte_received_route_error_process (copy_pkptr, dsr_tlv_ptr);
				
				break;
				}
			
			case (DSRC_ACK_REQUEST):
				{
				/* The packet contains an acknowledgement	*/
				/* request option. Process the option		*/
				status = dsr_rte_received_ack_request_process (copy_pkptr);
				
				if (status == OPC_COMPCODE_SUCCESS)
					{
					/* An acknowledgement was sent out for the	*/
					/* received acknowledgement request. Remove	*/
					/* the acknowledgement request option from	*/
					/* the packet received						*/
					op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
					dsr_pkt_support_option_remove (dsr_pkptr, DSRC_ACK_REQUEST);
					op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
					num_options--;
					count--;
					}
				
				break;
				}
				
			case (DSRC_ACKNOWLEDGEMENT):
				{
				/* The packet contains an acknowledgement option	*/
				/* The node should add to its route cache the		*/
				/* single link from the node identified by the ACK	*/
				/* source address to the node identified by the ACK	*/
				/* destination address.								*/
				
				/* Insert the route in the route cache based on			*/
				/* the requirement for caching overheard information	*/
				dsr_rte_route_cache_update (dsr_tlv_ptr, ip_dgram_fd_ptr);
				
				/* After possibly inserting the route in the route cache	*/
				/* process the received acknowledgement option				*/
				dsr_rte_received_acknowledgement_option_process (copy_pkptr, dsr_tlv_ptr);
				
				break;
				}
				
			case (DSRC_SOURCE_ROUTE):
				{
				/* The packet contains a DSR source route option	*/
				
				/* Insert the route in the route cache based on			*/
				/* the requirement for caching overheard information	*/
				dsr_rte_route_cache_update (dsr_tlv_ptr, ip_dgram_fd_ptr);
				
				/* After possibly inserting the route in the route cache	*/
				/* process the received DSR source route option				*/
				dsr_rte_received_dsr_source_route_option_process (copy_pkptr);
				
				break;
				}
				
			default:
				{
				/* Invalid option in packet	*/
				dsr_rte_error ("Invalid Option Type in DSR packet", OPC_NIL, OPC_NIL);
				}
			}
		}
	
	/* If the destination address in the IP packet		*/
	/* matches one of the receiving node's own IP		*/
	/* addresses and this is a application packet, 		*/
	/* remove the DSR header and all DSR options and 	*/
	/* pass the packet to the higher layer				*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_TRUE)
		{
		/* Decapsulate the DSR packet	*/
		return_pkptr = dsr_rte_ip_datagram_decapsulate (ip_pkptr);
	
		if (return_pkptr != OPC_NIL)
			{
			/* Send the IP packet to the higher layer	*/
			manet_rte_to_higher_layer_pkt_send_schedule (module_data_ptr, parent_prohandle, return_pkptr);
			}
		else
			{
			/* Destroy the packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			}
		}
	else
		{
		/* Destroy the packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		}
	
	FOUT;
	}


static void
dsr_rte_app_pkt_arrival_handle (Packet* ip_pkptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, 
							IpT_Dgram_Fields* ip_dgram_fd_ptr, Boolean discovery_performed)
	{
	DsrT_Path_Info*			path_ptr = OPC_NIL;
	InetT_Address*			next_hop_addr_ptr;
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	Packet*					dsr_pkptr = OPC_NIL;
	Boolean					maint_req_added = OPC_FALSE;
	Boolean					source_route_added = OPC_FALSE;
	List*					temp_lptr;
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					temp_str [2048];
	char*					route_str;
	InetT_Address*			copy_address_ptr;
	int						num_nodes;
	
	/** An application packet needs to be sent to 	**/
	/** its destination via the DSR network. 		**/
	/** Process the packet							**/
	FIN (dsr_rte_app_pkt_arrival_handle (<args>));
	
	/* Section 6.1.1	*/
	/* Determine if there is a route to the destination	*/
	/* of the packet in the route cache					*/
	path_ptr = dsr_route_cache_entry_access (route_cache_ptr, ip_dgram_fd_ptr->dest_addr, discovery_performed);
	
	if (path_ptr != OPC_NIL)
		{
		if (LTRACE_ACTIVE)
			{
			route_str = dsr_support_route_print (path_ptr->path_hops_lptr);
			op_prg_odb_print_major ("Found a route to the destination", route_str, OPC_NIL);
			op_prg_mem_free (route_str);
			}
		
		/* A route exists to the destination node in	*/
		/* this node's route cache. Get the next hop	*/
		next_hop_addr_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, 1);
		
		/* There is no maintenance scheduled for the next	*/
		/* hop. Check if maintenance is needed against the	*/
		/* maintenance holdoff time							*/
		if (dsr_maintenance_buffer_maint_needed (maint_buffer_ptr, *next_hop_addr_ptr) == OPC_TRUE)
			{
			if (LTRACE_ACTIVE)
				{
				inet_address_print (dest_hop_addr_str, *next_hop_addr_ptr);
				inet_address_to_hname (*next_hop_addr_ptr, dest_node_name);
				sprintf (temp_str, "to next hop node %s (%s) with ID (%d)", dest_hop_addr_str, dest_node_name, ack_request_identifier);
				op_prg_odb_print_major ("Adding a maintenance request option in packet", temp_str, OPC_NIL);
				}
			
			/* Create a IP datagram with a maintenance	*/
			/* request option in the DSR header			*/
			dsr_tlv_ptr = dsr_pkt_support_ack_request_tlv_create (ack_request_identifier);
	
			/* Create the DSR packet	*/
			dsr_pkptr = dsr_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
			/* Set the maintenance request option in the DSR packet header	*/
			dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
			
			/* Update the statistic for the number of maintenance requests sent	*/
			dsr_support_maintenace_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_TRUE);
			
			/* Set the flag to indicate that a maintenance request	*/
			/* option has been added to the DSR header				*/
			maint_req_added = OPC_TRUE;
			}
		
		/* If the next hop address is the destination	*/
		/* ,ie, only one hop to the destination, then	*/
		/* send the packet out directly without adding	*/
		/* a source route option to the IP datagram		*/
		if (inet_address_equal (ip_dgram_fd_ptr->dest_addr, *next_hop_addr_ptr) == OPC_FALSE)
			{
			/* The next hop is not the destination, ie,	*/
			/* there is more than one hop to reach the	*/
			/* destination. Add a source route option	*/
			/* along with the DSR header to the IP		*/
			/* datagram and then send out the packet	*/
			dsr_tlv_ptr = dsr_pkt_support_source_route_tlv_create (path_ptr->path_hops_lptr, path_ptr->first_hop_external, 
												path_ptr->last_hop_external, routes_export, OPC_FALSE);
			
			if (maint_req_added == OPC_FALSE)
				{
				/* Create the DSR packet if not already created	*/
				dsr_pkptr = dsr_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
				}
	
			/* Set the source route option in the DSR packet header	*/
			dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
			
			/* Set the flag to indicate that a source route option	*/
			/* has been added to the DSR header						*/
			source_route_added = OPC_TRUE;
			}
		else
			{
			if (routes_export)
				{
				/* Print the single hop route	*/
				temp_lptr = op_prg_list_create ();
				dsr_support_route_print_to_ot (ip_dgram_fd_ptr, temp_lptr);
				dsr_temp_list_clear (temp_lptr);
				}
			if (routes_dump)
				{
				if (inet_address_equal (ip_dgram_fd_ptr->src_addr, INETC_ADDRESS_INVALID) == OPC_FALSE)
					{
					/* Print the single hop route	*/
					temp_lptr = op_prg_list_create ();
				
					/* Read the source node	*/
					copy_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->src_addr);
					op_prg_list_insert (temp_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
					
					/* Read the destination node	*/
					copy_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->dest_addr);
					op_prg_list_insert (temp_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
				
					/* Dump the route	*/
					manet_rte_path_display_dump (temp_lptr);
					
					/* Free the contents of the list	*/
					num_nodes = op_prg_list_size (temp_lptr);
					
					while (num_nodes > 0)
						{
						copy_address_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
						inet_address_destroy_dynamic (copy_address_ptr);
						num_nodes--;
						}
					
					/* Free the list	*/
					dsr_temp_list_clear (temp_lptr);
					}
				}
			}
		
		if ((maint_req_added == OPC_TRUE) || (source_route_added == OPC_TRUE))
			{
			/* Encapsulate the DSR packet in the received IP datagram	*/
			dsr_rte_ip_datagram_encapsulate (ip_pkptr, dsr_pkptr, *next_hop_addr_ptr);
			}
		
		if (maint_req_added == OPC_TRUE)
			{
			/* A maintenance request has been added	*/
			/* Place a copy of the packet in the	*/
			/* maintenance buffer for retranmission	*/
			dsr_maintenance_buffer_pkt_enqueue (maint_buffer_ptr, ip_pkptr, *next_hop_addr_ptr, ack_request_identifier);
			
			/* Increment the ACK Request identifier	*/
			ack_request_identifier++;
			}
		
		/* Update the statistic for the total traffic sent	*/
		dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
		
		/* Send the packet out to the MAC		*/
		manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (*next_hop_addr_ptr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
		}
	else
		{
		/* No route exists to the destination	*/
		/* Perform route discovery by 			*/
		/* originating a route request as in	*/
		/* section 6.2.1						*/
		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("No route exists to destination", "Perform route discovery", OPC_NIL);
			}
		
		/* Do not originate the route request	*/
		/* if there is already a request sent 	*/
		/* to the same destination, simply 		*/
		/* enqueue the packet in send buffer. 	*/
		
		/* Convert dest_addr to string */
		inet_address_print (temp_str, ip_dgram_fd_ptr->dest_addr);
		
		/* Check if there is no route discovery already in process 	*/
		/* Start one by sending Route request						*/
		if (prg_string_hash_table_item_get (route_request_table_ptr->route_request_send_table, temp_str) == OPC_NIL)
			dsr_rte_route_request_send (ip_dgram_fd_ptr->dest_addr, non_propagating_request_function);
		
		/* Place the packet in the send buffer	*/
		dsr_send_buffer_packet_enqueue (send_buffer_ptr, ip_pkptr, ip_dgram_fd_ptr->dest_addr);
		}
	
	FOUT;
	}


static void
dsr_rte_received_route_request_process (Packet* ip_pkptr, DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Request_Option*		route_request_option_ptr = OPC_NIL;
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	DsrT_Path_Info*					path_ptr = OPC_NIL;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	char*							route_str;
	
	/** Process the received route request option	**/
	/** Section 6.2.2 of the draft					**/
	FIN (dsr_rte_received_route_request_process (<args>));
	
	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* Get the route request option	*/
	route_request_option_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, route_request_option_ptr->target_address);
		inet_address_to_hname (route_request_option_ptr->target_address, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with route",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("Received a route request option in packet", temp_str, route_str, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* If the target address of the route request	*/
	/* matches one of the node's own IP addresses	*/
	/* then the node should return a route reply	*/
	/* to the initiator of this route request		*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, route_request_option_ptr->target_address) == OPC_TRUE)
		{
		/* This node is the target of the route		*/
		/* request. Send a route reply				*/
		dsr_rte_route_reply_send (ip_pkptr, dsr_tlv_ptr);
		
		/* Destroy the IP packet			*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* If the source address of the IP datagram		*/
	/* belongs to this node, then discard the IP	*/
	/* datagram as it has received its own packet	*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->src_addr) == OPC_TRUE)
		{
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("Destroying the route request packet", 
				"as the source node received its own packet", OPC_NIL);
			}
		
		/* The originator of the route request has	*/
		/* received its own packet again. Discard	*/
		/* ths IP datagram							*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* If the node's own IP address appears in the	*/
	/* list of recorded addresses, then discard the	*/
	/* entire packet. Get the list of addresses		*/
	num_hops = op_prg_list_size (route_request_option_ptr->route_lptr);
	
	for (count = 0; count < (num_hops - 1); count++)
		{
		/* Access each hop and check if it belongs	*/
		/* to this node.							*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_request_option_ptr->route_lptr, count);
		
		/* Check if the hop belongs to the node		*/
		if (manet_rte_address_belongs_to_node (module_data_ptr, *hop_address_ptr) == OPC_TRUE)
			{
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Destroying the route request packet", 
					"as the node's own IP address appears in the list of recorded addresses", OPC_NIL);
				}
			
			/* The hop does belong to the node	*/
			/* Destroy the IP packet			*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			FOUT;
			}
		}
	
	/* Search the route request table for an 	*/
	/* entry from the initiator of this route	*/
	/* request with the same identification		*/
	if (dsr_route_request_forwarding_table_entry_exists (route_request_table_ptr, ip_dgram_fd_ptr->src_addr, 
													route_request_option_ptr->identification) == OPC_TRUE)
		{
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("Destroying the route request packet", 
				"as an entry already exists in the route request table for this identification value", OPC_NIL);
			}
		
		/* An entry already exists in the route	*/
		/* request table for this originating	*/
		/* node and the identification value	*/
		/* Destroy the IP datagram				*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	
	/* Check the TTL field of the IP datagram	*/
	if ((ip_dgram_fd_ptr->ttl - 1) == 0)
		{
		/* This may be either a non-propagating	*/
		/* request that was set to one hop, or 	*/
		/* the TTL field value of the packet 	*/
		/* has reached the maximum number		*/
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("Destroying the route request packet", 
				"as the TTL value of the IP datagram is 0", OPC_NIL);
			}
		
		/* Destroy the IP datagram				*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* None of the above criteria match. Process	*/
	/* the received route request					*/
	
	/* Add an entry for the route request in the	*/
	/* route request table							*/
	dsr_route_request_forwarding_table_entry_insert (route_request_table_ptr, ip_dgram_fd_ptr->src_addr, 
				route_request_option_ptr->target_address, route_request_option_ptr->identification);
	
	
	if (cached_route_replies_function)
		{
		/* Check if there exists a route from this node	*/
		/* to the destination if the cached route reply */
		/* functionality has been enabled on this node	*/
		path_ptr = dsr_route_cache_entry_access (route_cache_ptr, route_request_option_ptr->target_address, OPC_FALSE);
	
		if (path_ptr != OPC_NIL)
			{
			/* A route exists to the traget address from	*/
			/* this node. Send a "cached" route reply		*/
			/* based on certain restrictions				*/
			dsr_rte_cached_route_reply_send (dsr_tlv_ptr, path_ptr, ip_dgram_fd_ptr);
			
			/* Destroy the route request packet	   */
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			FOUT;
			}
		}
	
	/* No route exists to the destination from this node	*/
	/* Re-broadcast this packet with a short jitter.		*/
	dsr_rte_jitter_schedule (ip_pkptr, dsr_tlv_ptr->option_type);
	
	FOUT;
	}


static void
dsr_rte_received_route_reply_process (Packet* ip_pkptr, DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Reply_Option*		route_reply_option_ptr = OPC_NIL;
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					next_hop_addr_ptr;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	char*							route_str;
	
	/** Processes the received route reply option	**/
	/** Section 6.2.5 of the draft					**/
	FIN (dsr_rte_received_route_reply_process (<args>));
	
	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with route",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("Received a route reply option in packet", temp_str, route_str, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* If this node is the destination of the route reply	*/
	/* then no more processing needs to be done				*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_TRUE)
		{
		/* Destroy the route reply packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* This node is not the destination of the route reply	*/
	/* Determine the next hop to which this route reply		*/
	/* needs to be sent.									*/
	route_reply_option_ptr = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Get the number of hops	*/
	num_hops = op_prg_list_size (route_reply_option_ptr->route_lptr);
	
	for (count = (num_hops - 1); count >= 0; count--)
		{
		/* Get each hop and determine if it belongs	*/
		/* to this node.							*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_reply_option_ptr->route_lptr, count);
		
		if (manet_rte_address_belongs_to_node (module_data_ptr, *hop_address_ptr) == OPC_TRUE)
			{
			if (count == 0)
				{
				/* The next hop is the destination	*/
				next_hop_addr_ptr = &ip_dgram_fd_ptr->dest_addr;
				}
			else
				{
				/* This hop belongs to this node	*/
				/* Access the next hop address		*/
				next_hop_addr_ptr = (InetT_Address*) op_prg_list_access (route_reply_option_ptr->route_lptr, (count - 1));
				}
			
			break;
			}
		}
	
	if (count < 0)
		{
		/* None of the hops in the route reply	*/
		/* belong to this node. This is an		*/
		/* overheard packet. Discard this 		*/
		/* packet as this is only used to 		*/
		/* update the node's route cache		*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Forward the packet to the next hop address	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (*next_hop_addr_ptr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}


static void
dsr_rte_received_route_error_process (Packet* ip_pkptr, DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Error_Option*		route_error_option_ptr = OPC_NIL;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							unreach_node_name [OMSC_HNAME_MAX_LEN];
	char							unreach_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	
	/** Processes a received route error option	**/
	/* Section 6.3.5 of the draft				**/
	FIN (dsr_rte_received_route_error_process (<args>));
	
	/* Get the route error option	*/
	route_error_option_ptr = (DsrT_Route_Error_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, route_error_option_ptr->error_source_address);
		inet_address_to_hname (route_error_option_ptr->error_source_address, src_node_name);
		inet_address_print (dest_hop_addr_str, route_error_option_ptr->error_dest_address);
		inet_address_to_hname (route_error_option_ptr->error_dest_address, dest_node_name);
		inet_address_print (unreach_hop_addr_str, route_error_option_ptr->unreachable_node_address);
		inet_address_to_hname (route_error_option_ptr->unreachable_node_address, unreach_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) for unreachable node %s (%s)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, unreach_hop_addr_str, unreach_node_name);
		op_prg_odb_print_major ("Received a route error option in packet", temp_str, OPC_NIL);
		}
		
	/* Remove all routes from the route cache that have	*/
	/* the link from the error source address to the	*/
	/* unreachable node address							*/
	dsr_route_cache_all_routes_with_link_delete (route_cache_ptr, route_error_option_ptr->error_source_address, 
												route_error_option_ptr->unreachable_node_address);
	
	/* Destroy the IP packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}
	

static Compcode
dsr_rte_received_ack_request_process (Packet* ip_pkptr)
	{
	DsrT_Acknowledgement_Request*		ack_request_option_ptr = OPC_NIL;
	DsrT_Source_Route_Option*			source_route_option_ptr = OPC_NIL;
	DsrT_Packet_Option*					dsr_tlv_ptr = OPC_NIL;
	IpT_Dgram_Fields* 					ip_dgram_fd_ptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 			intf_ici_fdstruct_ptr = OPC_NIL;
	Packet*								dsr_pkptr = OPC_NIL;
	List*								options_lptr = OPC_NIL;
	int									num_options, count;
	InetT_Address						current_node_address;
	InetT_Address						previous_node_address;
	char								src_node_name [OMSC_HNAME_MAX_LEN];
	char								src_hop_addr_str [INETC_ADDR_STR_LEN];
	char								dest_node_name [OMSC_HNAME_MAX_LEN];
	char								dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char								temp_str [2048];
	
	/** Processes a received acknowledgement request	**/
	/** option. Section 6.3.3 of the draft				**/
	FIN (dsr_rte_received_ack_request_process (<args>));
	
	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* Get the DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Get the list of options	*/
	op_pk_nfd_access (dsr_pkptr, "Options", &options_lptr);
	
	/* Set the DSR packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	num_options = op_prg_list_size (options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access each option	*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (options_lptr, count);
		
		if (dsr_tlv_ptr->option_type == DSRC_ACK_REQUEST)
			{
			/* Get the acknowledgement request option	*/
			ack_request_option_ptr = (DsrT_Acknowledgement_Request*) dsr_tlv_ptr->dsr_option_ptr;
			}
		else if (dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
			{
			/* Get the source route option	*/
			source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
			}
		else if (dsr_tlv_ptr->option_type == DSRC_ACKNOWLEDGEMENT)
			{
			/* Do not process the packet if it contains	*/
			/* both an acknowledgement request and 		*/
			/* acknowledgement options					*/
			
			/* Destroy the IP packet	*/
			manet_rte_ip_pkt_destroy (ip_pkptr);
			
			FRET (OPC_COMPCODE_FAILURE);
			}
		}
	
	/* Get the current node address	*/
	if (source_route_option_ptr == OPC_NIL)
		{
		/* There is no source route option	*/
		/* Check if the destination address	*/
		/* of the IP packet belongs to one	*/
		/* of this node's own addresses		*/
		current_node_address = ip_dgram_fd_ptr->dest_addr;
		previous_node_address = ip_dgram_fd_ptr->src_addr;
		}
	else
		{
		/* There is a source route option	*/
		/* Get the current node address	and	*/
		/* check if this hop belongs to one	*/
		/* of this node's own addresses		*/
		current_node_address = dsr_pkt_support_source_route_hop_obtain (source_route_option_ptr, DsrC_Current_Hop, ip_dgram_fd_ptr->src_addr, 
																ip_dgram_fd_ptr->dest_addr);
		previous_node_address = dsr_pkt_support_source_route_hop_obtain (source_route_option_ptr, DsrC_Previous_Hop, ip_dgram_fd_ptr->src_addr, 
																ip_dgram_fd_ptr->dest_addr);
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, previous_node_address);
		inet_address_to_hname (previous_node_address, src_node_name);
		inet_address_print (dest_hop_addr_str, current_node_address);
		inet_address_to_hname (current_node_address, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with ID (%ld)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, ack_request_option_ptr->identification);
		op_prg_odb_print_major ("Received a acknowledgement request option in packet", temp_str, OPC_NIL);
		}
	
	if (manet_rte_address_belongs_to_node (module_data_ptr, current_node_address) == OPC_FALSE)
		{
		/* If the next hop address for this packet	*/
		/* does not match any of the node's own IP	*/
		/* addresses, do not process the option		*/
		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("Destroying the packet as next hop address for the received packet", 
				"does not match any of the node's own IP addresses", OPC_NIL);
			}
		
		/* Destroy the IP packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FRET (OPC_COMPCODE_FAILURE);
		}
	
	/* Neither of the above tests failed	*/
	/* Process the acknowledgement request	*/
	/* option by sending an acknowledgement	*/
	/* option to the previous node			*/
	dsr_rte_maintenance_send (current_node_address, previous_node_address, ack_request_option_ptr->identification);
	
	/* Destroy the IP packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FRET (OPC_COMPCODE_SUCCESS);
	}


static void
dsr_rte_received_acknowledgement_option_process (Packet* ip_pkptr, DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Acknowledgement*		ack_option_ptr = OPC_NIL;
	char						src_node_name [OMSC_HNAME_MAX_LEN];
	char						src_hop_addr_str [INETC_ADDR_STR_LEN];
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [2048];
	
	/** Processes a received acknowledgement option	**/
	/** Section 6.3.3 of the draft					**/
	FIN (dsr_rte_received_acknowledgement_option_process (<args>));
	
	/* Get the acknowledgement option	*/
	ack_option_ptr = (DsrT_Acknowledgement*) dsr_tlv_ptr->dsr_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, ack_option_ptr->ack_source_address);
		inet_address_to_hname (ack_option_ptr->ack_source_address, src_node_name);
		inet_address_print (dest_hop_addr_str, ack_option_ptr->ack_dest_address);
		inet_address_to_hname (ack_option_ptr->ack_dest_address, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with id (%ld)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, ack_option_ptr->identification);
		op_prg_odb_print_major ("Received a acknowledgement option in packet", temp_str, OPC_NIL);
		}
	
	/* Delete the packet that was waiting this confirmation	*/
	dsr_maintenance_buffer_pkt_confirmation_received (maint_buffer_ptr, ack_option_ptr->ack_source_address, ack_option_ptr->identification);
	
	/* Destroy the IP packet	*/
	manet_rte_ip_pkt_destroy (ip_pkptr);
	
	FOUT;
	}


static void
dsr_rte_received_dsr_source_route_option_process (Packet* ip_pkptr)
	{
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 		intf_ici_fdstruct_ptr = OPC_NIL;
	Packet*							dsr_pkptr = OPC_NIL;
	DsrT_Source_Route_Option*		source_route_option_ptr = OPC_NIL;
	InetT_Address					next_hop_addr;
	DsrT_Packet_Option*				ack_request_dsr_tlv_ptr = OPC_NIL;
	DsrT_Packet_Option*				dsr_tlv_ptr = OPC_NIL;
	DsrT_Packet_Option*				route_error_tlv_ptr = OPC_NIL;
	InetT_Address					rcvd_intf_address;
	InetT_Address					current_hop_address;
	Boolean							app_pkt_set = OPC_FALSE;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	char*							route_str;
	List*							temp_lptr;
	InetT_Address*					copy_address_ptr;
	InetT_Address*					hop_address_ptr;
	int								num_hops, count, num_nodes;
	InetT_Addr_Family				addr_family;
	
	/** Processes the received source route	**/
	/** option in the IP datagram			**/
	FIN (dsr_rte_received_dsr_source_route_option_process (<args>));
	
	/* Access the information from the incoming IP packet	*/
	manet_rte_ip_pkt_info_access (ip_pkptr, &ip_dgram_fd_ptr, &intf_ici_fdstruct_ptr);
	
	/* Figure out whether we are dealing with an*/
	/* IPv4 packet or an IPv6 packet.			*/
	addr_family = inet_address_family_get (&(ip_dgram_fd_ptr->dest_addr));

	/* Get the DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Check if an application packet is set	*/
	app_pkt_set = op_pk_nfd_is_set (dsr_pkptr, "data");
	
	/* Get the source route option from the DSR packet	*/
	dsr_tlv_ptr = dsr_rte_packet_option_get (dsr_pkptr, DSRC_SOURCE_ROUTE);
	
	/* Get the route error option from the DSR packet if one exists	*/
	route_error_tlv_ptr = dsr_rte_packet_option_get (dsr_pkptr, DSRC_ROUTE_ERROR);
	
	/* Set the DSR packet into the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Get the source route option	*/
	source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with route",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("Received a source route option in  packet", temp_str, route_str, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* Examine if there is an opportunity for automatic	*/
	/* route shortening. If this node is not the 		*/
	/* intended next hop, but is named in the later 	*/
	/* unexpanded portion of the source route, there is	*/
	/* an opportunity for automatic route shortening	*/
	
	/* Get the received interface address	*/
	rcvd_intf_address = manet_rte_rcvd_interface_address_get (module_data_ptr, intf_ici_fdstruct_ptr, addr_family);
	
	/* Get the current hop address in the source route	*/
	current_hop_address = dsr_pkt_support_source_route_hop_obtain (source_route_option_ptr, DsrC_Current_Hop, ip_dgram_fd_ptr->src_addr, 
																	ip_dgram_fd_ptr->dest_addr);
	
	/* If the intended next hop is not this node, then	*/
	/* check if there is an opportunity for automatic	*/
	/* route shortening.								*/
	if (inet_address_equal (rcvd_intf_address, current_hop_address) == OPC_FALSE)
		{
		/* Check and perform for automatic route shortening		*/
		/* Destroy the packet if route shortening is success 	*/
		/* or if it fails (i.e. overheard packet and this node 	*/
		/* not even in the source route) 						*/
		dsr_rte_automatic_route_shortening_check (ip_dgram_fd_ptr,
			source_route_option_ptr, intf_ici_fdstruct_ptr);
		
		/* Free the memory allocated to rcvd_intf_address */
		inet_address_destroy (rcvd_intf_address);

		/* Discard the overheard packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}

	/* Free the memory allocated to rcvd_intf_address */
	inet_address_destroy (rcvd_intf_address);
	
	/* If there are no more hops in the source route	*/
	/* this is the destination of the packet			*/
	if (source_route_option_ptr->segments_left == 0)
		{
		/* Export the route taken if the	*/
		/* source node has the attribute	*/
		/* set or the simulation attribute	*/
		/* has been enaabled				*/
		if ((source_route_option_ptr->export_route) && (route_error_tlv_ptr == OPC_NIL))
			{
			dsr_support_route_print_to_ot (ip_dgram_fd_ptr, source_route_option_ptr->route_lptr);
			}
		if ((routes_dump) && (route_error_tlv_ptr == OPC_NIL))
			{
			if (inet_address_equal (ip_dgram_fd_ptr->src_addr, INETC_ADDRESS_INVALID) == OPC_FALSE)
				{
				/* Print the single hop route	*/
				temp_lptr = op_prg_list_create ();
				
				/* Read the source node	*/
				copy_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->src_addr);
				op_prg_list_insert (temp_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
				
				/* Add the intermediate hops	*/
				num_hops = op_prg_list_size (source_route_option_ptr->route_lptr);
				
				for (count = 0; count < num_hops; count++)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, count);
					copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
					op_prg_list_insert (temp_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
					}
				
				/* Read the destination node	*/
				copy_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->dest_addr);
				op_prg_list_insert (temp_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
				
				/* Dump the route	*/
				manet_rte_path_display_dump (temp_lptr);
				
				/* Free the contents of the list	*/
				num_nodes = op_prg_list_size (temp_lptr);
					
				while (num_nodes > 0)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
					inet_address_destroy_dynamic (hop_address_ptr);
					num_nodes--;
					}
					
				/* Free the list	*/
				dsr_temp_list_clear (temp_lptr);
				}
			}
		
		/* Destroy the IP packet	*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* Get the next hop in the source route	*/
	next_hop_addr = dsr_pkt_support_source_route_hop_obtain (source_route_option_ptr, DsrC_Next_Hop, ip_dgram_fd_ptr->src_addr, 
																	ip_dgram_fd_ptr->dest_addr);
	
	/* If the next address or the destination	*/
	/* address is a multicast address, destroy	*/
	/* the packet and do not process further	*/
	if (inet_address_is_multicast (next_hop_addr) || inet_address_is_multicast (ip_dgram_fd_ptr->dest_addr))
		{
		/* The next hop or destination address is a 	*/
		/* multicast address. Destroy the packet		*/
		manet_rte_ip_pkt_destroy (ip_pkptr);
		
		FOUT;
		}
	
	/* There is no maintenance scheduled for the next	*/
	/* hop. Check if maintenance is needed against the	*/
	/* maintenance holdoff time							*/
	if (dsr_maintenance_buffer_maint_needed (maint_buffer_ptr, next_hop_addr) == OPC_TRUE)
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (dest_hop_addr_str, next_hop_addr);
			inet_address_to_hname (next_hop_addr, dest_node_name);
			sprintf (temp_str, "to next hop node %s (%s) with ID (%d)", dest_hop_addr_str, dest_node_name, ack_request_identifier);
			op_prg_odb_print_major ("Adding a maintenance request option in packet", temp_str, OPC_NIL);
			}
		
		/* Create a IP datagram with a maintenance	*/
		/* request option in the DSR header			*/
		ack_request_dsr_tlv_ptr = dsr_pkt_support_ack_request_tlv_create (ack_request_identifier);
		
		/* Update the statistic for the number of maintenance requests sent	*/
		dsr_support_maintenace_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_TRUE);
		
		/* Get the DSR packet from the IP datagram	*/
		op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
		
		/* Because DSR packet is IP's payload, update IP's header fields			*/
		op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
		ip_dgram_fd_ptr->orig_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->frag_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->original_size -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	
		/* Set the maintenance request option in the DSR packet header	*/
		dsr_pkt_support_option_add (dsr_pkptr, ack_request_dsr_tlv_ptr);
		
		/* Once DSR payload is resized, set IP's fields again appropriately			*/
		ip_dgram_fd_ptr->orig_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->frag_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->original_size += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		
		/* Set the DSR packet into the IP datagram	*/
		op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
		//dsr_rte_ip_datagram_encapsulate (ip_pkptr, dsr_pkptr, next_hop_addr);
		
		/* A maintenance request has been added	*/
		/* Place a copy of the packet in the	*/
		/* maintenance buffer for retranmission	*/
		dsr_maintenance_buffer_pkt_enqueue (maint_buffer_ptr, ip_pkptr, next_hop_addr, ack_request_identifier);
			
		/* Increment the ACK Request identifier	*/
		ack_request_identifier++;
		}
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	if (app_pkt_set == OPC_FALSE)
		{
		/* Update the statistics for the routing traffic sent	*/
		dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
		}
	
	/* Send the packet out to the MAC	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (next_hop_addr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}
	
	
	

/*********************************************************************/
/******************** PACKET CREATION FUNCTIONS **********************/
/*********************************************************************/

static void
dsr_rte_route_request_send (InetT_Address dest_address, Boolean non_prop_route_request)
	{
	DsrT_Packet_Option*			dsr_tlv_ptr = OPC_NIL;
	IpT_Dgram_Fields* 			ip_dgram_fd_ptr = OPC_NIL;
	Packet*						dsr_pkptr = OPC_NIL;
	Packet*						ip_pkptr = OPC_NIL;
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [2048];
	Ici*						ip_iciptr;
	int							mcast_major_port = IPC_MCAST_ALL_MAJOR_PORTS;
			
	/** Initiates a route request to a destination	**/
	FIN (dsr_rte_route_request_send (<args>));
	
	/* Create a route request TLV option	*/
	dsr_tlv_ptr = dsr_pkt_support_route_request_tlv_create (route_request_identifier, dest_address);
	
	/* Create the DSR packet	*/
	dsr_pkptr = dsr_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the route request option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
	
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The source address of the IP datagram is the node's	*/
	/* own IP address and the destination address of the	*/
	/* IP datagram is the limited broadcast address 		*/
	/* (255.255.255.255) for IPv4 or the all node link 		*/
	/* layer multicast address for IPv6						*/
	if (inet_address_family_get (&dest_address) == InetC_Addr_Family_v4)
		{
		ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, InetI_Broadcast_v4_Addr,
							InetI_Broadcast_v4_Addr, OPC_NIL);
		}
	else
		{
		ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr,
							InetI_Ipv6_All_Nodes_LL_Mcast_Addr, OPC_NIL);
		
		/* Install the ICI for IPv6 case */
		ip_iciptr = op_ici_create ("ip_rte_req_v4");
		op_ici_attr_set (ip_iciptr, "multicast_major_port", mcast_major_port);
		op_ici_install (ip_iciptr);	
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (dest_hop_addr_str, dest_address);
		inet_address_to_hname (dest_address, dest_node_name);
		sprintf (temp_str, "destined to node %s (%s) with ID (%d)", dest_hop_addr_str, dest_node_name, route_request_identifier);
		op_prg_odb_print_major ("Broadcasting a route request option in packet", temp_str, OPC_NIL);
		}
	
	/* Increment the route request identifier	*/
	route_request_identifier++;
	
	/* Access the IP datagram fields	*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* If the non-propagating route request feature	*/
	/* has been enabled, set the TTL field in the	*/
	/* route request packet to one					*/
	if (non_prop_route_request)
		{
		/* Set the TTL to one	*/
		ip_dgram_fd_ptr->ttl = 1;
		}
	else
		{
		/* Set the TTL to the default	*/
		ip_dgram_fd_ptr->ttl = IPC_DEFAULT_TTL;
		}

	/* Insert the originating route request information in	*/
	/* the originating route request table					*/
	dsr_route_request_originating_table_entry_insert (route_request_table_ptr, dest_address, ip_dgram_fd_ptr->ttl);
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Update the statistic for the total number of route requests sent	*/
	dsr_support_route_request_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, non_prop_route_request);
	
	/* Send the packet to the CPU which will broadcast it	*/
	/* after processing the packet							*/	
	manet_rte_to_cpu_pkt_send_schedule_with_jitter (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	/* Clear the ICI if installed */
	op_ici_install (OPC_NIL);	
	
	FOUT;
	}


static void
dsr_rte_route_reply_send (Packet* request_ip_pkptr, DsrT_Packet_Option* request_dsr_tlv_ptr)
	{
	IpT_Dgram_Fields* 				ip_dgram_fd_ptr = OPC_NIL;
	DsrT_Route_Request_Option*		route_request_option_ptr = OPC_NIL;
	DsrT_Packet_Option*				reply_dsr_tlv_ptr = OPC_NIL;
	Packet*							reply_ip_pkptr = OPC_NIL;
	Packet*							dsr_pkptr = OPC_NIL;
	InetT_Address*					next_hop_addr_ptr;
	InetT_Address*					node_address_ptr;
	int								num_hops;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							next_node_name [OMSC_HNAME_MAX_LEN];
	char							next_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	char*							route_str;
	ManetT_Nexthop_Info*			manet_nexthop_info_ptr = OPC_NIL;
	
	/** Sends out a route reply option on	**/
	/** receipt of a route request packet	**/
	/** to the source of the route request	**/
	FIN (dsr_rte_route_reply_send (<args>));
	
	/* Access the IP datagram fields	*/
	op_pk_nfd_access (request_ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* Access the route request option	*/
	route_request_option_ptr = (DsrT_Route_Request_Option*) request_dsr_tlv_ptr->dsr_option_ptr;
	
	/* Remove this node's address from the list	*/
	node_address_ptr = (InetT_Address*) op_prg_list_remove (route_request_option_ptr->route_lptr, OPC_LISTPOS_TAIL);
	inet_address_destroy_dynamic (node_address_ptr);
	
	/* The target address is this node. The route	*/
	/* reply will be in the order of the route		*/
	/* request. Hence, the next hop address will be	*/
	/* the address before the target address		*/
	
	/* Get the size of the route list	*/
	num_hops = op_prg_list_size (route_request_option_ptr->route_lptr);
	
	/* If the number of hops is zero, then the next hop	*/
	/* is the final destination address (the source	of 	*/
	/* the route request)								*/
	if (num_hops == 0)
		{
		/* The next hop is the source of the request	*/
		next_hop_addr_ptr = &ip_dgram_fd_ptr->src_addr;
		}
	else
		{
		next_hop_addr_ptr = (InetT_Address*) op_prg_list_access (route_request_option_ptr->route_lptr, OPC_LISTPOS_TAIL);
		}
	
	/* Create the route reply TLV option	*/
	reply_dsr_tlv_ptr = dsr_pkt_support_route_reply_tlv_create (ip_dgram_fd_ptr->src_addr, route_request_option_ptr->target_address,
											route_request_option_ptr->route_lptr, OPC_FALSE);
	
	/* Create the DSR packet	*/
	dsr_pkptr = dsr_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
	/* Set the route reply option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, reply_dsr_tlv_ptr);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The destination address of the IP datagram carrying	*/
	/* the route reply option is the address of the 		*/
	/* initiator of the route request						*/
	reply_ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, ip_dgram_fd_ptr->src_addr,
						*next_hop_addr_ptr, manet_nexthop_info_ptr);
	
	/* Insert this route request received in the forwarding	*/
	/* route request table									*/
	dsr_route_request_forwarding_table_entry_insert (route_request_table_ptr, ip_dgram_fd_ptr->src_addr, 
				route_request_option_ptr->target_address, route_request_option_ptr->identification);
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (reply_dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, route_request_option_ptr->target_address);
		inet_address_to_hname (route_request_option_ptr->target_address, src_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, dest_node_name);
		inet_address_print (next_hop_addr_str, *next_hop_addr_ptr);
		inet_address_to_hname (*next_hop_addr_ptr, next_node_name);	
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with next hop %s (%s) for request ID (%ld) with route",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, next_hop_addr_str, next_node_name, 
			route_request_option_ptr->identification);
		op_prg_odb_print_major ("Sending a route reply option in packet", temp_str, route_str, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* Update the statistic for the number of route replies sent from the destination	*/
	dsr_support_route_reply_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_FALSE);
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* DSR control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	/* Send the packet after a jitter	*/
	/* to the CPU						*/
	dsr_rte_jitter_schedule (reply_ip_pkptr, DSRC_ROUTE_REPLY);
	
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	FOUT;
	}
	
static void
dsr_rte_maintenance_send (InetT_Address source_address, InetT_Address dest_address, int ack_request_id)
	{
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	Packet*					dsr_pkptr = OPC_NIL;
	Packet*					ip_pkptr = OPC_NIL;
	char					src_node_name [OMSC_HNAME_MAX_LEN];
	char					src_hop_addr_str [INETC_ADDR_STR_LEN];
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					temp_str [2048];
	ManetT_Nexthop_Info*	manet_nexthop_info_ptr = OPC_NIL;
	
	/** Sends out a maintenance option to 	**/
	/** the previous hop that sent a 		**/
	/** maintenance request packet			**/
	FIN (dsr_rte_maintenance_send (<args>));
	
	/* Create an IP datagram with an	*/
	/* acknowledgement option			*/
	dsr_tlv_ptr = dsr_pkt_support_acknowledgement_tlv_create (ack_request_id, source_address, dest_address);
	
	/* Create the DSR packet	*/
	dsr_pkptr = dsr_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Set the acknowledgement option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The source address of the IP datagram is the node's	*/
	/* own IP address and the destination address of the	*/
	/* IP datagram is the next hop node address				*/
	ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, dest_address, 
								dest_address, manet_nexthop_info_ptr);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, source_address);
		inet_address_to_hname (source_address, src_node_name);
		inet_address_print (dest_hop_addr_str, dest_address);
		inet_address_to_hname (dest_address, dest_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with ID (%d)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, ack_request_id);
		op_prg_odb_print_major ("Sending an acknowledgement option in packet", temp_str, OPC_NIL);
		}
	
	/* Update the statistic for the number of acknowledgements sent	*/
	dsr_support_maintenace_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_FALSE);
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Send the packet to the CPU which will forward it	*/
	/* on the appropriate interface after processing	*/
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* DSR control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	FOUT;
	}
	

static void
dsr_rte_route_error_send (Packet* ip_pkptr, InetT_Address next_hop_addr, IpT_Dgram_Fields* ip_dgram_fd_ptr,
							IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	Packet*							dsr_pkptr = OPC_NIL;
	List*							tlv_options_lptr = OPC_NIL;
	DsrT_Packet_Option*				dsr_tlv_ptr = OPC_NIL;
	DsrT_Source_Route_Option*		source_route_option_ptr = OPC_NIL;
	InetT_Address					rcvd_intf_addr;
	InetT_Address*					prev_hop_addr_ptr;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					copy_hop_address_ptr;
	List*							route_lptr = OPC_NIL;
	int								count, num_options, size, num_hops;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							next_node_name [OMSC_HNAME_MAX_LEN];
	char							next_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	InetT_Addr_Family				addr_family;
	ManetT_Nexthop_Info*			manet_nexthop_info_ptr = OPC_NIL;
	
	/** Sends out a route error to the source	**/
	/** node of the IP datagram informing that	**/
	/** the next hop is unreachable 			**/
	FIN (dsr_rte_route_error_send (<args>));
	
	/* Figure out if we are dealing with an		*/
	/* IPv4 packet or an IPv6 packet.			*/
	addr_family = inet_address_family_get (&(ip_dgram_fd_ptr->dest_addr));

	/* Get the received interface address	*/
	rcvd_intf_addr = manet_rte_rcvd_interface_address_get (module_data_ptr, intf_ici_fdstruct_ptr, addr_family);
	
	/* Get the encapsulated DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Get the list of options	*/
	op_pk_nfd_access (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Set the DSR pcket in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access the source route option	*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		/* Get this hop from the source route	*/
		if (dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
			{
			/* Get the source route option	*/
			source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
			break;
			}
		}
	
	if (source_route_option_ptr == OPC_NIL)
		{
		/* No source route. Do not send a route error	*/
		FOUT;
		}
		
	/* Determine the previous hop to which	*/
	/* this route error needs to be sent	*/
	num_hops = op_prg_list_size (source_route_option_ptr->route_lptr);
	
	if (num_hops == 0)
		{
		/* This is the source node	*/
		/* No need to send a route 	*/
		/* error					*/
		FOUT;
		}
	
	for (count = 0; count < num_hops; count++)
		{
		/* Determine which hop belongs to this node	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, count);
		
		if (manet_rte_address_belongs_to_node (module_data_ptr, *hop_address_ptr) == OPC_TRUE)
			{
			if (count == 0)
				{
				/* The previous node is the source of the datagram	*/
				prev_hop_addr_ptr = &ip_dgram_fd_ptr->src_addr;
				}
			else
				{
				/* This hop belongs to this node	*/
				/* Access the previous hop address	*/
				/* to which the route error needs	*/
				/* to be sent						*/
				prev_hop_addr_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, (count - 1));
				}
			
			break;
			}
		}
	
	/* Create a list to store the source route back	*/
	route_lptr = op_prg_list_create ();
	
	/* Create the source route back to the source	*/
	/* to send this route error option				*/
	for (size = count; size >= 0; size--)
		{
		/* Access each hop starting from this node	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, size);
		
		/* Make a copy of the address	*/
		copy_hop_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		
		/* Insert the route into the list	*/
		op_prg_list_insert (route_lptr, copy_hop_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Insert the destination node in the list	*/
	copy_hop_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->src_addr);
	op_prg_list_insert (route_lptr, copy_hop_address_ptr, OPC_LISTPOS_TAIL);
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (src_hop_addr_str, rcvd_intf_addr);
		inet_address_to_hname (rcvd_intf_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, dest_node_name);
		inet_address_print (next_hop_addr_str, next_hop_addr);
		inet_address_to_hname (next_hop_addr, next_node_name);
		sprintf (temp_str, "from node %s (%s) destined to node %s (%s) with unreachable node %s (%s)",
			src_hop_addr_str, src_node_name, dest_hop_addr_str, dest_node_name, next_hop_addr_str, next_node_name);
		op_prg_odb_print_major ("Sending a route error option in packet", temp_str, OPC_NIL);
		}
	
	/* Create the DSR packet	*/
	dsr_pkptr = dsr_pkt_support_pkt_create (IpC_Protocol_Unspec);
	
	/* Create a route error option	*/
	dsr_tlv_ptr = dsr_pkt_support_route_error_tlv_create (DSRC_NODE_UNREACHABLE, source_route_option_ptr->salvage, 
					rcvd_intf_addr, ip_dgram_fd_ptr->src_addr, next_hop_addr);
	
	/* Set the route error option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
	
	/* Create the source route option	*/
	dsr_tlv_ptr = dsr_pkt_support_source_route_tlv_create (route_lptr, OPC_FALSE, OPC_FALSE, OPC_FALSE, OPC_FALSE);
	
	/* Set the source route option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
		
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The source address of the IP datagram is the node's	*/
	/* own IP address and the destination address of the	*/
	/* IP datagram is the next hop node address				*/
	ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, ip_dgram_fd_ptr->src_addr,
					*prev_hop_addr_ptr, manet_nexthop_info_ptr);
	
	/* Update the statistic for the total number of route errors sent	*/
	dsr_support_route_error_stats_update (stat_handle_ptr, global_stathandle_ptr);
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
	/* Send the packet to the CPU which will forward it	*/
	/* on the appropriate interface after processing	*/
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* DSR control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, ip_pkptr);
	
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	size = op_prg_list_size (route_lptr);
	
	while (size > 0)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (route_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		size --;
		}
	
	/* Free the temporary source route list	*/
	dsr_temp_list_clear (route_lptr);
	
	FOUT;
	}
	

static void
dsr_rte_cached_route_reply_send (DsrT_Packet_Option* dsr_tlv_ptr, DsrT_Path_Info* path_ptr, 
									IpT_Dgram_Fields* ip_dgram_fd_ptr)
	{
	List*							temp_lptr = OPC_NIL;
	DsrT_Route_Request_Option*		route_request_option_ptr = OPC_NIL;
	int								num_hops, count, size;
	InetT_Address*					copy_source_addr_ptr;
	InetT_Address*					copy_dest_addr_ptr;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					copy_hop_address_ptr;
	InetT_Address*					next_hop_address_ptr;
	InetT_Address*					next_address_ptr;
	DsrT_Packet_Option*				route_reply_dsr_tlv_ptr = OPC_NIL;
	Packet*							dsr_pkptr = OPC_NIL;
	Packet*							reply_ip_pkptr = OPC_NIL;
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							src_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	char							temp_str1 [2048];
	char*							route_str;
	ManetT_Nexthop_Info*			manet_nexthop_info_ptr = OPC_NIL;
	
	/** Sends a cached route reply to an	**/
	/** incoming route request				**/
	FIN (dsr_rte_cached_route_reply_send (<args>));
	
	/* Create a temporary list to store the entire route	*/
	temp_lptr = op_prg_list_create ();
	
	/* Insert the source node into the route	*/
	copy_hop_address_ptr = inet_address_create_dynamic (ip_dgram_fd_ptr->src_addr);
	op_prg_list_insert (temp_lptr, copy_hop_address_ptr, OPC_LISTPOS_TAIL);
	
	/* Get the route request option	*/
	route_request_option_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Number of hops from the source node to this	*/
	/* intermediate node							*/
	num_hops = op_prg_list_size (route_request_option_ptr->route_lptr);
	
	/* Compose the total route	*/
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_request_option_ptr->route_lptr, count);
		copy_hop_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (temp_lptr, copy_hop_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Get the number of hops traversed to	*/
	/* reach this current node				*/
	num_hops = op_prg_list_size (temp_lptr);
	
	/* Get the previous hop to which this	*/
	/* route reply needs to be sent			*/
	next_hop_address_ptr = (InetT_Address*) op_prg_list_access (temp_lptr, (num_hops - 2));
	
	/* Insert the remaining route from this node to the destination	*/
	/* Do not insert the first hop as this node is the common point	*/
	/* between the two routes										*/
	num_hops = op_prg_list_size (path_ptr->path_hops_lptr);
	
	for (count = 1; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, count);
		copy_hop_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (temp_lptr, copy_hop_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* There must be no duplicate addresses in the entire route	*/
	/* If there are any duplicate addresses, discard the packet	*/
	num_hops = op_prg_list_size (temp_lptr);
	
	/* Compare all the addresses to check if there	*/
	/* are any duplicate addresses					*/
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (temp_lptr, count);
		
		for (size = (count + 1); size < num_hops; size++)
			{
			next_address_ptr = (InetT_Address*) op_prg_list_access (temp_lptr, size);
			
			/* Compare the addresses	*/
			if (inet_address_ptr_equal (hop_address_ptr, next_address_ptr) == OPC_TRUE)
				{			
				/* There is a duplicate address	*/
								
				while (num_hops > 0)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
					inet_address_destroy_dynamic (hop_address_ptr);
					num_hops --;
					}
	
				op_prg_mem_free (temp_lptr);
				
				/* Discard the packet			*/
				FOUT;
				}
			}
		}
	
	/* There are no duplicate addresses	*/
	/* Create a route reply and send it	*/
	/* to the originator of the request	*/
	copy_source_addr_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
	copy_dest_addr_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_TAIL);
	
	/* Create a route reply option	*/
	route_reply_dsr_tlv_ptr = dsr_pkt_support_route_reply_tlv_create (*copy_source_addr_ptr, *copy_dest_addr_ptr, temp_lptr, path_ptr->last_hop_external);
	
	/* Create the DSR packet	*/
	dsr_pkptr = dsr_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
	/* Set the route reply option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, route_reply_dsr_tlv_ptr);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The destination address of the IP datagram carrying	*/
	/* the route reply option is the address of the 		*/
	/* initiator of the route request						*/
	reply_ip_pkptr = dsr_rte_ip_datagram_create (dsr_pkptr, ip_dgram_fd_ptr->src_addr,
							*next_hop_address_ptr, manet_nexthop_info_ptr);
	
	/* Insert this route request received in the forwarding	*/
	/* route request table									*/
	dsr_route_request_forwarding_table_entry_insert (route_request_table_ptr, ip_dgram_fd_ptr->src_addr, 
				*copy_dest_addr_ptr, route_request_option_ptr->identification);
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (route_reply_dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (dest_hop_addr_str, route_request_option_ptr->target_address);
		inet_address_to_hname (route_request_option_ptr->target_address, dest_node_name);
		sprintf (temp_str, "back to node %s (%s) with route", src_hop_addr_str, src_node_name);
		sprintf (temp_str1, "The route request packet was destined to node %s (%s)", dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("Sending a cached route reply option in packet", temp_str, route_str, temp_str1, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, reply_ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, reply_ip_pkptr);
	
	/* Update the statistic for the number of cached route replies sent	*/
	dsr_support_route_reply_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_TRUE);
	
	/* Send the packet to the CPU which will forward it	*/
	/* on the appropriate interface after processing	*/
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* DSR control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, reply_ip_pkptr);
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	/* Free the addresses and the list	*/
	inet_address_destroy_dynamic (copy_source_addr_ptr);
	inet_address_destroy_dynamic (copy_dest_addr_ptr);
	
	while (op_prg_list_size (temp_lptr) > 0)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		}
	
	op_prg_mem_free (temp_lptr);
	
	FOUT;
	}


static void
dsr_rte_jittered_pkt_send (void* pkptr1, int option_type)
	{
	Packet* 				pkptr;
	
	/** Sends a packet out after the jitter	**/
	FIN (dsr_rte_jittered_pkt_send (<args>));
	
	pkptr = (Packet*) pkptr1;
	
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, pkptr);
	
	switch (option_type)
		{
		case (DSRC_ROUTE_REQUEST):
			{
			/* A route request packet needs to be sent out	*/
			/* Broadcast this packet to all interfaces		*/
			manet_rte_datagram_broadcast (module_data_ptr, pkptr);
			
			break;
			}
			
		case (DSRC_ROUTE_REPLY):
			{
			/* Send the packet to the CPU which will forward it	*/
			/* on the appropriate interface after processing	*/
			
			manet_rte_to_cpu_pkt_send (module_data_ptr, parent_prohandle, parent_pro_id, pkptr);
			
			break;
			}
		}
	
	FOUT;
	}


static void
dsr_rte_send_buffer_check (List* route_lptr)
	{
	int							num_hops, num_pkts, count, size;
	List*						pkt_lptr = OPC_NIL;
	InetT_Address*				dest_addr;
	Packet*						pkptr = OPC_NIL;
	Ici*						pkt_info_ici_ptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 	intf_ici_fdstruct_ptr = OPC_NIL;
	IpT_Dgram_Fields* 			ip_dgram_fd_ptr = OPC_NIL;
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [2048];
	
	/** Checks to see if there is any packets	**/
	/** in the send buffer to the list of 		**/
	/** destinations for a route received. This	**/
	/** function should be called everytime a	**/
	/** route is inserted into the route cache	**/
	FIN (dsr_rte_send_buffer_check (<args>));
	
	/* Get the number of possible destinations	*/
	num_hops = op_prg_list_size (route_lptr);
	
	/* The first entry in the list is this	*/
	/* node itself. Start accessing from	*/
	/* the second entry						*/
	for (count = 1; count < num_hops; count++)
		{
		/* Get each possible destination	*/
		dest_addr = (InetT_Address*) op_prg_list_access (route_lptr, count);
		
		/* Check if there are any packets	*/
		/* waiting in the send buffer to be	*/
		/* sent to this destintion. If so,	*/
		/* remove them from the send buffer	*/
		pkt_lptr = dsr_send_buffer_pkt_list_get (send_buffer_ptr, *dest_addr, OPC_TRUE);
		
		if (pkt_lptr == OPC_NIL)
			{
			/* No packets are waiting to be	*/
			/* sent to this destination		*/
			continue;
			}
		
		/* Get the number of packets waiting	*/
		num_pkts = op_prg_list_size (pkt_lptr);
		
		if (LTRACE_ACTIVE)
			{
			inet_address_print (dest_hop_addr_str, *dest_addr);
			inet_address_to_hname (*dest_addr, dest_node_name);
			sprintf (temp_str, "The send buffer has %d packet queued to destination %s (%s)", num_pkts, dest_hop_addr_str, dest_node_name);
			op_prg_odb_print_major (temp_str, "Sending the packets to the destination", OPC_NIL);
			}
		
		for (size = 0; size < num_pkts; size++)
			{
			/* Get each packet	*/
			pkptr = (Packet*) op_prg_list_access (pkt_lptr, size);
			
			/* Get the ICI associated with the packet	*/
			/* Get the pointer to access the contents	*/
			pkt_info_ici_ptr = op_pk_ici_get (pkptr);
	
			/* Set the ICI back in the packet	*/
			op_pk_ici_set (pkptr, pkt_info_ici_ptr);
	
			/* Get the incoming packet information	*/
			op_ici_attr_get (pkt_info_ici_ptr, "rte_info_fields", &intf_ici_fdstruct_ptr);
	
			/* Set the incoming packet information	*/
			op_ici_attr_set (pkt_info_ici_ptr, "rte_info_fields", intf_ici_fdstruct_ptr);

			/* Access the fields of the IP datagram	*/
			op_pk_nfd_access (pkptr, "fields", &ip_dgram_fd_ptr);
			
			/* Process the packet and send it	*/
			/* out to the destination			*/
			dsr_rte_app_pkt_arrival_handle (pkptr, intf_ici_fdstruct_ptr, ip_dgram_fd_ptr, OPC_TRUE);
			}
		
		/* Destroy the packet list	*/
		dsr_temp_list_clear (pkt_lptr);
		}
	
	FOUT;
	}
		

static Compcode
dsr_rte_automatic_route_shortening_check (IpT_Dgram_Fields* ip_dgram_fd_ptr, 
	DsrT_Source_Route_Option* source_route_option_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	int						num_hops, count;
	Boolean					route_shortening_candidate = OPC_FALSE;
	int						location_found = OPC_INT_UNDEF;
	InetT_Address			overheard_node_address;
	List*					route_lptr;
	InetT_Address*			hop_address_ptr;
	InetT_Address*			copy_address_ptr;
	DsrT_Packet_Option*		route_reply_dsr_tlv_ptr;
	Packet*					route_reply_dsr_pkptr;
	Packet*					reply_ip_pkptr;
	char					src_node_name [OMSC_HNAME_MAX_LEN];
	char					src_hop_addr_str [INETC_ADDR_STR_LEN];
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					over_node_name [OMSC_HNAME_MAX_LEN];
	char					over_hop_addr_str [INETC_ADDR_STR_LEN];
	char					cur_node_name [OMSC_HNAME_MAX_LEN];
	char					cur_hop_addr_str [INETC_ADDR_STR_LEN];
	char					temp_str [2048];
	char					temp_str1 [2048];
	char*					route_str;
	ManetT_Nexthop_Info*	manet_nexthop_info_ptr = OPC_NIL;
	
	/** Performs automatic route shortening on the	**/
	/** received packet is there is an oportunity	**/
	/** for automatic route shortening				**/
	FIN (dsr_rte_automatic_route_shortening_check (<args>));
	
	/* Check if this source route is eligible for	*/
	/* automatic route shortening. To do this, 		*/
	/* check if this node address appears at a 		*/
	/* later unexpanded portion of the source route	*/
	num_hops = op_prg_list_size (source_route_option_ptr->route_lptr);
	
	for (count = (num_hops - source_route_option_ptr->segments_left + 1); count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, count);
		
		/* Check if this hop is the received interface	*/
		if (inet_address_equal (*hop_address_ptr, intf_ici_fdstruct_ptr->interface_received) == OPC_TRUE)
			{
			/* This is a candidate for automatic route shortening	*/
			route_shortening_candidate = OPC_TRUE;
			location_found = count;
			
			break;
			}
		}
	
	/* Check if this is the destination	*/
	if ((source_route_option_ptr->segments_left > 0) && 
		(inet_address_equal (ip_dgram_fd_ptr->dest_addr, intf_ici_fdstruct_ptr->interface_received)))
		{
		/* The destination node received the packet before 	*/
		/* the entire source route was completed. This is a */
		/* candidate for automatic route shortening			*/
		route_shortening_candidate = OPC_TRUE;
		location_found = num_hops;
		}
	
	if (route_shortening_candidate == OPC_FALSE)
		{
		/* This node address is not present in	*/
		/* the unexpanded portion of the source	*/
		/* route. There is no oportunity for	*/
		/* automatic route shortening			*/
		FRET (OPC_COMPCODE_FAILURE);
		}
		
	/* Get the previous hop information	*/
	overheard_node_address = dsr_pkt_support_source_route_hop_obtain (source_route_option_ptr, DsrC_Previous_Hop, ip_dgram_fd_ptr->src_addr, 
																		ip_dgram_fd_ptr->dest_addr);
	
	/* Check if there already exists an entry in the	*/
	/* "gratuitous" route reply table for this original	*/
	/* sender of the packet								*/
	if (dsr_grat_reply_entry_exists (grat_reply_table_ptr, ip_dgram_fd_ptr->src_addr, overheard_node_address) == OPC_TRUE)
		{
		/* There already exists an entry in the	*/
		/* gratuitous route reply table. Do not	*/
		/* perform automatic route shortening	*/
		FRET (OPC_COMPCODE_FAILURE);
		}
	
	/* Create a new entry in the "Gratuitous"	*/
	/* route reply table						*/
	dsr_grat_reply_table_insert (grat_reply_table_ptr, ip_dgram_fd_ptr->src_addr, overheard_node_address);
	
	/* Return a "gratuitous" route reply to the	*/
	/* source of the source route				*/
	route_lptr = op_prg_list_create ();
	
	for (count = 0; count < (num_hops - source_route_option_ptr->segments_left); count++)
		{
		/* Copy the first part of the source route	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	for (count = location_found; count < num_hops; count++)
		{
		/* Copy the last part of the source route	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option_ptr->route_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Create a route reply option	*/
	route_reply_dsr_tlv_ptr = dsr_pkt_support_route_reply_tlv_create (ip_dgram_fd_ptr->src_addr, ip_dgram_fd_ptr->dest_addr, 
										route_lptr, source_route_option_ptr->last_hop_external);
	
	/* Create the DSR packet	*/
	route_reply_dsr_pkptr = dsr_pkt_support_pkt_create (ip_dgram_fd_ptr->protocol);
	
	/* Set the route reply option in the DSR packet header	*/
	dsr_pkt_support_option_add (route_reply_dsr_pkptr, route_reply_dsr_tlv_ptr);
	
	/* Allocate memory for manet_nexthop_info_ptr */
	manet_nexthop_info_ptr = (ManetT_Nexthop_Info *) op_prg_mem_alloc (sizeof (ManetT_Nexthop_Info));
	
	/* Set the DSR packet in a newly created IP datagram	*/
	/* The destination address of the IP datagram carrying	*/
	/* the route reply option is the address of the 		*/
	/* initiator of the route request						*/
	reply_ip_pkptr = dsr_rte_ip_datagram_create (route_reply_dsr_pkptr, ip_dgram_fd_ptr->src_addr,
								overheard_node_address, manet_nexthop_info_ptr);
	
	if (LTRACE_ACTIVE)
		{
		route_str = dsr_support_option_route_print (route_reply_dsr_tlv_ptr);
		inet_address_print (src_hop_addr_str, ip_dgram_fd_ptr->src_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
		inet_address_print (over_hop_addr_str, overheard_node_address);
		inet_address_to_hname (overheard_node_address, over_node_name);
		inet_address_print (cur_hop_addr_str, intf_ici_fdstruct_ptr->interface_received);
		inet_address_to_hname (intf_ici_fdstruct_ptr->interface_received, cur_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "to node %s (%s) overheard from node %s (%s) by current node %s (%s) with route", 
			src_hop_addr_str, src_node_name, over_hop_addr_str, over_node_name, cur_hop_addr_str, cur_node_name);
		sprintf (temp_str1, "The packet was destined to node %s (%s)", dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("Automatic route shortening taking place", "Sending a route reply option in packet", 
								temp_str, route_str, temp_str1, OPC_NIL);
		op_prg_mem_free (route_str);
		}
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, reply_ip_pkptr);
	
	/* Update the statistics for the routing traffic sent	*/
	dsr_support_routing_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, reply_ip_pkptr);
	
	/* Send the packet to the CPU which will forward it	*/
	/* on the appropriate interface after processing	*/
	
	/* Install the event state 	*/
	/* This event will be processed in ip_rte_support.ex.c while receiving     */
	/* DSR control packets. manet_nexthop_info_ptr will point to structure	   */
	/* containing nexthop info, so IP table lookup is not again done for them. */
	op_ev_state_install (manet_nexthop_info_ptr, OPC_NIL);
	manet_rte_to_cpu_pkt_send_schedule (module_data_ptr, parent_prohandle, parent_pro_id, reply_ip_pkptr);
	op_ev_state_install (OPC_NIL, OPC_NIL);
	
	/* Free the memory of the temporary list	*/
	while (op_prg_list_size (route_lptr) > 0)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (route_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		}
	
	dsr_temp_list_clear (route_lptr);
	
	FRET (OPC_COMPCODE_SUCCESS);
	}
		


/*********************************************************/
/************** CACHING ROUTING INFORMATION **************/
/*********************************************************/


static void
dsr_rte_route_cache_update (DsrT_Packet_Option* dsr_tlv_ptr, IpT_Dgram_Fields* ip_dgram_fd_ptr)
	{
	DsrT_Route_Request_Option*		route_request_option_ptr = OPC_NIL;
	DsrT_Route_Reply_Option*		route_reply_option_ptr = OPC_NIL;
	DsrT_Source_Route_Option*		src_route_option_ptr = OPC_NIL;
	DsrT_Acknowledgement*			ack_option_ptr = OPC_NIL;
	InetT_Address*					hop_address_ptr;
	Boolean							first_hop_external = OPC_FALSE;
	Boolean							last_hop_external = OPC_FALSE;
	List*							route_lptr = OPC_NIL;
	List*							temp_route_lptr = OPC_NIL;
	List*							rcvd_route_lptr = OPC_NIL;
	int								num_hops, total_hops, count;
	int								size, node_count, route_position;
	
	/** A node forwarding or overhearing a packet	**/
	/** should add all usable routing information	**/
	/** in its own route cache. This function 		**/
	/** updates the route cache with the useful		**/
	/** information from the received packet		**/
	FIN (dsr_rte_route_cache_update (<args>));
	
	/** We are assuming that there is always bi-directional	**/
	/** communication between all nodes						**/
	
	/* Determine the type of option	*/
	switch (dsr_tlv_ptr->option_type)
		{
		case (DSRC_ROUTE_REQUEST):
			{
			/* If the source address of the IP datagram		*/
			/* belongs to this node, then do not insert the	*/
			/* route in the route cache						*/
			if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->src_addr) == OPC_TRUE)
				{
				FOUT;
				}
			
			/* Get the route request option	*/
			route_request_option_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
			
			/* If we are already in the list of address, then 	*/
			/* do not insert route in the route cache			*/
			/* Get the number of hops							*/
			num_hops = op_prg_list_size (route_request_option_ptr->route_lptr);
			
			if(num_hops > 0)
				{
				for (count = 0; count < (num_hops - 1); count++)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_access (route_request_option_ptr->route_lptr, count);
					if (manet_rte_address_belongs_to_node (module_data_ptr, *hop_address_ptr) == OPC_TRUE)
						{
						FOUT;
						}
					}
				}
			
		
			/* The route in the path needs	*/
			/* to be inverted as the source	*/
			/* of the route request is the	*/
			/* destination of the route		*/
			route_lptr = op_prg_list_create ();
			
			for (count = num_hops; count > 0; count--)
				{
				hop_address_ptr = (InetT_Address*) op_prg_list_access (route_request_option_ptr->route_lptr, (count - 1));
				
				/* Invert the list	*/
				op_prg_list_insert (route_lptr, hop_address_ptr, OPC_LISTPOS_TAIL);
				}
			
			/* Add the source node to the 	*/
			/* tail of the list as the		*/
			/* source node address will not	*/
			/* be present in address [i]	*/
			op_prg_list_insert (route_lptr, &ip_dgram_fd_ptr->src_addr, OPC_LISTPOS_TAIL);
			
			/* Add this route to the cache	*/
			/* The destination of this path	*/
			/* is the source node of the	*/
			/* route request				*/
			dsr_route_cache_entry_add (route_cache_ptr, route_lptr, OPC_FALSE, OPC_FALSE);
			
			/* Check if there are any packets	*/
			/* to be sent out in the send 		*/
			/* buffer to all the destinations	*/
			/* along this path					*/
			dsr_rte_send_buffer_check (route_lptr);
			
			/* Free the inverted route	*/
			dsr_temp_list_clear (route_lptr);
			
			break;
			}
			
		case (DSRC_ROUTE_REPLY):
		case (DSRC_SOURCE_ROUTE):
			{
			if (dsr_tlv_ptr->option_type == DSRC_ROUTE_REPLY)
				{
				/* Get the route reply option	*/
				route_reply_option_ptr = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				/* Set the received route	*/
				rcvd_route_lptr = route_reply_option_ptr->route_lptr;
				
				/* Set the external flags	*/
				last_hop_external = route_reply_option_ptr->last_hop_external;
				}
			else
				{
				/* Get the source route option	*/
				src_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				/* Set the received route	*/
				rcvd_route_lptr = src_route_option_ptr->route_lptr;
				
				/* Set the external flags	*/
				last_hop_external = src_route_option_ptr->last_hop_external;
				first_hop_external = src_route_option_ptr->first_hop_external;
				}
			
			/* Determine which hop in the 	*/
			/* route belongs to one	of this	*/
			/* node's interfaces			*/
			num_hops = op_prg_list_size (rcvd_route_lptr);
			
			/* Create the route from the source	*/
			/* node to the destination node		*/
			route_lptr = op_prg_list_create ();
			
			/* Add the source node to the 	*/
			/* head of the list as the		*/
			/* source node address will not	*/
			/* be present in address [i] 	*/
			/* for both the route reply 	*/
			/* and source route options		*/
			if (dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
				{
				op_prg_list_insert (route_lptr, &ip_dgram_fd_ptr->src_addr, OPC_LISTPOS_HEAD);
				}
			else
				{
				op_prg_list_insert (route_lptr, &ip_dgram_fd_ptr->dest_addr, OPC_LISTPOS_HEAD);
				}
			
			/* Insert the intermediate nodes	*/
			for (size = 0; size < num_hops; size++)
				{
				hop_address_ptr = (InetT_Address*) op_prg_list_access (rcvd_route_lptr, size);
				op_prg_list_insert (route_lptr, hop_address_ptr, OPC_LISTPOS_TAIL);
				}
			
			if (dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
				{
				/* Add the destination node to 	*/
				/* the tail of the list as the	*/
				/* destination node address 	*/
				/* will not be present in 		*/
				/* address [i] for the source 	*/
				/* route option. Do not add it	*/
				/* for the route reply option	*/
				/* as it is already present		*/
				op_prg_list_insert (route_lptr, &ip_dgram_fd_ptr->dest_addr, OPC_LISTPOS_TAIL);
				}
				
			total_hops = op_prg_list_size (route_lptr);
			
			node_count = 0;
			route_position = 0;
			
			/* Making sure if i am there only 			*/
			/* once in the route; Not more not less		*/
			for (count = 0; count < total_hops; count++)
				{
				/* Check if the hop belongs to the node	*/
				hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, count);
				
				if (manet_rte_address_belongs_to_node (module_data_ptr, *hop_address_ptr) == OPC_TRUE)
					{
					if( node_count == 0)
						{
						route_position = count;
						}
					node_count++;
					}
				}
			
				
			/* Check if an address was found or i am there more than once */
			if(node_count != 1)
				{
				/* Either No matching address was found	*/
				/* or im there twice, i.e. loop			*/
				/* Do not insert anything				*/
				
				dsr_temp_list_clear (route_lptr);				
				
				FOUT;
				}
				
			if (route_position < (total_hops - 1))
				{
				/* Add the route from this node to the destination	*/
				temp_route_lptr = op_prg_list_create ();
						
				for (size = route_position; size < total_hops; size++)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, size);
					op_prg_list_insert (temp_route_lptr, hop_address_ptr, OPC_LISTPOS_TAIL);
					}
				
				/* Add this route to the cache	*/
				dsr_route_cache_entry_add (route_cache_ptr, temp_route_lptr, 
					first_hop_external, last_hop_external);
				
				/* Check if there are any packets	*/
				/* to be sent out in the send 		*/
				/* buffer to all the destinations	*/
				/* along this path					*/
				dsr_rte_send_buffer_check (temp_route_lptr);
			
				/* Free the list	*/
				dsr_temp_list_clear (temp_route_lptr);
				}
				
			if (route_position > 0)
				{
				/* Create a list for the route from this	*/
				/* node to the source node					*/
				temp_route_lptr = op_prg_list_create ();
			
				for (size = route_position; size >= 0; size--)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, size);
					op_prg_list_insert (temp_route_lptr, hop_address_ptr, OPC_LISTPOS_TAIL);
					}
				
				/* Add this route to the cache	*/
				dsr_route_cache_entry_add (route_cache_ptr, temp_route_lptr, first_hop_external, last_hop_external);
				
				/* Check if there are any packets	*/
				/* to be sent out in the send 		*/
				/* buffer to all the destinations	*/
				/* along this path					*/
				dsr_rte_send_buffer_check (temp_route_lptr);
			
				/* Free the list	*/
				dsr_temp_list_clear (temp_route_lptr);
				}
			
			/* Free the list	*/
			dsr_temp_list_clear (route_lptr);
				
			break;
			}
		   
		case (DSRC_ACKNOWLEDGEMENT):
			{
			/* Get the acknowledgement option	*/
			ack_option_ptr = (DsrT_Acknowledgement*) dsr_tlv_ptr->dsr_option_ptr;
			
			/* Insert the route from this node	*/
			/* to the next hop node				*/
			route_lptr = op_prg_list_create ();
			
			/* Check if the destination address	*/
			/* belongs to this node				*/
			if (manet_rte_address_belongs_to_node (module_data_ptr, ack_option_ptr->ack_dest_address) == OPC_TRUE)
				{
				/* The destination ACK address belong to this node	*/
				op_prg_list_insert (route_lptr, &ack_option_ptr->ack_dest_address, OPC_LISTPOS_TAIL);
				op_prg_list_insert (route_lptr, &ack_option_ptr->ack_source_address, OPC_LISTPOS_TAIL);
				}
			else if (manet_rte_address_belongs_to_node (module_data_ptr, ack_option_ptr->ack_source_address) == OPC_TRUE)
				{
				/* The source ACK address belong to this node	*/
				op_prg_list_insert (route_lptr, &ack_option_ptr->ack_source_address, OPC_LISTPOS_TAIL);
				op_prg_list_insert (route_lptr, &ack_option_ptr->ack_dest_address, OPC_LISTPOS_TAIL);
				}
			else
				{
				/* Free the temporary list	*/
				dsr_temp_list_clear (route_lptr);
				
				/* None of the addresses belong to the node	*/
				FOUT;
				}
			
			/* Add this route to the cache	*/
			dsr_route_cache_entry_add (route_cache_ptr, route_lptr, OPC_FALSE, OPC_FALSE);
			
			/* Check if there are any packets	*/
			/* to be sent out in the send 		*/
			/* buffer to all the destinations	*/
			/* along this path					*/
			dsr_rte_send_buffer_check (route_lptr);
			
			/* Free the temporary list	*/
			dsr_temp_list_clear (route_lptr);
			
			break;
			}
		
		default:
			{
			/* This should never happen	*/
			dsr_rte_error ("Invalid option type in received packet", OPC_NIL, OPC_NIL);
			
			break;
			}
		}
	
	FOUT;
	}


static void
dsr_temp_list_clear (List* temp_lptr)
	{
	FIN (dsr_temp_list_clear (List* temp_lptr));
	
	if (temp_lptr == OPC_NIL)
		FOUT;
	
	while (op_prg_list_size (temp_lptr) > 0)
		op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);
	
	op_prg_mem_free (temp_lptr);
	
	FOUT;
	}


/********************************************************/
/****************** PACKET SALVAGING ********************/
/********************************************************/

static Boolean
dsr_rte_packet_salvage (Packet* ip_pkptr, InetT_Address unreachable_node_addr, IpT_Dgram_Fields* ip_dgram_fd_ptr, 
						IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	List*							routes_lptr = OPC_NIL;
	int								num_routes, count;
	DsrT_Path_Info*					path_ptr;
	InetT_Address*					next_hop_addr_ptr;
	Packet*							dsr_pkptr;
	List*							tlv_options_lptr;
	int								num_options;
	DsrT_Packet_Option*				old_dsr_tlv_ptr;
	DsrT_Packet_Option*				dsr_tlv_ptr;
	DsrT_Source_Route_Option*		old_source_route_option_ptr;
	DsrT_Source_Route_Option*		source_route_option_ptr;
	char							next_node_name [OMSC_HNAME_MAX_LEN];
	char							next_hop_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char							temp_str [2048];
	int								salvage = 0;
	int								count_i, num_hops = 0;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					new_hop_address_ptr;
	DsrT_Source_Route_Option*		new_source_route_option_ptr;
	
	/** Salvages a packet by finding an alternate	**/
	/** route to the destination if the next hop	**/
	/** node is unreachable							**/
	FIN (dsr_rte_packet_salvage (<args>));
	
	/* Check if there exists an alternate route	*/
	/* to the destination from this node		*/
	
	/* Get the list of routes to the destination	*/
	routes_lptr = dsr_route_cache_all_routes_to_dest_access (route_cache_ptr, ip_dgram_fd_ptr->dest_addr);
	
	if (routes_lptr == OPC_NIL)
		{
		/* No routes exist to the destination from this node	*/
		FRET (OPC_FALSE);
		}
	
	/* Get the number of routes	*/
	num_routes = op_prg_list_size (routes_lptr);
	
	/* Go through each route and find the best route which	*/
	/* does not have the unreachable node as the next hop	*/
	for (count = 0; count < num_routes; count++)
		{
		path_ptr = (DsrT_Path_Info*) op_prg_list_access (routes_lptr, count);
		
		/* The first element in the path is this source node	*/
		/* The second element in the path is the next hop node	*/
		/* Access the second element and make sure it is not 	*/
		/* the unreachable node.								*/
		next_hop_addr_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, 1);
		
		if (inet_address_equal (*next_hop_addr_ptr, unreachable_node_addr) == OPC_FALSE)
			{
			/* The next hop is another route not 		*/
			/* through the unreachable node. Use this	*/
			/* route as the alternate route				*/
			break;
			}
		}
	
	if (count == num_routes)
		{
		/* No path was found	*/
		FRET (OPC_FALSE);
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (next_hop_addr_str, unreachable_node_addr);
		inet_address_to_hname (unreachable_node_addr, next_node_name);
		inet_address_print (dest_hop_addr_str, ip_dgram_fd_ptr->dest_addr);
		inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
		sprintf (temp_str, "to destination %s (%s) as the next hop node %s (%s) is unreachable", 
					dest_hop_addr_str, dest_node_name, next_hop_addr_str, next_node_name);
		op_prg_odb_print_major ("Salvaging the packet", temp_str, OPC_NIL);
		}
	
	/* Get the encapsulated DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Get the list of options	*/
	op_pk_nfd_access (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Remove the old maintenance request option from the packet	*/
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count =0; count < num_options; count++)
		{
		
		old_dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		if (old_dsr_tlv_ptr->option_type == DSRC_ACK_REQUEST)
			{
			dsr_pkt_support_option_remove (dsr_pkptr, DSRC_ACK_REQUEST);
			break;
			}
		}
	
	/* Set the DSR packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access the source route option	*/
		old_dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		/* Get this hop from the source route	*/
		if (old_dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
			{
			/* Remove the source route option	*/
			old_dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_remove (tlv_options_lptr, count);
			old_source_route_option_ptr = (DsrT_Source_Route_Option*) old_dsr_tlv_ptr->dsr_option_ptr;
			salvage = old_source_route_option_ptr->salvage;
		
			/* Don't salvage a packet more than 15 times */
			if (salvage > 14)
				{
				
				/* Free the IP addresses for each hop */
				num_hops = op_prg_list_size (old_source_route_option_ptr->route_lptr);
				for (count_i = 0; count_i < num_hops; count_i++)
					{
					hop_address_ptr = (InetT_Address*) op_prg_list_access (old_source_route_option_ptr->route_lptr, count_i);
					inet_address_destroy_dynamic (hop_address_ptr);
					}
				/* Free the memory allocated	*/
				op_prg_mem_free (old_source_route_option_ptr->route_lptr);
				op_prg_mem_free (old_source_route_option_ptr);
				
				FRET (OPC_FALSE);
				}
			
			break;
			}
		}
	
	/* Create a new source route to the destination	*/
	dsr_tlv_ptr = dsr_pkt_support_source_route_tlv_create (path_ptr->path_hops_lptr, path_ptr->first_hop_external, 
							path_ptr->last_hop_external, OPC_FALSE, OPC_TRUE);
	
	/* Print function for new source route with ltrace */	
	if (LTRACE_ACTIVE)
		{
		printf ("New Route after Salvaging \n");
		new_source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
		num_hops = op_prg_list_size (new_source_route_option_ptr->route_lptr);
		printf("New SrcRt:    segment left: %d\n", new_source_route_option_ptr->segments_left);
		for (count_i = 0; count_i < num_hops; count_i++)
			{
			new_hop_address_ptr = (InetT_Address*) op_prg_list_access (new_source_route_option_ptr->route_lptr, count_i);
			inet_address_to_hname (*new_hop_address_ptr, dest_node_name);
			printf(" %s ", dest_node_name);
			}
		printf("\n\n");
		}
	
	/* Free the old src route memory */
	num_hops = op_prg_list_size (old_source_route_option_ptr->route_lptr);
	for (count_i = 0; count_i < num_hops; count_i++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (old_source_route_option_ptr->route_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		}

	/* Free the memory allocated */
	op_prg_mem_free (old_source_route_option_ptr->route_lptr);
	op_prg_mem_free (old_source_route_option_ptr);
	
	/* The segments left field needs to be initailized	*/
	/* including this node as it is not the source		*/
	source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Set the salvage to one more than the salvage of 	*/
	/* the source route that caused the route error		*/
	source_route_option_ptr->salvage = salvage + 1;
	
	/* Get the encapsulated DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Because DSR packet is IP's payload, update IP's header fields			*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	ip_dgram_fd_ptr->orig_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	ip_dgram_fd_ptr->frag_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	ip_dgram_fd_ptr->original_size -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	
	/* Set the new salvaged source route option in the DSR packet header	*/
	dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
	
	/* Once DSR payload is resized, set IP's fields again appropriately 	*/
	ip_dgram_fd_ptr->orig_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	ip_dgram_fd_ptr->frag_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	ip_dgram_fd_ptr->original_size += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
	
	/* Set the DSR packet in the IP datagram	*/
	//dsr_rte_ip_datagram_encapsulate (ip_pkptr, dsr_pkptr, *next_hop_addr_ptr);
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Check if maintenance is needed against the	*/
	/* maintenance holdoff time for the next hop	*/
	if (dsr_maintenance_buffer_maint_needed (maint_buffer_ptr, *next_hop_addr_ptr) == OPC_TRUE)
		{
		if (LTRACE_ACTIVE)
			{
			inet_address_print (dest_hop_addr_str, *next_hop_addr_ptr);
			inet_address_to_hname (*next_hop_addr_ptr, dest_node_name);
			sprintf (temp_str, "to next hop node %s (%s) with ID (%d)", dest_hop_addr_str, dest_node_name, ack_request_identifier);
			op_prg_odb_print_major ("Adding a maintenance request option in packet", temp_str, OPC_NIL);
			}
			
		/* Create a IP datagram with a maintenance	*/
		/* request option in the DSR header			*/
		dsr_tlv_ptr = dsr_pkt_support_ack_request_tlv_create (ack_request_identifier);
		
		/* Update the statistic for the number of maintenance requests sent	*/
		dsr_support_maintenace_stats_update (stat_handle_ptr, global_stathandle_ptr, OPC_TRUE);
		
		/* Get the encapsulated DSR packet from the IP datagram	*/
		op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
		
		/* Because DSR packet is IP's payload, update IP's header fields			*/
		ip_dgram_fd_ptr->orig_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->frag_len -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->original_size -= (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		
		/* Set the maintenance request option in the DSR packet header	*/
		dsr_pkt_support_option_add (dsr_pkptr, dsr_tlv_ptr);
		
		/* Once DSR payload is resized, set IP's fields again appropriately 	*/
		ip_dgram_fd_ptr->orig_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->frag_len += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		ip_dgram_fd_ptr->original_size += (OpT_Packet_Size) (op_pk_total_size_get (dsr_pkptr)/8);
		
		/* Set the DSR packet in the IP datagram	*/
		//dsr_rte_ip_datagram_encapsulate (ip_pkptr, dsr_pkptr, *next_hop_addr_ptr);
		op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
		
		/* A maintenance request has been added	*/
		/* Place a copy of the packet in the	*/
		/* maintenance buffer for retranmission	*/
		dsr_maintenance_buffer_pkt_enqueue (maint_buffer_ptr, ip_pkptr, *next_hop_addr_ptr, ack_request_identifier);
			
		/* Increment the ACK Request identifier	*/
		ack_request_identifier++;
		}
	
	/* Update the statistic for the total number of packets salvaged	*/
	dsr_support_packets_salvaged_stats_update (stat_handle_ptr, global_stathandle_ptr);
	
	/* Update the statistic for the total traffic sent	*/
	dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
		
	/* Send the packet out to the MAC	*/
	manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, inet_address_copy (*next_hop_addr_ptr), ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
	
	FRET (OPC_TRUE);
	}


/*********************************************************/
/******************** TIMER EXPIRY ***********************/
/*********************************************************/

EXTERN_C_BEGIN
void
dsr_rte_route_request_expiry_handle (InetT_Address* dest_address_ptr, int PRG_ARG_UNUSED (code))
	{
	List*					pkt_lptr = OPC_NIL;
	int						num_pkts;
	Packet*					pkptr = OPC_NIL;
	char					dest_node_name [OMSC_HNAME_MAX_LEN];
	char					dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char					temp_str [2048];
	
	/** Handles the route request expiry	**/
	FIN (dsr_rte_route_request_expiry_handle (<args>));
	
	/* Check if it is possible to schedule		*/
	/* another route request. It may not be 	*/
	/* possible if the maximum number of		*/
	/* retransmissions have been reached or if	*/
	/* the request period is greater than the	*/
	/* maximum request period					*/
	if (dsr_route_request_next_request_schedule_possible (route_request_table_ptr, *dest_address_ptr) == OPC_FALSE)
		{
		/* No more requests can be generated for	*/
		/* this destination node. Delete all 		*/
		/* packets in the send buffer to this 		*/
		/* destination node that is unreachable		*/
		
		/* Remove all packets from the send buffer	*/
		/* to this destination node					*/
		pkt_lptr = dsr_send_buffer_pkt_list_get (send_buffer_ptr, *dest_address_ptr, OPC_TRUE);
		num_pkts = op_prg_list_size (pkt_lptr);
		
		while (op_prg_list_size (pkt_lptr) > 0)
			{
			/* Destroy all packets to this destination	*/
			pkptr = (Packet*) op_prg_list_remove (pkt_lptr, OPC_LISTPOS_HEAD);
			manet_rte_ip_pkt_destroy (pkptr);
			
			/* Update the number of data packets discarded	*/
			op_stat_write (stat_handle_ptr->num_pkts_discard_shandle, 1.0);
			op_stat_write (global_stathandle_ptr->num_pkts_discard_global_shandle, 1.0);
			}
		
		/* Remove this route request from the	*/
		/* route request table					*/
		dsr_route_request_originating_table_entry_delete (route_request_table_ptr, *dest_address_ptr);
		inet_address_destroy_dynamic (dest_address_ptr);
		
		FOUT;
		}
	
	/* It is possible to schedule a new route request	*/
	/* Check if there are any packets that are still	*/
	/* queued to that destination						*/
	pkt_lptr = dsr_send_buffer_pkt_list_get (send_buffer_ptr, *dest_address_ptr, OPC_FALSE);
	num_pkts = op_prg_list_size (pkt_lptr);
	
	if (num_pkts == 0)
		{
		/* There are no packets queued to be sent	*/
		/* to this destination. Delete the request	*/
		dsr_route_request_originating_table_entry_delete (route_request_table_ptr, *dest_address_ptr);
		inet_address_destroy_dynamic (dest_address_ptr);
		
		FOUT;
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_ptr_print (dest_hop_addr_str, dest_address_ptr);
		inet_address_to_hname (*dest_address_ptr, dest_node_name);
		sprintf (temp_str, "to destination %s (%s)", dest_hop_addr_str, dest_node_name);
		op_prg_odb_print_major ("The route request timer has expired", "Rebroadcasting a route request packet", temp_str, OPC_NIL);
		}
	
	/* There are packets queued to the destination	*/
	/* Resend the route request						*/
	dsr_rte_route_request_send (*dest_address_ptr, OPC_FALSE);
	inet_address_destroy_dynamic (dest_address_ptr);
	dsr_temp_list_clear (pkt_lptr);
	
	FOUT;
	}


void
dsr_rte_maintenance_expiry_handle (void* maint_request_idx, int PRG_ARG_UNUSED (code))
	{
	Packet*						ip_pkptr = OPC_NIL;
	List*						pkt_lptr = OPC_NIL;
	IpT_Rte_Ind_Ici_Fields* 	intf_ici_fdstruct_ptr = OPC_NIL;
	IpT_Dgram_Fields* 			ip_dgram_fd_ptr = OPC_NIL;
	Ici*						pkt_info_ici_ptr = OPC_NIL;
	Boolean						max_retrans_reached = OPC_FALSE;
	Boolean						packet_salvaged = OPC_FALSE;
	InetT_Address				next_hop_addr;
	int							num_pkts;
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [2048];
	int* 						maint_request_id_ptr;
	
	/** Handles the expiry of the maintenance	**/
	/** request	timer. 							**/
	FIN (dsr_rte_maintenance_expiry_handle (<args>));
	
	maint_request_id_ptr = (int *) maint_request_idx;
	
	/* The maintenance request timer has expired	*/
	/* Handle this expiry for this request ID	*/
	pkt_lptr = dsr_maintenance_buffer_ack_request_expire (maint_buffer_ptr, *maint_request_id_ptr, &max_retrans_reached, &next_hop_addr);
	
	/* Get the number of packets waiting to that destination	*/
	num_pkts = op_prg_list_size (pkt_lptr);
	
	if (num_pkts == 0)
		{
		/* No packets in the maintenance	*/
		/* buffer with the incoming ID		*/
		dsr_temp_list_clear (pkt_lptr);
		
		FOUT;
		}
	
	if (LTRACE_ACTIVE)
		{
		inet_address_print (dest_hop_addr_str, next_hop_addr);
		inet_address_to_hname (next_hop_addr, dest_node_name);
		sprintf (temp_str, "to next hop node %s (%s) with maintenance ID (%d)", dest_hop_addr_str, dest_node_name, *maint_request_id_ptr);
		op_prg_odb_print_major ("The maintenance request timer has expired", temp_str, OPC_NIL);
		}
	
	/* Get each packet that was waiting the next hop	*/
	/* confirmation. If the maximum retransmissions is	*/
	/* not reached, then there will only be one packet	*/
	/* in the queue to retransmit. Else, there will be  */
	/* all packets waiting that next hop confirmation	*/
	while (num_pkts > 0)
		{
		/* Initialize the variables	*/
		packet_salvaged = OPC_FALSE;
		
		/* Get each packet from the list	*/
		ip_pkptr = (Packet*) op_prg_list_remove (pkt_lptr, OPC_LISTPOS_HEAD);
		
		/* Get the source address of the packet	*/
		/* Access the fields of the IP datagram	*/
		op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
		/* Get the ICI associated with the packet	*/
		/* Get the pointer to access the contents	*/
		pkt_info_ici_ptr = op_pk_ici_get (ip_pkptr);
	
		/* Get the incoming packet information	*/
		op_ici_attr_get (pkt_info_ici_ptr, "rte_info_fields", &intf_ici_fdstruct_ptr);
		
		/* Set the incoming packet information	*/
		op_ici_attr_set (pkt_info_ici_ptr, "rte_info_fields", intf_ici_fdstruct_ptr);
		
		/* Set the ICI back in the packet	*/
		op_pk_ici_set (ip_pkptr, pkt_info_ici_ptr);
		
		if (max_retrans_reached == OPC_TRUE)
			{
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("The maximum maintenance retransmissions have been reached", OPC_NIL, OPC_NIL);
				}
		
			if ((intf_ici_fdstruct_ptr->instrm == module_data_ptr->instrm_from_ip_encap) ||
				(intf_ici_fdstruct_ptr->instrm == IpC_Pk_Instrm_Child))				
				{
				/* The packet was originated by this node	*/
				/* as it arrived form the higher layer.		*/
				/* Remove all routes in the route cache 	*/
				/* with this next hop						*/
				dsr_route_cache_src_node_with_next_hop_routes_delete (route_cache_ptr, next_hop_addr);
				}
			else	
				{
				/* The next hop node is unreachable. Remove	*/
				/* all routes from this node's route cache	*/
				/* that have the link between this node and	*/
				/* the next hop unreachable node.			*/
				dsr_route_cache_src_node_with_next_hop_routes_delete (route_cache_ptr, next_hop_addr);
				
				/* The packet was not originated by this node	*/
				/* Send a route error message back to source	*/
				dsr_rte_route_error_send (ip_pkptr, next_hop_addr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
				
				if (packet_salvaging_function)
					{
					/* This is an intermediate node for which the	*/
					/* next hop is unreachable. Salvage the packet	*/
					/* if there exists another route in its cache	*/
					packet_salvaged = dsr_rte_packet_salvage (ip_pkptr, next_hop_addr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
					}
				}
			
			if (!packet_salvaged)
				{
				/* Destroy the IP packet	*/
				manet_rte_ip_pkt_destroy (ip_pkptr);
				
				/* Update the number of packets discarded	*/
				op_stat_write (stat_handle_ptr->num_pkts_discard_shandle, 1.0);
				op_stat_write (global_stathandle_ptr->num_pkts_discard_global_shandle, 1.0);
				}
			}
		else
			{
			/* Update the statistic for the total traffic sent	*/
			dsr_support_total_traffic_sent_stats_update (stat_handle_ptr, global_stathandle_ptr, ip_pkptr);
	
			/* The maximum attempts have not been exceeded	*/
			/* Retransmit the packet with the ack request	*/
			/* (no need to pass a copy of the next_hop_addr	*/
			/* because what we have is already a copy)		*/
			manet_rte_to_mac_pkt_send (module_data_ptr, ip_pkptr, next_hop_addr, ip_dgram_fd_ptr, intf_ici_fdstruct_ptr);
			}
		
		/* Decrement the counter	*/
		num_pkts--;
		}
	
	/* Free the memory used by the maintenance request ID.	*/
	op_prg_mem_free (maint_request_id_ptr);
	
	/* Destroy the list returned	*/
	dsr_temp_list_clear (pkt_lptr);
	
	FOUT;
	}

EXTERN_C_END

/*********************************************************/
/************* IP INTERFACING FUNCTIONS ******************/
/*********************************************************/

static Packet*
dsr_rte_ip_datagram_encapsulate (Packet* ip_pkptr, Packet* dsr_pkptr, InetT_Address next_hop_addr)
	{
	Packet*					app_pkptr = OPC_NIL;
	OpT_Packet_Size			app_pkt_size;
	OpT_Packet_Size			dsr_pkt_size;
	IpT_Dgram_Fields* 		ip_dgram_fd_ptr = OPC_NIL;
	OpT_Packet_Size			dsr_header_size;
	
	/** Encapsultes the DSR packet an the existing	**/
	/** IP packet that is passed into the function	**/
	/** The encapsulated IP datagram is returned	**/
	FIN (dsr_rte_ip_datagram_encapsulate (<args>));
	
	/* Access the fields of the IP datagram	*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* Remove the application packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &app_pkptr);
	
	/* Get DSR header size. This represents increment in original IP packet's payload	*/
	dsr_header_size = op_pk_total_size_get (dsr_pkptr);

	if (app_pkptr != OPC_NIL)
		{
		/* Get the size of the application packet	*/
		app_pkt_size = op_pk_total_size_get (app_pkptr);
	
		/* Set application packet in DSR's data field		*/
		/* Set application packet size as DSR payload size	*/
		op_pk_nfd_set (dsr_pkptr, "data", app_pkptr);
		op_pk_bulk_size_set (dsr_pkptr, app_pkt_size);
		}
	
	/* Get the size of the DSR packet	*/
	dsr_pkt_size = op_pk_total_size_get (dsr_pkptr);
	
	/* Set the DSR packet in the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data.						*/
	op_pk_bulk_size_set (ip_pkptr, dsr_pkt_size);
	
	/* Set the protocol type and next hop of the IP datagram 	*/
	ip_dgram_fd_ptr->next_addr = inet_address_copy (next_hop_addr);
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Dsr;
	
	/* IP fragment and original sizes have increased due to added DSR header as IP payload	*/
	ip_dgram_fd_ptr->frag_len = (OpT_Packet_Size) (dsr_pkt_size/8);
	ip_dgram_fd_ptr->orig_len += (OpT_Packet_Size) (dsr_header_size/8);
	ip_dgram_fd_ptr->original_size += (OpT_Packet_Size) (dsr_header_size/8);
	
	FRET (ip_pkptr);
	}


static Packet*
dsr_rte_ip_datagram_decapsulate (Packet* ip_pkptr)
	{
	Packet*					app_pkptr = OPC_NIL;
	Packet*					dsr_pkptr = OPC_NIL;
	OpT_Packet_Size			app_pkt_size = 0;
	IpT_Dgram_Fields* 		ip_dgram_fd_ptr = OPC_NIL;
	int						next_header;
	
	/** Decapsulates the DSR packet from the 	**/
	/** IP datagram and returns the DSR packet	**/
	FIN (dsr_rte_ip_datagram_decapsulate (<args>));
	
	/* Remove the DSR packet from the IP packet	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Remove the application packet from the DSR packet	*/
	/* if it is set in the packet							*/
	if (op_pk_nfd_is_set (dsr_pkptr, "data") == OPC_TRUE)
		{
		op_pk_nfd_get (dsr_pkptr, "data", &app_pkptr);
		}
	else
		{
		/* This is not an application level	*/
		/* packet. It is just a DSR packet	*/
		op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
		
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
	
	/* Get the protocol type from the DSR packet	*/
	op_pk_nfd_access (dsr_pkptr, "Next Header", &next_header);
	
	/* Access the fields of the IP datagram	*/
	op_pk_nfd_access (ip_pkptr, "fields", &ip_dgram_fd_ptr);
	
	/* Set the protocol type in the IP fields	*/
	ip_dgram_fd_ptr->protocol = next_header;
	
	/* Destroy the DSR packet	*/
	op_pk_destroy (dsr_pkptr);
	
	FRET (ip_pkptr);
	}


static Packet*
dsr_rte_ip_datagram_create (Packet* dsr_pkptr, InetT_Address dest_addr, 
				InetT_Address next_hop_addr, ManetT_Nexthop_Info* nexthop_info_ptr)
	{
	Packet*					ip_pkptr = OPC_NIL;
	IpT_Dgram_Fields*		ip_dgram_fd_ptr = OPC_NIL;
	InetT_Address			src_address;
	OpT_Packet_Size			dsr_pkt_size;
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	int						outstrm;
	short					output_intf_index;
	InetT_Address_Range* 	inet_addr_range_ptr;
	
	
	/** Encapsulates the DSR packet in an 	**/
	/** IP datagram and sets the fields		**/
	FIN (dsr_rte_ip_datagram_create (<args>));
	
	/* Get the size of the DSR packet	*/
	dsr_pkt_size = op_pk_total_size_get (dsr_pkptr);
	
	/* Create an IP datagram.	*/
	ip_pkptr = op_pk_create_fmt ("ip_dgram_v4");

	/* Set the ICMP packet in the data field of the IP datagram	*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);
	
	/* Set the bulk size of the IP packet to model the space	*/
	/* occupied by the encapsulated data. This is equal to the	*/
	/* the data packet plus the size of the ICMP header.		*/
	op_pk_bulk_size_set (ip_pkptr, dsr_pkt_size);

	/* Create fields data structure that contains orig_len,	*/
	/* ident, frag_len, ttl, src_addr, dest_addr, frag,		*/
	/* connection class, src and dest internal addresses.	*/
	ip_dgram_fd_ptr = ip_dgram_fdstruct_create ();

	/* Set the destination address for this IP datagram.	*/
	ip_dgram_fd_ptr->dest_addr = inet_address_copy (dest_addr);
	ip_dgram_fd_ptr->connection_class = 0;
	
	/* Set the correct protocol in the IP datagram.	*/
	ip_dgram_fd_ptr->protocol = IpC_Protocol_Dsr;

	/* Set the packet size-related fields of the IP datagram.	*/
	ip_dgram_fd_ptr->orig_len = dsr_pkt_size/8;
	ip_dgram_fd_ptr->frag_len = ip_dgram_fd_ptr->orig_len;
	ip_dgram_fd_ptr->original_size = 160 + dsr_pkt_size;

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
		
		/* Fill ManetT_Nexthop_Info fields if   	*/
		/* destination address is not broadcast		*/
		if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &output_intf_index,
			&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
			{
			
			/* Get the outgoing interface address	*/
			src_address = inet_address_range_addr_get (inet_addr_range_ptr);
			
			/* Set the source address */
			ip_dgram_fd_ptr->src_addr = inet_address_copy (src_address);
			
			if(nexthop_info_ptr != OPC_NIL)
				{
				nexthop_info_ptr->next_hop_addr = inet_address_copy (next_hop_addr);
				nexthop_info_ptr->output_table_index = (int) output_intf_index;
				nexthop_info_ptr->interface_info_ptr = iface_info_ptr;
				}
			else
				{
				printf("Error condition. dsr_ip_datagram_create() dsr_rte \n");
				}
			}
	
		}
	
	/*	Set the fields structure inside the ip datagram	*/
	ip_dgram_fields_set (ip_pkptr, ip_dgram_fd_ptr);

	FRET (ip_pkptr);
	}


/************************************************************/
/*************** INTERNAL SUPPORT FUNCTIONS *****************/
/************************************************************/

static DsrT_Packet_Type
dsr_rte_packet_type_determine (IpT_Dgram_Fields* ip_dgram_fd_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr)
	{
	/** This function determines the type	**/
	/** of packet received and the layer	**/
	/** from which the packet was received	**/
	FIN (dsr_rte_packet_type_determine (<args>));
	
	/* Check if this packet is a higher layer packet (inclusive of other IP child processes) */
	if ((intf_ici_fdstruct_ptr->instrm == module_data_ptr->instrm_from_ip_encap) ||
		(intf_ici_fdstruct_ptr->instrm == IpC_Pk_Instrm_Child))
		FRET (DsrC_Higher_Layer_Packet);
	
	/* Check if this is a broadcast packet	*/
	if (((inet_address_family_get (&ip_dgram_fd_ptr->dest_addr) == InetC_Addr_Family_v4) && 
		(inet_address_equal (ip_dgram_fd_ptr->dest_addr, InetI_Broadcast_v4_Addr) == OPC_TRUE)) ||
		((inet_address_family_get (&ip_dgram_fd_ptr->dest_addr) == InetC_Addr_Family_v6) && 
		(inet_address_equal (ip_dgram_fd_ptr->dest_addr, InetI_Ipv6_All_Nodes_LL_Mcast_Addr) == OPC_TRUE)))
		FRET (DsrC_Broadcast_Packet);
	
	/* Check if the packet has reached its destination	*/
	if (manet_rte_address_belongs_to_node (module_data_ptr, ip_dgram_fd_ptr->dest_addr) == OPC_TRUE)
		FRET (DsrC_Destination_Packet);
	
	/* Check if the packet has a DSR header	*/
	if (ip_dgram_fd_ptr->protocol != IpC_Protocol_Dsr)
		FRET (DsrC_Undef_Packet);
	
	/** The packet received is a DSR packet	**/
	/** which is not a broadcast packet and **/
	/** has not reached its destination. 	**/
	/** Determine if this packet is an 		**/
	/** inroute packet that was received by **/
	/** this node which is in the route OR	**/
	/** the packet is an overheard packet 	**/
	/** that was received by this node not	**/
	/** in the route of the packet			**/
	FRET (DsrC_Undef_Packet);
	}


static DsrT_Packet_Option*
dsr_rte_packet_option_get (Packet* dsr_pkptr, int type)
	{
	List*					tlv_options_lptr = OPC_NIL;
	int						num_options, count;
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	
	FIN (dsr_rte_packet_option_get (<args>));
	
	/* Get the list of options	*/
	op_pk_nfd_access (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	/* Find the option needed	*/
	for (count = 0; count < num_options; count++)
		{
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		if (dsr_tlv_ptr->option_type == type)
			{
			/* This is the required option	*/
			FRET (dsr_tlv_ptr);
			}
		}
	
	FRET (OPC_NIL);
	}
	


static void
dsr_rte_jitter_schedule (Packet* pkptr, int option_type)
	{
	double			jitter;
		
	/** Schedule the packet to be sent out	**/
	/** after a jitter						**/
	FIN (dsr_rte_jitter_schedule (<args>));
	
	jitter = oms_dist_outcome (broadcast_jitter_dist_ptr);
	
	/* Send the packet with a short jitter	*/
	op_intrpt_schedule_call (op_sim_time () + jitter, option_type, dsr_rte_jittered_pkt_send, pkptr);
	
	FOUT;
	}


static void
dsr_rte_error (const char* str1, const char* str2, const char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (dsr_rte_error <args>);
	
	op_sim_end ("DSR Routing Process : ", str1, str2, str3);
	
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
	void dsr_rte (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_dsr_rte_init (int * init_block_ptr);
	void _op_dsr_rte_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_dsr_rte_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_dsr_rte_alloc (VosT_Obtype, int);
	void _op_dsr_rte_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
dsr_rte (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (dsr_rte ());

		{
		/* Temporary Variables */
		Prohandle			invoker_prohandle;
		int					invoke_mode = -1;
		int					intrpt_type = -1;
		int					intrpt_code = -1;
		/* End of Temporary Variables */


		FSM_ENTER_NO_VARS ("dsr_rte")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "dsr_rte [init enter execs]")
				FSM_PROFILE_SECTION_IN ("dsr_rte [init enter execs]", state0_enter_exec)
				{
				/* Initializa the state variables	*/
				dsr_rte_sv_init ();
				
				/* Register the statistics	*/
				dsr_rte_stats_reg ();
				
				/* Initialize the log handles	*/
				dsr_notification_log_init ();
				
				/* Parse the attributes and create		*/
				/* and initialize the various buffers	*/
				dsr_rte_attributes_parse_buffers_create ();
				
				/* Register the DSR Routing process as a higher layer in IP	*/
				Ip_Higher_Layer_Protocol_Register ("dsr", &higher_layer_proto_id);
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "dsr_rte [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "wait", "tr_22", "dsr_rte [init -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "dsr_rte [wait enter execs]")
				FSM_PROFILE_SECTION_IN ("dsr_rte [wait enter execs]", state1_enter_exec)
				{
				/* Wait for a packet arrival	*/
				/* This will be an invoke from	*/
				/* another process, eg : CPU	*/
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"dsr_rte")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "dsr_rte [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("dsr_rte [wait exit execs]", state1_exit_exec)
				{
				invoker_prohandle = op_pro_invoker (own_prohandle, &invoke_mode);
				if (invoke_mode == OPC_PROINV_DIRECT)
					{
					/* The process was invoked directly by the kernel.	*/
					intrpt_type = op_intrpt_type ();
					intrpt_code = op_intrpt_code ();
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_TRANSIT_ONLY ((PACKET_ARRIVAL), 1, state1_enter_exec, dsr_rte_received_pkt_handle ();, wait, "PACKET_ARRIVAL", "dsr_rte_received_pkt_handle ()", "wait", "wait", "tr_24", "dsr_rte [wait -> wait : PACKET_ARRIVAL / dsr_rte_received_pkt_handle ()]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"dsr_rte")
		}
	}




void
_op_dsr_rte_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_dsr_rte_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		/* Print the route cache	*/
		char * temp_str = dsr_support_route_cache_print_to_string (route_cache_ptr);
		printf (temp_str);
		
		/* Print the maintenance buffer	*/
		dsr_maintenance_buffer_print (maint_buffer_ptr);
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_dsr_rte_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_dsr_rte_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_dsr_rte_svar function. */
#undef module_data_ptr
#undef own_mod_objid
#undef own_node_objid
#undef own_prohandle
#undef parent_prohandle
#undef proc_model_name
#undef broadcast_jitter_dist_ptr
#undef route_cache_ptr
#undef send_buffer_ptr
#undef maint_buffer_ptr
#undef higher_layer_proto_id
#undef maint_ack_req_timer
#undef ack_request_identifier
#undef own_pro_id
#undef parent_pro_id
#undef route_request_identifier
#undef grat_reply_table_ptr
#undef route_request_table_ptr
#undef pid_string
#undef stat_handle_ptr
#undef routes_export
#undef global_stathandle_ptr
#undef routes_dump
#undef packet_salvaging_function
#undef cached_route_replies_function
#undef non_propagating_request_function

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_dsr_rte_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_dsr_rte_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (dsr_rte)",
		sizeof (dsr_rte_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_dsr_rte_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	dsr_rte_state * ptr;
	FIN_MT (_op_dsr_rte_alloc (obtype))

	ptr = (dsr_rte_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "dsr_rte [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_dsr_rte_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	dsr_rte_state		*prs_ptr;

	FIN_MT (_op_dsr_rte_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (dsr_rte_state *)gen_ptr;

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
	if (strcmp ("proc_model_name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->proc_model_name);
		FOUT
		}
	if (strcmp ("broadcast_jitter_dist_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->broadcast_jitter_dist_ptr);
		FOUT
		}
	if (strcmp ("route_cache_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_cache_ptr);
		FOUT
		}
	if (strcmp ("send_buffer_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->send_buffer_ptr);
		FOUT
		}
	if (strcmp ("maint_buffer_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->maint_buffer_ptr);
		FOUT
		}
	if (strcmp ("higher_layer_proto_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->higher_layer_proto_id);
		FOUT
		}
	if (strcmp ("maint_ack_req_timer" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->maint_ack_req_timer);
		FOUT
		}
	if (strcmp ("ack_request_identifier" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ack_request_identifier);
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
	if (strcmp ("route_request_identifier" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_request_identifier);
		FOUT
		}
	if (strcmp ("grat_reply_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->grat_reply_table_ptr);
		FOUT
		}
	if (strcmp ("route_request_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_request_table_ptr);
		FOUT
		}
	if (strcmp ("pid_string" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pid_string);
		FOUT
		}
	if (strcmp ("stat_handle_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->stat_handle_ptr);
		FOUT
		}
	if (strcmp ("routes_export" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->routes_export);
		FOUT
		}
	if (strcmp ("global_stathandle_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_stathandle_ptr);
		FOUT
		}
	if (strcmp ("routes_dump" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->routes_dump);
		FOUT
		}
	if (strcmp ("packet_salvaging_function" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_salvaging_function);
		FOUT
		}
	if (strcmp ("cached_route_replies_function" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cached_route_replies_function);
		FOUT
		}
	if (strcmp ("non_propagating_request_function" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->non_propagating_request_function);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

