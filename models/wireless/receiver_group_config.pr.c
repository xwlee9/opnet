/* Process model C form file: receiver_group_config.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char receiver_group_config_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE8 5C91EEE8 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/******* Includes ********/
#include	<string.h>
#include	<math.h>
#include	<oms_pr.h>
#include	"prg_bin_hash.h"

/******* Constants *******/
#define		DrgmC_Sim_End_Time				0
#define		DrgmC_Calc_Never				-1
#define		DrgmC_Earth_Radius				6.378E06
#define		DrgmC_Dist_Evaluate_Any			0
#define		DrgmC_Dist_Evaluate_LOS			-1
#define		DrgmC_Pathloss_Evaluate_Any		0
#define		DgramC_Sel_Criteria_Strict		0
#define		DgramC_Sel_Criteria_Loose		1
#define		DgramC_Sel_Criteria_Static		2

/******* Enumerated Data Types ********/
typedef enum
	{
	DrgmC_Base_Rxgroup_From_Static,
	DrgmC_Base_Rxgroup_Empty
	} DrgmC_Base_Rxgroup;

typedef enum
	{
	DrgmC_Sel_Criteria_Ch_N_Dist_N_Snr,
	DrgmC_Sel_Criteria_Ch_N_Dist_Or_Snr,
	DrgmC_Sel_Criteria_Repeat_Static
	} DrgmC_Sel_Criteria;

typedef enum
	{
	DrgmC_Ch_Match_All,
	DrgmC_Ch_Match_Exact,
	DrgmC_Ch_Match_Exact_Or_Partial
	} DrgmC_Ch_Match;


/****** Data Structures *******/
typedef struct
	{
	Objid			txch_objid;
	Objid			node_objid;
	unsigned int	rxgroup_count;
	Objid*			rxgroup_pptr;
	} DrgmT_Txch_Info;

typedef struct
	{
	Objid		tx_node_objid;
	double		tx_lat;
	double		tx_lon;
	double		tx_alt;
	double		tx_x;
	double		tx_y;
	double		tx_z;
	Objid		rx_node_objid;
	double		rx_lat;
	double		rx_lon;
	double		rx_alt;
	double		rx_x;
	double		rx_y;
	double		rx_z;
	Boolean		dist_eval_result;
	} DrgmT_Cache;

typedef struct
	{
	Objid		ch_objid;
	double		drate;
	double		bw;
	double		freq;
	double		code;
	const char*	mod;
	} DrgmT_Channel_Info;

/****** Prototypes ******/
static void					dynamic_rx_group_sv_init (void);
static void					dynamic_rx_group_txch_info_init (void);
static void					dynamic_rx_group_rxch_info_init (void);
static void					dynamic_rx_group_compute (void);
static void					dynamic_rx_group_next_computation_schedule (void);
static Boolean				dynamic_rx_group_ch_match (Objid, Objid);
static Boolean				dynamic_rx_group_dist_evaluate (Objid, Objid);
static Boolean				dynamic_rx_group_pathloss_evaluate (Objid, Objid);
static Boolean				dynamic_rx_group_simple_earth_LOS_closure (double, double, 
										double, double, double, double, double, double);
static void					dynamic_rx_group_tmm_closure_init (void);
static Compcode				dynamic_rx_group_tmm_pathloss_calc (double, double, double, 
										double, double, double, double, double, double*, Objid, Objid);
static DrgmT_Channel_Info*	dynamic_rx_group_channel_info_mem_alloc (void);
static DrgmT_Txch_Info*		dynamic_rx_group_static_info_mem_alloc (void);

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
	List*	                  		rxgroup_model_lptr                              ;
	int	                    		base_rxgroup                                    ;
	DrgmC_Sel_Criteria	     		selection_criteria                              ;
	int	                    		channel_match                                   ;
	int	                    		distance_threshold                              ;
	int	                    		pathloss_threshold                              ;
	int	                    		consider_local_receivers                        ;
	int	                    		start_time                                      ;
	int	                    		end_time                                        ;
	int	                    		refresh_period                                  ;
	Boolean	                		all_transmitters                                ;
	List*	                  		txch_info_lptr                                  ;
	DrgmT_Cache	            		cache                                           ;
	Boolean	                		DrgmS_Closure_As_Simple_Earth                   ;
	TmmT_Propagation_Model*			DrgmS_Closure_Prop_Model_Ptr                    ;
	Log_Handle	             		DrgmS_TMM_Verbose_Log_H                         ;
	const char*	            		DrgmS_Closure_Tmm_Prop_Model_Name               ;
	PrgT_Bin_Hash_Table*	   		channel_info_table                              ;
	Objid	                  		my_objid                                        ;
	int	                    		num_rxgroup                                     ;
	} receiver_group_config_state;

#define rxgroup_model_lptr      		op_sv_ptr->rxgroup_model_lptr
#define base_rxgroup            		op_sv_ptr->base_rxgroup
#define selection_criteria      		op_sv_ptr->selection_criteria
#define channel_match           		op_sv_ptr->channel_match
#define distance_threshold      		op_sv_ptr->distance_threshold
#define pathloss_threshold      		op_sv_ptr->pathloss_threshold
#define consider_local_receivers		op_sv_ptr->consider_local_receivers
#define start_time              		op_sv_ptr->start_time
#define end_time                		op_sv_ptr->end_time
#define refresh_period          		op_sv_ptr->refresh_period
#define all_transmitters        		op_sv_ptr->all_transmitters
#define txch_info_lptr          		op_sv_ptr->txch_info_lptr
#define cache                   		op_sv_ptr->cache
#define DrgmS_Closure_As_Simple_Earth		op_sv_ptr->DrgmS_Closure_As_Simple_Earth
#define DrgmS_Closure_Prop_Model_Ptr		op_sv_ptr->DrgmS_Closure_Prop_Model_Ptr
#define DrgmS_TMM_Verbose_Log_H 		op_sv_ptr->DrgmS_TMM_Verbose_Log_H
#define DrgmS_Closure_Tmm_Prop_Model_Name		op_sv_ptr->DrgmS_Closure_Tmm_Prop_Model_Name
#define channel_info_table      		op_sv_ptr->channel_info_table
#define my_objid                		op_sv_ptr->my_objid
#define num_rxgroup             		op_sv_ptr->num_rxgroup

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	receiver_group_config_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((receiver_group_config_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
dynamic_rx_group_sv_init (void)
	{
	Objid				own_node_objid;
	Objid				affected_rxgroup_objid, affected_rxgroup_child_id;
	int					count;
	Prohandle			own_prohandle;
	OmsT_Pr_Handle		own_process_record_handle;
	char				proc_model_name [128];
	char				rxgroup_model_name [128];
	char*				model_name;
	int					selection_match;
	
	/** Initialize the state variables and read	**/
	/** in the attributes of the node			**/
	FIN (dynamic_rx_group_sv_init (void));
	
	/* Own object ID	*/
	my_objid = op_id_self ();
	
	/* Obtain the node's object identifier.	*/
	own_node_objid = op_topo_parent (my_objid);
	
	/* Obtain the process's process handle.	*/
	own_prohandle = op_pro_self ();
	
	/* Obtain the name of the process.							*/
	op_ima_obj_attr_get (my_objid, "process model", proc_model_name);

	/* Register this config object in the model wide registry	*/
	own_process_record_handle = (OmsT_Pr_Handle) 
		oms_pr_process_register (own_node_objid, my_objid, own_prohandle, proc_model_name);
	oms_pr_attr_set (own_process_record_handle,
		"protocol",				OMSC_PR_STRING,		"rx_group_config",
		OPC_NIL);
	
	/* Determine the set of possible transmitters	*/
	op_ima_obj_attr_get (my_objid, "Affected Transmitter Set", &affected_rxgroup_objid);
	num_rxgroup = op_topo_child_count (affected_rxgroup_objid, OPC_OBJTYPE_GENERIC);
	
	/* Create a list to store the rxgroup model names	*/
	rxgroup_model_lptr = op_prg_list_create ();
	
	/* Initialize the Rx Group Model is to Not All transmitters	*/
	all_transmitters = OPC_FALSE;
	
	for (count = 0; count < num_rxgroup; count++)
		{
		affected_rxgroup_child_id = op_topo_child (affected_rxgroup_objid, OPC_OBJTYPE_GENERIC, count);
		op_ima_obj_attr_get (affected_rxgroup_child_id, "Receiver Group Name", &rxgroup_model_name);
		
		/* If "ALL" try avoiding multiple string comparison. */
		if (!strcmp ("ALL", rxgroup_model_name))
			{
			all_transmitters = OPC_TRUE;
			
			break;
			}

		/* Allocate memory to store the model names	*/
		model_name = (char*) op_prg_mem_alloc (strlen ((rxgroup_model_name) + 1) * sizeof (char));
		strcpy (rxgroup_model_name, model_name);
		
		/* Insert the model into the list	*/
		op_prg_list_insert (rxgroup_model_lptr, model_name, OPC_LISTPOS_TAIL);
		}
		
	op_ima_obj_attr_get (my_objid, "Selection Criteria", &selection_match);
	
	/* Set the selection criteria based on the above attribute setting	*/
	if (selection_match == DgramC_Sel_Criteria_Static)
		{
		/* Reuse the rxgroup model on the receiver	*/
		selection_criteria = DrgmC_Sel_Criteria_Repeat_Static;
		}
	else if (selection_match == DgramC_Sel_Criteria_Strict)
		{
		/* Strict match for both distance and pathloss	*/
		selection_criteria = DrgmC_Sel_Criteria_Ch_N_Dist_N_Snr;
		}
	else
		{
		/* Match for distance or pathloss	*/
		selection_criteria = DrgmC_Sel_Criteria_Ch_N_Dist_Or_Snr;
		}
	
	op_ima_obj_attr_get (my_objid, "Base Rxgroup", &base_rxgroup);
	op_ima_obj_attr_get (my_objid, "Channel Match", &channel_match);
	op_ima_obj_attr_get (my_objid, "Distance Threshold", &distance_threshold);
	op_ima_obj_attr_get (my_objid, "Pathloss Threshold", &pathloss_threshold);
	op_ima_obj_attr_get (my_objid, "Local Receivers", &consider_local_receivers);
	op_ima_obj_attr_get (my_objid, "Start Time", &start_time);
	op_ima_obj_attr_get (my_objid, "End Time", &end_time);
	op_ima_obj_attr_get (my_objid, "Refresh Period", &refresh_period);

	/* We do sanity checks here. */
	if ((end_time != DrgmC_Sim_End_Time) && (end_time <= start_time))
		{
		op_sim_end ("Dyn Rxgroup Mgr:: End time is earlier than start time.", "","","");
		}
	
	/* Schedule the interrupt for the first time. */
	op_intrpt_schedule_self ((double) start_time, 0);
	
	/* Initialize TMM related static variables. */
	DrgmS_Closure_As_Simple_Earth = OPC_TRUE;
	DrgmS_Closure_Prop_Model_Ptr = OPC_NIL;
	DrgmS_Closure_Tmm_Prop_Model_Name = OPC_NIL;
	
	/* Initialize cache member variables. */
	cache.tx_node_objid = OPC_OBJID_INVALID;
	cache.rx_node_objid = OPC_OBJID_INVALID;
	
	/* Create the hash table to store channel information	*/
	channel_info_table = (PrgT_Bin_Hash_Table*) prg_bin_hash_table_create (8, sizeof (Objid));
	
	FOUT;
	}


static void
dynamic_rx_group_txch_info_init (void)
	{
	Objid					txch_objid;
	DrgmT_Txch_Info*		txch_info_ptr;
	DrgmT_Channel_Info*		tx_channel_info_ptr;
	char					rxgroup_model [128];
	char*					affected_rxgroup_model;
	int						txch_index, txch_obj_count;
	void*					old_contents_ptr;
	Boolean					match;
	int						count;
	char					mod_str [128];	
	
	/** Initializes the rx group information and channel	**/
	/** information for every transmitter channel 			**/
	FIN (dynamic_rx_group_txch_info_init (void));
	
	/* Create a list for caching purpose. */
	txch_info_lptr = op_prg_list_create ();
	
	/* Access transmitter channel objects. */
	txch_obj_count = op_topo_object_count (OPC_OBJTYPE_RATXCH);
	for (txch_index = 0; txch_index < txch_obj_count; txch_index++)
		{
		/* Store the base transmitter channel objects for rxgroup calculation. */
		/* Access the channel object in order. */
		txch_objid = op_topo_object (OPC_OBJTYPE_RATXCH, txch_index);
		
		/* The channel information has not yet been read	*/
		/* Parse the attributes of the channel and store	*/
		tx_channel_info_ptr = dynamic_rx_group_channel_info_mem_alloc ();
		
		/* Access the channel characteristics here. */
		op_ima_obj_attr_get (txch_objid, "data rate", &tx_channel_info_ptr->drate);
		op_ima_obj_attr_get (txch_objid, "bandwidth", &tx_channel_info_ptr->bw);
		op_ima_obj_attr_get (txch_objid, "min frequency", &tx_channel_info_ptr->freq);
		op_ima_obj_attr_get (txch_objid, "spreading code", &tx_channel_info_ptr->code);
		op_ima_obj_attr_get (op_topo_parent (op_topo_parent (txch_objid)), "modulation", mod_str);
		tx_channel_info_ptr->mod = prg_string_const (mod_str);
		
		/* Set the channel information in the table	*/
		prg_bin_hash_table_item_insert (channel_info_table, (const void *)&txch_objid, tx_channel_info_ptr, &old_contents_ptr);
		
		if (!all_transmitters)
			{
			/* See if the static rxgroup model matches. */
			op_ima_obj_attr_get (op_topo_parent (op_topo_parent (txch_objid)), "rxgroup model", &rxgroup_model);
			
			/* Initialize the match condition	*/
			match = OPC_FALSE;
					
			for (count = 0; count < num_rxgroup; count++)
				{
				affected_rxgroup_model = (char*) op_prg_list_access (rxgroup_model_lptr, count);
			
				if (!strcmp (rxgroup_model, affected_rxgroup_model))
					{
					/* This transmitter matches. */
					match = OPC_TRUE;
					
					break;
					}
				}
			
			if (match == OPC_FALSE)
				{
				/* No matching rxgroup model was found	*/
				/* Do not consider this transmitter    	*/
				continue;
				}
			}
		
		/* Create structure to store static Rx group related information. */
		txch_info_ptr = dynamic_rx_group_static_info_mem_alloc ();
		txch_info_ptr->txch_objid = txch_objid;
		txch_info_ptr->node_objid = op_topo_parent (op_topo_parent (op_topo_parent (txch_objid)));

		/* Now we have to determine if we want to save the rxgroup calculated by the static model. */
		if (base_rxgroup == DrgmC_Base_Rxgroup_From_Static)
			{
			/* Save the current rxgroup for base. */
			op_radio_txch_rxgroup_get (txch_info_ptr->txch_objid, &txch_info_ptr->rxgroup_count, 
				&txch_info_ptr->rxgroup_pptr);
			}
		else
			{
			/* Initialize the values to NIL. */
			txch_info_ptr->rxgroup_count = 0;
			txch_info_ptr->rxgroup_pptr = OPC_NIL;
			}
		
		/* Insert the ready structure into a list for later access. */
		op_prg_list_insert (txch_info_lptr, txch_info_ptr, OPC_LISTPOS_TAIL);
		}
	
	FOUT;
	}


static void
dynamic_rx_group_rxch_info_init (void)
	{
	Objid					rxch_objid;
	int						rxch_obj_count, rxch_index;
	DrgmT_Channel_Info*		rx_channel_info_ptr;
	void*					old_contents_ptr;
	char					mod_str [128];
	
	/** Initialize the receivers by parsing	**/
	/** its channel related attributes		**/
	FIN (dynamic_rx_group_rxch_info_init (void));
	
	/* Access receiver channel objects. */
	rxch_obj_count = op_topo_object_count (OPC_OBJTYPE_RARXCH);
	for (rxch_index = 0; rxch_index < rxch_obj_count; rxch_index++)
		{
		/* Access the channel object in order. */
		rxch_objid = op_topo_object (OPC_OBJTYPE_RARXCH, rxch_index);
		
		/* The channel information has not yet been read	*/
		/* Parse the attributes of the channel and store	*/
		rx_channel_info_ptr = dynamic_rx_group_channel_info_mem_alloc ();
		
		/* Access the channel characteristics here. */
		op_ima_obj_attr_get (rxch_objid, "data rate", &rx_channel_info_ptr->drate);
		op_ima_obj_attr_get (rxch_objid, "bandwidth", &rx_channel_info_ptr->bw);
		op_ima_obj_attr_get (rxch_objid, "min frequency", &rx_channel_info_ptr->freq);
		op_ima_obj_attr_get (rxch_objid, "spreading code", &rx_channel_info_ptr->code);
		op_ima_obj_attr_get (op_topo_parent (op_topo_parent (rxch_objid)), "modulation", mod_str);
		rx_channel_info_ptr->mod = prg_string_const (mod_str);

		/* Set the channel information in the table	*/
		prg_bin_hash_table_item_insert (channel_info_table, (const void *)&rxch_objid, rx_channel_info_ptr, &old_contents_ptr);
		}
	
	FOUT;
	}


static void
dynamic_rx_group_compute (void)
	{
	int					txch_index, rxch_index;
	int					txch_obj_count, rxch_obj_count;
	DrgmT_Txch_Info*	tmp_txch_info_ptr;
	Objid				tmp_rxch_objid;
	
	/** Computes the Rx group set for every	**/
	/** transmitter channel in the network	**/
	FIN (dynamic_rx_group_compute (void));
	
	/* Traverse through the cached txch object and find new rxgroup for each. */
	txch_obj_count = op_prg_list_size (txch_info_lptr);
	for (txch_index = 0; txch_index < txch_obj_count; txch_index++)
		{
		/* Access each information element and process it. */
		tmp_txch_info_ptr = (DrgmT_Txch_Info*) op_prg_list_access (txch_info_lptr, txch_index);
	
		/* Reset the existing rxgroup info. */
		op_radio_txch_rxgroup_set (tmp_txch_info_ptr->txch_objid, 0, OPC_NIL);
	
		/* Easiest comuptation is when we just repeatedly call static rxgroup model. */
		if (selection_criteria == DrgmC_Sel_Criteria_Repeat_Static)
			{
			op_radio_txch_rxgroup_compute (tmp_txch_info_ptr->txch_objid, OPC_TXCH_RXGROUP_PS);
			}
		else
			{
			/* Access the base receivers for computation. */
			if (base_rxgroup == DrgmC_Base_Rxgroup_From_Static)
				{
				rxch_obj_count = tmp_txch_info_ptr->rxgroup_count;
				}
			else
				{
				rxch_obj_count = op_topo_object_count (OPC_OBJTYPE_RARXCH);
				}
		
			/* Iterate through Rx channel object and compute validity. */
			for (rxch_index = 0; rxch_index < rxch_obj_count; rxch_index++)
				{
				/* Access receiver channel from correct set. */
				if (base_rxgroup == DrgmC_Base_Rxgroup_From_Static)
					{
					tmp_rxch_objid = tmp_txch_info_ptr->rxgroup_pptr [rxch_index];
					}
				else
					{
					tmp_rxch_objid = op_topo_object (OPC_OBJTYPE_RARXCH, rxch_index);
					}
		
				/* Skip the receiver channel if it is in the same node with	*/
				/* the transmitter and exclusion of the local receivers is	*/
				/* enabled.													*/
				if (consider_local_receivers == OPC_BOOLINT_DISABLED && 
					op_topo_parent (op_topo_parent (op_topo_parent (tmp_rxch_objid))) == tmp_txch_info_ptr->node_objid)
					continue;
				
				/* We start by comparing channels. */
				if (dynamic_rx_group_ch_match (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid))
					{
					/* If we are "OR" ing distance and SNR... */
					if (selection_criteria == DrgmC_Sel_Criteria_Ch_N_Dist_Or_Snr)
						{
						if (dynamic_rx_group_dist_evaluate (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid))
							{
							/* This is sufficient condition. */
							op_radio_txch_rxch_add (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid);
							}
						else
							{
							if (dynamic_rx_group_pathloss_evaluate (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid))
								{						
								/* This is sufficient condition. */
								op_radio_txch_rxch_add (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid);
								}
							}
						}
				
					/* If we are "AND" ing distance and SNR... */
					if (selection_criteria == DrgmC_Sel_Criteria_Ch_N_Dist_N_Snr)
						{
						if (dynamic_rx_group_dist_evaluate (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid) &&
							dynamic_rx_group_pathloss_evaluate (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid))
							{						
							/* This is sufficient condition. */
							op_radio_txch_rxch_add (tmp_txch_info_ptr->txch_objid, tmp_rxch_objid);
							}
						}
					}
				}/* End of for loop for rxch. */
			}
		}/* End of for loop for txch. */
	
	FOUT;
	}


static void
dynamic_rx_group_next_computation_schedule (void)
	{
	double			next_schedule_time;
	
	/** Schedule the next computation time for Rx	**/
	/** group calculation for the transmitters		**/
	FIN (dynamic_rx_group_next_computation_schedule (void));
	
	/* If the refresh period is set to "Never",	*/
	/* then do not recompute the Rx group		*/
	if (refresh_period == DrgmC_Calc_Never)
		FOUT;
	
	/* Schedule next computation. */
	next_schedule_time = op_sim_time () + (double) refresh_period;
	if ((end_time == DrgmC_Sim_End_Time) || (next_schedule_time <= (double) end_time))
		{
		op_intrpt_schedule_self (next_schedule_time, 0);
		}
	
	FOUT;
	}


static Boolean
dynamic_rx_group_ch_match (Objid txch_objid, Objid rxch_objid)
	{
	DrgmT_Channel_Info*		tx_channel_info_ptr;
	DrgmT_Channel_Info*		rx_channel_info_ptr;
		
	/** Function to compare two channel attriubute	**/
	/** of a txch and rxch object. 					**/
	FIN (dynamic_rx_group_ch_match (txch_objid, rxch_objid));
	
	if (channel_match == DrgmC_Ch_Match_All)
		{
		/* We do not care. */
		FRET (OPC_TRUE);
		}
	
	/* Access the transmitter channel information from the hash table	*/
	tx_channel_info_ptr = (DrgmT_Channel_Info*) prg_bin_hash_table_item_get (channel_info_table, (const void *)&txch_objid);
	
	if (tx_channel_info_ptr == OPC_NIL)
		{
		/* Error checking	*/
		FRET (OPC_FALSE);
		}
	
	/* Access the receiver channel information from the hash table	*/
	rx_channel_info_ptr = (DrgmT_Channel_Info*) prg_bin_hash_table_item_get (channel_info_table, (const void *)&rxch_objid);
	
	if (rx_channel_info_ptr == OPC_NIL)
		{
		/* Error checking	*/
		FRET (OPC_FALSE);
		}
		
	if (channel_match == DrgmC_Ch_Match_Exact)
		{
		/* All the channel characteristic has to match. */
		if ((tx_channel_info_ptr->drate == rx_channel_info_ptr->drate) 	&& 
			(tx_channel_info_ptr->bw == rx_channel_info_ptr->bw) 		&& 
			(tx_channel_info_ptr->freq == rx_channel_info_ptr->freq) 	&&
			(tx_channel_info_ptr->code == rx_channel_info_ptr->code) 	&& 
			(tx_channel_info_ptr->mod == rx_channel_info_ptr->mod))
			{
			/* All the channel characteristics match. */
			FRET (OPC_TRUE);
			}
			
		FRET (OPC_FALSE);
		}
			
	if (channel_match == DrgmC_Ch_Match_Exact_Or_Partial)
		{
		if ((tx_channel_info_ptr->drate == rx_channel_info_ptr->drate) && 
			(tx_channel_info_ptr->code == rx_channel_info_ptr->code) && 
			(tx_channel_info_ptr->mod == rx_channel_info_ptr->mod))
			{
			/* Now only the channels overlap... */
			if ((tx_channel_info_ptr->freq < rx_channel_info_ptr->freq + rx_channel_info_ptr->bw / 1000.0) && 
				(tx_channel_info_ptr->freq + tx_channel_info_ptr->bw / 1000.0 > rx_channel_info_ptr->freq))
				{
				FRET (OPC_TRUE);
				}
			}
		
		FRET (OPC_FALSE);
		}

	FRET (OPC_FALSE);
	}
	

static Boolean
dynamic_rx_group_dist_evaluate (Objid txch_objid, Objid rxch_objid)
	{
	double		tx_alt, tx_lat, tx_lon, tx_x, tx_y, tx_z; 
	double		rx_alt, rx_lat, rx_lon, rx_x, rx_y, rx_z, distance;
	Objid		tx_node_objid, rx_node_objid;
	Boolean		tx_node_cache_hit = OPC_FALSE;
	Boolean		rx_node_cache_hit = OPC_FALSE;
	
	/** Function to check distance between a 	**/
	/** transmitter and a receiver of two nodes	**/
	FIN (dynamic_rx_group_dist_evaluate (txch_objid, rxch_objid));
	
	if (distance_threshold == DrgmC_Dist_Evaluate_Any)
		{
		FRET (OPC_TRUE);
		}
	
	/* Find the location information for the transmitter and receiver. */
	tx_node_objid = op_topo_parent (op_topo_parent (op_topo_parent (txch_objid)));
	rx_node_objid = op_topo_parent (op_topo_parent (op_topo_parent (rxch_objid)));

	if (tx_node_objid == cache.tx_node_objid)
		{
		/* We have cache hit.  Use the cached info. */
		tx_node_cache_hit = OPC_TRUE;
		}
	
	if (rx_node_objid == cache.rx_node_objid)
		{
		/* We have cache hit.  Use the cached info. */
		rx_node_cache_hit = OPC_TRUE;
		}
		
	if (tx_node_cache_hit && rx_node_cache_hit)
		{
		/* The pair has cache hit.  Use the cached result. */
		FRET (cache.dist_eval_result);
		}
	
	if (tx_node_cache_hit)
		{
		/* Use the cached information. */
		tx_lat = cache.tx_lat; 
		tx_lon = cache.tx_lon; 
		tx_alt = cache.tx_alt; 
		tx_x = cache.tx_x; 
		tx_y = cache.tx_y; 
		tx_z = cache.tx_z;
		}
	else
		{
		op_ima_obj_pos_get (tx_node_objid,
			&tx_lat, &tx_lon, &tx_alt, &tx_x, &tx_y, &tx_z);
		
		/* Cache the information. */
		cache.tx_node_objid = tx_node_objid;
		cache.tx_lat = tx_lat; 
		cache.tx_lon = tx_lon; 
		cache.tx_alt = tx_alt; 
		cache.tx_x = tx_x; 
		cache.tx_y = tx_y; 
		cache.tx_z = tx_z;
		}
	
	if (rx_node_cache_hit)
		{
		/* Use the cached information. */
		rx_lat = cache.rx_lat; 
		rx_lon = cache.rx_lon; 
		rx_alt = cache.rx_alt; 
		rx_x = cache.rx_x; 
		rx_y = cache.rx_y; 
		rx_z = cache.rx_z;
		}
	else
		{
		op_ima_obj_pos_get (rx_node_objid,
			&rx_lat, &rx_lon, &rx_alt, &rx_x, &rx_y, &rx_z);
		
		/* Cache the information. */
		cache.rx_node_objid = rx_node_objid;
		cache.rx_lat = rx_lat; 
		cache.rx_lon = rx_lon; 
		cache.rx_alt = rx_alt; 
		cache.rx_x = rx_x; 
		cache.rx_y = rx_y; 
		cache.rx_z = rx_z;
		}
		
	/* See if this falls below the threshold value. */
	if (distance_threshold == DrgmC_Dist_Evaluate_LOS)
		{
		/* Check if we have direct LOS between the pair. */
		cache.dist_eval_result = dynamic_rx_group_simple_earth_LOS_closure (tx_x, tx_y, tx_z, tx_alt,
			rx_x, rx_y, rx_z, rx_alt);
		
		FRET (cache.dist_eval_result);
		}
	else
		{
		/* Find the distance between the tx_rx pair. */
		distance = prg_geo_lat_long_distance_get (tx_lat, tx_lon, tx_alt, rx_lat, rx_lon, rx_alt);
		
		if ((double) distance_threshold >= distance)
			{
			cache.dist_eval_result = OPC_TRUE;
			}
		else
			{
			cache.dist_eval_result = OPC_FALSE;
			}
		}
	
	FRET (cache.dist_eval_result);
	}


static DrgmT_Channel_Info*
dynamic_rx_group_channel_info_mem_alloc (void)
	{
	static Pmohandle		channel_info_pmh;
	DrgmT_Channel_Info*		channel_info_ptr = OPC_NIL;
	static Boolean			channel_info_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for storing channel	information	**/
	/** for every channel on every transceiver					**/
	FIN (dynamic_rx_group_channel_info_mem_alloc (void));
	
	if (channel_info_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for the packet	*/
		/* entry in the send buffer if not already done	*/
		channel_info_pmh = op_prg_pmo_define ("Channel Info Entry", sizeof (DrgmT_Channel_Info), 32);
		channel_info_pmh_defined = OPC_TRUE;
		}
	
	channel_info_ptr = (DrgmT_Channel_Info*) op_prg_pmo_alloc (channel_info_pmh);
	
	FRET (channel_info_ptr);
	}
	

static DrgmT_Txch_Info*
dynamic_rx_group_static_info_mem_alloc (void)
	{
	static Pmohandle		static_info_pmh;
	DrgmT_Txch_Info*		static_info_ptr = OPC_NIL;
	static Boolean			static_info_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for storing static Rx group	**/
	/** information calculated before the beginning of sim	**/
	FIN (dynamic_rx_group_static_info_mem_alloc (void));
	
	if (static_info_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for the packet	*/
		/* entry in the send buffer if not already done	*/
		static_info_pmh = op_prg_pmo_define ("Static Rx Group Info Entry", sizeof (DrgmT_Txch_Info), 32);
		static_info_pmh_defined = OPC_TRUE;
		}
	
	static_info_ptr = (DrgmT_Txch_Info*) op_prg_pmo_alloc (static_info_pmh);
	
	FRET (static_info_ptr);
	}
	

static Boolean
dynamic_rx_group_simple_earth_LOS_closure (double tx_x, double tx_y, double tx_z, double tx_alt, 
	double rx_x, double rx_y, double rx_z, double rx_alt)
	{
	Boolean		occlude;
	double		dif_x, dif_y, dif_z, dot_rx_dif;
	double		dot_tx_dif, rx_mag, dif_mag, cos_rx_dif, sin_rx_dif;
	double		orth_drop;

	/** Determinate for closure is direct line of sight	**/
	/** between transmitter	and receiver. The earth is 	**/
	/** modeled as simple sphere.						**/
	FIN (dynamic_rx_group_simple_earth_LOS_closure (<args>));

	/* Calculate difference vector (transmitter to receiver). */
	dif_x = rx_x - tx_x;
	dif_y = rx_y - tx_y;
	dif_z = rx_z - tx_z;

	/* Calculate dot product of (rx) and (dif) vectors. */
	dot_rx_dif = rx_x*dif_x + rx_y*dif_y + rx_z*dif_z;

	/* If angle (rx, dif) > 90 deg., there is no occlusion. */
	if (dot_rx_dif <= 0.0) 
		occlude = OPC_FALSE;
	else
		{
		/* Calculate dot product of (tx) and (dif) vectors. */
		dot_tx_dif = tx_x*dif_x + tx_y*dif_y + tx_z*dif_z;

		/* If angle (tx, dif) < 90) there is no occlusion. */
		if (dot_tx_dif >= 0.0) 
			{
			if ((tx_alt < 0) || (rx_alt < 0))
				{
				occlude = OPC_TRUE;
				}
			else
				{
				occlude = OPC_FALSE;
				}
			}
		else
			{
			/* Calculate magnitude of (rx) and (dif) vectors. */
			rx_mag = sqrt (rx_x*rx_x + rx_y*rx_y + rx_z*rx_z);
			dif_mag = sqrt (dif_x*dif_x + dif_y*dif_y + dif_z*dif_z);

			/* Calculate sin (rx, dif). */
			cos_rx_dif = dot_rx_dif / (rx_mag * dif_mag);
			sin_rx_dif = sqrt (1.0 - (cos_rx_dif * cos_rx_dif));

			/* Calculate length of orthogonal drop	*/
			/* from (dif) to earth center.			*/
			orth_drop = sin_rx_dif * rx_mag;

			/* The satellites are occluded iff this distance	*/
			/* is less than the earth's radius.					*/
			if (orth_drop < DrgmC_Earth_Radius)
				occlude = OPC_TRUE;
			else
				occlude = OPC_FALSE;
			}
		}

	FRET ((occlude == OPC_FALSE) ? OPC_TRUE : OPC_FALSE);
	}                


static Boolean
dynamic_rx_group_pathloss_evaluate (Objid txch_objid, Objid rxch_objid)
	{
	static Boolean			tmm_init = OPC_FALSE;
	double					tx_alt, tx_lat, tx_lon, tx_x, tx_y, tx_z; 
	double					rx_alt, rx_lat, rx_lon, rx_x, rx_y, rx_z, distance;
	Objid					tx_node_objid, rx_node_objid;
	double					path_loss, tx_center_freq;
	DrgmT_Channel_Info*		tx_channel_info_ptr;
	
	/** Function to check pathloss between a txch and rxch object. **/
	FIN (dynamic_rx_group_pathloss_evaluate (txch_objid, rxch_objid));
	
	/* Check if we need to bother. */
	if (DrgmC_Pathloss_Evaluate_Any == pathloss_threshold)
		{
		FRET (OPC_TRUE);
		}
	
	if (!tmm_init)
		{
		/* Initialize TMM here. */
		dynamic_rx_group_tmm_closure_init ();
		
		tmm_init = OPC_TRUE;
		}
			
	/* Find the location information for the transmitter and receiver. */
	tx_node_objid = op_topo_parent (op_topo_parent (op_topo_parent (txch_objid)));
	rx_node_objid = op_topo_parent (op_topo_parent (op_topo_parent (rxch_objid)));

	if (tx_node_objid == cache.tx_node_objid)
		{
		/* Use the cached information. */
		tx_lat = cache.tx_lat;
		tx_lon = cache.tx_lon; 
		tx_alt = cache.tx_alt; 
		tx_x = cache.tx_x; 
		tx_y = cache.tx_y; 
		tx_z = cache.tx_z;
		}
	else
		{
		op_ima_obj_pos_get (tx_node_objid,
			&tx_lat, &tx_lon, &tx_alt, &tx_x, &tx_y, &tx_z);
	
		/* Cache the information. */
		cache.tx_node_objid = tx_node_objid;
		cache.tx_lat = tx_lat; 
		cache.tx_lon = tx_lon; 
		cache.tx_alt = tx_alt; 
		cache.tx_x = tx_x; 
		cache.tx_y = tx_y; 
		cache.tx_z = tx_z;
		}
	
	if (rx_node_objid == cache.rx_node_objid)
		{
		/* Use the cached information. */
		rx_lat = cache.rx_lat; 
		rx_lon = cache.rx_lon; 
		rx_alt = cache.rx_alt; 
		rx_x = cache.rx_x; 
		rx_y = cache.rx_y; 
		rx_z = cache.rx_z;
		}
	else
		{
		op_ima_obj_pos_get (rx_node_objid,
				&rx_lat, &rx_lon, &rx_alt, &rx_x, &rx_y, &rx_z);
	
		/* Cache the information. */
		cache.rx_node_objid = rx_node_objid;
		cache.rx_lat = rx_lat; 
		cache.rx_lon = rx_lon; 
		cache.rx_alt = rx_alt; 
		cache.rx_x = rx_x; 
		cache.rx_y = rx_y; 
		cache.rx_z = rx_z;
		}
	
	/* Get the channel information to go along. */
	/* Access the transmitter channel information from the hash table	*/
	tx_channel_info_ptr = (DrgmT_Channel_Info*) prg_bin_hash_table_item_get (channel_info_table, (const void *)&txch_objid);
	
	/* Get transmission frequency in MHz. */
	tx_center_freq = tx_channel_info_ptr->freq + (tx_channel_info_ptr->bw / 2000.0);

	if (DrgmS_Closure_As_Simple_Earth)
		{
		/* Compute the distance info. */
		distance = prg_geo_lat_long_distance_get (tx_lat, tx_lon, tx_alt, rx_lat, rx_lon, rx_alt);
		
		/* Calculate the path loss using free space model. */
		path_loss = -(20.0 * log10 (tx_center_freq) + 20.0 * log10 (distance) - 27.558);
		}
	else
		{
		/* Use the TMM model in computing pathloss. */
		/* TMM expects frequency info in Hz. */
		if (OPC_COMPCODE_SUCCESS != dynamic_rx_group_tmm_pathloss_calc (tx_center_freq * 1000000.0, 
			tx_channel_info_ptr->bw * 1000.0, tx_lat, tx_lon, tx_alt, rx_lat, rx_lon, rx_alt,
			&path_loss, tx_node_objid, rx_node_objid))
			{
			/* Revert back to free space calculation. */
			distance = prg_geo_lat_long_distance_get (tx_lat, tx_lon, tx_alt, rx_lat, rx_lon, rx_alt);
		
			/* Calculate the path loss using free space model. */
			path_loss = -(20.0 * log10 (tx_center_freq) + 20.0 * log10 (distance) - 27.558);
			}
		}
		   
	/* Compare the calculated path loss informatin against the user specified value. */
	FRET ((path_loss >= pathloss_threshold) ? OPC_TRUE:OPC_FALSE);
	}


static void
dynamic_rx_group_tmm_closure_init (void)
	{
	int				using_tmm;
	Log_Handle		tmm_problem_log_handle;
	int				load_successful;
	char			line0_buf [512];
	char			line1_buf [512];

	/** This function is invoked once at the start of the simulation.	**/
	/** Options that will be used throughout the simulation duration are**/
	/** established in this function.									**/
	FIN (dynamic_rx_group_tmm_closure_init ());

	tmm_problem_log_handle = op_prg_log_handle_create (
		OpC_Log_Category_Configuration, 
		"TMM", "closure stage loading of propagation model", 20);
	/* If the nodes are out of valid computation areas 		*/
	/* such as for Longley-Rice, a very large number of log	*/
	/* entries could be generated.  Stop recording after 500*/
	DrgmS_TMM_Verbose_Log_H = op_prg_log_handle_create (
		OpC_Log_Category_Lowlevel, 
		"TMM", "path loss calculation", 500);

	/* Determine if we are executing as a simulation that wants to use TMM	*/
	if (prg_env_attr_value_get (PrgC_Env_Attr_Boolean, TMMC_ENV_SIMULATE, 
			&using_tmm) == PrgC_Compcode_Failure)
		using_tmm = OPC_FALSE;

	if (using_tmm == OPC_FALSE)
		{
		/* Simulation is not using TMM for path loss calculations.	*/
		/* use the sphericall closure model.		*/
		DrgmS_Closure_As_Simple_Earth = OPC_TRUE;
		if (tmm_verbose_get ())
			{
			op_prg_log_entry_write (tmm_problem_log_handle,
				"TMM initialization: using simple, default closure model of line-of-sight with spherical earth.");
			}
		}
	else
		{
		/** Simulation is using TMM for propagation.	**/
		load_successful = OPC_FALSE;
		
		/* Attempt to load the default TMM propagation model.	*/
		DrgmS_Closure_Tmm_Prop_Model_Name = tmm_default_propagation_model_get ();
		if (strcmp ("NONE", DrgmS_Closure_Tmm_Prop_Model_Name) == 0)
			{
			/* Propagation model called NONE, is speial and indicates that	*/
			/* no default propagation model is set. Use free space.			*/
			load_successful = OPC_FALSE;

			/* Use the simple shperical-earth line-of-sight closure model */
			DrgmS_Closure_As_Simple_Earth = OPC_TRUE;
			if (tmm_verbose_get ())
				{
				op_prg_log_entry_write (tmm_problem_log_handle,
					"TMM initialization: using simple, default closure model of line-of-sight with spherical earth.");
				}
			FOUT
			}
		
		DrgmS_Closure_Prop_Model_Ptr = tmm_propagation_model_get (DrgmS_Closure_Tmm_Prop_Model_Name);
		if (DrgmS_Closure_Prop_Model_Ptr == OPC_NIL)
			{
			/* Failed to load the requested model */
			/* Most likely cause is missing model from mod_dirs. */
			load_successful = OPC_FALSE;

			sprintf (line0_buf, "TMM: unable to load propagation model (%s). ",
				DrgmS_Closure_Tmm_Prop_Model_Name);
			strcpy (line1_buf, "Using default closure instead.");

			/* print message */
			op_sim_message (line0_buf, line1_buf);

			/* log message */
			op_prg_log_entry_write (tmm_problem_log_handle, 
				"Pipeline stage 'closure' during initialization for\n"
				"%s\n"
				"%s\n"
				"\n"
				"Check that your mod_dirs contains all 3 of the needed\n"
				"propagation model files (.prop.d, .prop.p and .prop.[so/dll]).\n",
				line0_buf,
				line1_buf);

			}
		else if (DrgmS_Closure_Prop_Model_Ptr->initialized_ok_flag == OPC_FALSE)
			{
			/* Propagation model's init function was has set the flag	*/
			/* "initialized_ok_flag" to indicate some problem inside		*/
			/* of the propagation model.								*/
			load_successful = OPC_FALSE;

			sprintf (line0_buf,
				"TMM propagation model (%s) reported an initialization problem.", 
				DrgmS_Closure_Tmm_Prop_Model_Name);
			strcpy (line1_buf, "Using default closure instead.");

			/* print message */
			op_sim_message (line0_buf, line1_buf);

			/* log message */
			op_prg_log_entry_write (tmm_problem_log_handle, 
				"Pipeline stage 'closure' during initialization for TMM:\n"
				"%s\n"
				"%s\n",
				line0_buf,
				line1_buf);
			}
		else
			{
			load_successful = OPC_TRUE;

			/* We've successfully obtained the propagation model.	*/
			/* We are going to use the path loss function from the	*/
			/* the model we have loaded.							*/
			DrgmS_Closure_As_Simple_Earth = OPC_FALSE;

			/* If verbose, log message */
			if (tmm_verbose_get ())
				{
				sprintf (line0_buf, 
					"TMM initialization: successfully loaded the propagation model (%s)",
					DrgmS_Closure_Tmm_Prop_Model_Name);
				op_prg_log_entry_write (tmm_problem_log_handle,
					line0_buf);
				}
			}

		if (load_successful == OPC_FALSE)
			{
			/* Use the simple shperical-earth line-of-sight closure model */
			DrgmS_Closure_As_Simple_Earth = OPC_TRUE;
			}
		}

	FOUT
	}

static Compcode
dynamic_rx_group_tmm_pathloss_calc (double tx_center_freq, double tx_bandwidth, double tx_lat, double tx_lon, 
	double tx_alt, double rx_lat, double rx_lon, double rx_alt, double* path_loss, Objid tx_node_objid, 
	Objid rx_node_objid)
	{
	TmmT_Position		tx_position;
	TmmT_Position		rx_position;
	void*				pipeline_invocation_state_ptr;
	int					verbose_active;
	int					str_index;
	char*				msg_str_ptrs [TMMC_LOSS_MESSAGE_BUF_NUM_STRS];
	char				log_str_buf [16 * TMMC_LOSS_MESSAGE_BUF_STR_SIZE];
	double				tmm_model_path_loss_dB;
	TmmT_Loss_Status	tmm_model_loss_status;
	char 				tmm_model_msg_buffer_v [TMMC_LOSS_MESSAGE_BUF_NUM_STRS] [TMMC_LOSS_MESSAGE_BUF_STR_SIZE];
	char				msg_buf0 [256];
	char				msg_buf1 [256];
	char				msg_buf2 [256];
	char				msg_buf3 [256];

	/** Call the propagation model's path loss calucation method.	**/
	/** The function can report:									**/
	/**		a signal loss value										**/
	/**		a error condition (e.g. invalid elevation)				**/
	FIN (dynamic_rx_group_tmm_pathloss_calc (<args>));

	/* Get transmitter's location. */
	tx_position.latitude = tx_lat;
	tx_position.longitude = tx_lon;
	tx_position.elevation = tx_alt;

	/* Get receiver's location. */
	rx_position.latitude = rx_lat;
	rx_position.longitude = rx_lon;
	rx_position.elevation = rx_alt;

	/* Propagation models shipped by OPNET don't use any extra state	*/
	/* so just for clarity set the local state pointer to null.			*/
	/* User created propagation models can pass any value they wish.	*/
	pipeline_invocation_state_ptr = OPC_NIL;

	/* Determine if TMM verbose is requested. */
	/* This can be set using the tmm_verbose environment attribute */
	verbose_active = tmm_verbose_get ();

	/*** Call the propagation model's path_loss_calc_method.			***/
	for (str_index = 0; str_index < TMMC_LOSS_MESSAGE_BUF_NUM_STRS; str_index++)
		tmm_model_msg_buffer_v [str_index] [0] = '\0';
	
	tmm_model_path_loss_dB = 
		DrgmS_Closure_Prop_Model_Ptr->path_loss_calc_method (
			DrgmS_Closure_Prop_Model_Ptr,
			pipeline_invocation_state_ptr,
			&tx_position,
			&rx_position,
			tx_center_freq,
			tx_bandwidth,
			verbose_active,
			&tmm_model_loss_status,
			tmm_model_msg_buffer_v );

	if (tmm_model_loss_status == TmmC_Loss_Error)
		{
		/* An error condition was reported by the propagation model. 	*/
		sprintf (msg_buf0, "Pathloss Calculation: Propagation Model %s:",
			DrgmS_Closure_Tmm_Prop_Model_Name );
		sprintf (msg_buf1, "Model reported error in computing path between transmitter node (ID %d) and receiver node (ID %d)",
			tx_node_objid, rx_node_objid);
		strcpy (msg_buf2, "Using simple-earth closure model for this transmission");
		strcpy (msg_buf3, "Messages reported by propagation model follow:");

		for (str_index = 0; str_index < TMMC_LOSS_MESSAGE_BUF_NUM_STRS; str_index++)
			{
			if (tmm_model_msg_buffer_v [str_index] [0] == '\0')
				{
				/* A null pointer to op_prg_odb_print_ will signify end of arguments */
				msg_str_ptrs [str_index] = OPC_NIL;
				break;
				}
			else
				{
				msg_str_ptrs [str_index] = &tmm_model_msg_buffer_v [str_index] [0];
				}
			}
		if (msg_str_ptrs [0] == OPC_NIL)
			{
			strcpy (msg_buf3, "<no messages reported by the propagation model>");
			}

		/* Entries in the log are added via 1 long format string		*/
		log_str_buf [0] = '\0';
		strcat (log_str_buf, msg_buf0);
		strcat (log_str_buf, "\n");

		strcat (log_str_buf, "  ");
		strcat (log_str_buf, msg_buf1);
		strcat (log_str_buf, "\n");

		strcat (log_str_buf, "  ");
		strcat (log_str_buf, msg_buf2);
		strcat (log_str_buf, "\n");

		strcat (log_str_buf, "  ");
		strcat (log_str_buf, msg_buf3);
		strcat (log_str_buf, "\n");

		for (str_index = 0; str_index < TMMC_LOSS_MESSAGE_BUF_NUM_STRS; str_index++)
			{
			if (tmm_model_msg_buffer_v [str_index] [0] != '\0')
				{
				strcat (log_str_buf, "    ");
				strcat (log_str_buf, tmm_model_msg_buffer_v [str_index]);
				strcat (log_str_buf, "\n");
				}
			else
				{
				/* emtpry string, so no more string in message buffer from propagation model	*/
				break;
				}
			}

		/* Log message indicating that we are reverting to simple freespace model 	*/
		/* for this packet transmission.											*/
		op_prg_log_entry_write (DrgmS_TMM_Verbose_Log_H, log_str_buf);
		
		/* Failed operation. */
		FRET (OPC_COMPCODE_FAILURE);
		}
	else
		{
		/* The propagation model was able to calculate a signal value.	*/
		*path_loss = tmm_model_path_loss_dB;
		}

	FRET (OPC_COMPCODE_SUCCESS);
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
	void receiver_group_config (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_receiver_group_config_init (int * init_block_ptr);
	void _op_receiver_group_config_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_receiver_group_config_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_receiver_group_config_alloc (VosT_Obtype, int);
	void _op_receiver_group_config_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
receiver_group_config (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (receiver_group_config ());

		{


		FSM_ENTER_NO_VARS ("receiver_group_config")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (Init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "Init", "receiver_group_config [Init enter execs]")
				FSM_PROFILE_SECTION_IN ("receiver_group_config [Init enter execs]", state0_enter_exec)
				{
				/* Read in the attributes and initialize the state variables	*/
				dynamic_rx_group_sv_init ();
				
				/* Initialize the transmitter Rx group and channel information	*/
				dynamic_rx_group_txch_info_init ();
				
				/* Initialize the receiver channel information	*/
				dynamic_rx_group_rxch_info_init ();
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (Init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "Init", "receiver_group_config [Init exit execs]")


			/** state (Init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Init", "Wait", "tr_-1", "receiver_group_config [Init -> Wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (Wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "Wait", state1_enter_exec, "receiver_group_config [Wait enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"receiver_group_config")


			/** state (Wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "Wait", "receiver_group_config [Wait exit execs]")
				FSM_PROFILE_SECTION_IN ("receiver_group_config [Wait exit execs]", state1_exit_exec)
				{
				/** Compute the dynamic RX group set	**/
				/** for every transmitter channel		**/
				dynamic_rx_group_compute ();
				
				/* Schedule the next computation time	*/
				dynamic_rx_group_next_computation_schedule ();
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (Wait) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Wait", "Wait", "tr_0", "receiver_group_config [Wait -> Wait : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"receiver_group_config")
		}
	}




void
_op_receiver_group_config_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_receiver_group_config_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_receiver_group_config_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_receiver_group_config_svar function. */
#undef rxgroup_model_lptr
#undef base_rxgroup
#undef selection_criteria
#undef channel_match
#undef distance_threshold
#undef pathloss_threshold
#undef consider_local_receivers
#undef start_time
#undef end_time
#undef refresh_period
#undef all_transmitters
#undef txch_info_lptr
#undef cache
#undef DrgmS_Closure_As_Simple_Earth
#undef DrgmS_Closure_Prop_Model_Ptr
#undef DrgmS_TMM_Verbose_Log_H
#undef DrgmS_Closure_Tmm_Prop_Model_Name
#undef channel_info_table
#undef my_objid
#undef num_rxgroup

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_receiver_group_config_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_receiver_group_config_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (receiver_group_config)",
		sizeof (receiver_group_config_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_receiver_group_config_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	receiver_group_config_state * ptr;
	FIN_MT (_op_receiver_group_config_alloc (obtype))

	ptr = (receiver_group_config_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "receiver_group_config [Init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_receiver_group_config_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	receiver_group_config_state		*prs_ptr;

	FIN_MT (_op_receiver_group_config_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (receiver_group_config_state *)gen_ptr;

	if (strcmp ("rxgroup_model_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rxgroup_model_lptr);
		FOUT
		}
	if (strcmp ("base_rxgroup" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->base_rxgroup);
		FOUT
		}
	if (strcmp ("selection_criteria" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->selection_criteria);
		FOUT
		}
	if (strcmp ("channel_match" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->channel_match);
		FOUT
		}
	if (strcmp ("distance_threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->distance_threshold);
		FOUT
		}
	if (strcmp ("pathloss_threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pathloss_threshold);
		FOUT
		}
	if (strcmp ("consider_local_receivers" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->consider_local_receivers);
		FOUT
		}
	if (strcmp ("start_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->start_time);
		FOUT
		}
	if (strcmp ("end_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->end_time);
		FOUT
		}
	if (strcmp ("refresh_period" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->refresh_period);
		FOUT
		}
	if (strcmp ("all_transmitters" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->all_transmitters);
		FOUT
		}
	if (strcmp ("txch_info_lptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->txch_info_lptr);
		FOUT
		}
	if (strcmp ("cache" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cache);
		FOUT
		}
	if (strcmp ("DrgmS_Closure_As_Simple_Earth" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->DrgmS_Closure_As_Simple_Earth);
		FOUT
		}
	if (strcmp ("DrgmS_Closure_Prop_Model_Ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->DrgmS_Closure_Prop_Model_Ptr);
		FOUT
		}
	if (strcmp ("DrgmS_TMM_Verbose_Log_H" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->DrgmS_TMM_Verbose_Log_H);
		FOUT
		}
	if (strcmp ("DrgmS_Closure_Tmm_Prop_Model_Name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->DrgmS_Closure_Tmm_Prop_Model_Name);
		FOUT
		}
	if (strcmp ("channel_info_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->channel_info_table);
		FOUT
		}
	if (strcmp ("my_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_objid);
		FOUT
		}
	if (strcmp ("num_rxgroup" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_rxgroup);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

