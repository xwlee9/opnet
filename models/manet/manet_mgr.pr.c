/* Process model C form file: manet_mgr.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char manet_mgr_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE2 5C91EEE2 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <manet.h>
#include <ip_rte_support.h>
#include <ip_rte_v4.h>

#define DSR					(manet_rte_protocol == IpC_Rte_Dsr)
#define TORA				(manet_rte_protocol == IpC_Rte_Tora)
#define AODV				(manet_rte_protocol == IpC_Rte_Aodv)
#define GRP					(manet_rte_protocol == IpC_Rte_Grp)

#define LTRACE_ACTIVE		(op_prg_odb_ltrace_active ("manet"))

/* Prototypes	*/
static void					manet_mgr_sv_init (void);
static void					manet_mgr_routing_protocol_determine (void);
static void					manet_mgr_routing_process_create (void);
static void					manet_mgr_error (char* str1, char* str2, char* str3);

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
	Prohandle	              		own_prohandle                                   ;	/* Own process handle  */
	IpT_Rte_Protocol	       		manet_rte_protocol                              ;	/* Type of MANET routing protocol running on this node  */
	Prohandle	              		child_prohandle                                 ;	/* Process handle for the child routing process  */
	Objid	                  		own_mod_objid                                   ;	/* Module Objid  */
	Objid	                  		own_node_objid                                  ;	/* Node Objid  */
	char	                   		proc_model_name [32]                            ;	/* Process Model Name  */
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;	/* Module Memory for the IP module  */
	Prohandle	              		parent_prohandle                                ;	/* Process handle of ip_dispatch  */
	int	                    		child_pro_id                                    ;	/* Process ID of the child routing process  */
	} manet_mgr_state;

#define own_prohandle           		op_sv_ptr->own_prohandle
#define manet_rte_protocol      		op_sv_ptr->manet_rte_protocol
#define child_prohandle         		op_sv_ptr->child_prohandle
#define own_mod_objid           		op_sv_ptr->own_mod_objid
#define own_node_objid          		op_sv_ptr->own_node_objid
#define proc_model_name         		op_sv_ptr->proc_model_name
#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define parent_prohandle        		op_sv_ptr->parent_prohandle
#define child_pro_id            		op_sv_ptr->child_pro_id

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	manet_mgr_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((manet_mgr_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
manet_mgr_sv_init (void)
	{
	OmsT_Pr_Handle		own_process_record_handle;
	
	/** Initializes the state variables	**/
	FIN (manet_mgr_sv_init (void));
	
	/* Access the module data memory	*/
	module_data_ptr = (IpT_Rte_Module_Data*) op_pro_modmem_access ();
	
	/* Obtain the module Objid	*/
	own_mod_objid = op_id_self ();

	/* Obtain the node's objid. */
	own_node_objid = op_topo_parent (own_mod_objid);

	/* Obtain own process handle. */
	own_prohandle = op_pro_self ();
	
	/* Obtain parent process handle	*/
	parent_prohandle = op_pro_parent (own_prohandle);
	
	/* Obtain the name of the process. It is the "process model" attribute of the node. */
	op_ima_obj_attr_get (own_mod_objid, "process model", proc_model_name);

	/* Register the process in the model-wide registry. */
	own_process_record_handle = (OmsT_Pr_Handle) 
		oms_pr_process_register (own_node_objid, own_mod_objid, own_prohandle, proc_model_name);

	/* Register the protocol attribute in the registry. */
	oms_pr_attr_set (own_process_record_handle, 
		"protocol",			OMSC_PR_STRING,		"manet", 
		OPC_NIL);
	
	FOUT;
	}


static void
manet_mgr_routing_protocol_determine (void)
	{
	int						count;
	List*					routing_proto_lptr = OPC_NIL;
	IpT_Interface_Info*		interface_info_ptr = OPC_NIL;
	char					routing_protocol_str [128];
	
	/** Determines the MANET routing protocol	**/
	/** running on this node. Only one MANET	**/
	/** routing protocol can currently run on 	**/
	/** all interfaces that have MANET enabled	**/
	FIN (manet_mgr_routing_protocol_determine (void));
	
	/* Check if any interface is running any of the	*/
	/* MANET routing protocols						*/
	for (count = 0; count < inet_rte_num_interfaces_get (module_data_ptr); count++)
		{
		/* Get each interface information	*/
		interface_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, count);
		
		/* Get the routing protocols running on the interfaces	*/
		routing_proto_lptr = ip_rte_intf_routing_prot_get (interface_info_ptr);
		
		/* Determine the MANET routing running on the interface	*/
		if (ip_interface_routing_protocols_contains (routing_proto_lptr, IpC_Rte_Dsr) == OPC_TRUE)
			{
			manet_rte_protocol = IpC_Rte_Dsr;
			sprintf (routing_protocol_str, "MANET Routing Protocol : Dynamic Source Routing (DSR)");
			break;
			}
		else if (ip_interface_routing_protocols_contains (routing_proto_lptr, IpC_Rte_Tora) == OPC_TRUE)
			{
			manet_rte_protocol = IpC_Rte_Tora;
			sprintf (routing_protocol_str, "MANET Routing Protocol : Temporarily Ordered Routing Algorithm (TORA)");
			break;
			}
		else if (ip_interface_routing_protocols_contains (routing_proto_lptr, IpC_Rte_Aodv) == OPC_TRUE)
			{
			manet_rte_protocol = IpC_Rte_Aodv;
			sprintf (routing_protocol_str, "MANET Routing Protocol : Ad-Hoc On-Demand Distance Vector Routing (AODV)");
			break;
			}
		else if (ip_interface_routing_protocols_contains (routing_proto_lptr, IpC_Rte_Grp) == OPC_TRUE)
			{
			manet_rte_protocol = IpC_Rte_Grp;
			sprintf (routing_protocol_str, "MANET Routing Protocol : Geographic Routing Protocol (GRP)");
			break;
			}
		}
	
	/* Set the process tag to the routing protocol running	*/
	op_pro_tag_set (own_prohandle, routing_protocol_str);
	
	FOUT;
	}


static void
manet_mgr_routing_process_create (void)
	{
	/** Spawns the appropriate MANET routing process	**/
	FIN (manet_mgr_routing_process_create (void));
	
	switch (manet_rte_protocol)
		{
		case (IpC_Rte_Dsr):
			{
			/* Create the process for dynamic source routing protocol	*/
			child_prohandle = op_pro_create ("dsr_rte", OPC_NIL /* No data passed */);

			/* Invoke the process so that can initialize itself	*/
			op_pro_invoke (child_prohandle, OPC_NIL);
			
			/* Get the process ID of the DSR process	*/
			child_pro_id = op_pro_id (child_prohandle);
			
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Spawning DSR Routing process", OPC_NIL, OPC_NIL);
				}
			
			break;
			}
		case (IpC_Rte_Tora):
			{
			/* Create the Tora Imep Manager process model */
			child_prohandle = op_pro_create ("manet_tora_imep_mgr", OPC_NIL);
			
			/* Thereafter invoke the process */
			op_pro_invoke (child_prohandle, OPC_NIL);
			
			/* Get the process ID of the Tora_Imep Manager process */
			child_pro_id = op_pro_id (child_prohandle);
			
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Spawning Tora Routing process", OPC_NIL, OPC_NIL);
				}
			
			break;
			}
		case (IpC_Rte_Aodv):
			{
			/* Create the Tora Imep Manager process model */
			child_prohandle = op_pro_create ("aodv_rte", OPC_NIL);
			
			/* Thereafter invoke the process */
			op_pro_invoke (child_prohandle, OPC_NIL);
			
			/* Get the process ID of the Tora_Imep Manager process */
			child_pro_id = op_pro_id (child_prohandle);
			
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Spawning AODV Routing process", OPC_NIL, OPC_NIL);
				}
			
			break;
			}
		case (IpC_Rte_Grp):
			{
			/* Create the GRP process model */
			child_prohandle = op_pro_create ("grp_rte", OPC_NIL);
			
			/* Thereafter invoke the process */
			op_pro_invoke (child_prohandle, OPC_NIL);
			
			/* Get the process ID of the GRP process */
			child_pro_id = op_pro_id (child_prohandle);
			
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Spawning GRP Routing process", OPC_NIL, OPC_NIL);
				}
			
			break;
			}
		default:
			{
			manet_mgr_error ("Invalid MANET routing protocol specified", OPC_NIL, OPC_NIL);
			}
		}
	
	FOUT;
	}


static void
manet_mgr_error (char* str1, char* str2, char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (manet_rte_error <args>);
	
	op_sim_end ("MANET Routing : ", str1, str2, str3);
	
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
	void manet_mgr (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_manet_mgr_init (int * init_block_ptr);
	void _op_manet_mgr_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_manet_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_manet_mgr_alloc (VosT_Obtype, int);
	void _op_manet_mgr_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
manet_mgr (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (manet_mgr ());

		{
		/* Temporary Variables */
		Prohandle		invoker_phandle;
		int				invoker_id;
		int				inv_mode;
		Packet*			pkptr = OPC_NIL;
		/* End of Temporary Variables */


		FSM_ENTER_NO_VARS ("manet_mgr")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "manet_mgr [init enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_mgr [init enter execs]", state0_enter_exec)
				{
				/* Initialize the state variables	*/
				manet_mgr_sv_init ();
				
				/* Determine the MANET routing protocol running	*/
				/* on this node. All interfaces that have MANET	*/
				/* enabled should have the same MANET routing 	*/
				/* running on them. Currently ,a node cannot 	*/
				/* have different/various MANET routing 		*/
				/* protocols running on different interfaces	*/
				manet_mgr_routing_protocol_determine ();
				
				/* Create and invoke the appropriate MANET		*/
				/* routing process that has been configured		*/
				manet_mgr_routing_process_create ();
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "manet_mgr [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "wait", "tr_11", "manet_mgr [init -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "manet_mgr [wait enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_mgr [wait enter execs]", state1_enter_exec)
				{
				/* Waiting for invokations from other processes	*/
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"manet_mgr")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "manet_mgr [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_mgr [wait exit execs]", state1_exit_exec)
				{
				/* Received a invocation from another process	*/
				/* Check if the process was invoked from the 	*/
				/* child or it was invoked from one of the CPUs	*/
				invoker_phandle = op_pro_invoker (own_prohandle, &inv_mode);
				invoker_id = op_pro_id (invoker_phandle);
				
				if (inv_mode == OPC_PROINV_INDIRECT)
					{
					/* Access the argument memory	*/
					pkptr = (Packet*) op_pro_argmem_access ();
					
					/* Check if this was invoked by the child process	*/
					if (invoker_id == child_pro_id)
						{
						/* The child routing process invoked this		*/
						/* parent process. Pass the invocation to IP	*/
						op_pro_invoke (parent_prohandle, pkptr);
						}
					else
						{
						/* This process was invoked by the CPU process	*/
						/* Pass the invocation to the child process		*/
						op_pro_invoke (child_prohandle, pkptr);
						}
					}
				else
					{
					/* This process was invoked directly by the kernel	*/
					/* This process should never be invoked directly	*/
					/* Terminate the simulation as unknown interrupt	*/
					op_sim_end ("MANET manager : Unknown interrupt received", OPC_NIL, OPC_NIL, OPC_NIL);
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "wait", "wait", "tr_12", "manet_mgr [wait -> wait : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"manet_mgr")
		}
	}




void
_op_manet_mgr_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_manet_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_manet_mgr_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_manet_mgr_svar function. */
#undef own_prohandle
#undef manet_rte_protocol
#undef child_prohandle
#undef own_mod_objid
#undef own_node_objid
#undef proc_model_name
#undef module_data_ptr
#undef parent_prohandle
#undef child_pro_id

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_manet_mgr_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_manet_mgr_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (manet_mgr)",
		sizeof (manet_mgr_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_manet_mgr_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	manet_mgr_state * ptr;
	FIN_MT (_op_manet_mgr_alloc (obtype))

	ptr = (manet_mgr_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "manet_mgr [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_manet_mgr_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	manet_mgr_state		*prs_ptr;

	FIN_MT (_op_manet_mgr_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (manet_mgr_state *)gen_ptr;

	if (strcmp ("own_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_prohandle);
		FOUT
		}
	if (strcmp ("manet_rte_protocol" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->manet_rte_protocol);
		FOUT
		}
	if (strcmp ("child_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->child_prohandle);
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
	if (strcmp ("proc_model_name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->proc_model_name);
		FOUT
		}
	if (strcmp ("module_data_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->module_data_ptr);
		FOUT
		}
	if (strcmp ("parent_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->parent_prohandle);
		FOUT
		}
	if (strcmp ("child_pro_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->child_pro_id);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

