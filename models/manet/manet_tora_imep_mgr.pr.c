/* Process model C form file: manet_tora_imep_mgr.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char manet_tora_imep_mgr_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE3 5C91EEE3 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <manet_tora_imep.h>
#include <manet.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <ip_higher_layer_proto_reg_sup.h>

#define		ToraS_Tora_Proc_Name			"manet_tora"
#define		ToraS_Imep_Proc_Name			"manet_imep"

#define 	LTRACE_TORA_IMEP_MGR_ACTIVE		(op_prg_odb_ltrace_active ("manet_tora_imep_mgr"))

/* For transition conditions.. */
#define		PK_FOR_IMEP				is_pk_for_imep
#define		TORA_PK_ARRIVAL			is_tora_pk_arrival
#define		UNROUTABLE_IP_PK		is_unroutable_ip_pk
#define		LINK_STATUS_CHANGE		is_link_status_change
#define		IMEP_PK_FOR_IP			is_imep_pk_for_ip
#define		IP_PK_FOR_IP			is_ip_pk_for_ip

/* structure used to bookeep multiple tora processes. */
typedef struct
	{
	Prohandle		phndl;
	ToraT_Invocation_Info*	ptc_mem_ptr;
	}
TImgrT_Tora_Proc_Info;

static void tora_imep_mgr_tora_proc_init (ToraC_Invocation_Code, IpT_Address, Packet*, LinkState);
static Compcode	tora_imep_mgr_tora_proc_find (IpT_Address, int*);


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
	int	                    		router_id                                       ;	/* Unique router id for a given AD-HOC network  */
	TImgrT_Tora_Proc_Info*	 		tora_proc_array [ToraC_Max_Domain_Node_Num]     ;	/* Array of TORA child processes spawned off  */
	int	                    		tora_proc_num                                   ;	/* Total number of local TORA processes  */
	Prohandle	              		imep_phndl                                      ;	/* local IMEP process handle  */
	Objid	                  		nd_objid                                        ;
	Objid	                  		tora_imep_compound_attr                         ;
	Objid	                  		tora_my_params_attr_id                          ;
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;
	Prohandle	              		manet_mgr_prohandle                             ;
	int	                    		manet_mgr_pro_id                                ;
	int	                    		ip_flush_timeout                                ;
	IpT_Interface_Info*	    		intf_info_ptr                                   ;	/* Local interface where TORA is set up  */
	Boolean	                		efficiency                                      ;	/* Flag for tora imep efficiency mode  */
	Prohandle	              		self_phndl                                      ;
	int	                    		tora_port_index                                 ;
	int	                    		strm_from_tora_interface                        ;
	} manet_tora_imep_mgr_state;

#define router_id               		op_sv_ptr->router_id
#define tora_proc_array         		op_sv_ptr->tora_proc_array
#define tora_proc_num           		op_sv_ptr->tora_proc_num
#define imep_phndl              		op_sv_ptr->imep_phndl
#define nd_objid                		op_sv_ptr->nd_objid
#define tora_imep_compound_attr 		op_sv_ptr->tora_imep_compound_attr
#define tora_my_params_attr_id  		op_sv_ptr->tora_my_params_attr_id
#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define manet_mgr_prohandle     		op_sv_ptr->manet_mgr_prohandle
#define manet_mgr_pro_id        		op_sv_ptr->manet_mgr_pro_id
#define ip_flush_timeout        		op_sv_ptr->ip_flush_timeout
#define intf_info_ptr           		op_sv_ptr->intf_info_ptr
#define efficiency              		op_sv_ptr->efficiency
#define self_phndl              		op_sv_ptr->self_phndl
#define tora_port_index         		op_sv_ptr->tora_port_index
#define strm_from_tora_interface		op_sv_ptr->strm_from_tora_interface

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	manet_tora_imep_mgr_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((manet_tora_imep_mgr_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void 
tora_imep_mgr_tora_proc_init (ToraC_Invocation_Code code, IpT_Address address, Packet* pk_ptr, LinkState link_state)
	{
	int								rid;
	ToraT_Topo_Struct*				tmp_topo_struct_ptr;
	Boolean							router_intf_addr;
	
	/** PURPOSE: Create a new TORA process. 		**/
	/** REQUIRES: Invocation code. 					**/
	/** EFFECTS: New proc is created and stored. 	**/
	FIN (tora_imep_mgr_tora_proc_init (code, address, pk_ptr, link_state));
	
	/* See if the address is one of the interface of TORA router roaming interfafce or the local LAN one. */
	
	if (!(tmp_topo_struct_ptr = tora_imep_sup_ip_addr_to_rid (&rid, address)))
		{
		/* Try locating the router with the LAN address. */
		if (!(tmp_topo_struct_ptr = tora_imep_lan_addr_to_router (address)))
			{
			/* Something is wrong. */
			op_pk_destroy (pk_ptr);
			FOUT;
			}
		else
			{
			router_intf_addr = OPC_FALSE;
			}
		}
	else
		{
		router_intf_addr = OPC_TRUE;
		}
			
	/* Prepare structures... */
	tora_proc_array [tora_proc_num] = (TImgrT_Tora_Proc_Info *) op_prg_mem_alloc (sizeof (TImgrT_Tora_Proc_Info));
	tora_proc_array [tora_proc_num]->ptc_mem_ptr = (ToraT_Invocation_Info *) op_prg_mem_alloc (sizeof (ToraT_Invocation_Info));
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->code = code;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->pk_ptr = pk_ptr;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->parent_phndl = op_pro_self ();
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->ip_flush_time = ip_flush_timeout;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->ip_rte_table = module_data_ptr->ip_route_table;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->net_mask = ip_address_create ("255.255.255.255");
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->lan_addr_range_ptr = OPC_NIL;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->lan_addr = IPC_ADDR_INVALID;
	tora_proc_array [tora_proc_num]->ptc_mem_ptr->link_state = link_state;
	
	if (code == ToraC_Invocation_Code_Destination)
		{
		tora_proc_array [tora_proc_num]->ptc_mem_ptr->address = address;
		tora_proc_array [tora_proc_num]->ptc_mem_ptr->destination_rid = router_id;
		}
	else
		{
		if (router_intf_addr)
			{
			tora_proc_array [tora_proc_num]->ptc_mem_ptr->address = address;
			}
		else
			{
			tora_proc_array [tora_proc_num]->ptc_mem_ptr->address = tmp_topo_struct_ptr->ip_addr;
			}
		
		tora_proc_array [tora_proc_num]->ptc_mem_ptr->destination_rid = tmp_topo_struct_ptr->rid;
		
		if (tmp_topo_struct_ptr->lan_intf_info_ptr)
			{
			tora_proc_array [tora_proc_num]->ptc_mem_ptr->lan_addr_range_ptr = 
				ip_rte_intf_addr_range_get (tmp_topo_struct_ptr->lan_intf_info_ptr);
			tora_proc_array [tora_proc_num]->ptc_mem_ptr->lan_addr = 
				ip_rte_intf_network_address_get (tmp_topo_struct_ptr->lan_intf_info_ptr);
			}
		}
	
	/* Create and invoke the process for initialization. */
	tora_proc_array [tora_proc_num]->phndl = op_pro_create
		(ToraS_Tora_Proc_Name, tora_proc_array [tora_proc_num]->ptc_mem_ptr);
	op_pro_invoke (tora_proc_array [tora_proc_num]->phndl, OPC_NIL);
	
	/* Invoke it one more time to handle current packets. */
	op_pro_invoke (tora_proc_array [tora_proc_num]->phndl, OPC_NIL);
	
	/* Update the proc count. */
	tora_proc_num++;
	
	FOUT;
	}

static Compcode
tora_imep_mgr_tora_proc_find (IpT_Address address, int*	index_ptr)
	{
	int					index;
	
	/** PURPOSE: Search through the local process which handles particular detination. **/
	/** REQUIRES: Destination address. 												   **/
	/** EFFECTS: The index for the destination or -1 if not found is returned. 		   **/
	FIN (tora_imep_mgr_tora_proc_find (address));
	
	for (index=0; index < tora_proc_num; index++)
		{
		if (tora_proc_array [index]->ptc_mem_ptr->address == address)
			{
			/* Found the process. */
			*index_ptr = index;
			FRET (OPC_COMPCODE_SUCCESS);
			}
		}
	
	for (index=0; index < tora_proc_num; index++)
		{
		/* Try the LAN side of this router. */
		if (tora_proc_array [index]->ptc_mem_ptr->lan_addr_range_ptr)
			{
			if (ip_address_range_check (address, 
				tora_proc_array [index]->ptc_mem_ptr->lan_addr_range_ptr))
				{
				/* Found the process. */
				*index_ptr = index;
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
	void manet_tora_imep_mgr (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_manet_tora_imep_mgr_init (int * init_block_ptr);
	void _op_manet_tora_imep_mgr_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_manet_tora_imep_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_manet_tora_imep_mgr_alloc (VosT_Obtype, int);
	void _op_manet_tora_imep_mgr_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
manet_tora_imep_mgr (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (manet_tora_imep_mgr ());

		{
		/* Temporary Variables */
		Boolean								is_pk_for_imep =OPC_FALSE, is_tora_pk_arrival=OPC_FALSE,
											is_unroutable_ip_pk=OPC_FALSE, is_link_status_change=OPC_FALSE, 
											is_imep_pk_for_ip = OPC_FALSE, is_ip_pk_for_ip = OPC_FALSE;
		Objid 								tmp_objid, module_id;
		char 								temp_node_name [256], msg [256];
		Packet* 							pkptr;
		
		Prohandle							invoker_phandle;
		int									invoker_id, tmp_rid;
		ToraC_Invocation_Code				invocation_code;
		int									inv_mode, destination, tora_proc_index;
		LinkState							tmp_link_state;
		
		ToraImepMgrT_Invocation_Struct* 	argmem_ptr;
		IpT_Dgram_Fields*					pk_fd_ptr;
		IpT_Interface_Info*					second_intf_info_ptr;
		
		IpT_Address							neighbor;
		Boolean								neighbor_found = OPC_FALSE;
		
		IpT_Interface_Info*					intf_ptr;
		int 								counter;
		Ici* 								ip_encap_ici_ptr;
		/* End of Temporary Variables */


		FSM_ENTER ("manet_tora_imep_mgr")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (Init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "Init", "manet_tora_imep_mgr [Init enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Init enter execs]", state0_enter_exec)
				{
				/* Access some of the basic vars first */
				module_id = op_id_self ();
				nd_objid = op_topo_parent (module_id);
				
				/* Resolve the router id in case they are not explicitly set. */
				op_ima_obj_attr_get (module_id, "manet_mgr.TORA/IMEP Parameters", &tora_imep_compound_attr);
				
				tora_my_params_attr_id = op_topo_child (tora_imep_compound_attr, OPC_OBJTYPE_GENERIC, 0);
				op_ima_obj_attr_get (tora_my_params_attr_id, "Router ID", &router_id);
				
				module_data_ptr = ip_support_module_data_get (nd_objid);
				
				/* Determine the interface where TORA is running */
				for (counter = 0; counter < ip_rte_num_interfaces_get (module_data_ptr); counter ++)
					{
					/* Get the interface pointer */
					intf_ptr = ip_rte_intf_tbl_access (module_data_ptr, counter);
					
					/* Get the list of routing protocols on the interface */
					if (ip_interface_routing_protocols_contains (ip_rte_intf_routing_prot_get (intf_ptr), IpC_Rte_Tora))
						{
						/* Assumption: We only support TORA running on a single interface currently. The two types of */
						/* nodes that are supported are gateways (with ethernet and wireless interfaces) and hosts 	  */
						/* with wireless interfaces. In both cases, the wireless interface is the only interface 	  */
						/* running TORA.																			  */
						tora_port_index = counter;
						break;
						}
					}
				
				strm_from_tora_interface = (int) ip_rte_intf_in_port_num_get (intf_ptr);
				
				intf_info_ptr = ip_rte_intf_tbl_access (module_data_ptr, tora_port_index);
				
				self_phndl = op_pro_self ();
				manet_mgr_prohandle = op_pro_parent (self_phndl);
				manet_mgr_pro_id = op_pro_id (manet_mgr_prohandle);
				
				/* We support single domain for now. */
				tora_imep_sup_router_id_assign (0, router_id, nd_objid, module_id);
				
				/* Initialize the index variable for the TORA processses. */
				tora_proc_num = 0;
				
				/* Resolve TORA related attributes. */
				op_ima_obj_attr_get (tora_my_params_attr_id, "TORA Parameters", &tmp_objid);
				tmp_objid = op_topo_child (tmp_objid, OPC_OBJTYPE_GENERIC, 0);
				op_ima_obj_attr_get (tmp_objid, "IP Packet Discard Timeout", &ip_flush_timeout);
				
				/* See if the efficiency mode is on. */
				op_ima_sim_attr_get (OPC_IMA_TOGGLE, "TORA IMEP Efficiency", &efficiency);
				
				/* Schedule the self interrupt to move to the exit execs */
				op_intrpt_schedule_self (op_sim_time (), -1);
				
				/* Register ODB command for this. */
				if (op_sim_debug ())
					{
					tora_imep_sup_debug_comm_reg ();
					}
				
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"manet_tora_imep_mgr")


			/** state (Init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "Init", "manet_tora_imep_mgr [Init exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Init exit execs]", state0_exit_exec)
				{
				/* Now it shoud have a unique router id value. */
				op_ima_obj_attr_get (tora_my_params_attr_id, "Router ID", &router_id);
				
				/* Spawn local desitnation IMEP process. */
				imep_phndl = op_pro_create (ToraS_Imep_Proc_Name, OPC_NIL);
				
				/* Register the router id in the global list. */
				if (!module_data_ptr->gateway_status)
					{
					/* Single physical interface. */
					tora_imep_sup_rid_register (nd_objid, op_id_self (), router_id, intf_info_ptr->addr_range_ptr->address, 
						OPC_NIL, op_pro_id (op_pro_self ()), op_pro_id (imep_phndl), strm_from_tora_interface);
					}
				else
					{
					/* Gateway node.  We currently support only one more. */
					/* So the port for the other interface is the one where TORA is not running */
					if (tora_port_index == 0)
						{
						second_intf_info_ptr = ip_rte_intf_tbl_access (module_data_ptr, 1); 
						}
					else
						{
						second_intf_info_ptr = ip_rte_intf_tbl_access (module_data_ptr, 0);
						}
					
					/*tora_imep_sup_rid_register (nd_objid, op_id_self (), router_id, intf_info_ptr->addr_range_ptr->address, 
						second_intf_info_ptr, op_pro_id (op_pro_self ()), op_pro_id (imep_phndl)); */
					
					tora_imep_sup_rid_register (nd_objid, op_id_self (), router_id, intf_info_ptr->addr_range_ptr->address, 
						second_intf_info_ptr, op_pro_id (op_pro_self ()), op_pro_id (imep_phndl), strm_from_tora_interface); 
				
					}
				
				/* Spawn local desitnation TORA process. */
				tora_imep_mgr_tora_proc_init (ToraC_Invocation_Code_Destination,
					intf_info_ptr->addr_range_ptr->address, OPC_NIL, ImepC_LinkState_Unknown);
				
				
				/* Invoke these resident procs for the first time. */
				op_pro_invoke (imep_phndl, OPC_NIL);
				
				/* Initialize custom animation. */
				if (LTRACE_TORA_IMEP_MGR_ACTIVE)
					{
					op_ima_obj_attr_get (nd_objid, "name", temp_node_name);
					op_prg_odb_print_major ("Spawning IMEP process", temp_node_name, OPC_NIL);
					}
				}
				FSM_PROFILE_SECTION_OUT (state0_exit_exec)


			/** state (Init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Init", "Idle", "tr_-1", "manet_tora_imep_mgr [Init -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "Idle", state1_enter_exec, "manet_tora_imep_mgr [Idle enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"manet_tora_imep_mgr")


			/** state (Idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "Idle", "manet_tora_imep_mgr [Idle exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Idle exit execs]", state1_exit_exec)
				{
				/* Find out the kind of invocation that this manager has received */
				invoker_phandle = op_pro_invoker (self_phndl, &inv_mode);
				invoker_id = op_pro_id (invoker_phandle);
				
				if (invoker_id == manet_mgr_pro_id)
					{
					/* Its a packet that IP is providing. Check the protocol type. */
					pkptr = (Packet*) op_pro_argmem_access ();
					op_pk_nfd_access (pkptr, "fields", &pk_fd_ptr);
				
					if (pk_fd_ptr->protocol == IpC_Protocol_Tora)
						{
						is_pk_for_imep = OPC_TRUE;
						}
					else
						{
						is_unroutable_ip_pk = OPC_TRUE;
						}
					}
				else
					{
					/* Either TORA or IMEP can be the invoker.  Check the code passed with the argument. */
					argmem_ptr = (ToraImepMgrT_Invocation_Struct*) op_pro_argmem_access ();
					
					switch (argmem_ptr->code)
						{
						case ToraImepMgrC_Invocation_Code_Pk_From_Imep_For_IP:
							{
							/* Packet from the IMEP child process, time to forward to IP */
							is_imep_pk_for_ip = OPC_TRUE;
							pkptr = (Packet*) argmem_ptr->pk_ptr;
				
							break;
							}
							
						case ToraImepMgrC_Invocation_Code_Pk_From_Imep_For_ULP:
							{
							/* Packet from the IMEP child process, meant for TORA */
							is_tora_pk_arrival = OPC_TRUE;
							pkptr = (Packet*) argmem_ptr->pk_ptr;
				
							break;
							}
							
						case ToraImepMgrC_Invocation_Code_Pk_From_Tora_For_IMEP:
							{
							/* Pakcet from one of the TORA child processes. */
							is_pk_for_imep = OPC_TRUE;
							pkptr = (Packet*) argmem_ptr->pk_ptr;
				
							break;
							}
							
						case ToraImepMgrC_Invocation_Code_Pk_From_Tora_For_IP:
							{
							/* Pakcet from one of the TORA child processes. */
							is_ip_pk_for_ip = OPC_TRUE;
							pkptr = (Packet*) argmem_ptr->pk_ptr;
				
							break;
							}
							
						case ToraImepMgrC_Invocation_Code_Link_Status_Update:
							{
							/* Link state update from IMEP. */
							is_link_status_change = OPC_TRUE;
							
							/* Aceess the information. */
							tmp_rid = argmem_ptr->m_router_id;
							tmp_link_state = argmem_ptr->link_state;
							}
						}
					
					op_prg_mem_free (argmem_ptr);
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (Idle) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Idle trans conditions]", state1_trans_conds)
			FSM_INIT_COND (PK_FOR_IMEP)
			FSM_TEST_COND (LINK_STATUS_CHANGE)
			FSM_TEST_COND (TORA_PK_ARRIVAL || UNROUTABLE_IP_PK)
			FSM_TEST_COND (IP_PK_FOR_IP || IMEP_PK_FOR_IP)
			FSM_TEST_LOGIC ("Idle")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "PK_FOR_IMEP", "", "Idle", "Forward to IMEP", "tr_7", "manet_tora_imep_mgr [Idle -> Forward to IMEP : PK_FOR_IMEP / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "LINK_STATUS_CHANGE", "", "Idle", "Invoke TORA Procs", "tr_9", "manet_tora_imep_mgr [Idle -> Invoke TORA Procs : LINK_STATUS_CHANGE / ]")
				FSM_CASE_TRANSIT (2, 4, state4_enter_exec, ;, "TORA_PK_ARRIVAL || UNROUTABLE_IP_PK", "", "Idle", "Invoke TORA", "tr_13", "manet_tora_imep_mgr [Idle -> Invoke TORA : TORA_PK_ARRIVAL || UNROUTABLE_IP_PK / ]")
				FSM_CASE_TRANSIT (3, 5, state5_enter_exec, ;, "IP_PK_FOR_IP || IMEP_PK_FOR_IP", "", "Idle", "Forward to IP", "tr_17", "manet_tora_imep_mgr [Idle -> Forward to IP : IP_PK_FOR_IP || IMEP_PK_FOR_IP / ]")
				}
				/*---------------------------------------------------------*/



			/** state (Forward to IMEP) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "Forward to IMEP", state2_enter_exec, "manet_tora_imep_mgr [Forward to IMEP enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Forward to IMEP enter execs]", state2_enter_exec)
				{
				/* Forward the packet to IMEP process for processing. */
				argmem_ptr = (ToraImepMgrT_Invocation_Struct*) op_prg_mem_alloc (sizeof (ToraImepMgrT_Invocation_Struct));
				argmem_ptr->code = ToraImepMgrC_Invocation_Code_Pk_From_Mgr;
				argmem_ptr->pk_ptr = pkptr;
				
				op_pro_invoke (imep_phndl, argmem_ptr);
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (Forward to IMEP) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "Forward to IMEP", "manet_tora_imep_mgr [Forward to IMEP exit execs]")


			/** state (Forward to IMEP) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Forward to IMEP", "Idle", "tr_8", "manet_tora_imep_mgr [Forward to IMEP -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Invoke TORA Procs) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "Invoke TORA Procs", state3_enter_exec, "manet_tora_imep_mgr [Invoke TORA Procs enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Invoke TORA Procs enter execs]", state3_enter_exec)
				{
				/* One of the link status changed.  Let all the TORA processes know. */
				for (tora_proc_index=0; tora_proc_index < tora_proc_num; tora_proc_index++)
					{
					/* Pass the information through the parmem. */
					tora_proc_array [tora_proc_index]->ptc_mem_ptr->code = 
						ToraC_Invocation_Code_Link_Status_Change;
					tora_proc_array [tora_proc_index]->ptc_mem_ptr->neighbor_rid = tmp_rid;
					tora_proc_array [tora_proc_index]->ptc_mem_ptr->link_state = tmp_link_state;
				
					op_pro_invoke (tora_proc_array [tora_proc_index]->phndl, OPC_NIL);
					
					if (tora_proc_array [tora_proc_index]->ptc_mem_ptr->destination_rid == tmp_rid)
						{
						neighbor_found = OPC_TRUE;
						}
					}
				
				if (!neighbor_found)
					{
					/* We do not yet have a TORA proc to handle the new found neighbor. */
					tora_imep_sup_rid_to_ip_addr (tmp_rid, &neighbor);
					tora_imep_mgr_tora_proc_init (ToraC_Invocation_Code_Link_Status_Change, neighbor, OPC_NIL, tmp_link_state);
					}
					
				
				if (LTRACE_TORA_IMEP_MGR_ACTIVE)
					{
					if (tmp_link_state == ImepC_LinkState_Up)
						{
						sprintf (msg, "Link status to RID:%d has been changed to UP.", tmp_rid);
						}
					else
						{
						sprintf (msg, "Link status to RID:%d has been changed to DOWN.", tmp_rid);
						}
						
					op_prg_odb_print_minor (msg, OPC_NIL);
					}
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (Invoke TORA Procs) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "Invoke TORA Procs", "manet_tora_imep_mgr [Invoke TORA Procs exit execs]")


			/** state (Invoke TORA Procs) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Invoke TORA Procs", "Idle", "tr_10", "manet_tora_imep_mgr [Invoke TORA Procs -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Invoke TORA) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "Invoke TORA", state4_enter_exec, "manet_tora_imep_mgr [Invoke TORA enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Invoke TORA enter execs]", state4_enter_exec)
				{
				if (is_unroutable_ip_pk)
					{
					/* Access the destination and see if we need to create a new TORA to handle this packet. */
					/* destination = pk_fd_ptr->dest_addr; */
					destination = pk_fd_ptr->dest_addr.address.ipv4_addr;
					invocation_code = ToraC_Invocation_Code_IP_PK;
					}
				else
					{
					/* Find and invoke TORA process based on the destination info from the packet. */
					op_pk_nfd_get (pkptr, "Destination", &destination);
					
					invocation_code = ToraC_Invocation_Code_Control_PK;
					}
				
				/* Search for the TORA proc with the address. */
				if (OPC_COMPCODE_SUCCESS == tora_imep_mgr_tora_proc_find (
					destination, &tora_proc_index))
					{
					/* Found the TORA proc to handle the packet. */
					tora_proc_array [tora_proc_index]->ptc_mem_ptr->code = invocation_code;
					tora_proc_array [tora_proc_index]->ptc_mem_ptr->pk_ptr = pkptr;
					
					op_pro_invoke (tora_proc_array [tora_proc_index]->phndl, OPC_NIL);
					}
				else
					{
					/* There is no process currently for this destination. Create a new one. */
					tora_imep_mgr_tora_proc_init (invocation_code, destination, pkptr, ImepC_LinkState_Unknown);
					}
				}
				FSM_PROFILE_SECTION_OUT (state4_enter_exec)

			/** state (Invoke TORA) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "Invoke TORA", "manet_tora_imep_mgr [Invoke TORA exit execs]")


			/** state (Invoke TORA) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Invoke TORA", "Idle", "tr_14", "manet_tora_imep_mgr [Invoke TORA -> Idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (Forward to IP) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "Forward to IP", state5_enter_exec, "manet_tora_imep_mgr [Forward to IP enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_tora_imep_mgr [Forward to IP enter execs]", state5_enter_exec)
				{
				/* Being invoked from manet_imep process model. The argument memory has the IP packet */
				/* that needs to be sent out.														  */
				if (efficiency && IMEP_PK_FOR_IP)
					{
					tora_imep_sup_packet_deliver (pkptr, nd_objid);
					}
				else
					{
					ip_encap_ici_ptr = op_ici_create ("ip_encap_req_v4");
					op_ici_attr_set (ip_encap_ici_ptr, "multicast_major_port", tora_port_index);
					op_ici_attr_set (ip_encap_ici_ptr, "multicast_minor_port", tora_port_index);
					op_ici_install (ip_encap_ici_ptr);
				
					op_pk_deliver (pkptr, module_data_ptr->module_id, module_data_ptr->instrm_from_ip_encap);
					}
				}
				FSM_PROFILE_SECTION_OUT (state5_enter_exec)

			/** state (Forward to IP) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "Forward to IP", "manet_tora_imep_mgr [Forward to IP exit execs]")


			/** state (Forward to IP) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Forward to IP", "Idle", "tr_18", "manet_tora_imep_mgr [Forward to IP -> Idle : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"manet_tora_imep_mgr")
		}
	}




void
_op_manet_tora_imep_mgr_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_manet_tora_imep_mgr_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		int			child_index, pro_id;
		char		ip_addr [32];
		
		/* Print out the current TORA child process information. */
		ip_address_print (ip_addr, intf_info_ptr->addr_range_ptr->address);
		printf ("\n\tTora Manager RID:%d\tIntf Addr:%s", router_id, ip_addr);
		printf ("\n\tChild TORA ID\tDestination Addr");
		printf ("\n\t======================================");
		for (child_index=0; child_index < tora_proc_num; child_index++)
			{
			pro_id = op_pro_id (tora_proc_array [child_index]->phndl);
			ip_address_print (ip_addr, tora_proc_array [child_index]->ptc_mem_ptr->address);
				
			printf ("\n\t%d\t\t%s\t%d", pro_id, ip_addr, tora_proc_array [child_index]->ptc_mem_ptr->address);
			}
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_manet_tora_imep_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_manet_tora_imep_mgr_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_manet_tora_imep_mgr_svar function. */
#undef router_id
#undef tora_proc_array
#undef tora_proc_num
#undef imep_phndl
#undef nd_objid
#undef tora_imep_compound_attr
#undef tora_my_params_attr_id
#undef module_data_ptr
#undef manet_mgr_prohandle
#undef manet_mgr_pro_id
#undef ip_flush_timeout
#undef intf_info_ptr
#undef efficiency
#undef self_phndl
#undef tora_port_index
#undef strm_from_tora_interface

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_manet_tora_imep_mgr_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_manet_tora_imep_mgr_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (manet_tora_imep_mgr)",
		sizeof (manet_tora_imep_mgr_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_manet_tora_imep_mgr_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	manet_tora_imep_mgr_state * ptr;
	FIN_MT (_op_manet_tora_imep_mgr_alloc (obtype))

	ptr = (manet_tora_imep_mgr_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "manet_tora_imep_mgr [Init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_manet_tora_imep_mgr_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	manet_tora_imep_mgr_state		*prs_ptr;

	FIN_MT (_op_manet_tora_imep_mgr_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (manet_tora_imep_mgr_state *)gen_ptr;

	if (strcmp ("router_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->router_id);
		FOUT
		}
	if (strcmp ("tora_proc_array" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->tora_proc_array);
		FOUT
		}
	if (strcmp ("tora_proc_num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tora_proc_num);
		FOUT
		}
	if (strcmp ("imep_phndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->imep_phndl);
		FOUT
		}
	if (strcmp ("nd_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->nd_objid);
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
	if (strcmp ("manet_mgr_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->manet_mgr_prohandle);
		FOUT
		}
	if (strcmp ("manet_mgr_pro_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->manet_mgr_pro_id);
		FOUT
		}
	if (strcmp ("ip_flush_timeout" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ip_flush_timeout);
		FOUT
		}
	if (strcmp ("intf_info_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->intf_info_ptr);
		FOUT
		}
	if (strcmp ("efficiency" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->efficiency);
		FOUT
		}
	if (strcmp ("self_phndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->self_phndl);
		FOUT
		}
	if (strcmp ("tora_port_index" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tora_port_index);
		FOUT
		}
	if (strcmp ("strm_from_tora_interface" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->strm_from_tora_interface);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

