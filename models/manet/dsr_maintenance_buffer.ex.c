/* dsr_maintenance_buffer.ex.c */
/* C support file for the route request table	*/


/****************************************/
/*		Copyright (c) 1987-2008			*/
/*		by OPNET Technologies, Inc.		*/
/*		(A Delaware Corporation)		*/
/*	7255 Woodmont Av., Suite 250  		*/
/*     Bethesda, MD 20814, U.S.A.       */
/*			All Rights Reserved.		*/
/****************************************/

/***** Includes *****/
#include <opnet.h>
#include <manet.h>
#include <dsr.h>
#include <dsr_ptypes.h>
#include <ip_addr_v4.h>


/**** Prototypes ****/
static void								dsr_maintenance_buffer_last_confirm_time_update (DsrT_Maintenance_Buffer*, InetT_Address);
static Boolean							dsr_maintenance_buffer_size_exceeded (DsrT_Maintenance_Buffer*);
static void								dsr_maintenance_buffer_size_stat_update (DsrT_Maintenance_Buffer*);
static DsrT_Maintenance_Buffer*			dsr_support_maint_buffer_mem_alloc (void);
static DsrT_Maintenance_Queue*			dsr_support_maintenance_buffer_next_hop_mem_alloc (void);
static void								dsr_support_maintenance_buffer_next_hop_mem_free (DsrT_Maintenance_Queue*);

EXTERN_C_BEGIN
void									dsr_rte_maintenance_expiry_handle (void*, int);
EXTERN_C_END


/**************************************************/
/************** MAINTENANCE BUFFER ****************/
/**************************************************/

DsrT_Maintenance_Buffer*
dsr_maintenance_buffer_create (int max_size, double maint_holdoff_time, double maint_ack_timer, 
								int max_maint_retrans, DsrT_Stathandles* stat_handle_ptr)
	{
	DsrT_Maintenance_Buffer*		maint_buffer_ptr = OPC_NIL;
	
	/** Creates the maintenance buffer to enqueue packets	**/
	/** waiting next hop reachability confirmation			**/
	FIN (dsr_maintenance_buffer_create (<args>));
	
	/* Allocate memory and creates the send buffer	*/
	maint_buffer_ptr = dsr_support_maint_buffer_mem_alloc ();
	
	/* Set the attributes of the maintenance buffer	*/
	maint_buffer_ptr->max_buffer_size = max_size;
	maint_buffer_ptr->holdoff_time = maint_holdoff_time;
	maint_buffer_ptr->ack_timeout = maint_ack_timer;
	maint_buffer_ptr->max_retransmissions = max_maint_retrans;
	maint_buffer_ptr->dsr_stat_ptr = stat_handle_ptr;
	
	FRET (maint_buffer_ptr);
	}


Compcode
dsr_maintenance_buffer_pkt_enqueue (DsrT_Maintenance_Buffer* maint_buffer_ptr, Packet* pkptr, 
												InetT_Address next_hop_address, int maint_id)
	{
	char							maint_id_str [20];
	DsrT_Maintenance_Queue*			pkt_info_ptr = OPC_NIL;
	void*							old_contents_ptr = OPC_NIL;
	int*							maint_id_ptr;
	
	/** Enqueues a packet into the maintenance buffer	**/
	/** Returns a Success if packet was enqueued		**/
	FIN (dsr_maintenance_buffer_pkt_enqueue (<args>));
	
	/* Get the maintenance ID string	*/
	sprintf (maint_id_str, "%d", maint_id);
	
	/* Check if there is space in the maintenance	*/
	/* buffer to enqueue the packet					*/
	if (dsr_maintenance_buffer_size_exceeded (maint_buffer_ptr) == OPC_TRUE)
		{
		/* There is no space in the maintenance buffer	*/
		/* to enqueue the packet. Do not enqueue packet	*/
		FRET (OPC_COMPCODE_FAILURE);
		}
	
	if (maint_buffer_ptr->max_retransmissions == 0)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* There is space in the maintenance buffer to	*/
	/* enqueue the packet. Check is the maintenance	*/
	/* ID string exists.							*/
	pkt_info_ptr = (DsrT_Maintenance_Queue*) 
		prg_string_hash_table_item_get (maint_buffer_ptr->maintenance_buffer_table, maint_id_str);
	
	if (pkt_info_ptr != OPC_NIL)
		{
		/* The maintenance ID has to be unique	*/
		op_sim_end ("Invalid maintenance identifier being inserted in the maintenance buffer", OPC_NIL, OPC_NIL, OPC_NIL);
		}
	
	/* No entry exists for the next hop	*/
	/* Create an entry and enqueue		*/
	pkt_info_ptr = dsr_support_maintenance_buffer_next_hop_mem_alloc ();
		
	/* Insert the packet into the queue	*/
	pkt_info_ptr->pkptr = manet_rte_ip_pkt_copy (pkptr);
	
	/* Set the next hop address	*/
	pkt_info_ptr->next_hop_addr = inet_address_copy (next_hop_address);
	
	/* Schedule the maintenance request expiry timer	*/
	maint_id_ptr  = (int *) op_prg_mem_alloc (sizeof (int));
	*maint_id_ptr = maint_id; 
	pkt_info_ptr->maint_request_expiry_timer = op_intrpt_schedule_call (
									op_sim_time () + maint_buffer_ptr->ack_timeout, DSRC_ROUTE_MAINTENANCE_TIMER, 
									dsr_rte_maintenance_expiry_handle, (void *) maint_id_ptr);
	
	/* Update the statistics	*/
	pkt_info_ptr->num_attempts++;
	
	/* Set the next hop queue into the maintenance table	*/
	prg_string_hash_table_item_insert (maint_buffer_ptr->maintenance_buffer_table, maint_id_str, pkt_info_ptr, &old_contents_ptr);
	
	/* Increment the size of the buffer	*/
	maint_buffer_ptr->current_buffer_size++;
	
	/* Update the statistic for the maintenance buffer size	*/
	dsr_maintenance_buffer_size_stat_update (maint_buffer_ptr);
	
	FRET (OPC_COMPCODE_SUCCESS);
	}


Boolean
dsr_maintenance_buffer_maint_needed (DsrT_Maintenance_Buffer* maint_buffer_ptr, InetT_Address next_hop_addr)
	{
	double						current_time;
	double*						confirm_time_ptr = OPC_NIL;
	char						next_hop_str [INETC_ADDR_STR_LEN];
	
	/** Determines whether next hop maintenance	**/
	/** is needed based on the maintenance		**/
	/** holdoff time.							**/
	FIN (dsr_maintenance_buffer_maint_needed (<args>));
	
	/* Get the next hop address as a 	*/
	/* string to index the hash table	*/
	inet_address_print (next_hop_str, next_hop_addr);
	
	/* Get the next hop maintenance information	*/
	confirm_time_ptr = (double*) prg_string_hash_table_item_get (maint_buffer_ptr->dest_confirm_time_table, next_hop_str);
	
	if (confirm_time_ptr == OPC_NIL)
		{
		/* Maintenace has never been done	*/
		FRET (OPC_TRUE);
		}
	
	/* Get the current time	*/
	current_time = op_sim_time ();
	
	/* If the time elapsed since the last maintenance	*/
	/* confirmation is greater than the holdoff time	*/
	/* then next hop confirmation is needed				*/
	if ((current_time - (*confirm_time_ptr)) > maint_buffer_ptr->holdoff_time)
		{
		FRET (OPC_TRUE);
		}
	else
		{
		FRET (OPC_FALSE);
		}
	}


List*
dsr_maintenance_buffer_ack_request_expire (DsrT_Maintenance_Buffer* maint_buffer_ptr, int maint_id,
										Boolean* max_retrans_reached, InetT_Address* next_hop_address)
	{
	List*						pkt_lptr = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
	char*						key_str;
	int							num_pkts, count;
	char						maint_id_str [20];
	DsrT_Maintenance_Queue*		pkt_info_ptr = OPC_NIL;
	DsrT_Maintenance_Queue*		check_pkt_info_ptr = OPC_NIL;
	Packet*						copy_pkptr = OPC_NIL;
	int*						maint_id_ptr;
	
	/* The acknowledgement request has expired	*/
	/* Handle the internals of the maintenance	*/
	/* buffer on expiry of this maintenance ID	*/
	FIN (dsr_maintenance_buffer_ack_request_expire (<args>));
	
	/* Create a packet list	*/
	pkt_lptr = op_prg_list_create ();
	
	/* Get the maintenance ID string	*/
	sprintf (maint_id_str, "%d", maint_id);
	
	/* Get the next hop maintenance information	*/
	pkt_info_ptr = (DsrT_Maintenance_Queue*) 
		prg_string_hash_table_item_get (maint_buffer_ptr->maintenance_buffer_table, maint_id_str);
	
	if (pkt_info_ptr == OPC_NIL)
		{
		/* No packets in the maintenance	*/
		/* buffer with the requested ID		*/
		FRET (pkt_lptr);
		}
	
	if (op_ev_valid (pkt_info_ptr->maint_request_expiry_timer) && op_ev_pending (pkt_info_ptr->maint_request_expiry_timer))
		{
		/* Cancel the maintenance request timer	if valid */
		op_ev_cancel (pkt_info_ptr->maint_request_expiry_timer);
		}
	
	if (pkt_info_ptr->num_attempts >= maint_buffer_ptr->max_retransmissions)
		{
		/* Set the return values	*/
		*max_retrans_reached = OPC_TRUE;
		*next_hop_address = inet_address_copy (pkt_info_ptr->next_hop_addr);
		
		/* Maximum retransmission attempts have been reached	*/
		/* This destination is unreachable. Remove all packets	*/
		/* in the maintenance buffer that are waiting this next	*/
		/* hop confirmation. Get all packets waiting next hop	*/
		/* confirmation and determine find this destination		*/
		keys_lptr = prg_string_hash_table_keys_get (maint_buffer_ptr->maintenance_buffer_table);
		num_pkts = op_prg_list_size (keys_lptr);
		for (count = 0; count < num_pkts; count++)
			{
			key_str = (char*) op_prg_list_access (keys_lptr, count);
			check_pkt_info_ptr = (DsrT_Maintenance_Queue*) prg_string_hash_table_item_get (maint_buffer_ptr->maintenance_buffer_table, key_str);
			
			/* Check for NIL ptr */
			if (check_pkt_info_ptr == PRGC_NIL)
				continue;
			
			if (inet_address_equal (check_pkt_info_ptr->next_hop_addr, (*next_hop_address)))
				{
				/* Do not retransmit this packet again. Remove this		*/
				/* packet information from the maintenance buffer		*/
				check_pkt_info_ptr = (DsrT_Maintenance_Queue*) 
						prg_string_hash_table_item_remove (maint_buffer_ptr->maintenance_buffer_table, key_str);
				
				/* Return a copy of the packet so as to send	*/
				/* a route error if needed						*/
				copy_pkptr = manet_rte_ip_pkt_copy (check_pkt_info_ptr->pkptr);
				op_prg_list_insert (pkt_lptr, copy_pkptr, OPC_LISTPOS_TAIL);
				
				/* Reduce the current size of the buffer	*/
				maint_buffer_ptr->current_buffer_size -= 1;
				
				/* Destroy the packet	*/
				dsr_support_maintenance_buffer_next_hop_mem_free (check_pkt_info_ptr);
				}
			}
		
		/* Update the statistic for the maintenance buffer size	*/
		dsr_maintenance_buffer_size_stat_update (maint_buffer_ptr);
		
		/* Free the keys list	*/
		op_prg_list_free (keys_lptr);
		op_prg_mem_free (keys_lptr);
		
		FRET (pkt_lptr);
		}
	
	/* Set the return values	*/
	*max_retrans_reached = OPC_FALSE;
	*next_hop_address = inet_address_copy (pkt_info_ptr->next_hop_addr);
	
	/* Maximum retransmission attempts have not been	*/
	/* reached. Retry the maintenance request			*/
	pkt_info_ptr->num_attempts++;
	
	/* Schedule the next timeout	*/
	maint_id_ptr  = (int *) op_prg_mem_alloc (sizeof (int));
	*maint_id_ptr = maint_id; 
	pkt_info_ptr->maint_request_expiry_timer = op_intrpt_schedule_call (
										op_sim_time () + maint_buffer_ptr->ack_timeout, DSRC_ROUTE_MAINTENANCE_TIMER, 
										dsr_rte_maintenance_expiry_handle, (void *) maint_id_ptr);
	
	/* Return a copy of the packet to retransmit	*/
	copy_pkptr = manet_rte_ip_pkt_copy (pkt_info_ptr->pkptr);
	op_prg_list_insert (pkt_lptr, copy_pkptr, OPC_LISTPOS_TAIL);
	
	FRET (pkt_lptr);
	}


void
dsr_maintenance_buffer_pkt_confirmation_received (DsrT_Maintenance_Buffer* maint_buffer_ptr, InetT_Address next_hop_addr, int maint_id)
	{
	DsrT_Maintenance_Queue*		pkt_info_ptr = OPC_NIL;
	char						maint_id_str [20];
	
	/** Next hop confirmation has been received	**/
	/** Returns all packets in the queue for	**/
	/** which next hop confirmation has been	**/
	/** received. The list that is returned 	**/
	/** needs to be destroyed by the user		**/
	FIN (dsr_maintenance_buffer_pkt_confirmation_received (<args>));
	
	/* Get the maintenance ID string	*/
	sprintf (maint_id_str, "%d", maint_id);
	
	/* Get the next hop maintenance information	*/
	pkt_info_ptr = (DsrT_Maintenance_Queue*) 
		prg_string_hash_table_item_remove (maint_buffer_ptr->maintenance_buffer_table, maint_id_str);
	
	if (pkt_info_ptr == OPC_NIL)
		{
		/* There are no packets waiting next	*/
		/* hop confirmation for that address	*/
		FOUT;
		}
	
	/* Update the confirmation time for this next hop	*/
	dsr_maintenance_buffer_last_confirm_time_update (maint_buffer_ptr, next_hop_addr);
	
	if (op_ev_valid (pkt_info_ptr->maint_request_expiry_timer) && op_ev_pending (pkt_info_ptr->maint_request_expiry_timer))
		{
		/* Cancel the maintenance request timer	*/
		/* as a confirmation has been received	*/
		/* for this next hop address			*/
		op_ev_cancel (pkt_info_ptr->maint_request_expiry_timer);
		}
	
	/* Destroy the packet that has been confirmed	*/
	dsr_support_maintenance_buffer_next_hop_mem_free (pkt_info_ptr);
	
	/* Reduce the current size of the buffer	*/
	maint_buffer_ptr->current_buffer_size -= 1;
	
	/* Update the statistic for the maintenance buffer size	*/
	dsr_maintenance_buffer_size_stat_update (maint_buffer_ptr);
	
	FOUT;
	}


void
dsr_maintenance_buffer_print (DsrT_Maintenance_Buffer* maint_buffer_ptr)
	{
	List*						keys_lptr = OPC_NIL;
	int							num_id, count;
	char*						maint_id_str = OPC_NIL;
	DsrT_Maintenance_Queue*		pkt_info_ptr = OPC_NIL;
	char						dest_node_name [OMSC_HNAME_MAX_LEN];
	char						dest_hop_addr_str [INETC_ADDR_STR_LEN];
	char						temp_str [512];
	
	/** Prints the information stored in the	**/
	/** maintenance buffer						**/
	FIN (dsr_maintenance_buffer_print (<args>));
	
	/* Get the keys	*/
	keys_lptr = prg_string_hash_table_keys_get (maint_buffer_ptr->maintenance_buffer_table);
	
	/* Get the number of unique IDs	*/
	num_id = op_prg_list_size (keys_lptr);
	
	if (num_id > 0)
		{
		printf ("-------------------------------------------------------------\n");
		printf ("-                     MAINTENANCE BUFFER                    -\n");
		printf ("-------------------------------------------------------------\n\n");
		printf ("DESTINATION\t\t\t\t\t\tMAINTENANCE ID\t\t\tNUMBER OF ATTEMPTS\n");
		printf ("-----------\t\t\t\t\t\t--------------\t\t\t------------------\n");
		}
	
	for (count = 0; count < num_id; count++)
		{
		maint_id_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Get the information for each unique ID	*/
		pkt_info_ptr = (DsrT_Maintenance_Queue*) 
			prg_string_hash_table_item_get (maint_buffer_ptr->maintenance_buffer_table, maint_id_str);
		
		/* Print the information about this maintenance request	*/
		inet_address_print (dest_hop_addr_str, pkt_info_ptr->next_hop_addr);
		inet_address_to_hname (pkt_info_ptr->next_hop_addr, dest_node_name);
		sprintf (temp_str, "%s (%s)", dest_hop_addr_str, dest_node_name);
		printf ("%-48s\t%-25s\t%d\n", temp_str, maint_id_str, pkt_info_ptr->num_attempts);
		}
	
	/* Free the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}
	


/******* Internally callable procedures	********/

static void
dsr_maintenance_buffer_last_confirm_time_update (DsrT_Maintenance_Buffer* maint_buffer_ptr, InetT_Address next_hop_addr)
	{
	char		next_hop_str [INETC_ADDR_STR_LEN];
	double		current_time;
	double*		current_time_ptr;
	void*		old_contents_ptr = OPC_NIL;
	
	/** Updates the confirmation time for the	**/
	/** for the next hop node indicating that a	**/
	/** maintenance confirmation has been 		**/
	/** received for this node					**/
	FIN (dsr_maintenance_buffer_last_confirm_time_update (<args>));
	
	/* Get the next hop address as a 	*/
	/* string to index the hash table	*/
	inet_address_print (next_hop_str, next_hop_addr);
	
	/* Current time	*/
	current_time = op_sim_time ();
	
	/* Allocate memory to store the current time	*/
	current_time_ptr = (double*) op_prg_mem_alloc (sizeof (double));
	op_prg_mem_copy (&current_time, current_time_ptr, sizeof (double));
	
	/* Get the next hop maintenance information	*/
	prg_string_hash_table_item_insert (maint_buffer_ptr->dest_confirm_time_table, next_hop_str, current_time_ptr, &old_contents_ptr);
	
	/* Free the stale information	*/
	if (old_contents_ptr != OPC_NIL)
		op_prg_mem_free (old_contents_ptr);
	
	FOUT;
	}


static Boolean
dsr_maintenance_buffer_size_exceeded (DsrT_Maintenance_Buffer* maint_buffer_ptr)
	{
	/** Determines if there is any space in the	**/
	/** maintenance buffer to enqueue a packet	**/
	FIN (dsr_maintenance_buffer_size_exceeded (<args>));
	
	if (maint_buffer_ptr->current_buffer_size >= maint_buffer_ptr->max_buffer_size)
		{
		/* No space available	*/
		FRET (OPC_TRUE);
		}
	else
		{
		/* There is space in the buffer	*/
		FRET (OPC_FALSE);
		}
	}


static void
dsr_maintenance_buffer_size_stat_update (DsrT_Maintenance_Buffer* maint_buffer_ptr)
	{
	/** Updates the maintenance buffer size statistic	**/
	FIN (dsr_maintenance_buffer_size_stat_update (<args>));
	
	/* Updates the size of the maintenance buffer statistic	*/
	op_stat_write (maint_buffer_ptr->dsr_stat_ptr->maintenance_buffer_size_shandle, (double) maint_buffer_ptr->current_buffer_size);
	
	FOUT;
	}


static DsrT_Maintenance_Buffer*
dsr_support_maint_buffer_mem_alloc (void)
	{
	DsrT_Maintenance_Buffer*		maint_buffer_ptr = OPC_NIL;
	
	/** Allocates memory for the maintenance buffer	**/
	FIN (dsr_support_maint_buffer_mem_alloc (void));
	
	maint_buffer_ptr = (DsrT_Maintenance_Buffer*) op_prg_mem_alloc (sizeof (DsrT_Maintenance_Buffer));
	maint_buffer_ptr->maintenance_buffer_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	maint_buffer_ptr->dest_confirm_time_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	maint_buffer_ptr->current_buffer_size = 0;
	maint_buffer_ptr->max_buffer_size = 0;
	
	FRET (maint_buffer_ptr);
	}


static DsrT_Maintenance_Queue*
dsr_support_maintenance_buffer_next_hop_mem_alloc (void)
	{
	static Pmohandle				maint_buffer_entry_pmh;
	static Boolean					maint_buffer_entry_pmh_defined = OPC_FALSE;
	DsrT_Maintenance_Queue*			pkt_info_ptr = OPC_NIL;
	
	/** Allocates memory for an new next	**/
	/** hop in the maintenance buffer		**/
	FIN (dsr_support_maintenance_buffer_next_hop_mem_alloc (void));
	
	if (maint_buffer_entry_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route reply	*/
		/* option in the DSR packets if not already defined	*/
		maint_buffer_entry_pmh = op_prg_pmo_define ("Maintenance Buffer Entry", sizeof (DsrT_Maintenance_Queue), 32);
		maint_buffer_entry_pmh_defined = OPC_TRUE;
		}
	
	pkt_info_ptr = (DsrT_Maintenance_Queue*) op_prg_pmo_alloc (maint_buffer_entry_pmh);
	pkt_info_ptr->num_attempts = 0;
	
	FRET (pkt_info_ptr);
	}


static void
dsr_support_maintenance_buffer_next_hop_mem_free (DsrT_Maintenance_Queue* pkt_info_ptr)
	{
	/** Free the mempry associated with a packet	**/
	/** in the maintenance buffer					**/
	FIN (dsr_support_maintenance_buffer_next_hop_mem_free (<args>));
	
	/* Destroy the packet	*/
	manet_rte_ip_pkt_destroy (pkt_info_ptr->pkptr);
	
	/* Free the IP address	*/
	inet_address_destroy (pkt_info_ptr->next_hop_addr);
	
	/* Free the structure	*/
	op_prg_mem_free (pkt_info_ptr);
	
	FOUT;
	}
