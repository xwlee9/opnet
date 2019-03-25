/* Process model C form file: manet_tora.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char manet_tora_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE3 5C91EEE3 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <manet_tora_imep.h>
#include <ip_rte_v4.h>


#define		OPT_TIMER		is_opt_timer
#define		DESTINATION		is_destination
#define		CLR_ARRIVAL		is_clr_arrival
#define		UNROUTABLE_IP_ARRIVAL	is_unroutable_ip_arrival
#define		QRY_ARRIVAL		is_qry_arrival
#define		LINK_UP			is_link_up
#define		OPT_ARRIVAL		is_opt_arrival
#define		UPD_ARRIVAL		is_upd_arrival
#define		DNLINK_AVAILABLE	num_dnstrm
#define		ROUTE_DISCOVERY		num_dnstrm
#define		LINK_REVERSAL 		num_dnstrm
#define		CLEARED_OUT		is_cleared_out

#define		ToraC_New_Packet		OPC_TRUE
#define		ToraC_Relay_Packet		OPC_FALSE
#define		ToraC_Height_Invalid	-1
#define		ToraC_Height_Destination		0

#define		TORA_TRACE			op_prg_odb_ltrace_active ("manet_tora")

#define		ToraC_Arrow_Width	4
#define		ToraC_Arrow_Offset	6
#define		ToraC_Circle_Radius	30

typedef enum
	{
	ToraC_Link_Status_Down,
	ToraC_Link_Status_Up,
	ToraC_Link_Status_Upstrm,
	ToraC_Link_Status_Downstrm,
	ToraC_Link_Status_Calculate,
	ToraC_Link_Status_Null
	}
ToraC_Link_Status;

const char*		ToraS_Link_Status [] = {"Disabled", "Enabled", "Upstream", "Downstream", "Calculate", "Nulled"};

typedef enum
	{
	ToraC_Neighbor_Level_Propagate,
	ToraC_Neighbor_Level_Reflect,
	ToraC_Neighbor_Level_Detect,
	ToraC_Neighbor_Level_Generate,
	ToraC_Neighbor_Level_Empty
	}
ToraC_Neighbor_Level;
	
typedef struct
	{
	Objid	nd_objid;
	Andid	arrow_did_array [3];
	Andid	arrow_did_array_double [3];
	}
ToraT_Anime_Info;

typedef	struct
	{
	double		tau;
	int			oid;
	Boolean		r;
	int			delta;
	int			id;
	ToraC_Link_Status		link_status;
	double		update_time;
	double		last_opt_time;
	ToraT_Anime_Info*	anim_info;
	}
ToraT_Height;

typedef enum
	{
	ToraC_Mode_On_Demand,
	ToraC_Mode_Proactive,
	ToraC_Mode_Default
	}
ToraC_Mode;

typedef enum
	{
	ToraC_Pk_Type_Opt,
	ToraC_Pk_Type_Upd,
	ToraC_Pk_Type_Qry,
	ToraC_Pk_Type_Clr
	}
ToraC_Pk_Type;

typedef enum
	{
	ToraC_Pk_To_IP,
	ToraC_Pk_To_IMEP
	}
ToraC_Pk_To;


/* Function declarations. */
static void		tora_packet_send (Packet*, ToraC_Pk_To);
static void		tora_generate_upd (Boolean, Packet*);
static void		tora_generate_qry (Boolean, Packet*);
static void		tora_generate_clr (Boolean, Packet*);
static void		tora_generate_opt (Boolean, Packet*);
static void		tora_queue_ip_pkt (Packet*);
static void		tora_ip_queue_flush (void);
static void		tora_queued_ip_send (void);
static void		tora_local_height_adjust (double, int, int, int);
static void		tora_extract_pk_info (ToraC_Pk_Type, Packet*, ToraT_Height*);
static void		tora_update_neighbor_list (ToraT_Height*, ToraC_Link_Status);
static ToraC_Link_Status 		tora_link_direction_get (ToraT_Height*);
static void		tora_generate_ctl_pk (ToraC_Pk_Type, Boolean, Packet*);
static ToraC_Neighbor_Level		tora_neighbor_list_level_check (ToraT_Height**);
static void		tora_trace_print (const char*);
static void		tora_downstrm_count (void);
static Compcode		tora_best_next_hop_find (int*);
static void		tora_link_reversal (ToraT_Height*);
static void		tora_draw_arrow_to_neighbor (Objid, ToraT_Height*);
static void		tora_neighbors_nullify (ToraT_Height*);
static Boolean	tora_is_same_ref_level (ToraT_Height*, ToraT_Height*);
static void		tora_route_inject (IpT_Address);
static Boolean	tora_is_neighbor_disabled (int);

/* Call-back Functions	*/
EXTERN_C_BEGIN
	
static void		tora_parent_invoke (void*, int);
static void		tora_draw_arrows_to_neighbors (void*, int);
static void		tora_draw_circle_on_destination (void*, int);
static int		tora_height_compare_proc (const void*, const void*);

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
	int	                    		router_id                                       ;	/* Router id of the local node  */
	ToraT_Invocation_Info*	 		parmem_ptr                                      ;	/* parent to child memory shared with the manager proc  */
	int	                    		mode_of_operation                               ;	/* proative or on-demand?  */
	int	                    		opt_interval                                    ;	/* OPT packet transmission interval  */
	List*	                  		unroutable_ip_pkt_lptr                          ;	/* Queue for unroutable IP packets waiting for route resolution  */
	Evhandle	               		ip_flush_timer_evhndl                           ;	/* Event handle for the timer to flush queued up unroutable IP packets  */
	Prohandle	              		self_phndl                                      ;	/* Local process handle  */
	List*	                  		neighbor_info_lptr                              ;	/* List of the neighbors with the same interest in the destination  */
	ToraT_Height	           		my_height                                       ;	/* My height relative to the destination  */
	int	                    		num_dnstrm                                      ;	/* Total number of downstream from this node to neighbors  */
	Objid	                  		my_node_objid                                   ;	/* My node objid  */
	IpT_Address	            		next_hop_addr                                   ;	/* IP address of the next hop neighbor for the destination  */
	int	                    		next_hop_rid                                    ;	/* Router ID of the next hop router  */
	Boolean	                		animation_enabled                               ;	/* Animation enable flag for the destination I am handling  */
	Andid	                  		circle_did_array [2]                            ;	/* Drawing circle in case I am the destination  */
	Stathandle*	            		route_discovery_delay_sthndl_ptr                ;	/* Stathandle to keep track of time spent in the query mode  */
	Stathandle*	            		ip_packet_dropped_local_sthndl_ptr              ;	/* stathandle to keep track of the number of packets due to route unavailability  */
	Stathandle*	            		ip_packet_dropped_global_sthndl_ptr             ;	/* stathandle to keep track of the number of packets due to route unavailability  */
	Boolean	                		query_mode                                      ;	/* flag indicating the route is discovered or not  */
	double	                 		time_in_query_mode                              ;	/* variable to store the total time spent in the query mode  */
	int	                    		dest_rid                                        ;	/* Router ID of the router we are after  */
	IpT_Address	            		my_intf_addr                                    ;	/* My interface IP address  */
	double	                 		last_upd_tx_time                                ;	/* last time I sent out UPD  */
	double	                 		last_linkup_time                                ;	/* The time at the last link came up  */
	Stathandle*	            		ip_queue_size_sthndl_ptr                        ;
	Boolean	                		infinite_loop_prevention_flag                   ;
	} manet_tora_state;

#define router_id               		op_sv_ptr->router_id
#define parmem_ptr              		op_sv_ptr->parmem_ptr
#define mode_of_operation       		op_sv_ptr->mode_of_operation
#define opt_interval            		op_sv_ptr->opt_interval
#define unroutable_ip_pkt_lptr  		op_sv_ptr->unroutable_ip_pkt_lptr
#define ip_flush_timer_evhndl   		op_sv_ptr->ip_flush_timer_evhndl
#define self_phndl              		op_sv_ptr->self_phndl
#define neighbor_info_lptr      		op_sv_ptr->neighbor_info_lptr
#define my_height               		op_sv_ptr->my_height
#define num_dnstrm              		op_sv_ptr->num_dnstrm
#define my_node_objid           		op_sv_ptr->my_node_objid
#define next_hop_addr           		op_sv_ptr->next_hop_addr
#define next_hop_rid            		op_sv_ptr->next_hop_rid
#define animation_enabled       		op_sv_ptr->animation_enabled
#define circle_did_array        		op_sv_ptr->circle_did_array
#define route_discovery_delay_sthndl_ptr		op_sv_ptr->route_discovery_delay_sthndl_ptr
#define ip_packet_dropped_local_sthndl_ptr		op_sv_ptr->ip_packet_dropped_local_sthndl_ptr
#define ip_packet_dropped_global_sthndl_ptr		op_sv_ptr->ip_packet_dropped_global_sthndl_ptr
#define query_mode              		op_sv_ptr->query_mode
#define time_in_query_mode      		op_sv_ptr->time_in_query_mode
#define dest_rid                		op_sv_ptr->dest_rid
#define my_intf_addr            		op_sv_ptr->my_intf_addr
#define last_upd_tx_time        		op_sv_ptr->last_upd_tx_time
#define last_linkup_time        		op_sv_ptr->last_linkup_time
#define ip_queue_size_sthndl_ptr		op_sv_ptr->ip_queue_size_sthndl_ptr
#define infinite_loop_prevention_flag		op_sv_ptr->infinite_loop_prevention_flag

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	manet_tora_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((manet_tora_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
tora_packet_send (Packet* pk_ptr, ToraC_Pk_To where)
	{
	int				code;
	
	/** PURPOSE: Send the packet through the manager proc.		**/
	/** REQUIRES: packet pointer.								**/
	/** EFFECTS: Invocation is scheduled. 						**/
	FIN (tora_packet_send (pk_ptr, where));
	
	if (where == ToraC_Pk_To_IP)
		{
		code = ToraImepMgrC_Invocation_Code_Pk_From_Tora_For_IP;
		}
	else
		{
		code = ToraImepMgrC_Invocation_Code_Pk_From_Tora_For_IMEP;
		}
		
	/* Schedule an interrupt to avoid circular invocation. */
	op_intrpt_schedule_call (op_sim_time (), code, tora_parent_invoke, pk_ptr);
	
	FOUT;
	}


static void
tora_parent_invoke (void* pkptr, int code)
	{
	ToraImepMgrT_Invocation_Struct*		invocation_info;
	Packet* 							pk_ptr;
	
	/* Helper function to avoid circular invocation. */
	FIN (tora_parent_invoke (pk_ptr, code));
	
	pk_ptr = (Packet*) pkptr;
	
	/* Fill in the invocation structure. */
	invocation_info = (ToraImepMgrT_Invocation_Struct*) op_prg_mem_alloc 
		(sizeof (ToraImepMgrT_Invocation_Struct));
	invocation_info->code = (ToraImepMgrC_Invocation_Code) code;
	invocation_info->pk_ptr = pk_ptr;
	
	/* Invoke the manager process. */
	op_pro_invoke (parmem_ptr->parent_phndl, invocation_info);
	
	FOUT;
	}
	
static void
tora_generate_ctl_pk (ToraC_Pk_Type pk_type, Boolean create_new, Packet* orig_pk_ptr)
	{
	/** PURPOSE: Generate a new control packet to send out.		**/
	/** REQUIRES: None.											**/
	/** EFFECTS: A new control packet is created and returned. 	**/
	FIN (tora_generate_ctl_pk (pk_type, create_new, orig_pk_ptr));
	
	if ((orig_pk_ptr == OPC_NIL) && (!create_new))
		{
		/* User error! */
		op_sim_end ("No packet to copy.", OPC_NIL, OPC_NIL, OPC_NIL);
		}
		
	switch (pk_type)
		{
		case ToraC_Pk_Type_Upd:
			{
			tora_generate_upd (create_new, orig_pk_ptr);
			break;
			}
			
		case ToraC_Pk_Type_Opt:
			{
			tora_generate_opt (create_new, orig_pk_ptr);
			break;
			}
			
		case ToraC_Pk_Type_Qry:
			{
			tora_generate_qry (create_new, orig_pk_ptr);
			break;
			}
			
		case ToraC_Pk_Type_Clr:
			{
			tora_generate_clr (create_new, orig_pk_ptr);
			break;
			}
		}
	
	FOUT;
	}

	
static void
tora_generate_upd (Boolean create_new, Packet* orig_pk_ptr)
	{
	Packet*				pk_ptr;
	
	/** PURPOSE: Generate a new Upd packet to send out.		**/
	/** REQUIRES: None.										**/
	/** EFFECTS: A new UPD packet is created and returned. 	**/
	FIN (tora_generate_upd (create_new, orig_pk_ptr));
	
	if (create_new)
		{
		/* Create the UPD packet. */
		pk_ptr = op_pk_create_fmt ("tora_upd_opt");

		/* Set the fields. */
		op_pk_nfd_set (pk_ptr, "Type", ToraC_Pk_Type_Upd);
		op_pk_nfd_set (pk_ptr, "Destination", parmem_ptr->address);
		op_pk_nfd_set (pk_ptr, "Destination_mask", parmem_ptr->net_mask);
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		op_pk_nfd_set (pk_ptr, "H_oid", my_height.oid);
		op_pk_nfd_set (pk_ptr, "H_tau", my_height.tau);
		op_pk_nfd_set (pk_ptr, "H_delta", my_height.delta);
		op_pk_nfd_set (pk_ptr, "H_r", my_height.r);
		}
	else
		{
		/* Start by copying the received packet. */
		pk_ptr = op_pk_copy (orig_pk_ptr);
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		op_pk_nfd_set (pk_ptr, "H_delta", my_height.delta);
		op_pk_nfd_set (pk_ptr, "H_r", my_height.r);
		}
		
	/* Send the packet out. */
	tora_packet_send (pk_ptr, ToraC_Pk_To_IMEP);
	
	/* Record the time to reduce redundancy. */
	last_upd_tx_time = op_sim_time ();

	FOUT;
	}


static void	
tora_generate_qry (Boolean create_new, Packet* orig_pk_ptr)
	{
	Packet*					pk_ptr;
	
	/** PURPOSE: Generate a new Qry packet to send out.		**/
	/** REQUIRES: None.										**/
	/** EFFECTS: A new QRY packet is created and returned. 	**/
	FIN (tora_generate_qry (create_new, orig_pk_ptr));
	
	if (create_new)
		{
		/* Create the QRY packet. */
		pk_ptr = op_pk_create_fmt ("tora_qry");

		/* Set the fields. */
		op_pk_nfd_set (pk_ptr, "Type", ToraC_Pk_Type_Qry);
		op_pk_nfd_set (pk_ptr, "Destination", parmem_ptr->address);
		}
	else
		{
		/* Simply copy the packet and send it out. */
		pk_ptr = op_pk_copy (orig_pk_ptr);
		}

	/* Send the packet out. */
	tora_packet_send (pk_ptr, ToraC_Pk_To_IMEP);

	FOUT;
	}


static void	
tora_generate_clr (Boolean create_new, Packet* orig_pk_ptr)
	{
	Packet*				pk_ptr;
	
	/** PURPOSE: Generate a new CLR packet to send out.		**/
	/** REQUIRES: None.										**/
	/** EFFECTS: A new CLR packet is created and returned. 	**/
	FIN (tora_generate_clr (create_new, orig_pk_ptr));
	
	if (create_new)
		{
		/* Create the CLR packet. */
		pk_ptr = op_pk_create_fmt ("tora_clr");

		/* Set the fields. */
		op_pk_nfd_set (pk_ptr, "Type", ToraC_Pk_Type_Clr);
		op_pk_nfd_set (pk_ptr, "Destination", parmem_ptr->address);
		op_pk_nfd_set (pk_ptr, "Destination_mask", parmem_ptr->net_mask);
		op_pk_nfd_set (pk_ptr, "H_tau", my_height.tau);
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		op_pk_nfd_set (pk_ptr, "H_oid", my_height.oid);
		}
	else
		{
		/* Copy the received packet. */
		pk_ptr = op_pk_copy (orig_pk_ptr);
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		}

	/* Send the packet out. */
	tora_packet_send (pk_ptr, ToraC_Pk_To_IMEP);

	FOUT;
	}


static void	
tora_generate_opt (Boolean create_new, Packet* orig_pk_ptr)
	{
	Packet*				pk_ptr;
	
	/** PURPOSE: Generate a new OPT packet to send out.		**/
	/** REQUIRES: None.										**/
	/** EFFECTS: A new OPT packet is created and returned. 	**/
	FIN (tora_generate_opt (create_new, orig_pk_ptr));
	
	if (create_new)
		{
		/* Create the OPT packet. */
		pk_ptr = op_pk_create_fmt ("tora_upd_opt");

		/* Set the fields. */
		op_pk_nfd_set (pk_ptr, "Type", ToraC_Pk_Type_Opt);
		op_pk_nfd_set (pk_ptr, "Destination", parmem_ptr->address);
		op_pk_nfd_set (pk_ptr, "Destination_mask", parmem_ptr->net_mask);
		op_pk_nfd_set (pk_ptr, "H_tau", op_sim_time ());
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		op_pk_nfd_set (pk_ptr, "H_oid", router_id);
		op_pk_nfd_set (pk_ptr, "Optimization_period", op_sim_time ());
		}
	else
		{
		/* Copy the received packet. */
		pk_ptr = op_pk_copy (orig_pk_ptr);
		op_pk_nfd_set (pk_ptr, "H_id", router_id);
		op_pk_nfd_set (pk_ptr, "H_delta", my_height.delta);
		}

	/* Send the packet out. */
	tora_packet_send (pk_ptr, ToraC_Pk_To_IMEP);

	FOUT;
	}


static void
tora_queue_ip_pkt (Packet* pk_ptr)
	{
	/** PURPOSE: Queue arrived unroutable IP packet for a while.		**/
	/** REQUIRES: Pakcet pointer.										**/
	/** EFFECTS: List is filled with the packet and the timer is set. 	**/
	FIN (tora_queue_ip_pkt (pk_ptr));
	
	/* Add the packet to the list. */
	op_prg_list_insert (unroutable_ip_pkt_lptr, pk_ptr, OPC_LISTPOS_TAIL);
	
	/* Setup timer if not there yet. */
	if (!op_ev_valid (ip_flush_timer_evhndl))
		{
		ip_flush_timer_evhndl = op_intrpt_schedule_self (op_sim_time () + (double) parmem_ptr->ip_flush_time, 0);
		}
	
	/* Record statistic. */
	if (!ip_queue_size_sthndl_ptr)
		{
		/* Register the stat for the first time. */
		ip_queue_size_sthndl_ptr = (Stathandle*) op_prg_mem_alloc (sizeof (Stathandle));
		*ip_queue_size_sthndl_ptr = op_stat_reg ("TORA_IMEP.Unroutable IP Packet Queue Size", dest_rid, OPC_STAT_LOCAL);
		}
	
	op_stat_write (*ip_queue_size_sthndl_ptr, op_prg_list_size (unroutable_ip_pkt_lptr));
	
	FOUT;
	}
		
static void
tora_ip_queue_flush (void)
	{
	int					queue_size;
	
	/** PURPOSE: Destroy queued IP packets.						**/
	/** REQUIRES: List pointer.									**/
	/** EFFECTS: List is emptied and packets are destroyed. 	**/
	FIN (tora_ip_queue_flush ());
	
	/* Destroy all the packets in the queue. */
	queue_size = op_prg_list_size (unroutable_ip_pkt_lptr);
	while (op_prg_list_size (unroutable_ip_pkt_lptr))
		{
		op_pk_destroy ((Packet*) op_prg_list_remove ( unroutable_ip_pkt_lptr, OPC_LISTPOS_HEAD));
		}
	
	/* Cancel the event if valid	*/
	if (op_ev_valid (ip_flush_timer_evhndl) && op_ev_pending (ip_flush_timer_evhndl))
		op_ev_cancel (ip_flush_timer_evhndl);
	
	/* Record statistic. */
	if (queue_size)
		{
		if (!ip_packet_dropped_local_sthndl_ptr)
			{
			/* Register the stats for the first time. */
			ip_packet_dropped_local_sthndl_ptr = (Stathandle*) op_prg_mem_alloc (sizeof (Stathandle));
			ip_packet_dropped_global_sthndl_ptr = (Stathandle*) op_prg_mem_alloc (sizeof (Stathandle));
				
			*ip_packet_dropped_local_sthndl_ptr = op_stat_reg ("TORA_IMEP.Dropped Unroutable IP Packet (pkts/sec)", dest_rid, OPC_STAT_LOCAL);
			*ip_packet_dropped_global_sthndl_ptr = op_stat_reg ("TORA_IMEP.Dropped Unroutable IP Packet (pkts/sec)", dest_rid, OPC_STAT_GLOBAL);
			}
		
		op_stat_write (*ip_packet_dropped_local_sthndl_ptr, queue_size);
		op_stat_write (*ip_packet_dropped_global_sthndl_ptr, queue_size);
		
		op_stat_write (*ip_packet_dropped_local_sthndl_ptr, 0.0);
		op_stat_write (*ip_packet_dropped_global_sthndl_ptr, 0.0);
		}
	
	FOUT;
	}

static void
tora_queued_ip_send (void)
	{
	/** PURPOSE: Send queued IP packets.						**/
	/** REQUIRES: List pointer.									**/
	/** EFFECTS: List is emptied and packets are destroyed. 	**/
	
	FIN (tora_queued_ip_send (void));
	
	/* See if we have anything to send first. */
	if (op_ev_valid (ip_flush_timer_evhndl) && op_ev_pending (ip_flush_timer_evhndl))
		{
		while (op_prg_list_size (unroutable_ip_pkt_lptr))
			{
			tora_packet_send ((Packet*) op_prg_list_remove (unroutable_ip_pkt_lptr, OPC_LISTPOS_HEAD), 
				ToraC_Pk_To_IP);
			}
		
		op_ev_cancel (ip_flush_timer_evhndl);
		}
	
	FOUT;
	}


static void
tora_local_height_adjust (double tau, int oid, int r, int delta)
	{
	/** PURPOSE: Adjust the local height.			**/
	/** REQUIRES: Tau, Oid, r, Delta values.		**/
	/** EFFECTS: Local height is updated. 			**/
	
	FIN (tora_local_height_adjust (tau, oid, r, delta));
	
	/* Reset the values according to the passed arguments. */
	my_height.tau = tau;
	my_height.oid = oid;
	my_height.r = r;
	my_height.delta = delta;

	FOUT;
	}


static void				
tora_extract_pk_info (ToraC_Pk_Type pk_type, Packet* pk_ptr, ToraT_Height*	tmp_height)
	{
	/** PURPOSE: Extract height information from the packet received.		**/
	/** REQUIRES: Packet type, packet and storage.							**/
	/** EFFECTS: Temop height is updated with the received data. 			**/
	
	FIN (tora_extract_pk_info (pk_type, pk_ptr, tmp_height));
	
	/* Extract common info first. */
	op_pk_nfd_get (pk_ptr, "H_tau", &tmp_height->tau);
	op_pk_nfd_get (pk_ptr, "H_oid", &tmp_height->oid);
	op_pk_nfd_get (pk_ptr, "H_id", &tmp_height->id);

	if ((pk_type == ToraC_Pk_Type_Opt) || (pk_type == ToraC_Pk_Type_Upd))
		{
		/* Get more. */
		op_pk_nfd_get (pk_ptr, "H_r", &tmp_height->r);
		op_pk_nfd_get (pk_ptr, "H_delta", &tmp_height->delta);
		
		if (pk_type == ToraC_Pk_Type_Opt)
			{
			op_pk_nfd_get (pk_ptr, "Optimization_period", &tmp_height->last_opt_time);
			}
		}
	
	FOUT;
	}

static void
tora_update_neighbor_list (ToraT_Height* neighbor_height, ToraC_Link_Status link_status)
	{
	ToraT_Height 				*target_neighbor, *new_neighbor;
	int							low, high;
	
	/** PURPOSE: Update the neighbor list with the received height info.**/
	/** REQUIRES: Local neighbor height list and the new height.		**/
	/** EFFECTS: A neighbor is inserted if new and updated if old. 		**/
	FIN (tora_update_neighbor_list (neighbor_height, link_status));
	
	/* Try finding existing entry first. */
	if ((target_neighbor = (ToraT_Height*) op_prg_list_elem_find (neighbor_info_lptr, 
		tora_height_compare_proc, neighbor_height, &low, &high)))
		{
		/* Special treatment if the link status only changed. */
		if (link_status == ToraC_Link_Status_Down)
			{
			/* Just change the link status and leave the height info alone. */
			target_neighbor->link_status = link_status;
			target_neighbor->update_time = neighbor_height->update_time;
			
			FOUT;
			}
		
		if (link_status == ToraC_Link_Status_Up)
			{
			/* Check for dulplicate update from IMEP and change the link status and leave the height info alone. */
			if ((target_neighbor->link_status == ToraC_Link_Status_Down) ||
				(target_neighbor->link_status == ToraC_Link_Status_Null))
				{
				target_neighbor->link_status = link_status;
				}
			
			target_neighbor->update_time = neighbor_height->update_time;
			
			FOUT;
			}

		if (target_neighbor->link_status == ToraC_Link_Status_Null)
			{
			/* Ignore if this update is the same ref level. */
			if (tora_is_same_ref_level (target_neighbor, neighbor_height))
				{
				FOUT;
				}
			}
		
		/* Update the height. */
		if (neighbor_height->id == dest_rid)
			{
			target_neighbor->tau =  ToraC_Height_Destination;
			}
		else
			{
			target_neighbor->tau = neighbor_height->tau;
			}
		
		target_neighbor->oid = neighbor_height->oid;
		target_neighbor->r = neighbor_height->r;
		target_neighbor->delta = neighbor_height->delta;
		target_neighbor->update_time = op_sim_time ();
		
		if (link_status == ToraC_Link_Status_Calculate)
			{
			/* Decide the link direction. */
			target_neighbor->link_status = tora_link_direction_get (target_neighbor);
			}
		else
			{
			target_neighbor->link_status = link_status;
			}
		
		if (animation_enabled)
			{
			/* Update the drawing as well. */
			tora_draw_arrow_to_neighbor (my_node_objid, target_neighbor);
			}
		}
	else
		{
		/* First time neighbor.  Insert it in the list. */
		new_neighbor = (ToraT_Height*) op_prg_mem_alloc (sizeof (ToraT_Height));
		op_prg_mem_copy (neighbor_height, new_neighbor, sizeof (ToraT_Height));
		new_neighbor->update_time = op_sim_time ();
			
		/* Special treatment if the link status only changed. */
		if ((link_status == ToraC_Link_Status_Up) ||
			(link_status == ToraC_Link_Status_Down))
			{
			/* Give it a default values. */
			new_neighbor->tau = ToraC_Height_Invalid;
			new_neighbor->oid = -1;
			new_neighbor->r = 0;
			new_neighbor->delta = 0;
			}
			
		if (neighbor_height->id == dest_rid)
			{
			new_neighbor->tau = ToraC_Height_Destination;
			}
		
		if (link_status == ToraC_Link_Status_Calculate)
			{
			/* Decide the link direction. */
			new_neighbor->link_status = tora_link_direction_get (new_neighbor);
			}
		else
			{
			new_neighbor->link_status = link_status;
			}
		
		if (animation_enabled)
			{
			/* We need to store some info on this guy. */
			new_neighbor->anim_info = (ToraT_Anime_Info*) 
				op_prg_mem_alloc (sizeof (ToraT_Anime_Info));
			new_neighbor->anim_info->nd_objid = tora_imep_sup_nd_objid_from_rid
				(new_neighbor->id);
			*new_neighbor->anim_info->arrow_did_array = OPC_ANIM_ID_NIL;
			*new_neighbor->anim_info->arrow_did_array_double = OPC_ANIM_ID_NIL;
			
			/* Draw something for the first time. */
			tora_draw_arrow_to_neighbor (my_node_objid, new_neighbor);
			}
		else
			{
			new_neighbor->anim_info = OPC_NIL;
			}
		
		op_prg_list_insert_sorted (neighbor_info_lptr, new_neighbor, tora_height_compare_proc);
		}
	
	FOUT;
	}
	
static int
tora_height_compare_proc (const void* height_a, const void* height_b)
	{
	ToraT_Height* 			height_1;
	ToraT_Height* 			height_2;
	
	/* Helper function to sort height information in the list.  */
	/* Sort by the unique router id. 							*/
	FIN (tora_height_compare_proc (height_1, height_2));
	
	height_1 = (ToraT_Height*) height_a;
	height_2 = (ToraT_Height*) height_b;

	if (height_1->id == height_2->id)
		FRET (0);
	
	if (height_1->id < height_2->id)
		FRET (1);
	
	/* (height_1->id > height_2->id) */
		FRET (-1);
	}

static ToraC_Link_Status
tora_link_direction_get (ToraT_Height* target_neighbor)
	{
	/** PURPOSE: Compare the neighbor height with mine and decide the link direction.	**/
	/** REQUIRES: A neighbor height pointer and mine.									**/
	/** EFFECTS: Link direction is returned and the total number of dnlink is updated. 	**/
	FIN (tora_link_direction_get (target_neighbor));
	
	/* If directly connected to the destination, always downstream. */
	if (target_neighbor->id == dest_rid)
		FRET (ToraC_Link_Status_Downstrm);
	
	if ((target_neighbor->oid == my_height.oid) && (target_neighbor->tau == my_height.tau))
		{
		if (target_neighbor->r == my_height.r)
			{
			/* Delta is the deciding factor in this case. */
			if (target_neighbor->delta < my_height.delta)
				{
				FRET (ToraC_Link_Status_Downstrm);
				}
			else if (target_neighbor->delta == my_height.delta)
				{
				/* In this case router id should be considered. */
				if (target_neighbor->id < router_id)
					{
					FRET (ToraC_Link_Status_Upstrm);
					}
				else
					{
					FRET (ToraC_Link_Status_Downstrm);
					}
				}
			else
				{
				FRET (ToraC_Link_Status_Upstrm);
				}
			}
		else if (target_neighbor->r > my_height.r)
			{
			FRET (ToraC_Link_Status_Upstrm);
			}
		else if (target_neighbor->r < my_height.r)
			{
			FRET (ToraC_Link_Status_Downstrm);
			}
		}
	else
		{
		/* This is a different reference level. Let the time decide. */
		if (target_neighbor->tau >= my_height.tau)
			{
			FRET (ToraC_Link_Status_Upstrm);
			}
		else
			{
			FRET (ToraC_Link_Status_Downstrm);
			}
		}
	
	op_sim_end ("Unhandled link comparison case.", OPC_NIL, OPC_NIL, OPC_NIL);
	
	/* Call FRET with a value just to prevent compilation warnings.	*/
	FRET (ToraC_Link_Status_Null);
	}
	
   
static ToraC_Neighbor_Level
tora_neighbor_list_level_check (ToraT_Height** max_ref_min_height_neighbor_pptr)
	{
	int		list_size, index;
	ToraT_Height *tmp_neighbor, *max_ref_min_height_neighbor = OPC_NIL;
	Boolean	single_neighbor_level=OPC_TRUE, single_r_value = OPC_TRUE;
	
	/** PURPOSE: Scan the neighbor list and see what we need to do. **/
	/** REQUIRES: none.												**/
	/** EFFECTS: neighbor level and max height neighbor if found. 	**/
	FIN (tora_neighbor_list_level_check (max_ref_min_height_neighbor));
	
	/* Scan through the neighbor list. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);

		/* Skip the disabled/only enabled or nulled links. */
		if ((tmp_neighbor->link_status == ToraC_Link_Status_Down) || 
			(tmp_neighbor->link_status == ToraC_Link_Status_Up) ||
			(tmp_neighbor->link_status == ToraC_Link_Status_Null))
			{
			continue;
			}

		if (!max_ref_min_height_neighbor)
			{
			max_ref_min_height_neighbor = tmp_neighbor;
			continue;
			}

		/* Compare the current neighbor with the current max. */
		if ((tmp_neighbor->oid == max_ref_min_height_neighbor->oid) &&
			(tmp_neighbor->tau == max_ref_min_height_neighbor->tau))
			{
			/* They have the same reference level.  Check the sublevel. */
			if (tmp_neighbor->r == max_ref_min_height_neighbor->r)
				{
				/* Delta is the deciding factor. */
				if (tmp_neighbor->delta < max_ref_min_height_neighbor->delta)
					{
					/* We found the lower height in the ref level. */
					max_ref_min_height_neighbor = tmp_neighbor;
					}
				else
					{
					/* Same reference level and higher or the same height. */
					}
				}
			else 
				{
				/* Different sublevel. */
				if (tmp_neighbor->r > max_ref_min_height_neighbor->r)
					{
					/* Higher due to sublevel. */
					max_ref_min_height_neighbor = tmp_neighbor;
					}
					
				single_r_value = OPC_FALSE;
				}
			}
		else
			{
			/* Different reference level.  Let the tau value decide the max level. */
			single_neighbor_level = OPC_FALSE;
			
			if (tmp_neighbor->tau > max_ref_min_height_neighbor->tau)
				{
				max_ref_min_height_neighbor = tmp_neighbor;
				}
			}
		}
		
	if (!max_ref_min_height_neighbor)
		{
		/* No valid neighbor yet. */
		FRET (ToraC_Neighbor_Level_Empty);
		}
		
	/* We have the max ref level. */			
	*max_ref_min_height_neighbor_pptr = max_ref_min_height_neighbor;

	if (single_neighbor_level == OPC_TRUE) 
		{
		if (single_r_value == OPC_TRUE)
			{
			if (max_ref_min_height_neighbor->r == 0)
				{
				/* Reflect condition. */
				FRET (ToraC_Neighbor_Level_Reflect);
				}
			else
				{
				/* This is a possible detection. */
				if (max_ref_min_height_neighbor->oid == router_id)
					{
					FRET (ToraC_Neighbor_Level_Detect);
					}
				else
					{
					/* Generate condition. */
					FRET (ToraC_Neighbor_Level_Generate);
					}
				}
			}
		else
			{
			/* Propagate condition. */
			FRET (ToraC_Neighbor_Level_Propagate);
			}
		}
	else
		{
		/* Another Propagate condition. */
		FRET (ToraC_Neighbor_Level_Propagate);
		}
	}


static void
tora_trace_print (const char* msg_1)
	{
	char					msg_2 [64];
	
	FIN (tora_trace_print (msg_1));
	
	/* Helper function to print out trace statements. */
	sprintf (msg_2, "For RID:%d", dest_rid);
	op_prg_odb_print_major ("MANET TORA:", msg_1, msg_2, OPC_NIL);
	
	FOUT;
	}


static void
tora_downstrm_count (void)
	{
	int							list_size, index;
	ToraT_Height 				*tmp_neighbor;
	
	/** PURPOSE: Count the number of downstreams available.	**/
	/** REQUIRES: Neighbor list.							**/
	/** EFFECTS: State variable will be updated. 			**/
	FIN (tora_downstrm_count ());
	
	/* Update the total number of downstreams at this moment. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (num_dnstrm=0, index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
		
		if (tmp_neighbor->link_status == ToraC_Link_Status_Downstrm)
			{
			num_dnstrm++;
			}
		}
	
	FOUT;
	}

static Compcode
tora_best_next_hop_find (int*	best_neighbor_rid)
	{
	int							list_size, index;
	ToraT_Height 				*tmp_neighbor, *best_so_far = OPC_NIL;
	
	/** PURPOSE: Scan the neighbor list and find the best next hop. **/
	/** REQUIRES: Local neighbor height list.						**/
	/** EFFECTS: RID of the best next hop router. 					**/
	FIN (tora_best_next_hop_find (void));
	
	/* Scan through the neighbor list. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
		
		if (tmp_neighbor->link_status == ToraC_Link_Status_Downstrm)
			{
			/* See if we have direct connection to destination. */
			if (tmp_neighbor->id == dest_rid)
				{
				*best_neighbor_rid = dest_rid;
				FRET (OPC_COMPCODE_SUCCESS);
				}
			
			/* If you only have one entry... */
			if (!best_so_far)
				{
				/* Now we have something to comapare against. */
				best_so_far = tmp_neighbor;
				continue;
				}
			else
				{
				/* We will only look for the shortest hop  ==  smallest delta. */
				if (best_so_far->delta > tmp_neighbor->delta)
					{
					/* Found the new best. */
					best_so_far = tmp_neighbor;
					}
				}
			}
		}
			
	if (best_so_far)
		{
		/* Found. */
		*best_neighbor_rid = best_so_far->id;
		
		FRET (OPC_COMPCODE_SUCCESS);
		}
	
	*best_neighbor_rid = -1;
	FRET (OPC_COMPCODE_FAILURE);
	}
		

static void
tora_link_reversal (ToraT_Height* ref_height)
	{
	int						list_size, index;
	ToraT_Height 			*tmp_neighbor;
	double					time;
	
	/** PURPOSE: We are updating the direction of the all the local links.	**/
	/** REQUIRES: Neighbor list.											**/
	/** EFFECTS: Link direction will be updated. 							**/
	FIN (tora_link_reversal (ref_height));
	
	/* Current time. */
	time = op_sim_time ();
	
	/* Update the total number of downstreams at this moment. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (num_dnstrm=0, index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
		
		if (ref_height)
			{
			/* Update the tau and oid and stuff. */
			tmp_neighbor->tau = ref_height->tau;
			tmp_neighbor->oid = ref_height->oid;
			tmp_neighbor->r = ref_height->r;
			tmp_neighbor->delta = ref_height->delta;
			}
		
		if ((tmp_neighbor->link_status == ToraC_Link_Status_Upstrm) ||
			(tmp_neighbor->link_status == ToraC_Link_Status_Downstrm))
			{
			/* Recalculate the direction. */
			tmp_neighbor->link_status = tora_link_direction_get (tmp_neighbor);
			}
		}
	
	/* Also update the next hop. */
	tora_best_next_hop_find (&next_hop_rid);
	
	FOUT;
	}


static void
tora_neighbors_nullify (ToraT_Height* reference_height)
	{
	int							list_size, index;
	ToraT_Height 				*tmp_neighbor;
	
	/** PURPOSE: We are updating the direction of the all the local links to be null.**/
	/** REQUIRES: Neighbor list.													 **/
	/** EFFECTS: Link direction will be updated. 									 **/
	FIN (tora_neighbors_nullify (reference_height));
	
	/* Update the total number of downstreams at this moment. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (num_dnstrm=0, index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
		
		if (tmp_neighbor->link_status != ToraC_Link_Status_Down)
			{
			if (reference_height)
				{					
				/* Nullify the direction of the neighbor with the same reference level. */
				if ((tmp_neighbor->tau == reference_height->tau) &&
					(tmp_neighbor->oid == reference_height->oid))
					{
					tmp_neighbor->link_status = ToraC_Link_Status_Null;
					}
				}
			else
				{
				tmp_neighbor->link_status = ToraC_Link_Status_Null;
				}
			}
		}
	
	FOUT;
	}


static void
tora_route_inject (IpT_Address next_hop_address)
	{
	IpT_Cmn_Rte_Table_Entry*			rte_table_entry;
	IpT_Port_Info						port_info;
	IpT_Next_Hop_Entry*					next_hop_entry;
	
	/** PURPOSE: Inject a found route.						**/
	/** REQUIRES: Handle to the common route table.			**/
	/** EFFECTS: A new route entry is created or updated. 	**/
	FIN (tora_route_inject (next_hop_address));

	/* If we are in gateway config, insert the LAN net address into the table. */
	if (parmem_ptr->lan_addr != IPC_ADDR_INVALID)
		{
		/* Check if we already have the entry. */
		if (ip_cmn_rte_table_entry_exists (parmem_ptr->ip_rte_table, parmem_ptr->lan_addr, 
			parmem_ptr->lan_addr_range_ptr->subnet_mask, &rte_table_entry) 
			== OPC_COMPCODE_FAILURE)
			{
			/* Insert a new entry into the route table. */
			ip_rte_addr_local_network (parmem_ptr->address, parmem_ptr->ip_rte_table->iprmd_ptr, OPC_NIL, 
				&port_info, OPC_NIL);
				
			Ip_Cmn_Rte_Table_Entry_Add (parmem_ptr->ip_rte_table, (void*) OPC_NIL, parmem_ptr->lan_addr,
				parmem_ptr->lan_addr_range_ptr->subnet_mask, next_hop_address, port_info, 1,
				IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_TORA ,IPC_NO_MULTIPLE_PROC), 1);
			}
		else
			{
			/* We have existing entry.  Update the next hop info. */
			next_hop_entry = (IpT_Next_Hop_Entry*) op_prg_list_access 
				(rte_table_entry->next_hop_list, OPC_LISTPOS_HEAD);
			next_hop_entry->next_hop = inet_address_from_ipv4_address_create (next_hop_address);
			}
		}
	else
		{
		/* Check if we already have the entry. */
		if (ip_cmn_rte_table_entry_exists (parmem_ptr->ip_rte_table, parmem_ptr->address, 
			IpI_Broadcast_Addr, &rte_table_entry) == OPC_COMPCODE_FAILURE)
			{
			/* Insert a new entry into the route table. */
			ip_rte_addr_local_network (next_hop_address, parmem_ptr->ip_rte_table->iprmd_ptr, OPC_NIL, 
				&port_info, OPC_NIL);
				
			Ip_Cmn_Rte_Table_Entry_Add (parmem_ptr->ip_rte_table, (void*) OPC_NIL, parmem_ptr->address,
				IpI_Broadcast_Addr, next_hop_address, port_info, 1,
				IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_TORA ,IPC_NO_MULTIPLE_PROC), 1);
			}
		else
			{
			/* We have existing entry.  Update the next hop info. */
			next_hop_entry = (IpT_Next_Hop_Entry*) op_prg_list_access 
				(rte_table_entry->next_hop_list, OPC_LISTPOS_HEAD);
			
			next_hop_entry->next_hop = inet_address_from_ipv4_address_create (next_hop_address);
			}
		}
	
	FOUT;
	}


static void
tora_draw_arrow_to_neighbor (Objid my_nd_objid, ToraT_Height *neighbor)
	{
	int					index, props, offset;
	Objid				src_objid, dest_objid;
	double				lat, lon, alt, x, y, z, src_nx, src_ny, dest_nx, dest_ny;
	int					src_vx, src_vy, dest_vx, dest_vy;
	char				src_name [128], dest_name [128];
	Boolean				src_subnet = OPC_FALSE, dest_subnet = OPC_FALSE;

	/** PURPOSE: We will draw a new arrow between the two nodes.	**/
	/** REQUIRES: Objid, direction and etc.							**/
	/** EFFECTS: A arrow with certain direction will be drawn. 		**/
	FIN (tora_draw_arrow_to_neighbor (my_nd_objid, neighbor));
	
	/* Erase the existing drawing. */
	if (*neighbor->anim_info->arrow_did_array != OPC_ANIM_ID_NIL)
		{
		for (index=0; index < 3; index++)
			{
			op_anim_igp_drawing_erase (Tora_ImepI_Anvid, neighbor->anim_info->arrow_did_array [index],
				OPC_ANIM_ERASE_MODE_XOR);
			}
		/* Reset the drawing. */
		*neighbor->anim_info->arrow_did_array = OPC_ANIM_ID_NIL;
		}
		
	/* Erase the existing drawing. */
	if (*neighbor->anim_info->arrow_did_array_double != OPC_ANIM_ID_NIL)
		{
		for (index=0; index < 3; index++)
			{
			op_anim_igp_drawing_erase (Tora_ImepI_Anvid, neighbor->anim_info->arrow_did_array_double [index],
				OPC_ANIM_ERASE_MODE_XOR);
			}
		/* Reset the drawing. */
		*neighbor->anim_info->arrow_did_array_double = OPC_ANIM_ID_NIL;
		}

	/* Determine the direction of the arrow. */
	if ((neighbor->link_status == ToraC_Link_Status_Down) || 
		(neighbor->link_status == ToraC_Link_Status_Up) ||
		(neighbor->link_status == ToraC_Link_Status_Null))
		FOUT;
	
	if (neighbor->link_status == ToraC_Link_Status_Downstrm)
		{
		src_objid = my_nd_objid;
		dest_objid = neighbor->anim_info->nd_objid;
		}
	else
		{
		src_objid = neighbor->anim_info->nd_objid;
		dest_objid = my_nd_objid;
		}

	/* We will move up the ladder to find the subnet being animated now. */
	while (op_topo_parent (src_objid) != Imep_Subnet_Objid)
		{
		src_subnet = OPC_TRUE;
		src_objid = op_topo_parent (src_objid);
		
		/* we want to handle the subnets only in the subnet being animated. */
		if (src_objid == 0)
			FOUT;
		}
		
	while (op_topo_parent (dest_objid) != Imep_Subnet_Objid)
		{
		dest_subnet = OPC_TRUE;
		dest_objid = op_topo_parent (dest_objid);
		
		/* we want to handle the subnets only in the subnet being animated. */
		if (dest_objid == 0)
			FOUT;
		}
		
	/* Force the location update. */
	op_ima_obj_pos_get (src_objid, &lat, &lon, &alt, &x, &y, &z);
	op_ima_obj_pos_get (dest_objid, &lat, &lon, &alt, &x, &y, &z);
	
	/* relative location. */
	op_ima_obj_attr_get (src_objid, "x position", &src_nx);
	op_ima_obj_attr_get (src_objid, "y position", &src_ny);

	op_ima_obj_attr_get (dest_objid, "x position", &dest_nx);
	op_ima_obj_attr_get (dest_objid, "y position", &dest_ny);
	
	/* Update the node and subnet location in case they moved. */
	op_ima_obj_attr_get (src_objid, "name", &src_name);
	if (src_subnet)
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_SUBNET, src_name, OPC_ANIM_OBJ_ATTR_XPOS, src_nx, OPC_EOL);
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_SUBNET, src_name, OPC_ANIM_OBJ_ATTR_YPOS, src_ny, OPC_EOL);
		}
	else
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, src_name, OPC_ANIM_OBJ_ATTR_XPOS, src_nx, OPC_EOL);
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, src_name, OPC_ANIM_OBJ_ATTR_YPOS, src_ny, OPC_EOL);
		}
	
	op_ima_obj_attr_get (dest_objid, "name", &dest_name);
	if (dest_subnet)
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_SUBNET, dest_name, OPC_ANIM_OBJ_ATTR_XPOS, dest_nx, OPC_EOL);
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_SUBNET, dest_name, OPC_ANIM_OBJ_ATTR_YPOS, dest_ny, OPC_EOL);
		}
	else
		{
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, dest_name, OPC_ANIM_OBJ_ATTR_XPOS, dest_nx, OPC_EOL);
		op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, dest_name, OPC_ANIM_OBJ_ATTR_YPOS, dest_ny, OPC_EOL);
		}

	/* Find the corresponding location from the animation viewer. */
	op_anim_ime_gen_pos (Tora_ImepI_Anvid, src_nx, src_ny, &src_vx, &src_vy);
	op_anim_ime_gen_pos (Tora_ImepI_Anvid, dest_nx, dest_ny, &dest_vx, &dest_vy);
	
	/* We want to use different color in case of the current next hop. */
	if (neighbor->id == next_hop_rid)
		{
		props = OPC_ANIM_PIXOP_XOR | OPC_ANIM_COLOR_RGB221 | OPC_ANIM_STYLE_SOLID | OPC_ANIM_RETAIN;
		}
	else
		{
		props = OPC_ANIM_PIXOP_XOR | OPC_ANIM_COLOR_RGB221 | OPC_ANIM_STYLE_DOTS | OPC_ANIM_RETAIN;
		}
	
	/* Draw!! */
	if (router_id > neighbor->id)
		{
		offset = - ToraC_Arrow_Offset;
		}
	else
		{
		offset = ToraC_Arrow_Offset;
		}
	
	if (neighbor->id == next_hop_rid)
		{
		tora_imep_sup_draw_arrow_by_position (src_vx, src_vy, dest_vx, dest_vy, props, ToraC_Arrow_Width, 
			offset, OPC_TRUE, neighbor->anim_info->arrow_did_array,	neighbor->anim_info->arrow_did_array_double);
		}
	else
		{
		tora_imep_sup_draw_arrow_by_position (src_vx, src_vy, dest_vx, dest_vy, props, ToraC_Arrow_Width, 
			offset, OPC_FALSE, neighbor->anim_info->arrow_did_array, OPC_NIL);
		}
	
	FOUT;
	}

static void
tora_draw_arrows_to_neighbors (void* PRG_ARG_UNUSED (struct_ptr), int PRG_ARG_UNUSED (code))
	{
	int					list_size, index;
	ToraT_Height 		*tmp_neighbor;

	/* Helper function to periodically redraw custom anime. */
	FIN (tora_draw_arrows_to_neighbors (struct_ptr, code));
	
	/* Redraw all the arrows for the neighbors. */
	list_size = op_prg_list_size (neighbor_info_lptr);
	for (index=0; index < list_size; index++)
		{
		tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
		
		tora_draw_arrow_to_neighbor (my_node_objid, tmp_neighbor);	
		}
	
	/* Schedule the next redraw. */
	op_intrpt_schedule_call (op_sim_time() + (double) tora_animation_refresh_interval,
		0, tora_draw_arrows_to_neighbors, OPC_NIL);
	
	FOUT;
	}
	
static Boolean
tora_is_same_ref_level (ToraT_Height *neighbor_1, ToraT_Height *neighbor_2)
	{
	/* Helper function to compare two neighbor ref level. */
	FIN (tora_is_same_ref_level (neighbor_1, neighbor_2));
	
	if ((neighbor_1->tau == neighbor_2->tau) &&
		(neighbor_1->oid == neighbor_2->oid) &&
		(neighbor_1->r == neighbor_2->r))
		{
		FRET (OPC_TRUE);
		}
	else
		{
		FRET (OPC_FALSE);
		}
	}

static void
tora_draw_circle_on_destination (void* PRG_ARG_UNUSED (struct_ptr), int PRG_ARG_UNUSED (code))
	{
	double						lat, lon, alt, x, y, z, src_nx, src_ny;
	int							src_vx, src_vy, props;
	char 						my_name [256];
	
	/* Helper function to periodically redraw custom anime. */
	FIN (tora_draw_circle_on_destination (struct_ptr, code));
	
	/* We do not want to do this if the destination is in a subnet. */
	if (op_topo_parent (my_node_objid) != Imep_Subnet_Objid)
		FOUT;
		
	/* Property of the circle. */
	props = OPC_ANIM_PIXOP_XOR | OPC_ANIM_COLOR_RGB221 | OPC_ANIM_RETAIN;
	
	/* Erase the existing drawing. */
	if (*circle_did_array != OPC_ANIM_ID_NIL)
		{
		op_anim_igp_drawing_erase (Tora_ImepI_Anvid, circle_did_array [0], OPC_ANIM_ERASE_MODE_XOR);
		op_anim_igp_drawing_erase (Tora_ImepI_Anvid, circle_did_array [1], OPC_ANIM_ERASE_MODE_XOR);
		
		circle_did_array [0] = OPC_ANIM_ID_NIL;
		}
		
	/* Force the location update. */
	op_ima_obj_pos_get (my_node_objid, &lat, &lon, &alt, &x, &y, &z);
	
	/* relative location. */
	op_ima_obj_attr_get (my_node_objid, "x position", &src_nx);
	op_ima_obj_attr_get (my_node_objid, "y position", &src_ny);

	/* Update the node position in case it moved. */
	op_ima_obj_attr_get (my_node_objid, "name", &my_name);
	op_anim_ime_nobj_update (Tora_ImepI_Anvid, OPC_ANIM_OBJTYPE_NODE, my_name, 
		OPC_ANIM_OBJ_ATTR_XPOS, src_nx, OPC_ANIM_OBJ_ATTR_YPOS, src_ny, OPC_EOL);
	
	/* Find the corresponding location from the animation viewer. */
	op_anim_ime_gen_pos (Tora_ImepI_Anvid, src_nx, src_ny, &src_vx, &src_vy);

	/* Draw! */
	circle_did_array [0] = op_anim_igp_circle_draw (Tora_ImepI_Anvid, props, src_vx, src_vy, ToraC_Circle_Radius);
	circle_did_array [1] = op_anim_igp_circle_draw (Tora_ImepI_Anvid, props, src_vx, src_vy, ToraC_Circle_Radius + 1);
	
	/* Schedule the next redraw. */
	op_intrpt_schedule_call (op_sim_time() + (double) tora_animation_refresh_interval,
		0, tora_draw_circle_on_destination, OPC_NIL);
	
	FOUT;
	}

static Boolean
tora_is_neighbor_disabled (int	rid)
	{
	ToraT_Height 		tmp_neighbor, *target_neighbor;
	int					low, high;

	/* Helper function to verify the neighbor is presumed down. */
	FIN (tora_is_neighbor_disabled (rid));
	
	tmp_neighbor.id = rid;
	if ((target_neighbor = (ToraT_Height*) op_prg_list_elem_find (neighbor_info_lptr, 
		tora_height_compare_proc, &tmp_neighbor, &low, &high)))
		{
		if (target_neighbor->link_status == ToraC_Link_Status_Down)
			FRET (OPC_TRUE);
		}
	
	FRET (OPC_FALSE);
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
	void manet_tora (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_manet_tora_init (int * init_block_ptr);
	void _op_manet_tora_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_manet_tora_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_manet_tora_alloc (VosT_Obtype, int);
	void _op_manet_tora_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
manet_tora (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (manet_tora ());

		{
		/* Temporary Variables */
		Boolean							is_opt_timer=OPC_FALSE, is_destination=OPC_FALSE,
										is_unroutable_ip_arrival=OPC_FALSE, is_qry_arrival=OPC_FALSE,
										is_cleared_out=OPC_FALSE;
		
		Objid							tmp_objid, my_module_id;
		Packet*							pk_ptr=OPC_NIL;
		int								control_pk_type, inv_mode, low, high;
		ToraT_Height					tmp_height, *tmp_height_ptr = OPC_NIL, *target_neighbor = OPC_NIL;
		ToraC_Neighbor_Level			neighbor_level;
		double							opt_timer;
		int								global_mode_of_op;
		char 							msg [256];
		/* End of Temporary Variables */


		FSM_ENTER ("manet_tora")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (Inactive) enter executives **/
			FSM_STATE_ENTER_UNFORCED (0, "Inactive", state0_enter_exec, "manet_tora [Inactive enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"manet_tora")


			/** state (Inactive) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "Inactive", "manet_tora [Inactive exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Inactive exit execs]", state0_exit_exec)
				{
				/* Examine the parent to child memory to find out what to do. */
				switch (parmem_ptr->code)
					{
					case ToraC_Invocation_Code_Control_PK:
						{
						/* We have received a control packet from a neighbor. */
						pk_ptr = parmem_ptr->pk_ptr;
						op_pk_nfd_get (pk_ptr, "Type", &control_pk_type);
						
						switch (control_pk_type)
							{
							case ToraC_Pk_Type_Opt:
								{
								/* OPT packet was received. */
								
								/* Extract information. */
								tora_extract_pk_info (ToraC_Pk_Type_Opt, pk_ptr, &tmp_height);
								
								if (tora_is_neighbor_disabled (tmp_height.id))
									break;
								
								/* Update the next hop information. */
								next_hop_rid = tmp_height.id;
								
								/* Update the local height. */
								tora_local_height_adjust (tmp_height.tau, tmp_height.oid, tmp_height.r, tmp_height.delta + 1);
								my_height.last_opt_time = tmp_height.last_opt_time;
								
								/* Update the neighbor list according to the received information. */
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Downstrm);
								
								/* Forward the opt packet. */
								tora_generate_ctl_pk (ToraC_Pk_Type_Opt, ToraC_Relay_Packet, pk_ptr);
								
								if (TORA_TRACE)
									{
									tora_trace_print ("Received an OPT packet.");
									}
				
								break;
								}
					
							case ToraC_Pk_Type_Upd:
								{
								/* UPD packet was received. */
								
								/* Extract information. */
								tora_extract_pk_info (ToraC_Pk_Type_Upd, pk_ptr, &tmp_height);
							
								/* Check to see this is valid packet reception. */
								if (tora_is_neighbor_disabled (tmp_height.id))
									break;
								
								/* Is the neighor null? */
								if (tmp_height.tau == ToraC_Height_Invalid)
									{
									/* Update the neighbor list according to the received information. */
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Null);
									}
								else
									{
									/* Update the next hop information. */
									next_hop_rid = tmp_height.id;
							
									/* Update the local height. */
									tora_local_height_adjust (tmp_height.tau, tmp_height.oid, tmp_height.r, tmp_height.delta + 1);
							
									/* Update the neighbor list according to the received information. */
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Calculate);
								
									/* Propagate the updated information. */
									tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
									
									if (TORA_TRACE)
										{
										sprintf (msg, "Received an UPD packet from %d. Relaying the packet.", tmp_height.id);
										tora_trace_print (msg);
										}
									}
									
								break;
								}
								
							case ToraC_Pk_Type_Qry:
								{
								/* QRY packet was received. */
								is_qry_arrival = OPC_TRUE;
								
								/* Forward the qry packet. */
								tora_generate_ctl_pk (ToraC_Pk_Type_Qry, ToraC_Relay_Packet, pk_ptr);
								
								if (TORA_TRACE)
									{
									tora_trace_print ("Received an QRY packet.");
									}
								
								break;
								}
					
							case ToraC_Pk_Type_Clr:
								{
								/* CLR packet was received. */
								if (TORA_TRACE)
									{
									tora_trace_print ("Received an CLR packet.");
									}
								
								break;
								}
							}
						/* Destroy the packet. */
						op_pk_destroy (pk_ptr);
						pk_ptr = OPC_NIL;
						
						break;
						}
					
					case ToraC_Invocation_Code_IP_PK:
						{
						/* Unroutable IP packet arrival. Queue it for now. */
						is_unroutable_ip_arrival = OPC_TRUE;
								
						/* Create a new QRY packet. */
						tora_generate_ctl_pk (ToraC_Pk_Type_Qry, ToraC_New_Packet, OPC_NIL);
					
						/* Queue the arrived packet. */
						tora_queue_ip_pkt (parmem_ptr->pk_ptr);
						
						if (TORA_TRACE)
							{
							tora_trace_print ("Received an IP packet and queued it.");
							}
						
						break;
						}
						
					case ToraC_Invocation_Code_Link_Status_Change:
						{
						/* Link status have changed.  See if the neighbor is the destination. */
						if (parmem_ptr->link_state == ImepC_LinkState_Up)
							{
							if (parmem_ptr->destination_rid == parmem_ptr->neighbor_rid)
								{
								/* Update the time flag. */
								last_linkup_time = op_sim_time ();
				
								/* Update the next hop information. */
								next_hop_rid = parmem_ptr->neighbor_rid;
							
								/* Adjust my height. */
								tora_local_height_adjust (last_linkup_time, parmem_ptr->neighbor_rid, 0, 1);
								
								/* Update the neighbor list according to the current information. */
								tmp_height.tau = ToraC_Height_Destination;
								tmp_height.update_time = last_linkup_time; 
								tmp_height.oid = parmem_ptr->neighbor_rid;
								tmp_height.r = 0;
								tmp_height.delta = 0;
								tmp_height.id = parmem_ptr->neighbor_rid;
								
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Downstrm);
				
								if (TORA_TRACE)
									tora_trace_print ("Link to the destination has been found for the first time.");
								}
							else
								{
								/* Update the link information. */
								tmp_height.id = parmem_ptr->neighbor_rid;
								tmp_height.update_time = last_linkup_time;
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Up);
								}
							}
						else
							{
							/* We will simply update the neighbor information in case we have them. */
							tmp_height.id = parmem_ptr->neighbor_rid;
							tmp_height.update_time = op_sim_time ();
							tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Down);
									
							if (TORA_TRACE)
								tora_trace_print ("Next hop neighbor has been changed for link down.");
							}
				
						break;
						}
					default:
						break;	
				    }
				
				/* Update the downstream count. */
				tora_downstrm_count ();
				}
				FSM_PROFILE_SECTION_OUT (state0_exit_exec)


			/** state (Inactive) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Inactive trans conditions]", state0_trans_conds)
			FSM_INIT_COND (UNROUTABLE_IP_ARRIVAL || QRY_ARRIVAL)
			FSM_TEST_COND (ROUTE_DISCOVERY)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("Inactive")
			FSM_PROFILE_SECTION_OUT (state0_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 3, state3_enter_exec, ;, "UNROUTABLE_IP_ARRIVAL || QRY_ARRIVAL", "", "Inactive", "Querying", "tr_-2", "manet_tora [Inactive -> Querying : UNROUTABLE_IP_ARRIVAL || QRY_ARRIVAL / ]")
				FSM_CASE_TRANSIT (1, 7, state7_enter_exec, ;, "ROUTE_DISCOVERY", "", "Inactive", "IP processing", "tr_-1", "manet_tora [Inactive -> IP processing : ROUTE_DISCOVERY / ]")
				FSM_CASE_TRANSIT (2, 0, state0_enter_exec, ;, "default", "", "Inactive", "Inactive", "tr_69", "manet_tora [Inactive -> Inactive : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Destination) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "Destination", state1_enter_exec, "manet_tora [Destination enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"manet_tora")


			/** state (Destination) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "Destination", "manet_tora [Destination exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Destination exit execs]", state1_exit_exec)
				{
				/* Who invoked me? */
				op_pro_invoker (self_phndl, &inv_mode);
							
				if (inv_mode == OPC_PROINV_DIRECT)
					{
					/* This must be the self interrupt for the OPT transmission. */
					is_opt_timer = OPC_TRUE;
					}
				else
					{
					/* Examine the parent to child memory to find out what to do. */
					if (parmem_ptr->code == ToraC_Invocation_Code_Control_PK)
						{
						/* We have received a control packet from a neighbor. Answer only QRY. */
						op_pk_nfd_get (parmem_ptr->pk_ptr, "Type", &control_pk_type);
						
						if (control_pk_type == ToraC_Pk_Type_Qry)
							{
							tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
								
							if (TORA_TRACE)
								tora_trace_print ("Received an QRY packet. Responding with an UPD");
							}
				
						op_pk_destroy (parmem_ptr->pk_ptr);
						}
					}
				
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (Destination) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Destination trans conditions]", state1_trans_conds)
			FSM_INIT_COND (OPT_TIMER)
			FSM_TEST_COND (!OPT_TIMER)
			FSM_TEST_LOGIC ("Destination")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "OPT_TIMER", "", "Destination", "Send OPT", "tr_5", "manet_tora [Destination -> Send OPT : OPT_TIMER / ]")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ;, "!OPT_TIMER", "", "Destination", "Destination", "tr_10", "manet_tora [Destination -> Destination : !OPT_TIMER / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Valid Route) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "Valid Route", state2_enter_exec, "manet_tora [Valid Route enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Valid Route enter execs]", state2_enter_exec)
				{
				if (pk_ptr)
					{
					/* Destroy the packet. */
					op_pk_destroy (pk_ptr);
					pk_ptr = OPC_NIL;
					}
						
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"manet_tora")


			/** state (Valid Route) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "Valid Route", "manet_tora [Valid Route exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Valid Route exit execs]", state2_exit_exec)
				{
				/* Examine the parent to child memory to find out what to do. */
				switch (parmem_ptr->code)
					{
					case ToraC_Invocation_Code_Control_PK:
						{
						/* We have received a control packet from a neighbor. */
						pk_ptr = parmem_ptr->pk_ptr;
						op_pk_nfd_get (pk_ptr, "Type", &control_pk_type);
						
						switch (control_pk_type)
							{
							case ToraC_Pk_Type_Opt:
								{
								/* Extract information. */
								tora_extract_pk_info (ToraC_Pk_Type_Opt, pk_ptr, &tmp_height);
								
								if (tora_is_neighbor_disabled (tmp_height.id))
									break;
								
								/* If the new OPT period, relay it. */
								if (tmp_height.last_opt_time == my_height.last_opt_time)
									{
									/* Update the neighbor list according to the received information. */
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Calculate);
										
									if (TORA_TRACE)
										{
										sprintf (msg, "Received an OPT packet from %d. Destroying the packet.", tmp_height.id);
										tora_trace_print (msg);
										}
									}
								else
									{
									if (tmp_height.last_opt_time > my_height.last_opt_time)
										{
										/* Update the local height. */
										tora_local_height_adjust (tmp_height.tau, tmp_height.oid, tmp_height.r, tmp_height.delta + 1);
										my_height.last_opt_time = tmp_height.last_opt_time;
				
										/* Update the next hop information. */
										next_hop_rid = tmp_height.id;
									
										/* Update the neighbor list according to the received information. */
										tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Calculate);
										
										/* Update the routing table. */
										tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
										tora_route_inject (next_hop_addr);
										
										/* Relay the packet. */
										tora_generate_ctl_pk (ToraC_Pk_Type_Opt, ToraC_Relay_Packet, pk_ptr);
									
										if (TORA_TRACE)
											{
											sprintf (msg, "Received an OPT packet from %d. Relaying the packet.", tmp_height.id);
											tora_trace_print (msg);
											}
										}
									else
										{
										/* This is an old OPT retransmitted. */
										if (TORA_TRACE)
											{
											sprintf (msg, "Received an OPT packet from %d. Destroying the packet.", tmp_height.id);
											tora_trace_print (msg);
											}
										}
									}
						
								break;
								}
					
							case ToraC_Pk_Type_Upd:
								{
								/* Update height accordingly. See if downstream link is available or not.  */
								
								/* Extract information. */
								tora_extract_pk_info (ToraC_Pk_Type_Upd, pk_ptr, &tmp_height);
								
								if (tora_is_neighbor_disabled (tmp_height.id))
									break;
									
								/* Is the neighor null? */
								if (tmp_height.tau == ToraC_Height_Invalid)
									{
									/* Update the neighbor list according to the received information. */
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Null);
									}
								else
									{
									/* Update the neighbor list according to the received information. */
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Calculate);
									}
						
								/* Try finding the next best next hop neighbor. */
								if (OPC_COMPCODE_SUCCESS == tora_best_next_hop_find (&next_hop_rid))
									{
									/* Update the routing table. */
									tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
									tora_route_inject (next_hop_addr);
									}
								else
									{
									/* We are out of downstreams. */
									}
								
								if (TORA_TRACE)
									{
									sprintf (msg, "Received an UPD packet from %d. Not relaying the packet.", tmp_height.id);
									tora_trace_print (msg);
									}
								
								break;
								}
								
							case ToraC_Pk_Type_Qry:
								{
								if (last_upd_tx_time < last_linkup_time)
									{
									/* Respond since we have a valid route to the destination. */
									tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
									}
								
								if (TORA_TRACE)
									{
									sprintf (msg, "Received an QRY packet.");
									tora_trace_print (msg);
									}
								
								break;
								}
					
							case ToraC_Pk_Type_Clr:
								{
								/* Extract information. */
								tora_extract_pk_info (ToraC_Pk_Type_Clr, pk_ptr, &tmp_height);
								
								if (tora_is_neighbor_disabled (tmp_height.id))
									break;
								
								if ((tmp_height.tau == my_height.tau) &&
									(tmp_height.oid == my_height.oid))
									{
									/* Same level. Nullify all the neighbors*/
									my_height.tau = ToraC_Height_Invalid;
									tora_neighbors_nullify (OPC_NIL);
									tora_generate_ctl_pk (ToraC_Pk_Type_Clr, ToraC_Relay_Packet, pk_ptr);
									
									is_cleared_out = OPC_TRUE;
									}
								else
									{
									/* Nullify affected neighbor heights. */				
									tora_neighbors_nullify (&tmp_height);
									}
											
								if (TORA_TRACE)
									{
									sprintf (msg, "Received an CLR packet from %d.", tmp_height.id);
									tora_trace_print (msg);
									}
								
								break;
								}
							}
				
						break;
						}
					
					case ToraC_Invocation_Code_IP_PK:
						{
						/* Send back to IP since this now can be routed. */
						tora_packet_send (parmem_ptr->pk_ptr, ToraC_Pk_To_IP);	
						
						break;
						}
						
					case ToraC_Invocation_Code_Link_Status_Change:
						{
						/* A link status has changed. We only act when the current next hop link is down or 
						the neighbor of the newly restored link is the destination. */
						
						/* See if the neighbor on the other side of this link is the destination. */
						if (parmem_ptr->link_state == ImepC_LinkState_Up)
							{
							/* Update the time flag. */
							last_linkup_time = op_sim_time ();
				
							if (parmem_ptr->destination_rid == parmem_ptr->neighbor_rid)
								{
								/* Adjust my height. */
								tora_local_height_adjust (last_linkup_time, parmem_ptr->neighbor_rid, 0, 1);
							
								/* Update the next hop information. */
								next_hop_rid = parmem_ptr->neighbor_rid;
								
								/* Update the local routing table info. */
								tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
								tora_route_inject (next_hop_addr);
							
								/* Update the neighbor list according to the current information. */
								tmp_height.tau = ToraC_Height_Destination;
								tmp_height.update_time = last_linkup_time; 
								tmp_height.oid = parmem_ptr->neighbor_rid;
								tmp_height.r = 0;
								tmp_height.delta = 0;
								tmp_height.id = parmem_ptr->neighbor_rid;
				
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Downstrm);
								
								if (TORA_TRACE)
									tora_trace_print ("Link to the destination has been restored.");
								}
							else
								{
								/* Update the link information. */
								tmp_height.id = parmem_ptr->neighbor_rid;
								tmp_height.update_time = last_linkup_time;
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Up);
								
								/* Share height information with the new neighbor. */
								if (infinite_loop_prevention_flag)
									{
									/* This is not supposed to happen according to the specification but can
									prevent infinite loop situation that occasionally happens. */
									tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
									}
								}
							}
						else
							{
							/* A link has been down. */
							if (parmem_ptr->neighbor_rid == next_hop_rid) 
								{
								/* Search for the existing neighbor and update information. */
								tmp_height.id = parmem_ptr->neighbor_rid;
								tmp_height.update_time = op_sim_time ();
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Down);
				
								/* Try finding the next best next hop neighbor. */
								if (OPC_COMPCODE_SUCCESS == tora_best_next_hop_find (&next_hop_rid))
									{
									/* Update the routing table. */
									tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
									tora_route_inject (next_hop_addr);
													
									if (TORA_TRACE)
										tora_trace_print ("Next hop neighbor has been changed for link down.");
									}
								else
									{
									/* We are out of downstreams. */
									}
								}
							else
								{
								/* We will simply update the neighbor information in case we have them. */
								tmp_height.id = parmem_ptr->neighbor_rid;
								tmp_height.update_time = op_sim_time ();
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Down);
								}
							
							break;
							}
				
						break;
						}
					default:
						break;
				    }
				
				/* Update the downstream count. */
				tora_downstrm_count ();
				
				
				
				}
				FSM_PROFILE_SECTION_OUT (state2_exit_exec)


			/** state (Valid Route) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Valid Route trans conditions]", state2_trans_conds)
			FSM_INIT_COND (!DNLINK_AVAILABLE && !CLEARED_OUT)
			FSM_TEST_COND (DNLINK_AVAILABLE)
			FSM_TEST_COND (!DNLINK_AVAILABLE && CLEARED_OUT)
			FSM_TEST_LOGIC ("Valid Route")
			FSM_PROFILE_SECTION_OUT (state2_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 6, state6_enter_exec, ;, "!DNLINK_AVAILABLE && !CLEARED_OUT", "", "Valid Route", "Init Update", "tr_16", "manet_tora [Valid Route -> Init Update : !DNLINK_AVAILABLE && !CLEARED_OUT / ]")
				FSM_CASE_TRANSIT (1, 2, state2_enter_exec, ;, "DNLINK_AVAILABLE", "", "Valid Route", "Valid Route", "tr_18", "manet_tora [Valid Route -> Valid Route : DNLINK_AVAILABLE / ]")
				FSM_CASE_TRANSIT (2, 3, state3_enter_exec, ;, "!DNLINK_AVAILABLE && CLEARED_OUT", "", "Valid Route", "Querying", "tr_61", "manet_tora [Valid Route -> Querying : !DNLINK_AVAILABLE && CLEARED_OUT / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Querying) enter executives **/
			FSM_STATE_ENTER_UNFORCED (3, "Querying", state3_enter_exec, "manet_tora [Querying enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Querying enter execs]", state3_enter_exec)
				{
				if (!query_mode)
					{
					/* Reset the next hop rid. */
					next_hop_rid = -1;
					
					/* Take the route entry from the cmn rte table. */
					if (parmem_ptr->lan_addr != IPC_ADDR_INVALID)
						{
						Ip_Cmn_Rte_Table_Route_Delete (parmem_ptr->ip_rte_table, parmem_ptr->lan_addr,
							parmem_ptr->lan_addr_range_ptr->subnet_mask,
							IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_TORA ,IPC_NO_MULTIPLE_PROC));
						}
					else
						{
						Ip_Cmn_Rte_Table_Route_Delete (parmem_ptr->ip_rte_table, parmem_ptr->address,
							IpI_Broadcast_Addr, IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_TORA ,IPC_NO_MULTIPLE_PROC));
						}
				
					/* Reset stat related variables. */
					query_mode = OPC_TRUE;
					if (time_in_query_mode == -1.0)
						{
						time_in_query_mode = op_sim_time ();
						}
					}
				
				if (pk_ptr)
					{
					/* Destroy the packet. */
					op_pk_destroy (pk_ptr);
					pk_ptr = OPC_NIL;
					}
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (7,"manet_tora")


			/** state (Querying) exit executives **/
			FSM_STATE_EXIT_UNFORCED (3, "Querying", "manet_tora [Querying exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Querying exit execs]", state3_exit_exec)
				{
				/* Who invoked me? */
				op_pro_invoker (self_phndl, &inv_mode);
							
				if (inv_mode == OPC_PROINV_DIRECT)
					{
					/* This must be the self interrupt for the IP flush timer. */
					tora_ip_queue_flush ();
					}
				else
					{
					/* Examine the parent to child memory to find out what to do. */
					switch (parmem_ptr->code)
						{
						case ToraC_Invocation_Code_Control_PK:
							{
							/* We have received a control packet from a neighbor. */
							pk_ptr = parmem_ptr->pk_ptr;
							op_pk_nfd_get (pk_ptr, "Type", &control_pk_type);
							
							switch (control_pk_type)
								{
								case ToraC_Pk_Type_Opt:
									{
									/* Extract information. */
									tora_extract_pk_info (ToraC_Pk_Type_Opt, pk_ptr, &tmp_height);
									
									if (tora_is_neighbor_disabled (tmp_height.id))
										break;
				
									/* If the new OPT period, we are good to go. */
									if (tmp_height.last_opt_time > my_height.last_opt_time)
										{
										/* Update the next hop information. */
										next_hop_rid = tmp_height.id;
								
										/* Update the local height. */
										tora_local_height_adjust (tmp_height.tau, tmp_height.oid, tmp_height.r, tmp_height.delta + 1);
										my_height.last_opt_time = tmp_height.last_opt_time;
								
										/* Update the neighbor list according to the received information. */
										tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Downstrm);
								
										/* Forward the opt packet. */
										tora_generate_ctl_pk (ToraC_Pk_Type_Opt, ToraC_Relay_Packet, pk_ptr);
								
										if (TORA_TRACE)
											{
											sprintf (msg, "Received an OPT packet from %d. Relaying the packet.", tmp_height.id);
											tora_trace_print (msg);
											}
										}
									else
										{
										if (TORA_TRACE)
											{
											sprintf (msg, "Received an OPT packet from %d. Destroying the packet.", tmp_height.id);
											tora_trace_print (msg);
											}
										}
									
									break;
									}
					
								case ToraC_Pk_Type_Upd:
									{
									/* UPD packet was received. */
								
									/* Extract information. */
									tora_extract_pk_info (ToraC_Pk_Type_Upd, pk_ptr, &tmp_height);
								
									if (tora_is_neighbor_disabled (tmp_height.id))
										break;
									
									/* Is the neighor null? */
									if (tmp_height.tau == ToraC_Height_Invalid)
										{
										/* Update the neighbor list according to the received information. */
										tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Null);
										}
									else
										{
										if ((target_neighbor = (ToraT_Height*) op_prg_list_elem_find (neighbor_info_lptr, 
											tora_height_compare_proc, &tmp_height, &low, &high)))
											{
											if (target_neighbor->link_status == ToraC_Link_Status_Null)
												{
												/* Ignore if this update is the same ref level. */
												if (tora_is_same_ref_level (target_neighbor, &tmp_height))
													{
													break;
													}
												}
											}
				
										/* Update the next hop information. */
										next_hop_rid = tmp_height.id;
							
										/* Update the local height. */
										tora_local_height_adjust (tmp_height.tau, tmp_height.oid, tmp_height.r, tmp_height.delta + 1);
							
										/* Update the neighbor list according to the received information. */
										tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Calculate);
				
										/* Propagate the updated information. */
										tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
										
										if (TORA_TRACE)
											{
											sprintf (msg, "Received an UPD packet from %d. Relaying the packet.", tmp_height.id);
											tora_trace_print (msg);
											}
										}
									
									break;
									}
								
								case ToraC_Pk_Type_Qry:
									{
									/* QRY packet was received. Ignore. */
									
									break;
									}
					
								case ToraC_Pk_Type_Clr:
									{
									/* CLR packet was received. Ignore. */
									
									break;
									}
								}
							/* Destroy the packet. */
							op_pk_destroy (pk_ptr);
							pk_ptr = OPC_NIL;
						
							break;
							}
					
						case ToraC_Invocation_Code_IP_PK:
							{
							/* Unroutable IP packet arrival. Queue it and set timer if not done yet. */
							tora_queue_ip_pkt (parmem_ptr->pk_ptr);
							
							if (TORA_TRACE)
								{
								tora_trace_print ("Received an IP packet and queued it.");
								}
				
							break;
							}
						
						case ToraC_Invocation_Code_Link_Status_Change:
							{
							/* A link status has changed. We only act when the neighbor of the newly 
								restored link is the destination. Otherwise we will rebroadcast QRY. */
						
							if (parmem_ptr->link_state == ImepC_LinkState_Up)
								{
								/* Update the time flag. */
								last_linkup_time = op_sim_time ();
				
								/* See if the neighbor on the other side of this link is the destination. */
								if (parmem_ptr->destination_rid == parmem_ptr->neighbor_rid)
									{
									/* Update the next hop information. */
									next_hop_rid = parmem_ptr->neighbor_rid;
								
									/* Adjust my height. */
									tora_local_height_adjust (last_linkup_time, parmem_ptr->neighbor_rid, 0, 1);
							
									/* Update the local routing table info. */
									tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
									tora_route_inject (next_hop_addr);			
									
									/* Update the neighbor list according to the current information. */
									tmp_height.tau = ToraC_Height_Destination;
									tmp_height.update_time = last_linkup_time; 
									tmp_height.oid = parmem_ptr->neighbor_rid;
									tmp_height.r = 0;
									tmp_height.delta = 0;
									tmp_height.id = parmem_ptr->neighbor_rid;
				
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Downstrm);
									
									/* Send out the UPD to neighbors. */
									tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
				
									if (TORA_TRACE)
										tora_trace_print ("Link to the destination has been restored.");
									}
								else
									{
									/* Update the link information. */
									tmp_height.id = parmem_ptr->neighbor_rid;
									tmp_height.update_time = last_linkup_time;
									tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Up);
						
									/* Send out QRY. */
									tora_generate_ctl_pk (ToraC_Pk_Type_Qry, ToraC_New_Packet, OPC_NIL);
										
									if (TORA_TRACE)
										tora_trace_print ("A link has been restored while querying. Sending QRY");
									}
								}
							else
								{
								/* We will simply update the neighbor information in case we have them. */
								tmp_height.id = parmem_ptr->neighbor_rid;
								tmp_height.update_time = op_sim_time ();
								tora_update_neighbor_list (&tmp_height, ToraC_Link_Status_Down);
										
								if (TORA_TRACE)
									tora_trace_print ("Next hop neighbor has been changed for link down.");
								}
				
							break;
							}
						default:
							break;
						}
					}
				
				/* Update the downstream count. */
				tora_downstrm_count ();
				}
				FSM_PROFILE_SECTION_OUT (state3_exit_exec)


			/** state (Querying) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Querying trans conditions]", state3_trans_conds)
			FSM_INIT_COND (!ROUTE_DISCOVERY)
			FSM_TEST_COND (ROUTE_DISCOVERY)
			FSM_TEST_LOGIC ("Querying")
			FSM_PROFILE_SECTION_OUT (state3_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 3, state3_enter_exec, ;, "!ROUTE_DISCOVERY", "", "Querying", "Querying", "tr_13", "manet_tora [Querying -> Querying : !ROUTE_DISCOVERY / ]")
				FSM_CASE_TRANSIT (1, 7, state7_enter_exec, ;, "ROUTE_DISCOVERY", "", "Querying", "IP processing", "tr_2", "manet_tora [Querying -> IP processing : ROUTE_DISCOVERY / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Send OPT) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "Send OPT", state4_enter_exec, "manet_tora [Send OPT enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Send OPT enter execs]", state4_enter_exec)
				{
				/* Send out the OPT packet. */
				tora_generate_ctl_pk (ToraC_Pk_Type_Opt, ToraC_New_Packet, OPC_NIL);
						
				/* Schedule the periodic OPT packet transmission. */
				op_intrpt_schedule_self (op_sim_time () + (double) opt_interval, 0);
				
				if (TORA_TRACE)
					{
					tora_trace_print ("Sending OPT packet to neighbors.");
					}
				}
				FSM_PROFILE_SECTION_OUT (state4_enter_exec)

			/** state (Send OPT) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "Send OPT", "manet_tora [Send OPT exit execs]")


			/** state (Send OPT) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Send OPT", "Destination", "tr_6", "manet_tora [Send OPT -> Destination : default / ]")
				/*---------------------------------------------------------*/



			/** state (Init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (5, "Init", "manet_tora [Init enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Init enter execs]", state5_enter_exec)
				{
				/* Access and parse the attribute information. */
				my_module_id = op_id_self ();
				my_node_objid = op_topo_parent (my_module_id);
				
				op_ima_obj_attr_get (my_module_id,	"manet_mgr.TORA/IMEP Parameters", &tmp_objid);
				tmp_objid = op_topo_child (tmp_objid, OPC_OBJTYPE_GENERIC, 0);
				op_ima_obj_attr_get (tmp_objid, "Router ID", &router_id);
				
				/* Parse the parent to child memory to find out if I am destination. */
				parmem_ptr = (ToraT_Invocation_Info*) op_pro_parmem_access ();
				
				/* Get the router ID of the destination of interest. */
				tora_imep_sup_ip_addr_to_rid (&dest_rid, parmem_ptr->address);
				
				/* See if we need to be animation ready. */
				tora_imep_support_animation_init ();
				animation_enabled = OPC_FALSE;
				
				if (parmem_ptr->code == ToraC_Invocation_Code_Destination)
					{
					is_destination = OPC_TRUE;
					my_height.tau = ToraC_Height_Destination;
				
					/* See if the OPT mode is on. */
					op_ima_sim_attr_get (OPC_IMA_INTEGER, "TORA Mode of Operation", &global_mode_of_op);
					
					if (global_mode_of_op == ToraC_Mode_Default)
						{
						op_ima_obj_attr_get (tmp_objid, "TORA Parameters", &tmp_objid);
						tmp_objid = op_topo_child (tmp_objid, OPC_OBJTYPE_GENERIC, 0);
						op_ima_obj_attr_get (tmp_objid, "Mode of Operation", &mode_of_operation);
						}
					else
						{
						mode_of_operation = global_mode_of_op;
						op_ima_obj_attr_get (tmp_objid, "TORA Parameters", &tmp_objid);
						tmp_objid = op_topo_child (tmp_objid, OPC_OBJTYPE_GENERIC, 0);
						}
					
					if (mode_of_operation == ToraC_Mode_Proactive)
						{
						/* Find the OPT transmit interval. */
						op_ima_obj_attr_get (tmp_objid, "OPT Transmit Interval", &opt_interval);
						
						/* Schedule the periodic OPT packet transmission. */
						opt_timer = op_sim_time () + op_dist_uniform ((double) opt_interval);
						op_intrpt_schedule_self (opt_timer, 0);
						
						if (TORA_TRACE)
							printf ("\nRouter:%d scheduled periodic OPT transmission at %.3f.",router_id, opt_timer);
						
						}
					
					if (tora_animation_requested)
						{
						/* See if I am the concerned destination. */
						if (tora_animated_dest_rid != -1)
							{
							if (tora_animated_dest_rid == router_id)
								{
								/* We will keep a circle around me. */
								animation_enabled = OPC_TRUE;
								
								/* Initialize the circle did array. */
								*circle_did_array = OPC_ANIM_ID_NIL;
								
								/* Schedule the periodic redraw. */
								op_intrpt_schedule_call (0.0, 0, tora_draw_circle_on_destination, OPC_NIL);
								}
							}
						}
					}
				else
					{
					/* Initialize some of the variables. */
					unroutable_ip_pkt_lptr = op_prg_list_create ();
					neighbor_info_lptr = op_prg_list_create ();
					next_hop_addr = IPC_ADDR_INVALID;
					next_hop_rid = -1;
					my_height.tau = ToraC_Height_Invalid;
					
					if (tora_animation_requested)
						{
						/* See if I am handling the concerned destination. */
						if (tora_animated_dest_rid != -1)
							{
							if (dest_rid == tora_animated_dest_rid)
								{
								animation_enabled = OPC_TRUE;
								
								/* Schedule the periodic redraw. */
								op_intrpt_schedule_call ((double) (tora_animation_refresh_interval * 
									((int)(op_sim_time () / (double) tora_animation_refresh_interval) + 1)),
									0, tora_draw_arrows_to_neighbors, OPC_NIL);
								}
							}
						}
					
					/* Initialize some of the stats for future use. */
					route_discovery_delay_sthndl_ptr = OPC_NIL;
					ip_packet_dropped_local_sthndl_ptr = OPC_NIL;
					ip_packet_dropped_global_sthndl_ptr = OPC_NIL;
					ip_queue_size_sthndl_ptr = OPC_NIL;
					
					/* Initialize some stat related variables. */
					query_mode = OPC_FALSE;
					time_in_query_mode = op_sim_time (); 
					
					/* See if the infinite loop prevention flag is on. */
					op_ima_sim_attr_get (OPC_IMA_TOGGLE, "TORA Infinite Loop Prevention", &infinite_loop_prevention_flag);
				
					}
				
				/* Some of the state variables are used in both cases. */
				self_phndl = op_pro_self ();
				my_height.id = router_id;
				num_dnstrm = 0;
				}
				FSM_PROFILE_SECTION_OUT (state5_enter_exec)

			/** state (Init) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "Init", "manet_tora [Init exit execs]")


			/** state (Init) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Init trans conditions]", state5_trans_conds)
			FSM_INIT_COND (DESTINATION)
			FSM_TEST_COND (!DESTINATION)
			FSM_TEST_LOGIC ("Init")
			FSM_PROFILE_SECTION_OUT (state5_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ;, "DESTINATION", "", "Init", "Destination", "tr_3", "manet_tora [Init -> Destination : DESTINATION / ]")
				FSM_CASE_TRANSIT (1, 0, state0_enter_exec, ;, "!DESTINATION", "", "Init", "Inactive", "tr_24", "manet_tora [Init -> Inactive : !DESTINATION / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Init Update) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "Init Update", state6_enter_exec, "manet_tora [Init Update enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [Init Update enter execs]", state6_enter_exec)
				{
				/* No more downstream. */
				if (parmem_ptr->code != ToraC_Invocation_Code_Link_Status_Change)
					{
					/* Check the neighbor level. */
					neighbor_level = tora_neighbor_list_level_check (&tmp_height_ptr);
									
					switch (neighbor_level)
						{
						case ToraC_Neighbor_Level_Propagate:
							{
							/* Neighbors have different reference level. tmp_height has the max level info. */
							tora_local_height_adjust (tmp_height_ptr->tau, tmp_height_ptr->oid, 
								tmp_height_ptr->r, tmp_height_ptr->delta - 1);
							
							/* Recalculate the neighbor heights. */
							tora_link_reversal (OPC_NIL);
							
							/* Send an Update to others. */
							tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
							
							if (TORA_TRACE)
								{
								tora_trace_print ("PROPAGATE condition. Seding out UPD packet.");
								}
				
							break;
							}
										
						case ToraC_Neighbor_Level_Reflect:
							{
							/* Neighbors have same reference level. height.r is 0. */
							tora_local_height_adjust (tmp_height_ptr->tau, tmp_height_ptr->oid, 1, 0);
											
							/* Recalculate the neighbor heights. */
							tora_link_reversal (OPC_NIL);
						
							/* Send an Update to others. */
							tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
							
							if (TORA_TRACE)
								{
								tora_trace_print ("REFLECT condition. Seding out UPD packet.");
								}
							
							break;
							}
										
						case ToraC_Neighbor_Level_Detect:
							{
							/* Neighbors have same reference level. height.r is 1. */
							my_height.r = 1;
				
							/* Detected partition.  Send out Clr. */
							tora_generate_ctl_pk (ToraC_Pk_Type_Clr, ToraC_New_Packet, OPC_NIL);
									
							/* Nullify affected neighbpr heights. */				
							my_height.tau = ToraC_Height_Invalid;
							tora_neighbors_nullify (OPC_NIL);
				
							if (TORA_TRACE)
								{
								tora_trace_print ("DETECT condition. Seding out CLR packet.");
								}
							
							break;
							}
							
						case ToraC_Neighbor_Level_Generate:
							{
							/* Neighbors have same reference level. height.r is 1. oid != my oid. */
				
							/* Define a new ref level. */
							tora_local_height_adjust (op_sim_time (), my_height.id, 0, 0);
							
							/* Reverse the link directions. */
							tora_link_reversal (OPC_NIL);
							
							/* Send an Update to others. */
							tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
									
							if (TORA_TRACE)
								{
								tora_trace_print ("GENERATE condition. Seding out UPD packet.");
								}
					
							break;
							}
						default:
							break;
						}
					}
				else
					{
					/* We lost the last downstrm due to link failure. Define a new ref level. */
					tora_local_height_adjust (op_sim_time (), my_height.id, 0, 0);
					
					/* Reverse the link directions. */
					tora_link_reversal (OPC_NIL);
				
					/* Only if we have a downlink after the reversal. */
					if (next_hop_rid != -1)
						{
						/* Send an Update to others. */
						tora_generate_ctl_pk (ToraC_Pk_Type_Upd, ToraC_New_Packet, OPC_NIL);
				
						if (TORA_TRACE)
							{
							tora_trace_print ("GENERATE condition. Seding out UPD packet.");
							}
						}
					else
						{
						/* Nullify my height. */				
						my_height.tau = ToraC_Height_Invalid;
					
						if (TORA_TRACE)
							{
							tora_trace_print ("GENERATE condition. Do nothing.");
							}
						}
							
					}
					
				/* Update the downstream count. */
				tora_downstrm_count ();
				
							
				
				}
				FSM_PROFILE_SECTION_OUT (state6_enter_exec)

			/** state (Init Update) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "Init Update", "manet_tora [Init Update exit execs]")


			/** state (Init Update) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora [Init Update trans conditions]", state6_trans_conds)
			FSM_INIT_COND (LINK_REVERSAL)
			FSM_TEST_COND (!DNLINK_AVAILABLE)
			FSM_TEST_LOGIC ("Init Update")
			FSM_PROFILE_SECTION_OUT (state6_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 7, state7_enter_exec, ;, "LINK_REVERSAL", "", "Init Update", "IP processing", "tr_56", "manet_tora [Init Update -> IP processing : LINK_REVERSAL / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "!DNLINK_AVAILABLE", "", "Init Update", "Querying", "tr_51", "manet_tora [Init Update -> Querying : !DNLINK_AVAILABLE / ]")
				}
				/*---------------------------------------------------------*/



			/** state (IP processing) enter executives **/
			FSM_STATE_ENTER_FORCED (7, "IP processing", state7_enter_exec, "manet_tora [IP processing enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora [IP processing enter execs]", state7_enter_exec)
				{
				/* Update the routing table and send out queued IP packets. */
				tora_imep_sup_rid_to_ip_addr (next_hop_rid, &next_hop_addr);
				tora_route_inject (next_hop_addr);
				
				tora_queued_ip_send ();
				
				/* Record statistics. */
				query_mode = OPC_FALSE;
				if (time_in_query_mode != -1.0)
					{
					if (!route_discovery_delay_sthndl_ptr)
						{
						/* We need to register stat for the first time. */
						route_discovery_delay_sthndl_ptr = (Stathandle*) op_prg_mem_alloc (sizeof (Stathandle));
						*route_discovery_delay_sthndl_ptr = op_stat_reg ("TORA_IMEP.Route Discovery Delay", dest_rid, OPC_STAT_LOCAL);
						}
				
					op_stat_write (*route_discovery_delay_sthndl_ptr, op_sim_time () - time_in_query_mode);
					
					/* Reset the time variable. */
					time_in_query_mode = -1.0;
					}
				
				if (TORA_TRACE)
					tora_trace_print ("Found the route.");
				}
				FSM_PROFILE_SECTION_OUT (state7_enter_exec)

			/** state (IP processing) exit executives **/
			FSM_STATE_EXIT_FORCED (7, "IP processing", "manet_tora [IP processing exit execs]")


			/** state (IP processing) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "IP processing", "Valid Route", "tr_55", "manet_tora [IP processing -> Valid Route : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (5,"manet_tora")
		}
	}




void
_op_manet_tora_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_manet_tora_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		int				list_size, index;
		ToraT_Height*	tmp_neighbor;
		
		/* Print the neighbor information. */
		printf ("\n\tCurrent Next Hop Router ID:%d", next_hop_rid);
		printf ("\n\ttau\t\toid\tr\tdelta\tid\tdirection\tupdated");
		printf ("\n\t=========================================================");
			
		printf ("\n\t%.3f\t\t%d\t%d\t%d\t%d\t%s\t%.3f", my_height.tau,
			my_height.oid, my_height.r, my_height.delta, my_height.id,
			ToraS_Link_Status [my_height.link_status], my_height.update_time);
		
		list_size = op_prg_list_size (neighbor_info_lptr);
		for (index=0; index < list_size; index++)
			{
			tmp_neighbor = (ToraT_Height*) op_prg_list_access (neighbor_info_lptr, index);
			
			printf ("\n\t%.3f\t\t%d\t%d\t%d\t%d\t%s\t%.3f", tmp_neighbor->tau,
				tmp_neighbor->oid, tmp_neighbor->r, tmp_neighbor->delta, tmp_neighbor->id,
				ToraS_Link_Status [tmp_neighbor->link_status], tmp_neighbor->update_time);
			}
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_manet_tora_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_manet_tora_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_manet_tora_svar function. */
#undef router_id
#undef parmem_ptr
#undef mode_of_operation
#undef opt_interval
#undef unroutable_ip_pkt_lptr
#undef ip_flush_timer_evhndl
#undef self_phndl
#undef neighbor_info_lptr
#undef my_height
#undef num_dnstrm
#undef my_node_objid
#undef next_hop_addr
#undef next_hop_rid
#undef animation_enabled
#undef circle_did_array
#undef route_discovery_delay_sthndl_ptr
#undef ip_packet_dropped_local_sthndl_ptr
#undef ip_packet_dropped_global_sthndl_ptr
#undef query_mode
#undef time_in_query_mode
#undef dest_rid
#undef my_intf_addr
#undef last_upd_tx_time
#undef last_linkup_time
#undef ip_queue_size_sthndl_ptr
#undef infinite_loop_prevention_flag

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_manet_tora_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_manet_tora_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (manet_tora)",
		sizeof (manet_tora_state));
	*init_block_ptr = 10;

	FRET (obtype)
	}

VosT_Address
_op_manet_tora_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	manet_tora_state * ptr;
	FIN_MT (_op_manet_tora_alloc (obtype))

	ptr = (manet_tora_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "manet_tora [Init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_manet_tora_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	manet_tora_state		*prs_ptr;

	FIN_MT (_op_manet_tora_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (manet_tora_state *)gen_ptr;

	if (strcmp ("router_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->router_id);
		FOUT
		}
	if (strcmp ("parmem_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->parmem_ptr);
		FOUT
		}
	if (strcmp ("mode_of_operation" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mode_of_operation);
		FOUT
		}
	if (strcmp ("opt_interval" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->opt_interval);
		FOUT
		}
	if (strcmp ("unroutable_ip_pkt_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->unroutable_ip_pkt_lptr);
		FOUT
		}
	if (strcmp ("ip_flush_timer_evhndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ip_flush_timer_evhndl);
		FOUT
		}
	if (strcmp ("self_phndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->self_phndl);
		FOUT
		}
	if (strcmp ("neighbor_info_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighbor_info_lptr);
		FOUT
		}
	if (strcmp ("my_height" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_height);
		FOUT
		}
	if (strcmp ("num_dnstrm" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_dnstrm);
		FOUT
		}
	if (strcmp ("my_node_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_node_objid);
		FOUT
		}
	if (strcmp ("next_hop_addr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->next_hop_addr);
		FOUT
		}
	if (strcmp ("next_hop_rid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->next_hop_rid);
		FOUT
		}
	if (strcmp ("animation_enabled" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->animation_enabled);
		FOUT
		}
	if (strcmp ("circle_did_array" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->circle_did_array);
		FOUT
		}
	if (strcmp ("route_discovery_delay_sthndl_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_discovery_delay_sthndl_ptr);
		FOUT
		}
	if (strcmp ("ip_packet_dropped_local_sthndl_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ip_packet_dropped_local_sthndl_ptr);
		FOUT
		}
	if (strcmp ("ip_packet_dropped_global_sthndl_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ip_packet_dropped_global_sthndl_ptr);
		FOUT
		}
	if (strcmp ("query_mode" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->query_mode);
		FOUT
		}
	if (strcmp ("time_in_query_mode" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->time_in_query_mode);
		FOUT
		}
	if (strcmp ("dest_rid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dest_rid);
		FOUT
		}
	if (strcmp ("my_intf_addr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_intf_addr);
		FOUT
		}
	if (strcmp ("last_upd_tx_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_upd_tx_time);
		FOUT
		}
	if (strcmp ("last_linkup_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_linkup_time);
		FOUT
		}
	if (strcmp ("ip_queue_size_sthndl_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ip_queue_size_sthndl_ptr);
		FOUT
		}
	if (strcmp ("infinite_loop_prevention_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->infinite_loop_prevention_flag);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

