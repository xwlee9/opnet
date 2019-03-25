/* Process model C form file: manet_rte_mgr.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char manet_rte_mgr_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE3 5C91EEE3 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <stdlib.h>
#include <opnet.h>
#include <oms_pr.h>
#include <ip_observer.h>
#include <ip_rte_support.h>
#include <ip_rte_v4.h>

#define OLSR				(manet_rte_protocol == IpC_Rte_Olsr)
#define IP_NOTIFICATION		((intrpt_type == OPC_INTRPT_REMOTE) && (intrpt_code == 0))
#define	FAILURE_RECOVERY	((intrpt_type == OPC_INTRPT_FAIL)  || (intrpt_type == OPC_INTRPT_RECOVER))
#define IDLE_CHILD			(intrpt_code == -10)
#define LTRACE_ACTIVE		(op_prg_odb_ltrace_active ("manet_rte_mgr"))

/* Prototypes	*/
static void					manet_rte_mgr_sv_init (void);
static void					manet_rte_mgr_routing_protocol_determine (void);
static void					manet_rte_mgr_packet_arrival (void);
static void					manet_rte_mgr_error (const char* str1, char* str2, char* str3);

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
	Prohandle	              		child_prohandle                                 ;	/* Process handle for the child routing process  */
	int	                    		child_pro_id                                    ;	/* Process ID of the child routing process  */
	} manet_rte_mgr_state;

#define child_prohandle         		op_sv_ptr->child_prohandle
#define child_pro_id            		op_sv_ptr->child_pro_id

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	manet_rte_mgr_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((manet_rte_mgr_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
manet_rte_mgr_sv_init (void)
	{
	Prohandle					own_prohandle;
	OmsT_Pr_Handle				own_process_record_handle;
	char						proc_model_name [128];
	Objid						own_mod_objid;
	Objid						own_node_objid;
		
	/** Initializes the state variables	**/
	FIN (manet_rte_mgr_sv_init (void));
	
	/* Set process model name. */
	strcpy (proc_model_name, "manet_rte_mgr");
		
	/* Obtain the module Objid	*/
	own_mod_objid = op_id_self ();

	/* Obtain the node's objid. */
	own_node_objid = op_topo_parent (own_mod_objid);
	
	/* Obtain own process handle. */
	own_prohandle = op_pro_self ();

	/* Register the process in the model-wide registry. */
	own_process_record_handle = (OmsT_Pr_Handle) 
		oms_pr_process_register (own_node_objid, own_mod_objid, own_prohandle, proc_model_name);
	
	/* Register the protocol attribute in the registry. */
	oms_pr_attr_set (own_process_record_handle, 
		"protocol",			OMSC_PR_STRING,		"manet_rte_mgr", 
		"module id", 		OMSC_PR_OBJID, 		own_mod_objid,
		OPC_NIL);
	
	FOUT;
	}


static void
manet_rte_mgr_routing_protocol_determine (void)
	{
	
	/** IP Notification has been received		**/
	/** Determine the MANET routing protocol	**/
	/** running on this node.  					**/
	/** Invoke the corresponding routing 		**/
	/** protocol.								**/
	/** Note: Only one MANET protocol can be	**/
	/** configured at any given time on a node	**/
	
	IpT_Rte_Module_Data* 	module_data_ptr = OPC_NIL;
	
	FIN (manet_rte_mgr_routing_protocol_determine (void));
	
	/*Get module data ptr */
	module_data_ptr = ip_support_module_data_get (op_topo_parent (op_id_self ()));
	
	switch (module_data_ptr->manet_info_ptr->rte_protocol)
		{
		case (IpC_Rte_Olsr):
			{
			/* Create the process for dynamic source routing protocol	*/
			child_prohandle = op_pro_create ("olsr_rte", OPC_NIL /* No data passed */);
			
			/* Invoke the process so that can initialize itself	*/
			op_pro_invoke (child_prohandle, OPC_NIL);
			
			/* Get the process ID of the OLSR process	*/
			child_pro_id = op_pro_id (child_prohandle);
			
			/* Register a handler to receive node failure and recovery interrupt */
			op_intrpt_type_register (OPC_INTRPT_FAIL, child_prohandle);
			op_intrpt_type_register (OPC_INTRPT_RECOVER, child_prohandle);
						
			if (LTRACE_ACTIVE)
				{
				op_prg_odb_print_major ("Spawning OLSR Routing process", OPC_NIL, OPC_NIL);
				}
			break;
			}
		
		default:
			{
			manet_rte_mgr_error ("Invalid MANET routing protocol specified", OPC_NIL, OPC_NIL);
			}
		}
			
	FOUT;
	}

static void
manet_rte_mgr_packet_arrival ()
	{
	char				msg1 [256];
	Packet*				packet_ptr 					= OPC_NIL;
			
	FIN (manet_rte_mgr_packet_arrival(<args>));

	/* Get the packet.												*/
	packet_ptr = op_pk_get (op_intrpt_strm ());
	
	/* Write the trace message										*/
	if (LTRACE_ACTIVE)
		{
		sprintf (msg1, "Manet_rte_mgr received a packet of ID <%d>", (int) op_pk_id (packet_ptr));
		op_prg_odb_print_major (msg1, OPC_NIL);
		}
	
	/* Give the packet to the child process (if the child exists). 	 */
	if (op_pro_valid (child_prohandle) == OPC_TRUE)
		op_pro_invoke (child_prohandle, packet_ptr);
	else
		{
		/* If the child is not active, the packet must */
		/* be sinked here in the parent process.       */
		op_pk_destroy (packet_ptr); 
		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_minor("Destroy the packet, as OLSR is not active", OPC_NIL); 
			}
		}
	
	FOUT;
	}


static void
manet_rte_mgr_error (const char* str1, char* str2, char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (manet_rte_mgr_error <args>);
	
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
	void manet_rte_mgr (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_manet_rte_mgr_init (int * init_block_ptr);
	void _op_manet_rte_mgr_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_manet_rte_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_manet_rte_mgr_alloc (VosT_Obtype, int);
	void _op_manet_rte_mgr_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
manet_rte_mgr (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (manet_rte_mgr ());

		{
		/* Temporary Variables */
		int				intrpt_type = -99;
		int				intrpt_code = -99;
		
		/* End of Temporary Variables */


		FSM_ENTER ("manet_rte_mgr")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "manet_rte_mgr [init enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_rte_mgr [init enter execs]", state0_enter_exec)
				{
				/* Initialize the state variables	*/
				manet_rte_mgr_sv_init ();
				
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "manet_rte_mgr [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "init", "wait_for_ip", "tr_2", "manet_rte_mgr [init -> wait_for_ip : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "manet_rte_mgr [wait enter execs]")
				FSM_PROFILE_SECTION_IN ("manet_rte_mgr [wait enter execs]", state1_enter_exec)
				{
				
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"manet_rte_mgr")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "manet_rte_mgr [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_rte_mgr [wait exit execs]", state1_exit_exec)
				{
				/* Get the interrupt type and code	*/
				intrpt_type = op_intrpt_type ();
				intrpt_code = op_intrpt_code ();
				
				if (intrpt_type == OPC_INTRPT_STRM)
					{
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major ("Packet Arrival", OPC_NIL);
						}
					
					/* Process the incoming packet	*/
					manet_rte_mgr_packet_arrival ();
					}
				else if ((intrpt_type == OPC_INTRPT_PROCESS) && IDLE_CHILD)
					{
					/* OLSR child has notified us that it is idle (due to */
					/* all its IPv6 interfaces being not active)		  */
					op_pro_destroy (child_prohandle); 
					
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major ("OLSR Child has no active interfaces - DESTROY it now", OPC_NIL);
						}
					
					}
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "wait", "wait", "tr_12", "manet_rte_mgr [wait -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait_for_ip) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "wait_for_ip", state2_enter_exec, "manet_rte_mgr [wait_for_ip enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"manet_rte_mgr")


			/** state (wait_for_ip) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "wait_for_ip", "manet_rte_mgr [wait_for_ip exit execs]")
				FSM_PROFILE_SECTION_IN ("manet_rte_mgr [wait_for_ip exit execs]", state2_exit_exec)
				{
				/* Determine interrupt details */
				intrpt_type = op_intrpt_type();
				intrpt_code = op_intrpt_code();
				
				if (IP_NOTIFICATION)
					{
					/* Determine the MANET routing protocol running	*/
					/* on this node. Invoke the appropriate routing */
					/* protocol.									*/
					manet_rte_mgr_routing_protocol_determine ();
					}
				
				/* This process ignores the failure and recovery	*/
				/* interrupts that are received before IP			*/
				/* notification, since it has no MANET process		*/
				/* spawned, yet, to process those interrupts.		*/
				}
				FSM_PROFILE_SECTION_OUT (state2_exit_exec)


			/** state (wait_for_ip) transition processing **/
			FSM_PROFILE_SECTION_IN ("manet_rte_mgr [wait_for_ip trans conditions]", state2_trans_conds)
			FSM_INIT_COND (IP_NOTIFICATION)
			FSM_TEST_COND (FAILURE_RECOVERY)
			FSM_TEST_LOGIC ("wait_for_ip")
			FSM_PROFILE_SECTION_OUT (state2_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ;, "IP_NOTIFICATION", "", "wait_for_ip", "wait", "tr_6", "manet_rte_mgr [wait_for_ip -> wait : IP_NOTIFICATION / ]")
				FSM_CASE_TRANSIT (1, 2, state2_enter_exec, ;, "FAILURE_RECOVERY", "", "wait_for_ip", "wait_for_ip", "tr_3", "manet_rte_mgr [wait_for_ip -> wait_for_ip : FAILURE_RECOVERY / ]")
				}
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"manet_rte_mgr")
		}
	}




void
_op_manet_rte_mgr_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_manet_rte_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_manet_rte_mgr_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_manet_rte_mgr_svar function. */
#undef child_prohandle
#undef child_pro_id

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_manet_rte_mgr_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_manet_rte_mgr_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (manet_rte_mgr)",
		sizeof (manet_rte_mgr_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_manet_rte_mgr_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	manet_rte_mgr_state * ptr;
	FIN_MT (_op_manet_rte_mgr_alloc (obtype))

	ptr = (manet_rte_mgr_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "manet_rte_mgr [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_manet_rte_mgr_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	manet_rte_mgr_state		*prs_ptr;

	FIN_MT (_op_manet_rte_mgr_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (manet_rte_mgr_state *)gen_ptr;

	if (strcmp ("child_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->child_prohandle);
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

