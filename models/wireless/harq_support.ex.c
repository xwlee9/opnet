/****************************************/
/*     Copyright (c) 1987-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*       (A Delaware Corporation)      	*/
/*    7255 Woodmont Av., Suite 250     	*/
/*     Bethesda, MD 20814, U.S.A.      	*/
/*       All Rights Reserved.          	*/
/****************************************/

/* Procedures for dealing with HARQ packet transmission/reception.																								*/

#include <opnet.h>
#include <string.h>
#include <stdlib.h>
#include <prg_bin_hash.h>
#include <math.h>

/* HARQ specific header																																			*/
#include <harq_support.h>

/* Visual C++ 6.0 does not have math constants	*/
#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif	

/* Internal data structures.																																	*/

/* Transmission state of an HARQ channel.																														*/
typedef struct HarqT_Transmission_State
	{
	Packet*						harq_pkptr;
	int							num_rxmt;
	HarqT_Packet_Info*			harq_pk_info_ptr; 				/* HARQ information of the transmitted packet.													*/
	void*						harq_tx_state_info_ptr; 		/* Generic pointer for client specific data structures. 										*/
	Boolean						is_packet_transmission_conceived; 	/* Asking for a new HARQ channel starts the packet transmission process.					*/
	double						transmit_time;					/* For statistic writing...to compute end to end delay of a successful HARQ transmission.		*/	
	} HarqT_Transmission_State;

/* Receiver state of an HARQ channel.																															*/
typedef struct HarqT_Receiver_State
	{
	Boolean					is_new_transmission; 				/* Whether the packet that was received was new transmission.									*/
	Boolean					is_packet_decoded; 					/* This makes the receiver knows whether to ACK or NACK											*/
	double					accumulated_snr_db;
	Boolean					is_packet_received;					/* Packets can lost unpredictably. The caller needs a mechanism to know if packets are received	*/	
	void*					harq_rx_state_info_ptr; 			/* Generic pointer for client specific data structures.											*/
	} HarqT_Receiver_State;

/* Information about channels with acknowledgement timer (in integer units...description below).																*/
/* We assume that HARQ acknowledgement time is described in units of data (integer). In WiMAX, this turns														*/
/* out to be number of frames. For other technologies, it may be approximated as a multiple of some basic														*/
/* time interval.																																				*/
typedef struct HarqT_Explicit_Ack_Channel_Info
	{
	int*										blocked_chan_array;		/* Each element indicates a blocked channel waiting to be scheduled for explicit ack	*/
	int											num_blocked_channels;
	struct	HarqT_Explicit_Ack_Channel_Info*	next;
	struct	HarqT_Explicit_Ack_Channel_Info*	tail; 					/* For easy access, store the tail only at the head										*/
	} HarqT_Explicit_Ack_Channel_Info;

/* This structure was predefined in harq.h.																														*/
struct HarqT_Conn_Info
	{
	HarqT_Conn_Parameters*		harq_conn_params_ptr;
	HarqT_Transmission_State*	harq_tx_state_ptr;
	HarqT_Receiver_State*		harq_rx_state_ptr;
	HarqT_Entity_Role			harq_role;
	OpT_Int64					connection_identifier; 			/* To look up this connection in a hash table if needed, store the key as well for easy access.	*/
	};	

/* Global data structures.																																		*/
PrgT_Bin_Hash_Table*			harq_conn_params_table;
PrgT_Bin_Hash_Table*			harq_tx_elem_table;
PrgT_Bin_Hash_Table*			harq_rx_elem_table;
PrgT_Bin_Hash_Table*			harq_available_chan_table; 
PrgT_Bin_Hash_Table*			harq_ack_waiting_chan_table;
PrgT_Bin_Hash_Table*			harq_explicit_ack_chan_table;

/* Flag to indicate if the package was initialized.																												*/
Boolean							is_harq_package_initialized = OPC_FALSE;

/* Categorized memory handle to categorize all HARQ related memory																								*/
/* No need for multiple pools, as HARQ memory is fairly constant.																								*/
Cmohandle						harq_cat_mem_handle;

/* Macro definition for string handling.                            																							*/
#define PKPRINT_STRING_INSERT(str, contents, list)                             \
	{                                                                          \
	str = (char*) op_prg_mem_alloc ((strlen (contents) + 1) * sizeof (char));  \
	strcpy (str, contents);                                                    \
	op_prg_list_insert (list, str, OPC_LISTPOS_TAIL);                          \
	}
	
/************************************Internal Function Prototypes************************************************************************************************/
Compcode 					harq_support_transmission_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr);
Compcode 					harq_support_receiver_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr);
Compcode 					harq_support_avail_chan_list_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr);
Compcode 					harq_support_ack_waiting_chan_list_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr);
Compcode 					harq_support_explicit_ack_chan_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr);
void						harq_support_explicit_timed_ack_schedule (OpT_Int64 connection_identifier, int harq_channel);
void						harq_support_transmission_channel_free (OpT_Int64 connection_identifier, int harq_channel, HarqT_Ack_Method ack_method);
void						harq_support_channel_ack_waiting_insert (OpT_Int64 connection_identifier, int* harq_channel_id_ptr);
void						harq_support_explicit_ack_transmission_channel_find (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, 
										int* harq_channel_ptr, OpT_Packet_Size* size_fitted_ptr);
Boolean						harq_support_implicit_ack_transmission_channel_find (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, 
										int* harq_channel_ptr, OpT_Packet_Size* size_fitted_ptr);
void						harq_support_buffer_packet_fit_calculate (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, 
										int harq_channel, OpT_Packet_Size* size_fitted_ptr);
void						harq_support_ack_method_return (int ack_method, char* ack_method_str);
void						harq_support_caller_role_return (int caller_role, char* caller_role_str);
void						harq_support_buffer_aggr_return (Boolean buffer_aggr_flag, char* buffer_aggr_str);
Boolean						harq_support_channel_trace_active (OpT_Int64 connection_identifier, int harq_channel_id);
void 						harq_support_pk_info_print (void* field_ptr, PrgT_List* list_ptr);
/****************************************************************************************************************************************************************/

HarqT_Conn_Info*
harq_support_conn_params_register (HarqT_Conn_Parameters* harq_conn_params_ptr, HarqT_Entity_Role harq_role, OpT_Int64 connection_identifier)
	{
	HarqT_Conn_Info*				conn_info_ptr;
	HarqT_Transmission_State*		harq_transmission_state_ptr;
	HarqT_Receiver_State*			harq_reception_state_ptr;

	FIN (harq_support_conn_params_register (...));
	
	/* Step 1: Check for the sanity of attribute values.																										*/
	if (harq_conn_params_ptr->ack_delay < 0)
		{
		op_sim_end ("invalid HARQ configuration: ack delay must be at least 0", "", "", "");
		
		if (harq_conn_params_ptr->ack_delay > 0)
			harq_conn_params_ptr->ack_delay += 1;
		}
	
	if (harq_conn_params_ptr->num_channels <= 0)
		op_sim_end ("at least 1 HARQ channel must be present", "Upper limit is technology specific", 
			"This package does not check validity of upper limit yet", "");
	
	if (harq_conn_params_ptr->max_rxmt < 1)
		op_sim_end ("\nAt least 1 retransmission attempts are necessary for HARQ", "", "", "");
	
	if (harq_conn_params_ptr->buffer_size <= 0)
		op_sim_end ("\nHARQ buffer size must have at least 1 bit", "", "", "");
	
	if ((harq_conn_params_ptr->ack_method != HarqC_Ack_Implicit) && (harq_conn_params_ptr->ack_method != HarqC_Ack_Explicit))
		op_sim_end ("\nending in harq_support_conn_params_register", 
			"please indicate if this connection supports implicit or explicit acknowledgement", "", "");
	
	/* If the package has not been initialized yet, first initialize it.																						*/
	if (!is_harq_package_initialized)
		{
		/* Allocate memory for hash tables.																														*/
		harq_conn_params_table 						= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64));
		harq_tx_elem_table 							= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64));
		harq_rx_elem_table 							= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64));
		harq_available_chan_table 					= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64)); 
		harq_explicit_ack_chan_table 				= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64));
		harq_ack_waiting_chan_table 				= prg_bin_hash_table_create ((int) ceil (log (64)/M_LN2), sizeof (OpT_Int64));
		
		/* Get a categorized memory handle.																														*/
		harq_cat_mem_handle = op_prg_cmo_define ("HARQ Memory");
		
		/* Reset package initialization variable.																												*/
		is_harq_package_initialized = OPC_TRUE;
		}
	
	/* Step 2: This function returns a connection handle to the caller. Allocate memory for it.																	*/
	conn_info_ptr = (HarqT_Conn_Info*) op_prg_cmo_alloc (harq_cat_mem_handle, sizeof (HarqT_Conn_Info));	
	if (conn_info_ptr == OPC_NIL)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", "memory allocation for conn_info_ptr failed");	
	
	/* Also assign the info ptr to the connection handle.																										*/
	conn_info_ptr->harq_conn_params_ptr = harq_conn_params_ptr;
	
	/* Fill out the role of this connection.																													*/
	conn_info_ptr->harq_role = harq_role;
	
	/* Store the hash key.																																		*/
	conn_info_ptr->connection_identifier = connection_identifier;

	/* Step 3: Insert incoming harq element in harq_conn_params_table																							*/ 
	/* Since the function is called by both the end points of the connection, first check if this element already exists.										*/
	if (prg_bin_hash_table_item_get (harq_conn_params_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL)
		prg_bin_hash_table_item_insert (harq_conn_params_table, (OpT_Int64*) (&connection_identifier), (void*) harq_conn_params_ptr, OPC_NIL);
	
	/* Step 4: Create appropriate transmission/receiver elements for this connection and insert them in hash tables.											*/
	/* The transmission element is only created if the transmitter is doing the registration for HARQ connection.												*/
	if ((prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL) && 
		(harq_role == HarqC_Transmitter) && (harq_support_transmission_elem_create (connection_identifier, conn_info_ptr) == OPC_COMPCODE_FAILURE))
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", 
			"memory allocation failed in harq_support_transmission_elem_create");
	
	/* It is possible that the transmission element exists already. If so, simply stick it to the incoming handle and return it to the caller.					*/
	else if (((prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier)) != PRGC_NIL)) && (harq_role == HarqC_Transmitter)) 
		{
		harq_transmission_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		conn_info_ptr->harq_tx_state_ptr = harq_transmission_state_ptr;
		conn_info_ptr->harq_rx_state_ptr = OPC_NIL;
		}
	
	/* The receiver element is only created if the receiver is doing the registration for HARQ connection.														*/
	if ((prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL) && 
		(harq_role == HarqC_Receiver) && (harq_support_receiver_elem_create (connection_identifier, conn_info_ptr) == OPC_COMPCODE_FAILURE))
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", 
			"memory allocation failed in harq_support_receiver_elem_create");
	
	/* It is possible that the receiver element exists already. If so, simply stick it to the incoming handle and return it to the caller.						*/
	else if ((prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier)) != PRGC_NIL) && (harq_role == HarqC_Receiver))
		{
		harq_reception_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		conn_info_ptr->harq_tx_state_ptr = OPC_NIL;
		conn_info_ptr->harq_rx_state_ptr = harq_reception_state_ptr;
		}
	
	/* Step 5: Create the open and blocked channel information for the given HARQ connection.																	*/
	if ((prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL) && 
		(harq_support_avail_chan_list_create (connection_identifier, conn_info_ptr) == OPC_COMPCODE_FAILURE))
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", 
			"memory allocation failed in harq_support_avail_chan_list_create");
	
	if ((prg_bin_hash_table_item_get (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL) && 
		(harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit) && (harq_support_explicit_ack_chan_elem_create (connection_identifier, conn_info_ptr) == OPC_COMPCODE_FAILURE))
			op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", 
				"memory allocation failed in harq_support_explicit_ack_chan_elem_create");
	
	/* Step 6: For both implicit/explicit connections, create a ack_waiting list and insert it in the hash table.												*/
	if ((prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier)) == PRGC_NIL) && 
		(harq_support_ack_waiting_chan_list_create (connection_identifier, conn_info_ptr) == OPC_COMPCODE_FAILURE))
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_register", 
			"memory allocation failed in harq_support_ack_waiting_chan_list_create");
	
	/* If we came here, registration was successful.																											*/
	FRET (conn_info_ptr);
	}

/* Given connection information, find HARQ parameters.																											*/
HarqT_Conn_Parameters*
harq_support_conn_params_get (HarqT_Conn_Info* conn_info_ptr)
	{
	FIN (harq_support_conn_params_get (...));
	
	if (conn_info_ptr == OPC_NIL)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_conn_params_get", 
					"invalid harq handle provided");
	
	FRET (conn_info_ptr->harq_conn_params_ptr);
	}
		

/* This generic function finds a HARQ transmission channel from the given HARQ transmission element.															*/
/* This function takes packet_size as an input, and finds an open channel.																						*/
Boolean 
harq_support_transmission_channel_find (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, int* harq_channel_ptr, OpT_Packet_Size* size_fitted_ptr)
	{
	Boolean					channel_from_ack_waiting = OPC_FALSE;					/* For implicit acks, to indicate if the channel comes from ack waiting.	*/
	
	FIN (harq_support_transmission_channel_find (...));
	
	/* If the caller is asking to find a new channel:																											*/
	if (*harq_channel_ptr == HARQC_REQUEST_TRANSMISSION_CHANNEL)
		{
		/* Depending upon whether this connection is for UL or DL, a channel may be returned from a different location.											*/
		if (conn_info_ptr->harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit)
			{
			harq_support_explicit_ack_transmission_channel_find (conn_info_ptr, packet_size, harq_channel_ptr, size_fitted_ptr);
			
			/* Return OPC_FALSE by default.																														*/	
			FRET (OPC_FALSE);
			}
		else
			{
			channel_from_ack_waiting = harq_support_implicit_ack_transmission_channel_find (conn_info_ptr, packet_size, harq_channel_ptr, size_fitted_ptr);
		
			/* Return the channel_from_ack_waiting flag...this does not matter for explicit acknowledgements but of consequences for implicit acks.				*/
			FRET (channel_from_ack_waiting);
			}
		}
	
	/* The caller is attempting to reuse the existing channel. Find if the incoming packet_size can be fitted.													*/
	else
		{
		harq_support_buffer_packet_fit_calculate (conn_info_ptr, packet_size, *harq_channel_ptr, size_fitted_ptr);
		
		/* Simply return OPC_FALSE																																*/
		FRET (OPC_FALSE);
		}
	}

/* The following function marks the HARQ connection with the beginning of the HARQ transmission on the given HARQ channel.										*/
void
harq_support_transmission_inception_indicate (HarqT_Conn_Info* conn_info_ptr, int harq_channel)
	{
	OpT_Int64								connection_identifier;
	HarqT_Transmission_State*				harq_tx_state_arr;
	HarqT_Receiver_State*					harq_rx_state_arr;
		
	FIN (harq_support_transmission_inception_indicate (...));
	
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Get the transmission and receiver elements. 																												*/
	if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
		{
		harq_tx_state_arr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		harq_rx_state_arr = conn_info_ptr->harq_rx_state_ptr;
		}
	else
		{
		harq_tx_state_arr = conn_info_ptr->harq_tx_state_ptr;
		harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		}

	/* Now set the flag on this channel that indicates beginning of the transmission.																			*/
	harq_tx_state_arr [harq_channel].is_packet_transmission_conceived = OPC_TRUE;
	
	/* At the beginning of a new transmission, HARQ packet reception flag is set to FALSE.																		*/
	harq_rx_state_arr [harq_channel].is_packet_received = OPC_FALSE;
	
	/* User specified HARQ connection/channel trace.																											*/
	if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
		{
		char			msg [1024];
		char			msg1 [64];	
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel); 
		sprintf (msg, "\nConceiving transmission on channel");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}


	FOUT;
	}

Packet*
harq_support_packet_transmission_prepare (HarqT_Conn_Info* conn_info_ptr, Packet* pkptr, int harq_channel)
	{
	OpT_Int64								connection_identifier;
	HarqT_Receiver_State*					harq_rx_state_arr;
	HarqT_Packet_Info*						harq_pk_info_ptr;
	Packet*									packet_to_be_transmitted = OPC_NIL;
	
	FIN (harq_support_packet_transmission_prepare (...));
	
	/* Assume receiving this packet will be successful before the start of any new transmission/retransmission.													*/
	connection_identifier = conn_info_ptr->connection_identifier;
	harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
	harq_rx_state_arr [harq_channel].is_packet_decoded = OPC_TRUE;
	
	/* Find if this is a retransmission.																														*/
	if (pkptr == OPC_NIL)
		{
		/* Access the transmission element corresponding to incoming HARQ channel.																				*/
		/* Find if an packet exists at this location.																											*/
		if (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr == OPC_NIL)
			/* Something is wrong. Can't retransmit...end simulation.																							*/
			op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_packet_transmission_prepare", 
				"attempting retransmission of a nonexistant packet");
		
		/* Otherwise find the number of retransmission attempts.																								*/
		else if (conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt == conn_info_ptr->harq_conn_params_ptr->max_rxmt)
			/* Something is wrong. A packet cannot be scheduled for retransmission if maximum number of retries have been attempted.							*/
			op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_packet_transmission_prepare", 
				"Attempting retransmission when max retransmission tries have been over");
		else
			{
			/* Copy the packet from the transmission channel 																									*/
			packet_to_be_transmitted = op_pk_copy (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr);
			
			/* Indicate that this packet is a retransmission.																									*/
			/* The packet and its copies carry a constant memory location indexed to the transmission element.													*/
			/* It is enough to access this memory and make the appropriate change.																				*/
			harq_pk_info_ptr = conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pk_info_ptr;
			harq_pk_info_ptr->is_new_transmission = OPC_FALSE;
			
			/* Increase the retransmission attempts of this packet on the transmitter channel																	*/
			conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt += 1;
			
			/* User specified HARQ connection/channel trace.																									*/
			if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
				{
				char			msg [1024];
				char			msg1 [64];	
				sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel); 
				sprintf (msg, "\nRetransmitting %dth time: packet: "OPC_INT64_FMT", packet size: "OPC_INT64_FMT, 
					conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt, 
					op_pk_id (packet_to_be_transmitted), op_pk_total_size_get (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr));
				strcat (msg, msg1);
				op_prg_odb_print_major (msg, OPC_NIL);
				}
			
			/* Retransmission rate statistic can be written here.																								*/	
			if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->rxt_rate_stathandle))
				{
				op_stat_write (conn_info_ptr->harq_conn_params_ptr->rxt_rate_stathandle, 1.0);
				op_stat_write (conn_info_ptr->harq_conn_params_ptr->rxt_rate_stathandle, 0.0);
				}
			}
		
		FRET (packet_to_be_transmitted);
		}
	
	/* This is a new packet.																																	*/
	else
		{
		/* If there is already an existing packet on the transmission channel, destroy this packet. it is OK to have some pending packets on the channel		*/
		/* as these are leftovers from the transiency period of the mobility. We do not know if this packet is acknowledged or not, so don't write any stat.	*/
		if (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr != OPC_NIL)
			{
			op_pk_destroy (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr);
			conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr = OPC_NIL;
			conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt = 0;
			}

		/* Encode the incoming packet with HARQ specific information.																						*/
		/* Set the elements of the structure harq_pk_info_ptr																								*/
		harq_pk_info_ptr = conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pk_info_ptr;
		harq_pk_info_ptr->connection_identifier = conn_info_ptr->connection_identifier; /* what is the connection ID of this HARQ transmission				*/
			
		/* The function transmission_channel_find has set a pointer when it gave the channel to the caller at the element									*/
		/* harq_chan_id_ptr.																																*/			
			
		harq_pk_info_ptr->is_new_transmission = OPC_TRUE; 							/* This is a new transmission											*/
		harq_pk_info_ptr->is_decode_success = OPC_TRUE; 							/* This value is filled by pipelines if packet is successfully decoded.	*/
					/* We set this to TRUE because packets which are successfully decoded anyway are NOT redirected to HARQ for efficiency purposes.		*/
		harq_pk_info_ptr->packet_size = op_pk_total_size_get (pkptr);
			
		/* Now this structure is set on the named packet field "harq information"																			*/
		op_pk_nfd_set (pkptr, "harq information", harq_pk_info_ptr, harq_support_pk_info_mem_copy, 
			harq_support_pk_info_mem_destroy, sizeof (HarqT_Packet_Info));
			
		/* Finally make a copy of this packet, and store it in the transmission channel.																	*/
		conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr = op_pk_copy (pkptr);
			
		/* User specified HARQ connection/channel trace.																									*/
		if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
			{
			char			msg [1024];
			char			msg1 [64];
			sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
			sprintf (msg, "\nTransmitting packet: "OPC_INT64_FMT", packet size: "OPC_INT64_FMT, op_pk_id (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr), 
				op_pk_total_size_get (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr));
			strcat (msg, msg1);
			op_prg_odb_print_major (msg, OPC_NIL);
			}
			
		/* Transmission rate statistic can be written here.																									*/	
		if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->tx_rate_stathandle))
			{
			op_stat_write (conn_info_ptr->harq_conn_params_ptr->tx_rate_stathandle, 1.0);
			op_stat_write (conn_info_ptr->harq_conn_params_ptr->tx_rate_stathandle, 0.0);
			}
			
		/* Also write the load statistic in bits per second.																								*/
		if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->load_stathandle))
			{
			op_stat_write (conn_info_ptr->harq_conn_params_ptr->load_stathandle, (double) harq_pk_info_ptr->packet_size);
			op_stat_write (conn_info_ptr->harq_conn_params_ptr->load_stathandle, 0.0);
			}
		
		/* Record the transmission time since it may be used for statistic writing later.																	*/
		conn_info_ptr->harq_tx_state_ptr [harq_channel].transmit_time = op_sim_time ();
		
		/* We are done. The caller will access pkptr, and send it over the phy.																				*/
		}
		
	/* caller does not need another packet...simply return OPC_NIL.																							*/
	FRET (OPC_NIL);
	}

/* In rare cases, a channel may have been blocked for transmission, but the caller may decide to abort it. 														*/
void 
harq_support_transmission_abort (HarqT_Conn_Info* conn_info_ptr, int harq_channel)
	{
	OpT_Int64						connection_identifier;
	HarqT_Transmission_State*		harq_tx_state_arr;
	HarqT_Receiver_State*			harq_rx_state_arr;
	List*							avail_chan_lptr;
	int*							freed_chan_ptr;
	
	FIN (harq_support_transmission_abort (...));
	
	if ((harq_channel < 0) || (harq_channel > conn_info_ptr->harq_conn_params_ptr->num_channels))
		{
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_transmission_abort", 
			"attempting to clear an invalid harq channel");
		}
	
	/* This function is expected to be called by the transmitter. Get the connection identifier.																*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Get the transmission and receiver elements. 																												*/
	if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
		{
		harq_tx_state_arr = conn_info_ptr->harq_tx_state_ptr;
		harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		}
	else
		{
		harq_tx_state_arr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		harq_rx_state_arr = conn_info_ptr->harq_rx_state_ptr;
		}
	
	/* Get the available channels list from the hash table lookp.																								*/
	avail_chan_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
		
	/* Now clear the transmission state and insert this channel in the available list table.																	*/
	harq_tx_state_arr [harq_channel].num_rxmt = 0;
	harq_tx_state_arr [harq_channel].harq_pk_info_ptr->packet_size = 0;
	freed_chan_ptr = harq_tx_state_arr [harq_channel].harq_pk_info_ptr->harq_channel_id_ptr;
	
	/* The above channel has been freed. Insert its pointer in the available list 																				*/
	op_prg_list_insert (avail_chan_lptr, freed_chan_ptr, OPC_LISTPOS_HEAD);
	
	if (freed_chan_ptr == OPC_NIL)
	printf ("\ntest: %d "OPC_INT64_FMT, harq_channel, connection_identifier);
	
	harq_tx_state_arr [harq_channel].harq_pk_info_ptr->harq_channel_id_ptr = OPC_NIL;
	
	/* Since the transmission channel is about to be freed, also reset the receiver channel state for future new transmissions on this channel.					*/
	harq_rx_state_arr [harq_channel].accumulated_snr_db = 0;
	harq_rx_state_arr [harq_channel].is_packet_decoded = OPC_TRUE;

	/* Finally restore the packet send inception and packet receive flags. This channel is now brand new for new transmissions.									*/
	harq_tx_state_arr [harq_channel].is_packet_transmission_conceived = OPC_FALSE;
	harq_rx_state_arr [harq_channel].is_packet_received = OPC_FALSE;
	
	/* User specified HARQ connection/channel trace.																											*/
	if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
		{
		char			msg [1024];
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
		sprintf (msg, "\nCleared HARQ channel from transmission and inserted it in the available list of channels.");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}	
	
	FOUT;
	}

void
harq_support_packet_coding_gain_calculate (HarqT_Packet_Info* harq_pk_info_ptr, HarqT_Combining_Scheme harq_scheme, double* effective_snr_db_ptr, double packet_fraction)
	{
	HarqT_Receiver_State*			harq_rx_state_arr;
	OpT_Int64						connection_identifier;
	double							accumulated_snr_db;
	double							final_accumulated_snr;
	int								harq_chan_id;
	double							incoming_snr_db;

	FIN (harq_support_packet_coding_gain_calculate (...));
	
	/* At this point, OPNET supports Chase Combining only.																										*/
	if (harq_scheme == HarqC_Chase_Combining)
		{
		double				effective_snr;
		
		/* First find the HARQ channel ID.																														*/
		harq_chan_id = *(harq_pk_info_ptr->harq_channel_id_ptr);
		
		/* Step 1: Get the key to look up the appropriate receiver element for this connection in the hash table.												*/
		connection_identifier = harq_pk_info_ptr->connection_identifier;
		
		/* Step 2: Access the receiver state of the current transmission																						*/
		harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		
		/* Step 3: If this is a new packet, this receiver channel should be made "free". This only constitutes reseting the flag.								*/
		if (harq_pk_info_ptr->is_new_transmission)
			{
			/* The accumulated SNR for the whole packet is simply the weighted sum of the SNR of each segment.													*/
			/* The caller is responsible for ensuring that the sum of all weights (packet_fraction) is 1 for the given MAC/PHY packet.							*/
			/* the caller also snsures that all segments are transmitted first before any segment is retransmitted...in WiMAX, this means						*/
			/* all segments are transmitted in the same burst.																									*/
			harq_rx_state_arr [harq_chan_id].accumulated_snr_db += packet_fraction * (*effective_snr_db_ptr); 		
			
			/* User specified HARQ connection/channel trace.																									*/
			if (harq_support_channel_trace_active (connection_identifier, harq_chan_id))
				{
				char			msg [1024];
				char			msg1 [64];
				sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_chan_id);
				sprintf (msg, "\nChase combining of new packet: accumulated SNR = incoming SNR = %lf", harq_rx_state_arr [harq_chan_id].accumulated_snr_db);
				strcat (msg, msg1);
				op_prg_odb_print_major (msg, OPC_NIL);
				}
			}
		else
			{
			/* cache the incoming SNR for a possible trace.																										*/
			incoming_snr_db = *effective_snr_db_ptr;
			
			/* This packet is retransmitted. Do Chase Combining.																								*/
			accumulated_snr_db = harq_rx_state_arr [harq_chan_id].accumulated_snr_db;
			
			/* Do the addition of the 2 SNR values.																												*/
			effective_snr = pow (10.0, accumulated_snr_db/10) + pow (10.0, (*effective_snr_db_ptr)/10);
			
			/* Return the effective snr to the caller.																											*/
			*effective_snr_db_ptr = 10.0 * log10 (effective_snr);
			
			/* Again, since the incoming packet may be a fraction of the whole packet, we only need to augment the accumulated SNR taking the contribution		*/
			/* from the current segment only. The fallback of this approach is that the subsequent segments of this packet are decoded with higher probability.	*/
			/* This is OK to us, because it may indeed reflect the reality of how decoding is performed in real systems.										*/					
			final_accumulated_snr = pow (10.0, accumulated_snr_db/10) + packet_fraction * pow (10.0, (incoming_snr_db)/10);
			
			harq_rx_state_arr [harq_chan_id].accumulated_snr_db = 10.0 * log10 (final_accumulated_snr); 		
			
			/* User specified HARQ connection/channel trace.																									*/
			if (harq_support_channel_trace_active (connection_identifier, harq_chan_id))
				{
				char			msg [1024];
				char			msg1 [64];
				sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_chan_id);
				sprintf (msg, "\nChase combining of retransmitted packet: incoming SNR: %lf, packet fraction %lf, returned effective SNR: %lf, accumulated SNR = %lf",
					incoming_snr_db, packet_fraction, *effective_snr_db_ptr, harq_rx_state_arr [harq_chan_id].accumulated_snr_db);
				strcat (msg, msg1);
				op_prg_odb_print_major (msg, OPC_NIL);
				}
			}
		} /* Chase Combining case ends.																															*/
	
	FOUT;
	}

void
harq_support_packet_decode_failed (HarqT_Packet_Info* harq_pk_info_ptr)	
	{
	FIN (harq_support_packet_decode_failed (...));
	
	/* This packet failed decoding...mark this information in its packet info ptr.																				*/
	harq_pk_info_ptr->is_decode_success = OPC_FALSE;
	
	/* User specified HARQ connection/channel trace.																											*/
	if (harq_support_channel_trace_active (harq_pk_info_ptr->connection_identifier, *(harq_pk_info_ptr->harq_channel_id_ptr)))
		{
		char			msg [128];
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", harq_pk_info_ptr->connection_identifier, *(harq_pk_info_ptr->harq_channel_id_ptr));
		sprintf (msg, "\nDecoding failed.");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	FOUT;
	}

void
harq_support_packet_decode_succeeded (HarqT_Packet_Info* harq_pk_info_ptr)	
	{
	FIN (harq_support_packet_decode_succeeded (...));
	
	/* This packet succeeded decoding...mark this information in its packet info ptr.																				*/
	harq_pk_info_ptr->is_decode_success = OPC_TRUE;
	
	/* User specified HARQ connection/channel trace.																												*/
	if (harq_support_channel_trace_active (harq_pk_info_ptr->connection_identifier, *(harq_pk_info_ptr->harq_channel_id_ptr)))
		{
		char			msg [128];
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", harq_pk_info_ptr->connection_identifier, *(harq_pk_info_ptr->harq_channel_id_ptr));
		sprintf (msg, "\nDecoding succeeded.");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	FOUT;
	}

/* This function updates the receiver state at the reception of each packet segment belonging to the whole MAC PDU.												*/
void
harq_support_packet_segment_receive_update (HarqT_Packet_Info* harq_pk_info_ptr)
	{
	OpT_Int64						connection_identifier;
	HarqT_Receiver_State*			harq_rx_state_arr;
	int								harq_chan_id;
		
	FIN (harq_support_packet_segment_receive_update (...));
	
	/* Find connection parameters, because for implicit acks, we must check if this represents maximum possible retransmissions.								*/
	connection_identifier = harq_pk_info_ptr->connection_identifier;
	harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
	
	/* Find the HARQ channel ID.																																*/
	harq_chan_id = *(harq_pk_info_ptr->harq_channel_id_ptr);
	
	harq_rx_state_arr [harq_chan_id].is_packet_decoded 		= harq_pk_info_ptr->is_decode_success;
	
	FOUT;
	}

Boolean
harq_support_packet_receive_update (HarqT_Packet_Info* harq_pk_info_ptr, Boolean* is_packet_decoded, int* harq_channel_ptr, Boolean update_receiver_state)
	{
	OpT_Int64						connection_identifier;
	HarqT_Receiver_State*			harq_rx_state_arr;
	HarqT_Transmission_State*		harq_tx_state_arr;
	HarqT_Conn_Parameters*			harq_conn_params_ptr;
	int								harq_chan_id;
		
	FIN (harq_support_packet_receive_update (...));
	
	/* The caller of this function passes the receiver connection handle, and the harq_pk_info_ptr of the received packet.										*/
	/* The harq_pk_info_ptr has the up-to-date information about the received packet. This data structure must not be altered or deallocated here.				*/
	
	/* First find the appropriate HARQ receiver element from the connection identifier in the packet info														*/
	connection_identifier = harq_pk_info_ptr->connection_identifier;
	harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
				
	/* Find the transmit state pointer. This function may be called from the transmitter or the receiver.														*/
	harq_tx_state_arr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	
	/* Find connection parameters, because for implicit acks, we must check if this represents maximum possible retransmissions.								*/
	harq_conn_params_ptr = (HarqT_Conn_Parameters*) prg_bin_hash_table_item_get (harq_conn_params_table, (OpT_Int64*) (&connection_identifier));

	/* Find the HARQ channel ID.																																*/
	harq_chan_id = *(harq_pk_info_ptr->harq_channel_id_ptr);
	
	/* Step 1: Record the received state of this packet on the receiver channel.																				*/
	/* The SNR value has already been recorded. So we only need to record if this was a transmission/retransmission and 										*/
	/* whether the packet was successfully decoded or not.																										*/
	harq_rx_state_arr [harq_chan_id].is_new_transmission 	= harq_pk_info_ptr->is_new_transmission;
	
	/* Since the packet has been received, also set the is_packet_received flag to TRUE. This tells the transmitter that packet was not lost in pipelines.		*/
	harq_rx_state_arr [harq_chan_id].is_packet_received 	= OPC_TRUE;
	
	/* reception of the packet means packet transmission process has been complete on the HARQ channel. Turn off the transmission_inception flag.				*/
	harq_tx_state_arr [harq_chan_id].is_packet_transmission_conceived = OPC_FALSE;
	
	if (update_receiver_state)
		harq_rx_state_arr [harq_chan_id].is_packet_decoded 		= harq_pk_info_ptr->is_decode_success;
	
	/* Fill in the value of whether this packet is decoded or not.																								*/
	*is_packet_decoded = harq_rx_state_arr [harq_chan_id].is_packet_decoded;
	
	/* For the benefit of the receiver, also tell the receiver which HARQ channel this packet is coming in.														*/
	*harq_channel_ptr = harq_chan_id;
	
	/* User defined connection/channel trace.																													*/
	if (harq_support_channel_trace_active (connection_identifier, harq_chan_id))
		{
		char			msg [256];
		char			msg1 [64];
		char			packet_new_arr [2][16] = {"not new", "new"};
		char			packet_decoded_arr [2][32] = {"not decoded", "decoded successfully"};	
		
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_chan_id);
		sprintf (msg, "\nIncoming packet: Is new? (%s), Is decoding successful? (%s).", 
			packet_new_arr [harq_rx_state_arr [harq_chan_id].is_new_transmission], packet_decoded_arr [harq_rx_state_arr [harq_chan_id].is_packet_decoded]);
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* Depending upon whether the connection handle corresponds to an implicit or explicit acknowledgement,														*/
	/* this function makes some provisions to facilitate the same.																								*/
	if (harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit)
		{
		harq_support_explicit_timed_ack_schedule (connection_identifier, harq_chan_id);
		
		/* Give an indication to the receiver that it needs to take appropriate steps for acknowledgement schedule.												*/
		/* Since actual acknowledgements are executed at a later time after "ack_delay", the indication to the caller											*/
		/* is that of success at this time. The caller does not use this indication right away.																	*/
		FRET (OPC_TRUE);
		}
	/* If this connection was for an implicit acknowledgement, the caller must schedule its own mechanism.														*/
	/* However we "peek" into the sender's record for "this connection" to indicate the caller if maximum number of retries have exceeded.						*/
	else
		{
		/* If this packet was successfully decoded, this acknowledgement channel must be inserted in the ack_waiting list for this connection.					*/
		/* This channel will be used to send some new HARQ transmission.																						*/
		/* Also insertion in ack-waiting list indicates that the packet was acked and will be subsequently destroyed. Clear the transmission channel.			*/
		/* Because the same transmission channel is going to be used possibly for new transmissions (unless forcibly removed).									*/
		if ((harq_pk_info_ptr->is_decode_success) ||
			(harq_tx_state_arr [harq_chan_id].num_rxmt == harq_conn_params_ptr->max_rxmt))
			{
			/* Statistic writing: If the packet was successfully decoded, end to end delay and throughput statistics can be updated. Otherwise dropped 			*/
			/* packet statistic can be updated.																													*/												
			if (harq_pk_info_ptr->is_decode_success)
				{
				if (op_stat_valid (harq_conn_params_ptr->delay_stathandle))
					op_stat_write (harq_conn_params_ptr->delay_stathandle, op_sim_time () - harq_tx_state_arr [harq_chan_id].transmit_time);
				if (op_stat_valid (harq_conn_params_ptr->throughput_stathandle))
					{
					op_stat_write (harq_conn_params_ptr->throughput_stathandle, (double) harq_pk_info_ptr->packet_size);
					op_stat_write (harq_conn_params_ptr->throughput_stathandle, 0.0);
					}
				}
			
			/* The packet is dropped due to maximum retransmission retries. Update statistics if they are valid.												*/	
			else
				{
				if (op_stat_valid (harq_conn_params_ptr->dropped_rate_stathandle))
					{
					op_stat_write (harq_conn_params_ptr->dropped_rate_stathandle, 1.0);
					op_stat_write (harq_conn_params_ptr->dropped_rate_stathandle, 0.0);
					}
				}
			
			/* The channel is inserted in the ack-waiting list. Notice that overrun of max retransmission means channel is free anyway.							*/
			harq_support_channel_ack_waiting_insert (connection_identifier, harq_pk_info_ptr->harq_channel_id_ptr);
			
			/* This channel is now free for new transmissions.																									*/
			harq_support_transmission_channel_free (connection_identifier, harq_chan_id, HarqC_Ack_Implicit);
			
			/* Since the transmission channel is about to be freed, also reset the receiver channel state for future new transmissions on this channel.			*/
			harq_rx_state_arr [harq_chan_id].accumulated_snr_db = 0;
			
			/* The rest of the fields do not need any resetting, because they are dynamically reset at the time of a new transmission.							*/
			
			/* Correction: Also set the packet_decoded field to OPC_TRUE.																						*/
			harq_rx_state_arr [harq_chan_id].is_packet_decoded = OPC_TRUE;
			}
		
		/* If the packet was NOT decoded successfully, there must be a retransmission on this same channel. At present, this channel is neither					*/
		/* available in the available list nor in the ack waiting list. We expect retransmissions to come with the channel number itself.						*/
		
		/* User defined connection/channel trace.																												*/
		if (harq_support_channel_trace_active (connection_identifier, harq_chan_id))
			{
			char			msg [256];
			char			msg1 [64];
			sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_chan_id);
			sprintf (msg, "\nImplicit ACK connection. Number of retransmissions on this channel: %d, max possible retransmissions: %d", 
				harq_tx_state_arr [harq_chan_id].num_rxmt, harq_conn_params_ptr->max_rxmt);
			strcat (msg, msg1);
			op_prg_odb_print_major (msg, OPC_NIL);
			}
		
		/* Return code:																																			*/
		if (harq_tx_state_arr [harq_chan_id].num_rxmt == harq_conn_params_ptr->max_rxmt)
			{
			FRET (OPC_FALSE);
			}
		else
			{
			FRET (OPC_TRUE);
			}
		}
	}

/* This function returns all the channels whose transmission was lost in air without execution of harq_support_packet_receive_update. Caller is expected		*/
/* to take appropriate action in such events depending upon the severity of this loss.																			*/
void 
harq_support_lost_transmission_channels_return (HarqT_Conn_Info* conn_info_ptr, int* lost_channel_array)
	{
	OpT_Int64									connection_identifier;
	HarqT_Receiver_State*						harq_rx_state_arr;
	HarqT_Transmission_State*					harq_tx_state_arr;
	int											i, j;
	
	FIN (harq_support_lost_transmission_channels_return);
	
	/* First find the key																																		*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Get the transmission and receiver elements. 																												*/
	if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
		{
		harq_tx_state_arr = conn_info_ptr->harq_tx_state_ptr;
		harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		}
	else
		{
		harq_tx_state_arr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		harq_rx_state_arr = conn_info_ptr->harq_rx_state_ptr;
		}
	
	/* Initialize the array with invalid channels.																												*/		
	for (i = 0; i < conn_info_ptr->harq_conn_params_ptr->num_channels; i++)
		lost_channel_array [i] = HARQC_INVALID_CHANNEL;
	
	/* It is possible that 1 end has not yet registered the HARQ connection...exit in that case.	*/
	if ((harq_tx_state_arr == OPC_NIL) || (harq_rx_state_arr == OPC_NIL))
		FOUT;
	
	j = 0;
	
	/* Now loop over the transmission and receiver elements and find those elements which began transmission but did not complete reception.					*/
	for (i = 0; i < conn_info_ptr->harq_conn_params_ptr->num_channels; i++)
		{
		if ((harq_tx_state_arr [i].is_packet_transmission_conceived == OPC_TRUE) &&
			(harq_rx_state_arr [i].is_packet_received == OPC_FALSE))
			lost_channel_array [j++] = i;
		}
	
	FOUT;
	}

/* The following function forces a NACK on a particular channel of a given connection. This is typically done when a packet is lost in air without				*/
/* executing the function harq_support_packet_receive_update.																									*/
void
harq_support_nack_force (HarqT_Conn_Info* conn_info_ptr, int harq_channel)
	{
	OpT_Int64									connection_identifier;
	HarqT_Receiver_State*						harq_rx_state_arr;
	HarqT_Transmission_State*					harq_tx_state_arr;
	
	FIN (harq_support_nack_force (...));
	
	/* This simple function does 2 things: The first is making an appropriate entry in the receiver state about packet not getting received. Secondly,			*/
	/* the HARQ channel is scheduled for a timed acknowledgement (which will indicate NACK when its time comes).												*/
	
	connection_identifier = conn_info_ptr->connection_identifier;

	/* Get the transmission and receiver elements. 																												*/
	if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
		{
		harq_tx_state_arr = conn_info_ptr->harq_tx_state_ptr;
		harq_rx_state_arr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		}
	else
		{
		harq_tx_state_arr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		harq_rx_state_arr = conn_info_ptr->harq_rx_state_ptr;
		}
	
	harq_rx_state_arr [harq_channel].is_packet_received 	= OPC_TRUE;
	
	harq_rx_state_arr [harq_channel].is_new_transmission = (harq_tx_state_arr [harq_channel].num_rxmt == 0);
	
	harq_tx_state_arr [harq_channel].is_packet_transmission_conceived = OPC_FALSE;

	harq_rx_state_arr [harq_channel].is_packet_decoded 		= OPC_FALSE;
	
	if (conn_info_ptr->harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit)
		harq_support_explicit_timed_ack_schedule (connection_identifier, harq_channel);

	
	FOUT;
	}

/* Given a connection pointer, this function returns all harq channels whose explicit ack timer has become 0.													*/
/* IMPORTANT: Note that as soon as the ack channels are returned, their memory is erased. This means we 														*/
/* assume that acks are guaranteed to be decoded at the transmitter. Currently we do not allow lost acks.														*/
void 
harq_support_explicit_ack_connections_return (HarqT_Conn_Info* conn_info_ptr, int* explicit_ack_array)
	{
	OpT_Int64									connection_identifier;
	HarqT_Explicit_Ack_Channel_Info*			head_ptr; 																			/* The head ptr 			*/
	HarqT_Explicit_Ack_Channel_Info*			tmp_ptr;
	int											i;
	
	FIN (harq_support_explicit_ack_connections_return (...));
	
	/* First find the key																																		*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Using this key, access the hash table element in the explicit ack table																					*/
	head_ptr = (HarqT_Explicit_Ack_Channel_Info*) prg_bin_hash_table_item_remove (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* The head_ptr has the list of current HARQ channels, whose explicit acks must be immediately elicited.													*/
	/* Return these elements to the caller in the given argument.																								*/
	/* First initialize the array.																																*/
	for (i = 0; i < conn_info_ptr->harq_conn_params_ptr->num_channels; i++)
		explicit_ack_array [i] = HARQC_INVALID_CHANNEL;
	
	/* Now return the ack channels whose ack_delay timer has expired.																							*/
	for (i = 0; i < head_ptr->num_blocked_channels; i++)
		explicit_ack_array [i] = head_ptr->blocked_chan_array [i];
	
	if ((head_ptr->num_blocked_channels > 0) && (HARQ_EXPLICIT_ACK_TRACE))
		{
		char			msg [1024];
		char			msg1 [8];
		
		sprintf (msg, "\nThe following explicit ack channels are returned to the caller for acknowledgement\n");
		for (i = 0; i < head_ptr->num_blocked_channels; i++)
			{
			sprintf (msg1, " %d ", head_ptr->blocked_chan_array [i]);
			strcat (msg, msg1);
			}
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* Now the elements of the head_ptr are returned to the caller. Reorganize the internal data structures.													*/
	if (head_ptr->next == OPC_NIL)
		{
		/* In this case, the ack_delay = 0. Reset all relevant elements, and exit from the function.															*/
		head_ptr->num_blocked_channels = 0;
		/* It is NOT required to reset the blocked_chan_array. The knoelwdge of how many channels are															*/
		/* blocked is really contained in num_blocked_channels, which is reset appropriately. We simply															*/
		/* overwrite channel elements in the array when new channels are blocked in future.																		*/
		}
	else
		{
		/* ack delay is at least 1, which means the current head_ptr now must become the tail, and its next element should become the head.						*/
		
		/* First, reset the num_blocked_channels again.																											*/
		head_ptr->num_blocked_channels = 0;
		
		/* cache the current head_ptr																															*/
		tmp_ptr = head_ptr;
		
		/* Now reset the head_ptr and store the new tail in it																									*/
		head_ptr = tmp_ptr->next;
		head_ptr->tail = tmp_ptr;
		
		/* The tmp_ptr was storing the previous tail_ptr...now the tmp_ptr becomes the tail_ptr.																*/
		tmp_ptr->next = OPC_NIL;
		tmp_ptr->tail->next = tmp_ptr;
		tmp_ptr->tail = OPC_NIL;
		}
	
	/* Reinsert head_ptr in the bin hash table.																													*/
	prg_bin_hash_table_item_insert (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier), (void*) head_ptr, OPC_NIL);
	
	FOUT;
	}

/* A function that returns if the HARQ packet on the given channel should be acknowledged.																		*/
Boolean
harq_support_connection_acknowledge (HarqT_Conn_Info* conn_info_ptr, int harq_channel_id)
	{
	OpT_Int64					connection_identifier;
	HarqT_Transmission_State*	harq_transmit_state_ptr;
	HarqT_Receiver_State*		harq_receive_state_ptr;
	
	FIN (harq_support_connection_acknowledge (...));
	
	/* Find the connection identifier.																															*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* User defined connection/channel trace.																													*/
	if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel_id))
		{
		char			msg [1024];
		char			msg1 [64];
		char			packet_ack_arr [2][16] = {"not acked", "acked"};
		
		/* Find the transmit and receive state pointers. This function may be called from the transmitter or the receiver.										*/
		if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
			{
			harq_transmit_state_ptr = conn_info_ptr->harq_tx_state_ptr;
			harq_receive_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
			}
		else
			{
			harq_receive_state_ptr = conn_info_ptr->harq_rx_state_ptr;
			harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
			}

		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel_id);
		sprintf (msg, "\nIs this packet being acknowledged? (%s). How many retransmission attempts have occured? (%d)", 
			packet_ack_arr [harq_receive_state_ptr [harq_channel_id].is_packet_decoded], harq_transmit_state_ptr [harq_channel_id].num_rxmt);
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* Find the receive state pointer. This function may be called from the transmitter or the receiver.														*/
	if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
		harq_receive_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
	else
		harq_receive_state_ptr = conn_info_ptr->harq_rx_state_ptr;
	
	/* Simply return the Boolean flag that indicates if the packet was decoded.																					*/
	FRET (harq_receive_state_ptr [harq_channel_id].is_packet_decoded);
	}

/* This function takes a connection handle and an HARQ channel ID and finds if the last transmission was new/retransmission.									*/
Boolean
harq_support_last_transmission_was_new (HarqT_Conn_Info* conn_info_ptr, int harq_channel_id)
	{
	HarqT_Transmission_State*				harq_transmit_state_ptr;
	OpT_Int64								connection_identifier;
	
	FIN (harq_support_last_transmission_was_new (...));
	
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Find the transmit state for this connection.																												*/
	if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
		harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	else
		harq_transmit_state_ptr = conn_info_ptr->harq_tx_state_ptr;
	
	/* User defined connection/channel trace.																													*/
	if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel_id))
		{
		char			msg [1024];
		char			msg1 [64];
		char			new_transmit_arr [2][32] = {"retransmission", "new transmission"};
		
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel_id);
		sprintf (msg, "\nCaller querying if the last transmission was new...(%s). ", 
			new_transmit_arr [harq_transmit_state_ptr [harq_channel_id].num_rxmt == 0]);
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* If num_rxmt == 0, it was a new transmission, otherwise a retransmission.																					*/
	FRET ((harq_transmit_state_ptr [harq_channel_id].num_rxmt) == 0)
	}

Compcode 
harq_support_ack_nack_process (HarqT_Conn_Info* conn_info_ptr, int harq_channel, Boolean is_packet_acked)
	{
	OpT_Int64					connection_identifier;
	HarqT_Receiver_State*		harq_receive_state_ptr;
	OpT_Packet_Size				packet_size;	
		
	FIN (harq_support_ack_nack_process (...));
	
	/* If the packet is acked, the transmission element must be cleared right away.																				*/
	/* Also if max number of retransmissions have reached, take the same action.																				*/
	/* The indication of this fact has already been provided. Sender will not schedule retransmission for this packet causing a crash later somewhere.			*/
	if ((is_packet_acked) || (conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt == conn_info_ptr->harq_conn_params_ptr->max_rxmt))
		{
		/* can't receive an ack for a nonexistant packet																										*/
		if (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr == OPC_NIL)
			{
			printf ("\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
			op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_ack_nack_process", "received acknowledgement for nonexistant packet");
			}
		
		/* Cache the size of the packet to be destroyed for a possible statistic writing.																		*/
		packet_size = op_pk_total_size_get (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr);
		
		op_pk_destroy (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr);
		conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr = OPC_NIL;
		conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt = 0;
		
		/* User defined connection/channel trace.																												*/
		if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
			{
			char			msg [1024];
			char			msg1 [512];
			char			msg2 [64];
			sprintf (msg2, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
			sprintf (msg, "\nEither we received an ack or max retransmissions have exceeded. Packet from HARQ transmission channel destroyed.");
			strcat (msg, msg2);
			sprintf (msg1, "\nAlso retransmission attempts reset. The channel is freed only conditionally (for implicit acks, it MAY NOT be freed)");
			strcat (msg, msg1);
			op_prg_odb_print_major (msg, OPC_NIL);
			}
		
		/* It is not necessary to reset transmission packet information. It will be initialized for the next packet transmission.								*/
		
		/* When this channel is cleared for transmission, it must be made available for future transmissions.													*/
		/* Call this function only for explicit acknowledgements. For implicit acknowledgement, the channel cannot simply be freed.								*/
		/* This is so because for implicit ack channels, a channel is theoretically never freed. The same channel is simply used for next transmission			*/
		/* However the caller has another function to remove this implicit ack channel forcefully from the list, at which point, it is freed the same way here.	*/
		if (conn_info_ptr->harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit)
			{
			connection_identifier = conn_info_ptr->connection_identifier;
			
			/* Statistic writing: If the packet was successfully decoded, end to end delay and throughput statistics can be updated. Otherwise dropped 			*/
			/* packet statistic can be updated.																													*/												
			if (is_packet_acked)
				{
				if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->delay_stathandle))
					op_stat_write (conn_info_ptr->harq_conn_params_ptr->delay_stathandle, 
						op_sim_time () - conn_info_ptr->harq_tx_state_ptr [harq_channel].transmit_time);
				if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->throughput_stathandle))
					{
					op_stat_write (conn_info_ptr->harq_conn_params_ptr->throughput_stathandle, (double) packet_size);
					op_stat_write (conn_info_ptr->harq_conn_params_ptr->throughput_stathandle, 0.0);
					}
				}
			/* The packet is dropped due to maximum retransmission retries. Update statistics if they are valid.												*/	
			else
				{
				if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->dropped_rate_stathandle))
					{
					op_stat_write (conn_info_ptr->harq_conn_params_ptr->dropped_rate_stathandle, 1.0);
					op_stat_write (conn_info_ptr->harq_conn_params_ptr->dropped_rate_stathandle, 0.0);
					}
				}
			
			harq_support_transmission_channel_free (conn_info_ptr->connection_identifier, harq_channel, HarqC_Ack_Explicit);
			
			/* The transmission channel is about to be freed. Also reset the receiver channel state information for future new transmissions.					*/
			/* First find the receiver state array for the given connection.																					*/
			harq_receive_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
			
			/* The only field that needs reset is accumulated_snr_db...the remaining fields are automatically reset upon new transmission.						*/
			harq_receive_state_ptr [harq_channel].accumulated_snr_db = 0;
			
			/* Correction: is_decode_success also needs to be reset to OPC_TRUE. This is so because multiple segments access the same field and the decision	*/
			/* of decoding really depends upon receiving all of them correctly. If either sets this field to FALSE, no further segments can reset it back.		*/
			harq_receive_state_ptr [harq_channel].is_packet_decoded = OPC_TRUE;
			}
		
		/* Since the transmission channel was cleared, indicate to the caller with the value OPC_COMPCODE_FAILURE												*/
		FRET (OPC_COMPCODE_FAILURE);
		}
	
	if ((!is_packet_acked) && (conn_info_ptr->harq_tx_state_ptr [harq_channel].num_rxmt < conn_info_ptr->harq_conn_params_ptr->max_rxmt))
		{
		/* can't receive an ack for a nonexistant packet																										*/
		if (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr == OPC_NIL)
			{
			printf ("\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
			op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_ack_back_process", "received acknowledgement for nonexistant packet");
			}
		
		/* User defined connection/channel trace.																												*/
		if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, harq_channel))
			{
			char			msg [512];
			char			msg1 [64];	
			sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, harq_channel);
			sprintf (msg, "\nPacket nacked and more retransmission attempts are possible.");
			strcat (msg, msg1);
			op_prg_odb_print_major (msg, OPC_NIL);
			}
		
		}
	/* Return OPC_COMPCODE_SUCCESS to the caller. The channel is still blocked.																					*/
	FRET (OPC_COMPCODE_SUCCESS);
	}

/* Function to return the number of unacknowledged channels in the implicit ack-waiting list. 																	*/
int
harq_support_implicit_ack_channel_num_get (HarqT_Conn_Info*	conn_info_ptr)
	{
	OpT_Int64					connection_identifier;
	List*						harq_ack_waiting_lptr;
		
	FIN (harq_support_implicit_ack_channel_num_get (...));
		
	/* Find the connection identifier.																															*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Find the ack_waiting_list for this connection.																											*/
	harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));

	/* return the size of the list.																																*/
	FRET (op_prg_list_size (harq_ack_waiting_lptr));
	}

/* This function is called by the caller to "remove" a perpetual channel from the implicit-ack's cycle. In this case, this channel now becomes "free".			*/
/* Typically this function is called when a lot of channels are "stuck" in ack_waiting list and the caller simply wants to use them to send "blank" acks.		*/
/* caller is expected to call this function in a loop...i.e. this function is designed to remove only 1 ack channel. If the return value is 					*/
/* HARQC_INVALID_CHANNEL, then the caller must stop. The list is now empty.																						*/
int
harq_support_one_implicit_ack_channel_remove (HarqT_Conn_Info* conn_info_ptr)
	{
	OpT_Int64					connection_identifier;
	List*						harq_ack_waiting_lptr;
	List*						harq_avail_lptr;
	int*						harq_channel_id_ptr;
	HarqT_Transmission_State*	harq_transmit_state_ptr;
	
	FIN (harq_support_one_implicit_ack_channel_remove);
	
	/* Find the connection identifier.																															*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Find the transmit state for this connection.																												*/
	if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
		harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	else
		harq_transmit_state_ptr = conn_info_ptr->harq_tx_state_ptr;

	/* Find the ack_waiting_list for this connection.																											*/
	harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* It is possible that the list is empty.																													*/
	if (op_prg_list_size (harq_ack_waiting_lptr) == 0)
		FRET (HARQC_INVALID_CHANNEL);
	
	/* Otherwise, remove this element from this list, and a) put in the "free list", b) remove the reference of this channel from the transmission element.		*/
	
	/* First find the list of available channels.																												*/
	harq_avail_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* Find the first ack_waiting channel.																														*/
	harq_channel_id_ptr = (int*) op_prg_list_remove (harq_ack_waiting_lptr, OPC_LISTPOS_HEAD);
	
	/* Insert this element in the available list 																												*/
	op_prg_list_insert (harq_avail_lptr, harq_channel_id_ptr, OPC_LISTPOS_TAIL);
	
	/* Force removal of this channel may not be accompanied by packet reception...simply remove the transmission_conceived flag.								*/
	harq_transmit_state_ptr [*harq_channel_id_ptr].is_packet_transmission_conceived = OPC_FALSE;
	
	/* Remove the reference of this element from the packet info of the transmission element...this is done for clarity 										*/
	harq_transmit_state_ptr [*harq_channel_id_ptr].harq_pk_info_ptr->harq_channel_id_ptr = OPC_NIL;
	
	/* User defined connection/channel trace.																													*/
	if (harq_support_channel_trace_active (conn_info_ptr->connection_identifier, *harq_channel_id_ptr))
		{
		char			msg [512];
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", conn_info_ptr->connection_identifier, *harq_channel_id_ptr);
		sprintf (msg, "\nCaller forcibly removing the HARQ channel from ack-waiting list. It is inserted in the available list.");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* Finally return this element to the caller.																												*/
	FRET (*harq_channel_id_ptr);
	}

/* This function returns the size of the packet that needs to be retransmitted to the caller.																	*/
/* This is important in connection oriented systems (WiMAX), where a prior allocation must be made.																*/
OpT_Packet_Size
harq_support_retransmission_size_request (HarqT_Conn_Info* conn_info_ptr, int harq_channel)
	{
	FIN (harq_retransmission_size_request (...));
	
	/* If the caller is calling this function, the transmission channel must have a packet.																		*/
	if (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr == OPC_NIL)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_retransmission_size_request", 
			"no packet exists on harq transmission channel for retransmitting");

	FRET (op_pk_total_size_get (conn_info_ptr->harq_tx_state_ptr [harq_channel].harq_pkptr));
	}

/* Function to retrieve necessary retransmission information and return to the caller. Caller implements his/her own callback.									*/	
void*
harq_support_custom_retransmission_information_retrieve (HarqT_Conn_Info* conn_info_ptr, int harq_channel, HarqT_Custom_Information_Retrieval_Proc custom_proc)
	{
	void*							custom_retransmission_information;
	OpT_Int64						connection_identifier;
	HarqT_Transmission_State*		harq_transmit_state_ptr;
	
	FIN (harq_support_custom_retransmission_information_retrieve (...));
	
	/* Find the transmission channel and extract the retransmission packet stored on this channel. Abort if there is no packet.									*/						
	connection_identifier = conn_info_ptr->connection_identifier;
	harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	
	if (harq_transmit_state_ptr [harq_channel].harq_pkptr == OPC_NIL)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_custom_retransmission_information_retrieve", 
			"Client trying to extract retransmission information, but there is no packet on this HARQ handle and channel");
	
	/* Simply call the callback implemented by the client.																										*/
	custom_retransmission_information = custom_proc (harq_transmit_state_ptr [harq_channel].harq_pkptr);
	
	FRET (custom_retransmission_information);
	}

/* A function to flush the state of the given HARQ connection. Typically happens when the TX/RX association is broken via node failure or mobility.				*/	
void 
harq_support_connection_flush (HarqT_Conn_Info* conn_info_ptr)
	{
	OpT_Int64							connection_identifier;
	HarqT_Conn_Parameters*				harq_conn_params_ptr;
	HarqT_Transmission_State*			harq_transmit_state_ptr;
	HarqT_Receiver_State*				harq_receive_state_ptr;
	List*								harq_available_lptr;
	List*								harq_ack_waiting_lptr;
	int									i, j;
	HarqT_Explicit_Ack_Channel_Info*	head_ptr;
	HarqT_Explicit_Ack_Channel_Info*	current_ptr;
	
	FIN (harq_support_connections_flush (...));
	
	/* Find the transmission state and receiver state arrays, and find the available channel list first.														*/	
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Find the connection parameters.																															*/			
	harq_conn_params_ptr = conn_info_ptr->harq_conn_params_ptr;
	
	harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	harq_receive_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
	harq_available_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* Clear the transmission and the receiver state first.																										*/
	/* Go over all the channels. For transmission state, destroy any pending packets and release the channel to the available state.							*/
	/* For the receiver state, reset all the chase combining information.																						*/
	for (i = 0; i < harq_conn_params_ptr->num_channels; i++)
		{
		/* Transmission channel clearence: Destroy any pending packets. Insert the associated channel back in the list of available channels.					*/
		if ((conn_info_ptr->harq_role == HarqC_Transmitter) && (harq_transmit_state_ptr [i].harq_pkptr != OPC_NIL))
			{
			op_pk_destroy (harq_transmit_state_ptr [i].harq_pkptr);
			harq_transmit_state_ptr [i].harq_pkptr = OPC_NIL;
			harq_transmit_state_ptr [i].num_rxmt = 0;
			
			/* Write the dropped packet statistic if applicable.																								*/
			if (op_stat_valid (harq_conn_params_ptr->dropped_rate_stathandle))
				{
				op_stat_write (harq_conn_params_ptr->dropped_rate_stathandle, 1.0);
				op_stat_write (harq_conn_params_ptr->dropped_rate_stathandle, 0.0);
				}
			}
		
		/* If a valid channel is found, insert it in the list of available channels.																			*/
		if (harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr != OPC_NIL)
			{
			op_prg_list_insert (harq_available_lptr, harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr, OPC_LISTPOS_TAIL);
			
			/* It is possible that the in-transmission channel is also in the ack-waiting list. If so, find this channel and remove from ack-waiting 			*/
			/* list in order to avoid incorrect double counting.																								*/
			if (harq_conn_params_ptr->ack_method == HarqC_Ack_Implicit)
				{
				int* 		ack_waiting_channel;
		
				harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
				for (j = 0; j < op_prg_list_size (harq_ack_waiting_lptr); j++)
					{
					ack_waiting_channel = (int*) op_prg_list_access (harq_ack_waiting_lptr, j);
					if (*ack_waiting_channel == i)
						{
						op_prg_list_remove (harq_ack_waiting_lptr, j);
						break;
						}
					}
				}

			/* Reset the HARQ packet size to 0. Rest of the elements don't need to be reset since they are set at the time of new transmission again.			*/
			harq_transmit_state_ptr [i].harq_pk_info_ptr->packet_size = 0;
			
			/* Finally set the HARQ channel as NIL on the packet info ptr.																						*/
			harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr = OPC_NIL;
			
			/* Restore the packet transmission flag																												*/
			harq_transmit_state_ptr [i].is_packet_transmission_conceived = OPC_FALSE;
			}
			
		/* Receiver state clearence: Make sure that no chase combining specific information is left behind on any receiver channel.								*/
		harq_receive_state_ptr [i].accumulated_snr_db = 0;
		harq_receive_state_ptr [i].is_packet_decoded = OPC_TRUE;
		harq_receive_state_ptr [i].is_packet_received = OPC_FALSE;
		}
	
	/* Now for Implicit acknowledgement connections, transfer all ack-waiting channels back to list of available channels. 										*/
	if (harq_conn_params_ptr->ack_method == HarqC_Ack_Implicit)
		{
		int* 		ack_waiting_channel;
		
		harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
		while (op_prg_list_size (harq_ack_waiting_lptr) > 0)
			{
			ack_waiting_channel = (int*) op_prg_list_remove (harq_ack_waiting_lptr, OPC_LISTPOS_HEAD);
			op_prg_list_insert (harq_available_lptr, ack_waiting_channel, OPC_LISTPOS_TAIL);
			}
		}
	
	/* Finally for all explicit acknowledgement connections, clear the list of all explicitly timed acknowledgements.											*/
	else
		{
		head_ptr = (HarqT_Explicit_Ack_Channel_Info*) prg_bin_hash_table_item_get (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier));
		
		current_ptr = head_ptr;
		for (i = 0; i <= harq_conn_params_ptr->ack_delay; i++)
			{
			/* In order to clear the list of pending channels, it is *enough* to set the index or maximum elements to 0. 										*/
			current_ptr->num_blocked_channels = 0;
			
			/* Move on to the next element.																														*/	
			current_ptr = current_ptr->next;
			}
		}
	
	/* If the size of the available channel list at this point is anything but the number of channels, abort. Something is wrong.								*/
	if (op_prg_list_size (harq_available_lptr) != harq_conn_params_ptr->num_channels)
		op_sim_error (OPC_SIM_ERROR_ABORT, "Aborting from harq_support_connection_flush", 
			"Available channel list does not match total number of HARQ channels after the flush operation");
	
	FOUT;
	}

void	
harq_support_conn_info_print (HarqT_Conn_Info* conn_info_ptr)
	{
	char		ack_method_str [32];
	char		caller_role_str [32];
	char		buffer_aggr_str [16];	
	char		ack_delay_str [16] = "Unused";
	
	FIN (harq_support_conn_info_print (...));
	
	harq_support_ack_method_return (conn_info_ptr->harq_conn_params_ptr->ack_method, ack_method_str);
	harq_support_caller_role_return (conn_info_ptr->harq_role, caller_role_str);
	harq_support_buffer_aggr_return (conn_info_ptr->harq_conn_params_ptr->buffer_aggr_flag, buffer_aggr_str);
	
	printf ("\n********************************HARQ connection information************************************");
	printf ("\nConnection Identifier: "OPC_INT64_FMT, conn_info_ptr->connection_identifier);
	printf ("\nInteger portion of connection ID: %d", (int) conn_info_ptr->connection_identifier);
	printf ("\nAcknowledgement Method: %s, Caller function: %s", ack_method_str, caller_role_str);
	printf ("\nConnection Information:");
	
	if (conn_info_ptr->harq_conn_params_ptr->ack_method == HarqC_Ack_Explicit)
		{
		printf ("\n\tparameters (num_channels, max_rxmt, ack_delay, buffer_size (bits), buffer_aggr_flag) = (%d, %d, %d, %d, %s)",
			conn_info_ptr->harq_conn_params_ptr->num_channels, conn_info_ptr->harq_conn_params_ptr->max_rxmt,
			conn_info_ptr->harq_conn_params_ptr->ack_delay, conn_info_ptr->harq_conn_params_ptr->buffer_size,
			buffer_aggr_str);
		}
	else
		{
		printf ("\n\tparameters (num_channels, max_rxmt, ack_delay, buffer_size (bits), buffer_aggr_flag) = (%d, %d, %s, %d, %s)",
			conn_info_ptr->harq_conn_params_ptr->num_channels, conn_info_ptr->harq_conn_params_ptr->max_rxmt,
			ack_delay_str, conn_info_ptr->harq_conn_params_ptr->buffer_size,
			buffer_aggr_str);
		}
	printf ("\n***********************************************************************************************\n");

	FOUT;
	}

/* The function below prints out the channel state as seen by the transmitter or receiver at any given instant.														*/
/* Implicit channels can be found in: free/ack-waiting/in transmission states.																						*/
/* Explicit channels can be found in: free/blocked-for-ack/in tranmission states.																					*/			
void 
harq_support_all_channels_of_connection_print (HarqT_Conn_Info*	conn_info_ptr)
	{
	HarqT_Conn_Parameters*				harq_conn_params_ptr;
	HarqT_Transmission_State* 			harq_transmit_state_ptr;
	List*								harq_ack_waiting_lptr;
	List*								harq_available_lptr;
	int									i, j;
	int*								harq_chan_ptr;
	HarqT_Explicit_Ack_Channel_Info*	head_ptr;
	HarqT_Explicit_Ack_Channel_Info*	current_ptr;
	char								msg [8192];
	char								msg1 [1096];
	char								msg2 [64];
	OpT_Int64 							connection_identifier;
	
	FIN (harq_support_all_channels_of_connection_print (...));
	
	/* Find the connection identifier																																*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Find the transmit element.																																	*/
	harq_transmit_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	
	/* Find connection parameters element.																															*/
	harq_conn_params_ptr = (HarqT_Conn_Parameters*) prg_bin_hash_table_item_get (harq_conn_params_table, (OpT_Int64*) (&connection_identifier));
	
	/* If this is an implicit ack, find the list of channels from: 1) free, 2) ack-waiting, 3) from the transmit state.												*/
	if (harq_conn_params_ptr->ack_method == HarqC_Ack_Implicit)
		{
		harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
		harq_available_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
		
		sprintf (msg, "\nPrinting the list of available channels for HARQ connection "OPC_INT64_FMT"\n", connection_identifier);
		for (i = 0; i < op_prg_list_size (harq_available_lptr); i++)
			{
			harq_chan_ptr = (int*) op_prg_list_access (harq_available_lptr, i);
			sprintf (msg2, " %d ", *harq_chan_ptr);
			strcat (msg, msg2);
			}
		
		sprintf (msg1, "\nPrinting the list of ack-waiting channels for HARQ connection "OPC_INT64_FMT"\n", connection_identifier);
		strcat (msg, msg1);
		for (i = 0; i < op_prg_list_size (harq_ack_waiting_lptr); i++)
			{
			harq_chan_ptr = (int*) op_prg_list_access (harq_ack_waiting_lptr, i);
			sprintf (msg2, " %d ", *harq_chan_ptr);
			strcat (msg, msg2);
			
			}
		
		sprintf (msg1, "\nPrinting the list of in-transmission channels for HARQ connection "OPC_INT64_FMT"\n", connection_identifier);
		strcat (msg, msg1);
		for (i = 0; i < harq_conn_params_ptr->num_channels; i++)
			{
			/* Find all the channels that are under transmission right now.																							*/			
			if (harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr != OPC_NIL)
				{
				sprintf (msg2, " %d ", *(harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr));
				strcat (msg, msg2);
				}
			}
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* This is an explicit ack, find a list of channels from: 1) free, 2) blocked, 3) from the transmit state.														*/
	else
		{
		harq_available_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
		head_ptr = (HarqT_Explicit_Ack_Channel_Info*) prg_bin_hash_table_item_get (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier));
		
		sprintf (msg, "\nPrinting the list of available channels for HARQ connection "OPC_INT64_FMT"\n", connection_identifier);
		for (i = 0; i < op_prg_list_size (harq_available_lptr); i++)
			{
			harq_chan_ptr = (int*) op_prg_list_access (harq_available_lptr, i);
			sprintf (msg2, " %d ", *harq_chan_ptr);
			strcat (msg, msg2);
			}
		
		sprintf (msg1, "\nPrinting the list of explicit-ack-waiting channels for HARQ connection "OPC_INT64_FMT, connection_identifier);
		strcat (msg, msg1);
		
		/* To find the list of blocked channels, we must loop over the entire linked list keeping a count of blocked channels.										*/
		/* Each element in the linked list has a record of how many channels are blocked in that particular element (representing running counter < ack_delay) 		*/		
		current_ptr = head_ptr;
		for (i = 0; i <= harq_conn_params_ptr->ack_delay; i++)
			{
			sprintf (msg1, "\n\t%d channels will be explicitly acknowledged in %d frames from now. These channels are:", current_ptr->num_blocked_channels, i);
			strcat (msg, msg1);
			for (j = 0; j < current_ptr->num_blocked_channels; j++)
				{
				sprintf (msg2, " %d ", current_ptr->blocked_chan_array [j]);
				strcat (msg, msg2);
				}
			
			/* Move on to the next element.																															*/	
			current_ptr = current_ptr->next;
			}
		
		sprintf (msg1, "\nPrinting the list of in-transmission channels for HARQ connection "OPC_INT64_FMT"\n", connection_identifier);
		strcat (msg, msg1);
		for (i = 0; i < harq_conn_params_ptr->num_channels; i++)
			{
			/* Find all the channels that are under transmission right now.																							*/			
			if (harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr != OPC_NIL)
				{
				sprintf (msg2, " %d ", *(harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr));
				strcat (msg, msg2);
				}
			}
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	FOUT;
	}

/* This function registers the client specified packet formats to the HARQ print proc.																				*/
void
harq_support_packet_format_print_proc_register (char* packet_format_name)
	{
	FIN (harq_support_packet_format_print_proc_register (...));
	
	op_pk_format_print_proc_set (packet_format_name, "harq information", harq_support_pk_info_print); 
	
	FOUT;
	}

/* Each HARQ packet carries HARQ specific information. Copy and destroy procs for the same field.																	*/
HarqT_Packet_Info* 		
harq_support_pk_info_mem_copy (HarqT_Packet_Info* copy_structure)
	{
	FIN (harq_support_pk_info_mem_copy (...));
	
	/****IMPORTANT****																																				*/
	/* HARQ packet carries a constant memory, which is associated with the transmission channel element.															*/
	/* The contents of this memory may change time to time, but this memory is allocated only once. 																*/
	/* This reduces memory allocation/deallocation operations significantly.																						*/
	/* Thus the copy proc returns the same pointer to the caller.																									*/
	
	FRET (copy_structure);
	}

void 		
harq_support_pk_info_mem_destroy (HarqT_Packet_Info* destroy_structure)
	{
	FIN (harq_support_pk_info_mem_destroy (...));
	
	/* Since the memory allocated for the packet was a constant, it must not be released.																			*/
	/* So return from this function without doing anything.																											*/
	
	FOUT;
	}

/****************************************************************INTERNAL FUNCTIONS**********************************************************************************/

/* Function to create HARQ transmission element for the given HARQ connection.																						*/
Compcode 
harq_support_transmission_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr)
	{
	
	HarqT_Transmission_State* 	harq_transmit_state_ptr;
	int							num_harq_channels;
	int							i;
	
	FIN (harq_support_transmission_elem_create (...));
	
	/* Find out how many channels this connection supports.																											*/
	num_harq_channels = conn_info_ptr->harq_conn_params_ptr->num_channels;
	
	/* Allocate an array of the size of num_channels. Each HARQ channel is thus identified by array index.															*/
	harq_transmit_state_ptr = (HarqT_Transmission_State*) op_prg_cmo_alloc (harq_cat_mem_handle, num_harq_channels * sizeof (HarqT_Transmission_State)); 
	
	/* In rare cases, memory allocation may fail if the system is overloaded with memory.																			*/
	if (harq_transmit_state_ptr == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* Initialize the transmission element.																															*/
	for (i = 0; i < num_harq_channels; i++)
		{
		harq_transmit_state_ptr [i].harq_pkptr 				= OPC_NIL;
		harq_transmit_state_ptr [i].num_rxmt 					= 0;
		/* There is no need to initialize the packet info memory except the field that contains the pointer to the transmission channel.							*/
		harq_transmit_state_ptr [i].harq_pk_info_ptr			= (HarqT_Packet_Info*)op_prg_cmo_alloc (harq_cat_mem_handle, sizeof (HarqT_Packet_Info));
		harq_transmit_state_ptr [i].harq_pk_info_ptr->harq_channel_id_ptr = OPC_NIL;
		harq_transmit_state_ptr [i].harq_tx_state_info_ptr	= OPC_NIL;
		harq_transmit_state_ptr [i].is_packet_transmission_conceived = OPC_FALSE;
		}
	
	/* Since this is called for the transmitter, only set the transmission state in connection handle.																*/
	conn_info_ptr->harq_tx_state_ptr = harq_transmit_state_ptr;
	conn_info_ptr->harq_rx_state_ptr = OPC_NIL;
	
	/* Insert this transmission element in the hash table.																											*/
	prg_bin_hash_table_item_insert (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier), (void*) harq_transmit_state_ptr, OPC_NIL);
	
	/* Operation successful.																																		*/
	FRET (OPC_COMPCODE_SUCCESS);
	}

Compcode 
harq_support_receiver_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr)
	{
	HarqT_Receiver_State* 	harq_receive_state_ptr;
	int							num_harq_channels;
	int							i;
	
	FIN (harq_support_receiver_elem_create (...));
	
	/* Find out how many channels this connection supports.																											*/
	num_harq_channels = conn_info_ptr->harq_conn_params_ptr->num_channels;
	
	/* Allocate an array of the size of num_channels. Each HARQ channel is thus identified by array index.															*/
	harq_receive_state_ptr = (HarqT_Receiver_State*) op_prg_cmo_alloc (harq_cat_mem_handle, num_harq_channels * sizeof (HarqT_Receiver_State)); 
	
	/* In rare cases, memory allocation may fail if the system is overloaded with memory.																			*/
	if (harq_receive_state_ptr == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* Initialize the receiver element.																																*/
	for (i = 0; i < num_harq_channels; i++)
		{
		harq_receive_state_ptr [i].is_packet_decoded 		= OPC_TRUE;
		harq_receive_state_ptr [i].is_new_transmission 		= OPC_FALSE;
		harq_receive_state_ptr [i].accumulated_snr_db		= 0;
		harq_receive_state_ptr [i].harq_rx_state_info_ptr	= OPC_NIL;
		harq_receive_state_ptr [i].is_packet_received		= OPC_FALSE;
		}
	
	/* Since this is called for the receiver, only set the receiver state in connection handle.																		*/
	conn_info_ptr->harq_tx_state_ptr = OPC_NIL;
	conn_info_ptr->harq_rx_state_ptr = harq_receive_state_ptr;
	
	/* Insert this receiver element in the hash table.																												*/
	prg_bin_hash_table_item_insert (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier), (void*) harq_receive_state_ptr, OPC_NIL);
	
	/* Operation successful.																																		*/
	FRET (OPC_COMPCODE_SUCCESS);
	}

/* This function creates a list that can be used to store open HARQ channels for transmission.																		*/
Compcode
harq_support_avail_chan_list_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr)	
	{
	List*		harq_avail_chan_lptr;
	int*		harq_channel_ptr;
	int			i;
	
	FIN (harq_support_avail_chan_list_create (...));
	
	harq_avail_chan_lptr = op_prg_list_create ();
	
	if (harq_avail_chan_lptr == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* At the beginning, all HARQ channels are available. Each HARQ channel is identified by an integer starting from 0, and increasing monotonically by 1. 		*/
	/* The maximum number of channels is specified in the connection_identifier.																					*/
	for (i = 0; i < conn_info_ptr->harq_conn_params_ptr->num_channels; i++)
		{
		/* Allocate a new integer element. 																															*/
		harq_channel_ptr = (int*) op_prg_cmo_alloc (harq_cat_mem_handle, sizeof (int));
		
		/* Allocate the channel number.																																*/
		*harq_channel_ptr = i;
		
		/* Insert this harq channel in the list.																													*/
		op_prg_list_insert (harq_avail_chan_lptr, harq_channel_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Insert the list in the hash table.																															*/
	prg_bin_hash_table_item_insert (harq_available_chan_table, (OpT_Int64*) (&connection_identifier), (void*) harq_avail_chan_lptr, OPC_NIL);
	
	/* Operation successful.																																		*/
	FRET (OPC_COMPCODE_SUCCESS);
	}

/* This function creates a list that can be used to store open HARQ channels for ack waiting state.																	*/
Compcode
harq_support_ack_waiting_chan_list_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr)	
	{
	List*		harq_ack_waiting_chan_lptr;
	
	FIN (harq_support_ack_waiting_chan_list_create (...));
	
	harq_ack_waiting_chan_lptr = op_prg_list_create ();
	
	if (harq_ack_waiting_chan_lptr == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* At the beginning, no channel is waiting for acknowledgement. Simply insert the list in the hash table and exit.												*/
	
	/* Insert the list in the hash table.																															*/
	prg_bin_hash_table_item_insert (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier), (void*) harq_ack_waiting_chan_lptr, OPC_NIL);
	
	/* Operation successful.																																		*/
	FRET (OPC_COMPCODE_SUCCESS);
	}


/* This function creates an element to store running timers of explicit acknowledgements																			*/
Compcode 
harq_support_explicit_ack_chan_elem_create (OpT_Int64 connection_identifier, HarqT_Conn_Info* conn_info_ptr)
	{
	HarqT_Explicit_Ack_Channel_Info*		head_ptr;
	HarqT_Explicit_Ack_Channel_Info*		harq_explicit_ack_info_ptr;
	HarqT_Explicit_Ack_Channel_Info*		new_harq_explicit_ack_info_ptr;
	int										i;
	int										num_harq_channels;
	int										explicit_ack_delay;
	
	FIN (harq_support_explicit_ack_chan_elem_create (...));
	
	/* The explicit ack element is a linked list, with number of elements = (ack_delay) + 1. 																		*/
	/* Each element of the linked list stores an array. The array element stores which channel is blocked.															*/
	/* We also store the number of blocked channels for easy access of the array.																					*/
	
	/* Create the head pointer of this linked list.																													*/
	harq_explicit_ack_info_ptr = (HarqT_Explicit_Ack_Channel_Info*) op_prg_cmo_alloc (harq_cat_mem_handle, sizeof (HarqT_Explicit_Ack_Channel_Info));
	
	if (harq_explicit_ack_info_ptr == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* initialize the head_ptr:	*/
	head_ptr = harq_explicit_ack_info_ptr;
	
	/* Find the number of harq channels and explicit ack_delay for this HARQ connection.																			*/
	num_harq_channels = conn_info_ptr->harq_conn_params_ptr->num_channels;
	explicit_ack_delay = conn_info_ptr->harq_conn_params_ptr->ack_delay;

	/* Initialize the head_ptr	*/
	head_ptr->num_blocked_channels = 0; 															/* At present, no channels are blocked for ack waiting.			*/
	
	/* We will retain uninitialized values here, because we keep the count of blocked channels and know how many array elements to read.							*/
	head_ptr->blocked_chan_array = (int*) op_prg_cmo_alloc (harq_cat_mem_handle, num_harq_channels * sizeof (int)); 
																													
	head_ptr->next = OPC_NIL;
	head_ptr->tail = OPC_NIL;
	
	if (head_ptr->blocked_chan_array == OPC_NIL)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* Find out how many elements need to be created by reading the ack_delay																						*/
	for (i = 0; i < explicit_ack_delay; i++)
		{
		/* Go through the same initializations.																														*/
		new_harq_explicit_ack_info_ptr = (HarqT_Explicit_Ack_Channel_Info*) op_prg_cmo_alloc (harq_cat_mem_handle, sizeof (HarqT_Explicit_Ack_Channel_Info));
		
		if (new_harq_explicit_ack_info_ptr == OPC_NIL)
			FRET (OPC_COMPCODE_FAILURE);
		
		new_harq_explicit_ack_info_ptr->num_blocked_channels = 0;
		new_harq_explicit_ack_info_ptr->blocked_chan_array = (int*) op_prg_cmo_alloc (harq_cat_mem_handle, num_harq_channels * sizeof (int));
		new_harq_explicit_ack_info_ptr->next = OPC_NIL;
		new_harq_explicit_ack_info_ptr->tail = OPC_NIL;
		
		if (new_harq_explicit_ack_info_ptr->blocked_chan_array == OPC_NIL)
			FRET (OPC_COMPCODE_FAILURE);
		
		/* Now complete the chain.																																	*/
		harq_explicit_ack_info_ptr->next = new_harq_explicit_ack_info_ptr;
		harq_explicit_ack_info_ptr = new_harq_explicit_ack_info_ptr;
		}
	
	/* The last harq_explicit_ack_info_ptr represents the tail. Store it in the head pointer for later easy access.													*/
	head_ptr->tail = harq_explicit_ack_info_ptr;
	
	/* Finally insert this head_ptr in the hash table.																												*/
	prg_bin_hash_table_item_insert (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier), (void*) head_ptr, OPC_NIL);
	
	/* Operation successful.																																		*/
	FRET (OPC_COMPCODE_SUCCESS);
	}

/* This function inserts the incoming harq_channel_id in the hash table element for explicit ack schedule.															*/
void 
harq_support_explicit_timed_ack_schedule (OpT_Int64 connection_identifier, int harq_channel_id)
	{
	HarqT_Conn_Parameters*				harq_conn_params_ptr;
	HarqT_Explicit_Ack_Channel_Info*	head_ptr;
	HarqT_Explicit_Ack_Channel_Info*	tail_ptr;
	
	FIN (harq_support_explicit_timed_ack_schedule (...));
	
	/* Find the connection parameters.																																*/
	harq_conn_params_ptr = (HarqT_Conn_Parameters*) prg_bin_hash_table_item_get (harq_conn_params_table, (OpT_Int64*) (&connection_identifier));
	
	/* Access this connection in the explicit ack hash table.																										*/
	head_ptr = (HarqT_Explicit_Ack_Channel_Info*) prg_bin_hash_table_item_get (harq_explicit_ack_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* From the head_ptr, access the tail_ptr. If there is no tail, i.e. ack_delay = 0, then head = tail.															*/
	tail_ptr = (head_ptr->tail == OPC_NIL) ? head_ptr : head_ptr->tail;
	
	/* If the number of blocked_channels already equals the maximum, then we are dealing with a bug. Channels cannot exceed maximum allowable limit.				*/
	if (tail_ptr->num_blocked_channels == harq_conn_params_ptr->num_channels)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_explicit_timed_ack_schedule", "more channels being scheduled than what is allowed");
	
	/* Insert the incoming channel_id at the appropriate array index and increment the count																		*/
	tail_ptr->blocked_chan_array [tail_ptr->num_blocked_channels] = harq_channel_id;
	tail_ptr->num_blocked_channels += 1;
	
	/* User defined connection/channel trace.																														*/
	if (harq_support_channel_trace_active (connection_identifier, harq_channel_id))
		{
		char			msg [256];
		char			msg1 [64];
		
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_channel_id);
		sprintf (msg, "\nScheduling channel for explicit acknowledgement.");
		strcat (msg, msg1);
		}
	
	FOUT;
	}

/* The function frees a given HARQ channel for future transmissions.																								*/
void						
harq_support_transmission_channel_free (OpT_Int64 connection_identifier, int harq_channel, HarqT_Ack_Method ack_method)
	{
	int*						harq_channel_id_ptr;
	List*						harq_avail_list_lptr;
	HarqT_Transmission_State*	harq_tx_state_ptr;
	char						msg [2048];
	HarqT_Conn_Parameters*		harq_conn_params_ptr;
	
	FIN (harq_support_transmission_channel_free (...));
	
	/* Clean up HARQ packet info pointer...there is really no need to reset any other data structure elements, except we need to return the channel pointer			*/
	/* back to the list of available channels, so that it can be picked up for future transmissions.																*/
	
	/* Find the transmission state of this connection bearing in mind that the incoming connection handle can be passed from the receiver.							*/
	harq_tx_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
	
	/* Find connection parameters.																																	*/
	harq_conn_params_ptr = (HarqT_Conn_Parameters*) prg_bin_hash_table_item_get (harq_conn_params_table, (OpT_Int64*) (&connection_identifier)); 
	
	/* Non-NIL sanity check																																			*/
	if (harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->harq_channel_id_ptr == OPC_NIL)
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_transmission_channel_free", "attempted to free a nonexistant channel");
	
	harq_channel_id_ptr = harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->harq_channel_id_ptr;
	
	/* User defined connection/channel trace.																														*/
	if (harq_support_channel_trace_active (connection_identifier, harq_channel))
		{
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_channel);
		sprintf (msg, "\nBeginning freeing of the HARQ transmission channel.");
		strcat (msg, msg1);
		}

	/* The channel is freed and inserted to the available list only for explicit ack method. For implicit ack, the only way channel can become freed is				*/
	/* force free by the sender regardless of whether maximum retransmission attempts have occurred or not.															*/
	if (ack_method != HarqC_Ack_Implicit)
		{
		harq_avail_list_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
		op_prg_list_insert (harq_avail_list_lptr, harq_channel_id_ptr, OPC_LISTPOS_HEAD);
		
		/* The transmission element's packet info ptr can now let go of the reference it is holding...we do this for clarity purposes.								*/
		harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->harq_channel_id_ptr = OPC_NIL;
		
		/* User defined connection/channel trace.																													*/
		if (harq_support_channel_trace_active (connection_identifier, harq_channel))
			{
			char			msg2 [256];
			sprintf (msg2, "\nExplicit ack channel has been inserted in the list of available channels.");
			strcat (msg, msg2);
			}
		}
		
	/* Finally make the packet_size 0. This is done because the HARQ buffer has been cleared of its packet and it should indicate so.								*/
	harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->packet_size = 0;
	
	/* User defined connection/channel trace.																														*/
	if (harq_support_channel_trace_active (connection_identifier, harq_channel))
		{
		char			msg2 [256];
		sprintf (msg2, "\nChannel free for new transmission.");
		strcat (msg, msg2);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	FOUT;
	}

/* This function inserts a channel in the ack waiting list for that connection																						*/
/* The caller can also solicit all implicit ack channels that are currently waiting to be acknowledged.																*/
/* For explicit acknowledgements, this list is expected to be freed immediately.																					*/
void 
harq_support_channel_ack_waiting_insert (OpT_Int64 connection_identifier, int* harq_channel_ptr)
	{
	List*					harq_ack_waiting_lptr;
	
	FIN (harq_support_channel_ack_waiting_insert (...));
	
	/* We are getting a live pointer. Insert this in the appropriate ack_waiting list.																				*/
	harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
	op_prg_list_insert (harq_ack_waiting_lptr, harq_channel_ptr, OPC_LISTPOS_TAIL);
	
	/* User defined connection/channel trace.																														*/
	if (harq_support_channel_trace_active (connection_identifier, *harq_channel_ptr))
		{
		char			msg [512];
		char			msg1 [64];
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, *harq_channel_ptr);
		sprintf (msg, "\nDecoding successful, inserting this channel in ack-waiting list. To be used for future new HARQ transmissions.");
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
		
	FOUT;
	}

/* If the channel is taken out of any list (list of free channels or ack_waiting channels), remove it from that list.												*/
/* Once the channel is taken out of the appropriate list, insert it in the harq_pk_info_ptr. This is where the pointer is preserved until it is released.			*/
/* Remember that the conn_info_ptr may be called for UL...in this case, using the key, find appropriate tx state array and set this channel for the SS.				*/	
void
harq_support_explicit_ack_transmission_channel_find (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, 
															int* harq_channel_ptr, OpT_Packet_Size* size_fitted_ptr)
	{
	List*						harq_avail_lptr;
	int*						harq_avail_chan_ptr;
	OpT_Int64					connection_identifier;
	HarqT_Transmission_State*	harq_tx_state_ptr;
	HarqT_Receiver_State*		harq_rx_state_ptr;
	
	FIN (harq_support_explicit_ack_transmission_channel_find (...));
	
	/* Find the connection identifier.																																*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	/* Examine if the channel can be found from the available channel list.																							*/
	harq_avail_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));

	/* Statistic writing.																																		*/
	if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->avail_chan_stathandle))
		op_stat_write (conn_info_ptr->harq_conn_params_ptr->avail_chan_stathandle, (double) op_prg_list_size (harq_avail_lptr));
	
	if (op_prg_list_size (harq_avail_lptr) > 0)
		{
		/* Only access the first channel in the list of available channels.																							*/
		harq_avail_chan_ptr = (int*) op_prg_list_access (harq_avail_lptr, OPC_LISTPOS_HEAD);
			
		/* Fit the incoming packet size in the HARQ buffer size defined by this channel.																			*/
		harq_support_buffer_packet_fit_calculate (conn_info_ptr, packet_size, *harq_avail_chan_ptr, size_fitted_ptr);
		
		/* It is possible for a channel to be available, while nothing can be fitted anymore. This happens if the buffer aggregation is used.						*/
		/* In pathological cases, it is very likely that only a few channels are in used, but they are used heavily. Thus we check if something was fitted.			*/
		/* If nothing was fitted, we do not return any channel and give the caller a false impression that channels are available.									*/
		if (*size_fitted_ptr == 0)
			{
			FOUT;
			}
			
		/* At this point, remove the channel that was accessed.																										*/
		harq_avail_chan_ptr = (int*) op_prg_list_remove (harq_avail_lptr, OPC_LISTPOS_HEAD);
		
		/* Set this channel value on the pointer coming into this function.																							*/
		*harq_channel_ptr = *harq_avail_chan_ptr;
		
		/* Once this channel has been deemed fit for transmission, the pointer must be stored in the corresponding packet info pointer of the transmission element.	*/
		/* First find the transmission element bearing in mind that it may need to be found somewhere else(as the receiver may be calling this function).			*/
		if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
			/* Find the harq_tx_state_ptr for this connection.																										*/
			harq_tx_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		else
			/* the transmission state pointer is available directly.																								*/
			harq_tx_state_ptr = conn_info_ptr->harq_tx_state_ptr;
		
		/* Now store this channel pointer																															*/
		harq_tx_state_ptr [*harq_avail_chan_ptr].harq_pk_info_ptr->harq_channel_id_ptr = harq_avail_chan_ptr;

		/* At this point, find the corresponding receiver element and set is_packet_received to OPC_FALSE. It will be switched to TRUE if the packet is received.	*/
		if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
			harq_rx_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		else
			harq_rx_state_ptr = conn_info_ptr->harq_rx_state_ptr;
		
		harq_rx_state_ptr [*harq_avail_chan_ptr].is_packet_received = OPC_FALSE;
	
		/* Return from the function.																																*/
		FOUT;
		}
	else
		{
		/* can't return any channel. Do not do anything.																											*/
		FOUT;
		}
	
	}

/* The function below finds a channel for transmission for implicit acknowledgements. The return value indicates if this channel comes from a "available list"		*/
/* or implicit ack waiting list. This enables the caller to either use the channel for implicit ack or as is.														*/
Boolean					
harq_support_implicit_ack_transmission_channel_find (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, 
																int* harq_channel_ptr, OpT_Packet_Size* size_fitted_ptr)
	{
	OpT_Int64					connection_identifier;
	List*						harq_ack_waiting_lptr;
	List*						harq_avail_lptr;
	int*						harq_ack_waiting_chan_ptr;
	int*						harq_avail_chan_ptr;
	HarqT_Transmission_State*	harq_tx_state_ptr;
	HarqT_Receiver_State*		harq_rx_state_ptr;
	
	FIN (harq_support_implicit_ack_transmission_channel_find (...));
	
	/* Find the connection identifier.																																*/
	connection_identifier = conn_info_ptr->connection_identifier;
	
	harq_ack_waiting_lptr = (List*) prg_bin_hash_table_item_get (harq_ack_waiting_chan_table, (OpT_Int64*) (&connection_identifier));
	harq_avail_lptr = (List*) prg_bin_hash_table_item_get (harq_available_chan_table, (OpT_Int64*) (&connection_identifier));
	
	/* Statistic writing.																																			*/
	if (op_stat_valid (conn_info_ptr->harq_conn_params_ptr->avail_chan_stathandle))
		op_stat_write (conn_info_ptr->harq_conn_params_ptr->avail_chan_stathandle, (double) op_prg_list_size (harq_avail_lptr) + 
			(double) op_prg_list_size (harq_ack_waiting_lptr));
	
	/* First determine if a channel can be returned from ack waiting list.																							*/
	if (op_prg_list_size (harq_ack_waiting_lptr) > 0)
		{
		/* Only access the first channel in the ack waiting list.																									*/
		harq_ack_waiting_chan_ptr = (int*) op_prg_list_access (harq_ack_waiting_lptr, OPC_LISTPOS_HEAD);
		
		/* Fit the incoming packet size in the HARQ buffer size defined by this channel.																			*/
		harq_support_buffer_packet_fit_calculate (conn_info_ptr, packet_size, *harq_ack_waiting_chan_ptr, size_fitted_ptr);

		/* It is possible for a channel to be available, while nothing can be fitted anymore. This happens if the buffer aggregation is used.						*/
		/* In pathological cases, it is very likely that only a few channels are in used, but they are used heavily. Thus we check if something was fitted.			*/
		/* If nothing was fitted, we do not return any channel and give the caller a false impression that channels are available.									*/
		if (*size_fitted_ptr == 0)
			{
			FRET (OPC_FALSE);
			}
		
		/* At this point, remove the channel that was accessed.																										*/
		harq_ack_waiting_chan_ptr = (int*) op_prg_list_remove (harq_ack_waiting_lptr, OPC_LISTPOS_HEAD);
		
		/* Set this channel value on the pointer coming into this function.																							*/
		*harq_channel_ptr = *harq_ack_waiting_chan_ptr;
		
		/* Once this channel has been deemed fit for transmission, the pointer must be stored in the corresponding packet info pointer of the transmission element.	*/
		/* First find the transmission element bearing in mind that it may need to be found somewhere else(as the receiver may be calling this function).			*/
		if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
			{
			/* Find the harq_tx_state_ptr for this connection.																										*/
			harq_tx_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
			}
		else
			/* the transmission state pointer is available directly.																								*/
			harq_tx_state_ptr = conn_info_ptr->harq_tx_state_ptr;
		
		/* Now store this channel pointer																															*/
		harq_tx_state_ptr [*harq_ack_waiting_chan_ptr].harq_pk_info_ptr->harq_channel_id_ptr = harq_ack_waiting_chan_ptr;

		/* At this point, find the corresponding receiver element and set is_packet_received to OPC_FALSE. It will be switched to TRUE if the packet is received.	*/
		if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
			harq_rx_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		else
			harq_rx_state_ptr = conn_info_ptr->harq_rx_state_ptr;
		
		harq_rx_state_ptr [*harq_ack_waiting_chan_ptr].is_packet_received = OPC_FALSE;
		
		if (harq_support_channel_trace_active (connection_identifier, *harq_channel_ptr))
			{
			char			msg [512];
			char			msg1 [64];
			
			sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, *harq_channel_ptr);
			sprintf (msg, "Implicit ack: Found channel from ack waiting list. Buffer filled = "OPC_INT64_FMT " queried = "OPC_INT64_FMT, 
				*size_fitted_ptr, packet_size);
			strcat (msg, msg1);
			op_prg_odb_print_major (msg, OPC_NIL);
			}
		
		/* Return OPC_TRUE as we gave away an ack waiting channel.																									*/
		FRET (OPC_TRUE);
		}
	else
		{
		/* Examine if the channel can be found from the available channel list.																						*/
		if (op_prg_list_size (harq_avail_lptr) > 0)
			{
			/* Only access the first channel in the list of available channels.																						*/
			harq_avail_chan_ptr = (int*) op_prg_list_access (harq_avail_lptr, OPC_LISTPOS_HEAD);
			
			/* Fit the incoming packet size in the HARQ buffer size defined by this channel.																		*/
			harq_support_buffer_packet_fit_calculate (conn_info_ptr, packet_size, *harq_avail_chan_ptr, size_fitted_ptr);
			
			/* It is possible for a channel to be available, while nothing can be fitted anymore. This happens if the buffer aggregation is used.					*/
			/* In pathological cases, it is very likely that only a few channels are in used, but they are used heavily. Thus we check if something was fitted.		*/
			/* If nothing was fitted, we do not return any channel and give the caller a false impression that channels are available.								*/
			if (*size_fitted_ptr == 0)
				{
				FRET (OPC_FALSE);
				}
			
			/* At this point, remove the channel that was accessed.																									*/
			harq_avail_chan_ptr = (int*) op_prg_list_remove (harq_avail_lptr, OPC_LISTPOS_HEAD);
		
			/* Set this channel value on the pointer coming into this function.																						*/
			*harq_channel_ptr = *harq_avail_chan_ptr;
		
			/* Once this channel has been deemed fit for transmission, the pointer must be stored in the corresponding packet info  of the transmission element.	*/
			/* First find the transmission element bearing in mind that it may need to be found somewhere else(as the receiver may be calling this function).		*/
			if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
				{
				/* Find the harq_tx_state_ptr for this connection.																									*/
				harq_tx_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
				}
			else
				/* the transmission state pointer is available directly.																							*/
				harq_tx_state_ptr = conn_info_ptr->harq_tx_state_ptr;
		
			/* Now store this channel pointer																														*/
			harq_tx_state_ptr [*harq_avail_chan_ptr].harq_pk_info_ptr->harq_channel_id_ptr = harq_avail_chan_ptr;
			
			/* At this point, find the corresponding receiver element and set is_packet_received to OPC_FALSE. It will be switched to TRUE if the packet is received.	*/
			if (conn_info_ptr->harq_rx_state_ptr == OPC_NIL)
				harq_rx_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
			else
				harq_rx_state_ptr = conn_info_ptr->harq_rx_state_ptr;
		
			harq_rx_state_ptr [*harq_avail_chan_ptr].is_packet_received = OPC_FALSE;
			
			if (harq_support_channel_trace_active (connection_identifier, *harq_channel_ptr))
				{
				char			msg [512];
				char			msg1 [64];
			
				sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, *harq_channel_ptr);
				sprintf (msg, "Implicit ack: Found channel from available list. Buffer filled = "OPC_INT64_FMT " queried = "OPC_INT64_FMT, 
					*size_fitted_ptr, packet_size);
				strcat (msg, msg1);
				op_prg_odb_print_major (msg, OPC_NIL);
				}
		
			/* Return OPC-FALSE as we gave away an available channel.																								*/
			FRET (OPC_FALSE);
			}
		else
			{
			/* can't return any channel. Do not do anything.																										*/
			FRET (OPC_FALSE);
			}
		
		}
	
	}

/* This function calculates if an incoming packet_size can be fitted in the given harq_channel of the incoming HARQ connection.										*/
/* If the full packet cannot be fitted, then we fit whatever is possible and indicate the remaining unfitted portion in size_fitted_ptr.							*/
void					
harq_support_buffer_packet_fit_calculate (HarqT_Conn_Info* conn_info_ptr, OpT_Packet_Size packet_size, int harq_channel, OpT_Packet_Size* size_fitted_ptr)
	{
	OpT_Packet_Size 				available_size;
	OpT_Int64						connection_identifier;
	HarqT_Transmission_State*		harq_tx_state_ptr;
	HarqT_Receiver_State*			harq_rx_state_ptr;
		
	FIN (harq_support_buffer_packet_fit_calculate ());
	
	/* Standard error check: harq_channel must not exceed its range.																								*/
	if ((harq_channel < 0) || (harq_channel > conn_info_ptr->harq_conn_params_ptr->num_channels))
		op_sim_error (OPC_SIM_ERROR_ABORT, "aborting from harq_support_buffer_packet_fit_calculate", "incorrect harq_channel passed in the function");
	
	/* Find the connection identifier.																																*/
	connection_identifier = conn_info_ptr->connection_identifier;

	/* This function can be called for either the transmitter or the receiver of the HARQ connection. Calling by receiver doesn't make sense 						*/
	/* until one realizes the role of "allocation" in connection oriented systems like WiMAX. The point here is that we may or may not have 						*/
	/* the tx state ptr, which we must first find.																													*/
	if (conn_info_ptr->harq_tx_state_ptr == OPC_NIL)
		{
		/* Find the harq_tx_state_ptr for this connection.																											*/
		harq_tx_state_ptr = (HarqT_Transmission_State*) prg_bin_hash_table_item_get (harq_tx_elem_table, (OpT_Int64*) (&connection_identifier));
		harq_rx_state_ptr = conn_info_ptr->harq_rx_state_ptr;
		}
	else
		{
		/* the transmission state pointer is available directly.																									*/
		harq_tx_state_ptr = conn_info_ptr->harq_tx_state_ptr;
		harq_rx_state_ptr = (HarqT_Receiver_State*) prg_bin_hash_table_item_get (harq_rx_elem_table, (OpT_Int64*) (&connection_identifier));
		}
	
	/* In transition, it is possible that there is no transmission/receiver state yet created. In this case, simply indicate unavailability to the caller and exit.	*/
	if ((harq_tx_state_ptr == OPC_NIL) || (harq_rx_state_ptr == OPC_NIL))
		{
		*size_fitted_ptr = 0;
		FOUT;
		}
	
	/* This function is very simple. Calculate the "available buffer size" corresponding to the harq_channel.														*/
	/* To do this calculation, consider the fact that buffer aggregation flag may be enabled for this connection.													*/
	
	if (conn_info_ptr->harq_conn_params_ptr->buffer_aggr_flag == OPC_TRUE)
		{
		int							i;	
		
		/* In this case, the aggregate capacity is considered.																										*/
		available_size = conn_info_ptr->harq_conn_params_ptr->num_channels * conn_info_ptr->harq_conn_params_ptr->buffer_size;
		
		/* From this aggregate size, subtract the sizes of all occupied buffers.																					*/
		for (i = 0; i < conn_info_ptr->harq_conn_params_ptr->num_channels; i++)
			available_size -= (harq_tx_state_ptr [i].harq_pk_info_ptr->packet_size);
		}
	
	/* Otherwise we only need to peek inside the current channel, and find the available space.																		*/
	else
		{
		available_size = conn_info_ptr->harq_conn_params_ptr->buffer_size;
		
		/* It is possible we are reusing the same channel...so subtract the available size from it.																	*/
		available_size -= (harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->packet_size);
		}
	
	if (harq_support_channel_trace_active (connection_identifier, harq_channel))
		{
		char			msg [512];
		char			msg1 [64];
			
		sprintf (msg1, "\n"OPC_INT64_FMT"/%d\n", connection_identifier, harq_channel);
		sprintf (msg, "Available size found to be: "OPC_INT64_FMT, available_size);
		strcat (msg, msg1);
		op_prg_odb_print_major (msg, OPC_NIL);
		}
	
	/* If the available_size < 0 for some reason, make it 0. Slight mismatches are expected because the caller may "slightly overfill" HARQ channels.				*/
	/* Notice that available_size can be 0. This is because the caller may find some open transmission channels, but due to buffer_aggr_flag, some other			*/
	/* channels may be overloaded and hence other channels may not be used. 																						*/
	if (available_size < 0)
		available_size = 0;
	
	/* Fill in the size_fitted_ptr value																															*/
	if (available_size >= packet_size)
		*size_fitted_ptr = packet_size;
	else
		*size_fitted_ptr = available_size;
	
	/* At this point, on the channel harq_channel, we set the data size *size_fitted_ptr. We should increment the packet_size by that value. 						*/
	/* Note that when the actual packet comes here for transmission, we will set the value of the actual packet. We are only setting this value internally			*/
	/* right now, because the harq channels are found one by one by the external caller. The caller needs to know how much the current channel is filled.			*/
	/* If the buffer aggregation was on, we can continue overfilling one single channel, which does not violate any protocol primitive.								*/
	harq_tx_state_ptr [harq_channel].harq_pk_info_ptr->packet_size += *size_fitted_ptr;
	
	FOUT;
	}

/* Returns the string representation of the ack method - for tracing purposes																						*/
void
harq_support_ack_method_return (int ack_method, char* return_value)
	{
	char	ack_method_array [3][32] = {"Implicit Acknowledgement", "Explicit Acknowledgement", "Invalid Method"};
	
	FIN (harq_support_ack_method_return (...));
	
	if ((ack_method < 0) || (ack_method > 1))
		strcpy (return_value, ack_method_array [2]);
	else
		strcpy (return_value, ack_method_array [ack_method]);
	
	FOUT;
	}

/* Returns the string representation of the caller role - for tracing purposes																						*/
void
harq_support_caller_role_return (int caller_role, char* return_value)
	{
	char	caller_role_array [3][32] = {"Harq Transmitter", "Harq Receiver", "Unknown"};
	
	FIN (harq_support_caller_role_return (...));
	
	if ((caller_role < 0) || (caller_role > 1))
		strcpy (return_value, caller_role_array [2]);
	else
		strcpy (return_value, caller_role_array [caller_role]);
	
	FOUT;
	}

/* Returns the string representation of the buffer aggr flag - for tracing purposes																					*/
void						
harq_support_buffer_aggr_return (Boolean buffer_aggr_flag, char* return_value)
	{
	FIN (harq_support_buffer_aggr_return (...));
	
	if (buffer_aggr_flag)
		strcpy (return_value, "Enabled");
	else
		strcpy (return_value, "Disabled");
	
	FOUT;
	}

/* The following function takes the connection information and the HARQ channel ID and creates a string representation from channel key and channel ID.				*/
/* The format of the string = "<key>/<ID>". Then the function determines if the trace with this string is active, and if so, it will return OPC_TRUE.				*/
#ifdef HARQD_CHANNEL_TRACE
Boolean
harq_support_channel_trace_active (OpT_Int64 connection_identifier, int harq_channel_id)
	{
	char					harq_trace_str [128];
	Boolean					trace_active;
	
	FIN (harq_support_channel_trace_active (...));
	
	/* Use the sprintf function to get the string representation from connection ID and channel ID. 																*/
	sprintf (harq_trace_str, OPC_INT64_FMT"/%d", connection_identifier, harq_channel_id);
	
	/* Is there a trace with the string harq_trace_str?																												*/
	trace_active = op_prg_odb_ltrace_active (harq_trace_str);
	
	FRET (trace_active);
	}
#endif
	
void 
harq_support_pk_info_print (void* field_ptr, PrgT_List* list_ptr)
	{
	HarqT_Packet_Info*			harq_pk_info_ptr;
	char						temp_str [2048];
	char*						alloc_str;
	int							harq_channel;
	char 						new_transmit_array [2][32] = {"Retransmission", "New Transmission"};
	char						decode_array [2][16] = {"No Decode", "Decode"};

	FIN (harq_support_pk_info_print (<args>));

	/* First obtain the packet field ptr																															*/
	harq_pk_info_ptr = (HarqT_Packet_Info*) field_ptr;

	harq_channel = *(harq_pk_info_ptr->harq_channel_id_ptr);
	
	/* Use list ptr to print all information																														*/
	sprintf (temp_str, "\nHARQ Information: Connection ID: "OPC_INT64_FMT", HARQ channel: %d, (%s), (%s), Packet size: "OPC_INT64_FMT, 
		harq_pk_info_ptr->connection_identifier, harq_channel, new_transmit_array [harq_pk_info_ptr->is_new_transmission], 
		decode_array [harq_pk_info_ptr->is_decode_success], harq_pk_info_ptr->packet_size);
	PKPRINT_STRING_INSERT (alloc_str, temp_str, list_ptr);
		
	FOUT;
	}
/********************************************************************************************************************************************************************/
