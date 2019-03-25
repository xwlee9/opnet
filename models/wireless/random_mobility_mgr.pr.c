/* Process model C form file: random_mobility_mgr.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char random_mobility_mgr_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C9240A1 5C9240A1 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include "mobility_support.h"
#include "oms_tan.h"

/* Function prototypes. */
static void			mobility_mgr_sv_init (void);
static void			mobility_mgr_profile_parse (void);

static Mobility_Profile_Desc*				 mobility_mgr_profile_desc_parse (Objid profile_comp_objid, int profile_index);
static void*						         mobility_mgr_random_waypoint_desc_parse (Objid RW_comp_objid, const char* profile_name);
static void                                  mobility_mgr_node_profile_parse (void);
static Mobility_Profile_Desc*                mobility_mgr_find_profile_name_ptr (char* profile_name);
static void                                  mobility_mgr_print_all_profiles (void);
static void					 				 mobility_mgr_error (char* str1, char* str2, char* str3);

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
	Mobility_Model	         		mobility_model                                  ;	/* Type of Mobility Model  */
	Objid	                  		my_objid                                        ;	/* Module Objid  */
	List*	                  		all_profile_definitions_lptr                    ;	/* List pointer of the Profile Definitions  */
	Log_Handle	             		config_log_handle                               ;	/* Log handle for the configuration  */
	} random_mobility_mgr_state;

#define mobility_model          		op_sv_ptr->mobility_model
#define my_objid                		op_sv_ptr->my_objid
#define all_profile_definitions_lptr		op_sv_ptr->all_profile_definitions_lptr
#define config_log_handle       		op_sv_ptr->config_log_handle

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	random_mobility_mgr_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((random_mobility_mgr_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
mobility_mgr_sv_init (void)
	{	
	/** Initializes the state variables	**/
	FIN (mobility_mgr_sv_init (void));
	
	/* Obtain the module Objid	*/
	my_objid = op_id_self ();
	
	/* Create the list of all profile definitions */
	all_profile_definitions_lptr = op_prg_list_create ();
	
	/* Register the log handle */
	config_log_handle = op_prg_log_handle_create (
		OpC_Log_Category_Configuration, "Mobility Manager", "Mobility Domain Configuration", 128);

	FOUT;
	}


static void
mobility_mgr_profile_parse (void)
	{
	Objid   		            profile_comp_objid;
	Objid   		            profile_objid;
	char                        profile_name [256];
	int     		        	num_of_profiles, profile_index;
	Mobility_Profile_Desc* 		profile_defn_ptr;
		
	/** This function determines the Mobility Model Profiles	**/
	FIN (mobility_mgr_profile_parse ());
	
	/* Get the compound object for profiles */
    op_ima_obj_attr_get (my_objid, "Random Mobility Definition", &profile_comp_objid);

	/* Find the number of entries in the profile definitions */
    num_of_profiles = op_topo_child_count (profile_comp_objid, OPC_OBJTYPE_GENERIC);
	
    /* Call the profile description to read each of the specificaitons*/
    for (profile_index = 0; profile_index < num_of_profiles; profile_index++)
        {		
		/* Get the compound object id*/
		profile_objid  = op_topo_child (profile_comp_objid, OPC_OBJTYPE_GENERIC, profile_index);
	
		/* Get the Profile Name */
		op_ima_obj_attr_get (profile_objid, "Profile Name", profile_name);
		
	  	/* Check out if the list has this Profile */	
		profile_defn_ptr = mobility_mgr_find_profile_name_ptr (profile_name);
		
		/* Profile Name has to be unique in the list        */
		/* If there is more then one, ignore the later ones */			
		if (profile_defn_ptr == OPC_NIL)
			{
			/* Parse the new Profile */
			profile_defn_ptr = (Mobility_Profile_Desc *) mobility_mgr_profile_desc_parse (profile_comp_objid, profile_index);
			
			/* Insert the profile into the list*/
			op_prg_list_insert (all_profile_definitions_lptr, profile_defn_ptr, OPC_LISTPOS_TAIL);
			}
		}
	
	FOUT;
	}


static Mobility_Profile_Desc*
mobility_mgr_find_profile_name_ptr (char* profile_name)
	{
	int                         i;
	Mobility_Profile_Desc* 		profile_defn_ptr;
	
	/** This function finds and returns the Profile pointer  **/ 
	/** having the profile name                              **/
	FIN (mobility_mgr_find_profile_name_ptr (<arg>));
	
	/* Loop through all Profiles */
	for (i = 0; i < op_prg_list_size (all_profile_definitions_lptr); i++)	
		{
		profile_defn_ptr = (Mobility_Profile_Desc *) op_prg_list_access (all_profile_definitions_lptr, i);
		if (strcmp (profile_defn_ptr->Profile_Name, profile_name) == 0 )
			FRET (profile_defn_ptr);
		}
	
	FRET (OPC_NIL);
	}

	   
static Mobility_Profile_Desc*
mobility_mgr_profile_desc_parse (Objid profile_comp_objid, int profile_index)
	{
	Objid                             profile_objid;
	Mobility_Profile_Desc*            profile_ptr; 
	char                              temp_name[256];
	Objid                             RW_comp_objid;
	
	/** This function parses the Profile description attributes **/
	FIN (mobility_profile_desc_parse (<args>));
	
	/* Allocate memory for the mobility profile structure */
	profile_ptr = (Mobility_Profile_Desc *) op_prg_mem_alloc (sizeof (Mobility_Profile_Desc));
		
	/* Create the list for node ids*/
	profile_ptr->Mobile_entity_id_list = op_prg_list_create();
	
	/* Get the compound attribute for a particular profile having a */
    /* unique profile name from the compound object of the profile objid   */
    profile_objid  = op_topo_child (profile_comp_objid, OPC_OBJTYPE_GENERIC, profile_index);
	
	/* Get the Profile Name */
	op_ima_obj_attr_get (profile_objid, "Profile Name", temp_name);
	profile_ptr->Profile_Name = (char *) op_prg_mem_alloc (strlen (temp_name) + 1);
    strcpy (profile_ptr->Profile_Name, temp_name);
	
	/* Get the Mobility Model */
	op_ima_obj_attr_get (profile_objid, "Mobility Model", temp_name);
	profile_ptr->Mobility_Model = (char *) op_prg_mem_alloc (strlen (temp_name) + 1);
    strcpy (profile_ptr->Mobility_Model, temp_name);
	
	profile_ptr->Mobility_Desc_ptr = OPC_NIL;
	if (strcmp (profile_ptr->Mobility_Model, "Random Waypoint") == 0)
		{
		/* Get the compound object for the profile */
		op_ima_obj_attr_get (profile_objid, "Random Waypoint Parameters", &RW_comp_objid);
		
		/* Get the random waypoint mobility model specifications*/
		profile_ptr->Mobility_Desc_ptr = (void *) mobility_mgr_random_waypoint_desc_parse (
			RW_comp_objid, profile_ptr->Profile_Name);
		}
	
	FRET (profile_ptr);
	}


static void*
mobility_mgr_random_waypoint_desc_parse (Objid RW_comp_objid, const char* profile_name)
	{
	Objid                             subRW_comp_objid, domain_id;
	Mobility_Random_Waypoint_Desc*    RW_desc_ptr;
	char                              speed[256];
	char                              pause_time[256];
	char                              start_time[256];
	char                              domain_name[256], mob_domain_name [256];
	int                               i, domain_count;
	double                            x_position, y_position, X_min, X_max, Y_min, Y_max;
	double                            x_span, y_span;
	double							  Stop_Time, Update_Freq; 	
	Boolean                           is_domain_found = OPC_FALSE;
	Boolean							  record_trajectory;
 	
	/** This function parses the description of the			**/   
	/** random waypoint mobility model            			**/
	FIN (mobility_mgr_random_waypoint_desc_parse (RW_comp_objid, profile_name));
	
	/* Get the compound object */
	subRW_comp_objid  = op_topo_child (RW_comp_objid, OPC_OBJTYPE_GENERIC, 0);
	
	/* Read the Start Time */
	op_ima_obj_attr_get (subRW_comp_objid, "Start Time", 					start_time);
	
	/* Quit parsing if there is no start time specified for	*/
	/* the profile.											*/
	if (strcmp (start_time, "None") == 0)
		{
		FRET (OPC_NIL);
		}

	/* Read the Mobility Domain Name.						*/
	op_ima_obj_attr_get (subRW_comp_objid, "Mobility Domain Name", mob_domain_name);

	/* Check out if any mobility domain is selected.		*/
	if (strcmp (mob_domain_name, "Not Used") != 0 ) 
		{
		/* Find the domain id */
		domain_count = op_topo_object_count (OPC_OBJTYPE_WDOMAIN);
			
		for (i = 0; i < domain_count; i++)
			{
			domain_id = op_topo_object (OPC_OBJTYPE_WDOMAIN, i);			
			op_ima_obj_attr_get (domain_id, "name", domain_name);
						
			/* if you find the domain, determine the		*/
			/* coordinates from the domain object.			*/
			if (strcmp (mob_domain_name, domain_name) == 0 )
				{
				op_ima_obj_attr_get (domain_id, "x position", &x_position);
				op_ima_obj_attr_get (domain_id, "y position", &y_position);
				op_ima_obj_attr_get (domain_id, "x span", &x_span);
				op_ima_obj_attr_get (domain_id, "y span", &y_span);
				
				X_min = x_position - (x_span / 2);
				X_max = x_position + (x_span / 2);
				Y_min = y_position - (y_span / 2);
				Y_max = y_position + (y_span / 2);	
				
				is_domain_found = OPC_TRUE;
				break;
				}			
			}
		
		if (is_domain_found == OPC_FALSE)
			{
			/* Write the error log message.					*/
			op_prg_log_entry_write (config_log_handle,
				"ERROR:\n"
				" There is no mobility domain found in the network with the\n"
				" name specified under the \"Mobility Domain Name\" attribute\n" 	
				" for the random mobility profile \"%s\".\n"
				"\n"	
				"REMEDIAL ACTION:\n"
				" The mobility of the nodes that use this profile will not\n"
				" be modeled.\n"
				"\n" 	
				"SUGGESTIONS:\n"
				" 1. Make sure that the network contains a mobility domain\n"
				"    instance with the specified name.\n"
				" 2. Set the \"Mobility Domain Name\" to the name of one of\n"
				"    mobility domains that already exists in the network.\n"
				" 3. Set the \"Mobility Domain Name\" to \"Not Used\" and\n"
				"    configure the attributes \"x_min\", \"y_min\", \"x_max\"\n"
				"    and \"y_max\" to specify the movement area for the\n"
				"    mobility profile.\n", profile_name);
			
			/* Disable the profile.							*/
			FRET (OPC_NIL);
			}
		} 
	else
		/* No mobility domain is selected so read the    */
		/* coordinates directly from the compund object  */
		{
		op_ima_obj_attr_get (subRW_comp_objid, "x_min", &X_min);
		op_ima_obj_attr_get (subRW_comp_objid, "x_max", &X_max);
		op_ima_obj_attr_get (subRW_comp_objid, "y_min", &Y_min);
		op_ima_obj_attr_get (subRW_comp_objid, "y_max", &Y_max);		
		}

	/* Read other parameters */
	op_ima_obj_attr_get (subRW_comp_objid, "Stop Time", 					&Stop_Time);	
	op_ima_obj_attr_get (subRW_comp_objid, "Animation Update Frequency",	&Update_Freq);
	op_ima_obj_attr_get (subRW_comp_objid, "Speed", 						speed);
	op_ima_obj_attr_get (subRW_comp_objid, "Pause Time", 					pause_time);
	op_ima_obj_attr_get (subRW_comp_objid, "Record Trajectory", 			&record_trajectory);
	
	/* Allocate memory  */
	RW_desc_ptr = (Mobility_Random_Waypoint_Desc *) op_prg_mem_alloc (sizeof (Mobility_Random_Waypoint_Desc));
	
	/* Read the distributions for speed and pause time.		*/
	RW_desc_ptr->Start_Time = oms_dist_load_from_string (start_time);	
	RW_desc_ptr->Stop_Time  = Stop_Time;
	RW_desc_ptr->Update_Freq= Update_Freq; 
	RW_desc_ptr->Speed      = oms_dist_load_from_string (speed);
	RW_desc_ptr->Pause_Time = oms_dist_load_from_string (pause_time);
   	RW_desc_ptr->X_min 		= X_min;
	RW_desc_ptr->X_max		= X_max;
   	RW_desc_ptr->Y_min 		= Y_min;
	RW_desc_ptr->Y_max		= Y_max;
	RW_desc_ptr->Record_Trajectory = record_trajectory;
	
	if (is_domain_found)
		RW_desc_ptr->Mobility_Domain = prg_string_copy (mob_domain_name);
	
	FRET (RW_desc_ptr);
	}


static void
mobility_mgr_node_profile_parse (void)
	{
	int                         i, node_count, subnet_count;
	Objid                       node_id, subnet_id;
	MobilityT_Mobile_Node_Rec*  mobile_node_ptr = OPC_NIL;
	MobilityT_Mobile_Node_Rec*  mobile_subnet_ptr = OPC_NIL;
	char                        profile_name[256];
	Mobility_Profile_Desc* 		profile_defn_ptr = OPC_NIL;
	
	/** This function parses each node's mobility profile name        **/
	/** and then inserts the node id to th corresponding profile list **/
	FIN (mobility_mgr_node_profile_parse ());
	
	/* The size of the list is equal to the number of objects	*/
	/* in the network that we are looking for.					*/
	node_count = op_topo_object_count (OPC_OBJTYPE_NDMOB);
	subnet_count = op_topo_object_count (OPC_OBJTYPE_SUBNET_MOB);

	for (i = 0; i < node_count; i++)
		{
		mobile_node_ptr = (MobilityT_Mobile_Node_Rec *) op_prg_mem_alloc (sizeof (MobilityT_Mobile_Node_Rec));
		
		node_id  = op_topo_object (OPC_OBJTYPE_NDMOB, i);
		
		mobile_node_ptr->mobile_entity_objid = node_id;
		mobile_node_ptr->movement_delay = 0.0;

		if (op_ima_obj_attr_exists (node_id, "Mobility Profile Name") == OPC_TRUE)
			{

			op_ima_obj_attr_get (node_id, "Mobility Profile Name", profile_name);
			profile_defn_ptr = mobility_mgr_find_profile_name_ptr (profile_name);
			if (profile_defn_ptr == OPC_NIL)
				{
				char node_name[512], warning_string[1024], profile_string[512];
				oms_tan_hname_get (node_id, node_name);
				sprintf (profile_string, "\nRandom Mobility Profile: %s",profile_name);
				sprintf (warning_string, "configured on node: %s is not defined in the mobility profile configuration node.\n", node_name);
				op_sim_error (OPC_SIM_ERROR_WARNING, profile_string, warning_string);
				}
			else
				{
				op_prg_list_insert (profile_defn_ptr->Mobile_entity_id_list, mobile_node_ptr, OPC_LISTPOS_TAIL);
				}
			}

		if (profile_defn_ptr == OPC_NIL)
			{
			op_prg_mem_free (mobile_node_ptr);
			}
		}
	
	for (i = 0; i < subnet_count; i++)
		{
		mobile_subnet_ptr = (MobilityT_Mobile_Node_Rec *) op_prg_mem_alloc (sizeof (MobilityT_Mobile_Node_Rec));
		
		subnet_id  = op_topo_object (OPC_OBJTYPE_SUBNET_MOB, i);
		
		mobile_subnet_ptr->mobile_entity_objid = subnet_id;
		mobile_subnet_ptr->movement_delay = 0.0;

		if (op_ima_obj_attr_exists (subnet_id, "Mobility Profile Name") == OPC_TRUE)
			{

			op_ima_obj_attr_get (subnet_id, "Mobility Profile Name", profile_name);
			profile_defn_ptr = mobility_mgr_find_profile_name_ptr (profile_name);
			if (profile_defn_ptr == OPC_NIL)
				{
				char subnet_name[512], warning_string[1024], profile_string[512];
				oms_tan_hname_get (subnet_id, subnet_name);
				sprintf (profile_string, "\nRandom Mobility Profile: %s",profile_name);
				sprintf (warning_string, "configured on subnet: %s is not defined in the mobility profile configuration node.\n", subnet_name);
				op_sim_error (OPC_SIM_ERROR_WARNING, profile_string, warning_string);
				}
			else
				{
				op_prg_list_insert (profile_defn_ptr->Mobile_entity_id_list, mobile_subnet_ptr, OPC_LISTPOS_TAIL);
				}
			}

		if (profile_defn_ptr == OPC_NIL)
			{
			op_prg_mem_free (mobile_subnet_ptr);
			}
		}
	
	FOUT;
	}

		

static void
mobility_mgr_profile_process_create (void)
	{
	int							num_profiles;
	int                         i_th_profile;
	Mobility_Profile_Desc* 		profile_defn_ptr;
	Prohandle                   child_prohandle;
	
	/** This function spawns the appropriate Mobility Profile	**/
	/** process for each profile.								**/
	FIN (mobility_mgr_profile_process_create ());
	
	/* Loop through all Profiles.								*/
	num_profiles = op_prg_list_size (all_profile_definitions_lptr);
	for (i_th_profile = 0; i_th_profile < num_profiles; i_th_profile++)	
		{
		/* Access the Mobility Model Profile.					*/
		profile_defn_ptr = (Mobility_Profile_Desc *) op_prg_list_access (all_profile_definitions_lptr, i_th_profile);
		
		if (strcmp (profile_defn_ptr->Mobility_Model, "Random Waypoint") == 0 && 
			((Mobility_Random_Waypoint_Desc *) profile_defn_ptr->Mobility_Desc_ptr != OPC_NIL))
			{
			/* Create the process for the random waypoint		*/
			/* mobility model.									*/
			child_prohandle = op_pro_create ("random_waypoint", OPC_NIL);
	    
			/* Invoke the process so that can initialize itself.*/
			op_pro_invoke (child_prohandle, profile_defn_ptr);
			}
		}	

	FOUT;
	}


static void
mobility_mgr_print_all_profiles (void)
	{
	int                         i;
	Mobility_Profile_Desc* 		profile_defn_ptr;
	
	/** This Function prints all Mobility Profiles.				**/
	FIN (mobility_mgr_print_all_profiles ());
	
	printf ("Printing all Profile Definitions\n");
	
	for (i = 0; i < op_prg_list_size  (all_profile_definitions_lptr); i++)	
		{
		profile_defn_ptr = (Mobility_Profile_Desc *) op_prg_list_access (all_profile_definitions_lptr, i);
		mobility_support_print_profile (profile_defn_ptr);
		}
	
	FOUT;
	}


static void
mobility_mgr_error (char* str1, char* str2, char* str3)
	{
	/** This function generates an error and terminates the		**/
	/** simulation.												**/
	FIN (manet_rte_error <args>);
	
	op_sim_end ("MOBILITY Manager : ", str1, str2, str3);
	
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
	void random_mobility_mgr (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_random_mobility_mgr_init (int * init_block_ptr);
	void _op_random_mobility_mgr_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_random_mobility_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_random_mobility_mgr_alloc (VosT_Obtype, int);
	void _op_random_mobility_mgr_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
random_mobility_mgr (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (random_mobility_mgr ());

		{
		/* Temporary Variables */
		int 	mobility_modeling_status;
		/* End of Temporary Variables */


		FSM_ENTER_NO_VARS ("random_mobility_mgr")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "init", "random_mobility_mgr [init enter execs]")
				FSM_PROFILE_SECTION_IN ("random_mobility_mgr [init enter execs]", state0_enter_exec)
				{
				/* Initialize the state variables	*/
				mobility_mgr_sv_init ();
				
				/* Get the status */
				op_ima_obj_attr_get (my_objid, "Mobility Modeling Status", &mobility_modeling_status);
				
				if (mobility_modeling_status == OPC_BOOLINT_ENABLED)
					{
					/* Parse the Mobility Model Profiles one by one.	*/
					mobility_mgr_profile_parse ();
				
					/* Decide which node picks which profile.	*/
					mobility_mgr_node_profile_parse ();
					
					/* Create child processes for each profile.	*/
					mobility_mgr_profile_process_create ();
					}
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"random_mobility_mgr")


			/** state (init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "init", "random_mobility_mgr [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_MISSING ("init")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"random_mobility_mgr")
		}
	}




void
_op_random_mobility_mgr_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_random_mobility_mgr_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		/* Print out all the mobility profiles configured.	*/
		mobility_mgr_print_all_profiles ();
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_random_mobility_mgr_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_random_mobility_mgr_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_random_mobility_mgr_svar function. */
#undef mobility_model
#undef my_objid
#undef all_profile_definitions_lptr
#undef config_log_handle

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_random_mobility_mgr_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_random_mobility_mgr_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (random_mobility_mgr)",
		sizeof (random_mobility_mgr_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_random_mobility_mgr_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	random_mobility_mgr_state * ptr;
	FIN_MT (_op_random_mobility_mgr_alloc (obtype))

	ptr = (random_mobility_mgr_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "random_mobility_mgr [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_random_mobility_mgr_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	random_mobility_mgr_state		*prs_ptr;

	FIN_MT (_op_random_mobility_mgr_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (random_mobility_mgr_state *)gen_ptr;

	if (strcmp ("mobility_model" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mobility_model);
		FOUT
		}
	if (strcmp ("my_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_objid);
		FOUT
		}
	if (strcmp ("all_profile_definitions_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->all_profile_definitions_lptr);
		FOUT
		}
	if (strcmp ("config_log_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->config_log_handle);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

