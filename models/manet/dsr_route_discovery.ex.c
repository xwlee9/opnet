/* dsr_route_discovery.ex.c */
/* C support file for the route discovery	*/
/* which includes route requests and route	*/
/* reply message information buffers		*/


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
#include <dsr.h>
#include <dsr_ptypes.h>
#include <ip_addr_v4.h>

/**** Prototypes ****/
static void							dsr_route_request_max_request_entries_check (PrgT_String_Hash_Table*, int);
static void							dsr_route_request_table_size_check (DsrT_Route_Request_Table*);
static void							dsr_request_table_size_stat_update (DsrT_Route_Request_Table*);
static DsrT_Route_Request_Table*	dsr_request_table_mem_alloc (void);
static DsrT_Request_Orig_Entry*		dsr_route_request_orig_entry_mem_alloc (void);
static void							dsr_route_request_orig_entry_mem_free (DsrT_Request_Orig_Entry*);
static DsrT_Request_Forward_Entry*	dsr_route_request_forward_entry_mem_alloc (void);
static void							dsr_route_request_forward_entry_mem_free (DsrT_Request_Forward_Entry*);
static void							dsr_expired_grat_reply_entries_delete (DsrT_Route_Reply_Table*);
static DsrT_Route_Reply_Table*		dsr_grat_reply_table_mem_alloc (void);
static DsrT_Route_Reply_Entry*		dsr_grat_reply_entry_mem_alloc (void);
static void							dsr_grat_reply_entry_mem_free (DsrT_Route_Reply_Entry*);

EXTERN_C_BEGIN
void							dsr_rte_route_request_expiry_handle (void*, int);
EXTERN_C_END


/**********************************************************/
/***************** ROUTE REQUEST TABLE ********************/
/**********************************************************/
DsrT_Route_Request_Table*
dsr_route_request_tables_create (int max_size, int max_request_entries, int max_request_retrans,
								double init_request_timeout, double max_request_timeout, 
								double non_propagating_request_timer, 
								DsrT_Stathandles* stat_handle_ptr)
	{
	DsrT_Route_Request_Table*		route_request_table_ptr = OPC_NIL;
	
	/** Creates and initializes the route request table	**/	
	FIN (dsr_route_request_tables_create (<args>));
	
	route_request_table_ptr = dsr_request_table_mem_alloc ();
	route_request_table_ptr->max_table_size = max_size;
	route_request_table_ptr->max_request_table_entries = max_request_entries;
	route_request_table_ptr->max_request_retransmissions = max_request_retrans;
	route_request_table_ptr->init_request_period = init_request_timeout;
	route_request_table_ptr->max_request_period = max_request_timeout;
	route_request_table_ptr->non_prop_request_period = non_propagating_request_timer;
	route_request_table_ptr->dsr_stat_ptr = stat_handle_ptr;
	
	FRET (route_request_table_ptr);
	}	


void
dsr_route_request_originating_table_entry_insert (DsrT_Route_Request_Table* route_request_table_ptr, InetT_Address dest_address, 
											int ttl_value)
	{
	DsrT_Request_Orig_Entry*		route_request_orig_entry = OPC_NIL;
	char							dest_str [INETC_ADDR_STR_LEN];
	void*							old_contents_ptr = OPC_NIL;
	Boolean							new_entry = OPC_FALSE;
	InetT_Address*					copy_dest_address_ptr;
	double 							backoff_period;
	
	/** Inserts the route request into the originating	**/
	/** route request table								**/
	FIN (dsr_route_request_originating_table_entry_insert (<args>));
	
	/* Get the destination address as a */
	/* string to index the hash table	*/
	inet_address_print (dest_str, dest_address);
	
	/* Check if there exists an entry for this destination	*/
	/* node in the route request table						*/
	route_request_orig_entry = (DsrT_Request_Orig_Entry*) prg_string_hash_table_item_get 
													(route_request_table_ptr->route_request_send_table, dest_str);
	
	if (route_request_orig_entry == OPC_NIL)
		{
		/* There does not exist an entry for this destination	*/
		/* Check if there is space in the route request table	*/
		/* If the table is full, delete the entry which is 		*/
		/* least recently used (LRU)							*/
		dsr_route_request_table_size_check (route_request_table_ptr);
		
		/* There does not exist an entry for	*/
		/* this destination in the table		*/
		route_request_orig_entry = dsr_route_request_orig_entry_mem_alloc ();
			
		/* Insert the entry in the hash table	*/
		prg_string_hash_table_item_insert (route_request_table_ptr->route_request_send_table, dest_str, 
							route_request_orig_entry, &old_contents_ptr);
		
		/* Increment the size of the request table	*/
		route_request_table_ptr->current_table_size++;
		
		/* Update the request table size statistic	*/
		dsr_request_table_size_stat_update (route_request_table_ptr);
		
		/* Set the new entry flag	*/
		new_entry = OPC_TRUE;
		}
	
	/* Schedule the next route request if there	*/
	/* is no route reply within that period		*/
	if (new_entry)
		{
		if (ttl_value == 1)
			{
			/* This is a non-propagating request	*/
			/* Set the timer to the non propagating	*/
			/* request timer.						*/
			route_request_orig_entry->current_request_period = route_request_table_ptr->non_prop_request_period;
			}
		else
			{
			/* This is a propagating request.		*/
			/* This is a new entry. Initialize the	*/
			/* request period to the initial value	*/
			route_request_orig_entry->current_request_period = route_request_table_ptr->init_request_period;
			}
		}
	else
		{
		/****** THIS IS AN EXISTING ENTRY *******/
		/* Make sure the previous timer is 		*/
		/* either the current event or has 		*/
		/* already passed. If not, cancel it	*/
		if (op_ev_valid (route_request_orig_entry->retry_timer) && op_ev_pending (route_request_orig_entry->retry_timer))
			op_ev_cancel (route_request_orig_entry->retry_timer);
		
		if (route_request_orig_entry->current_request_period == route_request_table_ptr->non_prop_request_period)
			{
			/* The previous route request for this destination	*/
			/* was a non propagating request. Set the new 		*/
			/* request to the initial request period			*/
			route_request_orig_entry->current_request_period = route_request_table_ptr->init_request_period;
			}
		else
			{
			/* This is an existing entry which was	*/
			/* a propagating request the previous 	*/
			/* attempt. Implement the exponential  	*/
			/* backoff algorithm by doubling the  	*/
			/* request period for every timeout		*/
			
			/* Do exponential backoff on the time	*/
			/* interval between this and last RREQ	*/			
			backoff_period = 2 * (op_sim_time () - route_request_orig_entry->last_route_req_time);
			route_request_orig_entry->current_request_period = backoff_period;
			}
		}
	
	/* Schedule the expiry timer to recreate a	*/
	/* new route request if needed				*/
	copy_dest_address_ptr = inet_address_create_dynamic (dest_address);
	route_request_orig_entry->retry_timer = op_intrpt_schedule_call (
		op_sim_time () + route_request_orig_entry->current_request_period, DSRC_ROUTE_REQUEST_TIMER,
											dsr_rte_route_request_expiry_handle, copy_dest_address_ptr);
	
	route_request_orig_entry->last_route_req_time = op_sim_time ();
	route_request_orig_entry->ttl = ttl_value;
	route_request_orig_entry->num_route_discoveries++;
	
	FOUT;
	}


void
dsr_route_request_originating_table_entry_delete (DsrT_Route_Request_Table* route_request_table_ptr, InetT_Address dest_address)
	{
	DsrT_Request_Orig_Entry*		route_request_orig_entry = OPC_NIL;
	char							dest_str [INETC_ADDR_STR_LEN];
	
	/** Deletes an entry from the originating request table	**/
	FIN (dsr_route_request_originating_table_entry_delete (<args>));
	
	/* Get the destination address as a */
	/* string to index the hash table	*/
	inet_address_print (dest_str, dest_address);
	
	/* Check if there exists an entry for this destination	*/
	/* node in the route request table						*/
	route_request_orig_entry = (DsrT_Request_Orig_Entry*) prg_string_hash_table_item_remove 
													(route_request_table_ptr->route_request_send_table, dest_str);
	
	/* Decrement the table size	*/
	route_request_table_ptr->current_table_size--;
	
	/* Update the request table size statistic	*/
	dsr_request_table_size_stat_update (route_request_table_ptr);
	
	/* Free the entry	*/
	dsr_route_request_orig_entry_mem_free (route_request_orig_entry);
	
	FOUT;
	}


Boolean
dsr_route_request_next_request_schedule_possible (DsrT_Route_Request_Table* route_request_table_ptr, 
													InetT_Address dest_address)
	{
	DsrT_Request_Orig_Entry*		route_request_orig_entry = OPC_NIL;
	char							dest_str [INETC_ADDR_STR_LEN];
	
	/** Determines whether to schedule another route request	**/
	/** for the node to the same destination					**/
	FIN (dsr_route_request_next_request_schedule_possible (<args>));
	
	/* Get the destination address as a */
	/* string to index the hash table	*/
	inet_address_print (dest_str, dest_address);
	
	/* Check if there exists an entry for this destination	*/
	/* node in the route request table						*/
	route_request_orig_entry = (DsrT_Request_Orig_Entry*) prg_string_hash_table_item_get 
													(route_request_table_ptr->route_request_send_table, dest_str);
	
	if (route_request_orig_entry->num_route_discoveries >= route_request_table_ptr->max_request_retransmissions)
		{
		/* Maximum number of retransmissions have been reached	*/
		/* No more route requests can be scheduled for this	node*/
		FRET (OPC_FALSE);
		}
	
	/* The next request attempt will double the request timeout	*/
	/* Check if this new doubled request timeout is within the	*/
	/* maximum request period requirements.						*/
	if ((route_request_orig_entry->current_request_period * 2) >= route_request_table_ptr->max_request_period)
		{
		/* The maximum request period has been reached	*/
		/* No more route requests can be scheduled for	*/
		/* this	node									*/
		FRET (OPC_FALSE);
		}
	
	FRET (OPC_TRUE);
	}


void
dsr_route_request_forwarding_table_entry_insert (DsrT_Route_Request_Table* route_request_table_ptr, InetT_Address src_address, 
											InetT_Address dest_address, int request_id)
	{
	PrgT_String_Hash_Table*			src_requests_forward_table = OPC_NIL;
	DsrT_Request_Forward_Entry*		route_request_forward_entry = OPC_NIL;
	char							source_str [INETC_ADDR_STR_LEN];
	char							req_id_str [INETC_ADDR_STR_LEN];
	void*							old_contents_ptr = OPC_NIL;
	
	/** Inserts a route request received from another node	**/
	/** that is to be forwarded into the route request		**/
	/** forward table.										**/
	FIN (dsr_route_request_forwarding_table_entry_insert (<args>));
	
	/* Get the source address as a 		*/
	/* string to index the hash table	*/
	inet_address_print (source_str, src_address);
	
	/* Check if there exists an entry for this source	*/
	/* node in the route request table					*/
	src_requests_forward_table = (PrgT_String_Hash_Table*) prg_string_hash_table_item_get 
													(route_request_table_ptr->route_request_forward_table, source_str);
	
	if (src_requests_forward_table == OPC_NIL)
		{
		/* There does not exist an entry for this source		*/
		/* Check if there is space in the route request table	*/
		/* If the table is full, delete the entry which is 		*/
		/* least recently used (LRU)							*/
		dsr_route_request_table_size_check (route_request_table_ptr);
		
		/* Create the table of route requests received from this source	*/
		src_requests_forward_table = prg_string_hash_table_create (10, 16);
		
		/* Insert the new table for that source into	*/
		/* the forwarded requests table					*/
		prg_string_hash_table_item_insert (route_request_table_ptr->route_request_forward_table, source_str, 
								src_requests_forward_table, &old_contents_ptr);
		
		/* Increment the size of the request table	*/
		route_request_table_ptr->current_table_size++;
		
		/* Update the request table size statistic	*/
		dsr_request_table_size_stat_update (route_request_table_ptr);
		}
	
	/* Get the request ID string	*/
	sprintf (req_id_str, "%d", request_id);
	
	/* Check if there already exists an entry for	*/
	/* the request ID from this source				*/
	route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_get 
													(src_requests_forward_table, req_id_str);
	
	if (route_request_forward_entry != OPC_NIL)
		{
		/* There already exists a duplicate entry 	*/
		/* for this request ID from this source		*/
		/* Do not insert this entry					*/
		FOUT;
		}
	
	/* Check if there is space to insert the entry	*/
	/* Delete the least recently used entry if 		*/
	/* there is no space available					*/
	dsr_route_request_max_request_entries_check (src_requests_forward_table, route_request_table_ptr->max_request_table_entries);
	
	/* Allocate memory for the forwarded route request entry	*/
	route_request_forward_entry = dsr_route_request_forward_entry_mem_alloc ();
	route_request_forward_entry->target_address = inet_address_copy (dest_address);
	route_request_forward_entry->route_request_id = request_id;
	route_request_forward_entry->last_used_time = op_sim_time ();
	
	/* Insert this entry in the route request forward table	*/
	prg_string_hash_table_item_insert (src_requests_forward_table, req_id_str, route_request_forward_entry, &old_contents_ptr);
	
	FOUT;
	}
	

Boolean
dsr_route_request_forwarding_table_entry_exists (DsrT_Route_Request_Table* route_request_table_ptr, 
											InetT_Address src_address, int request_id)
	{
	PrgT_String_Hash_Table*			src_requests_forward_table = OPC_NIL;
	DsrT_Request_Forward_Entry*		route_request_forward_entry = OPC_NIL;
	char							source_str [INETC_ADDR_STR_LEN];
	char							req_id_str [INETC_ADDR_STR_LEN];
	
	/** Determines if an entry exists in the	**/
	/** forwarding request table for a request	**/
	/** ID from a source						**/
	FIN (dsr_route_request_forwarding_table_entry_exists (<args>));
	
	/* Get the source address as a		*/
	/* string to index the hash table	*/
	inet_address_print (source_str, src_address);
	
	/* Check if there exists an entry for this source	*/
	/* node in the route request table					*/
	src_requests_forward_table = (PrgT_String_Hash_Table*) prg_string_hash_table_item_get 
													(route_request_table_ptr->route_request_forward_table, source_str);
	
	if (src_requests_forward_table == OPC_NIL)
		{
		/* There does not exist an entry for this source	*/
		FRET (OPC_FALSE);
		}
	
	/* Get the request ID string	*/
	sprintf (req_id_str, "%d", request_id);
	
	/* Check if there exists an entry for this	*/
	/* request ID from this source				*/
	route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_get 
													(src_requests_forward_table, req_id_str);
	
	if (route_request_forward_entry == OPC_NIL)
		{
		/* There does not exist an entry for this	*/
		/* request ID from this source				*/
		FRET (OPC_FALSE);
		}
	
	/* Update the last used time	*/
	route_request_forward_entry->last_used_time = op_sim_time ();
	
	/* There does exist an entry	*/
	FRET (OPC_TRUE);
	}
	

static void
dsr_route_request_table_size_check (DsrT_Route_Request_Table* route_request_table_ptr)
	{
	List*							keys_lptr = OPC_NIL;
	int								num_sources, num_requests;
	int								count, size;
	List*							request_keys_lptr = OPC_NIL;
	PrgT_String_Hash_Table*			src_requests_forward_table = OPC_NIL;
	char*							key_str = OPC_NIL;
	char*							req_id_str = OPC_NIL;
	DsrT_Request_Forward_Entry*		route_request_forward_entry = OPC_NIL;
	double							most_recently_used_time = 0.0;
	char*							most_recently_used_source_key = OPC_NIL;
	double							least_recently_used_time = OPC_DBL_INFINITY;
	char*							least_recently_used_source_key = OPC_NIL;
	
	/** Checks the size of the route request table	**/
	/** and deletes the least recently used entry	**/
	/** if the table is full						**/
	FIN (dsr_route_request_table_size_check (<void>));
	
	if (route_request_table_ptr->current_table_size < route_request_table_ptr->max_table_size)
		{
		/* There is space in the request table	*/
		FOUT;
		}
	
	/* There is no more space in the route request table	*/
	/* Delete the least recently used entry from the 		*/
	/* forwarding request table. No entries are deleted		*/
	/* from the originating request table since they are 	*/
	/* needed for retranmission. When the entries in the	*/
	/* originating request table are not needed anymore,	*/
	/* they are deleted										*/
		
	/* Determine the least recently used entry in the 		*/
	/* forwarding request table								*/
	keys_lptr = prg_string_hash_table_keys_get (route_request_table_ptr->route_request_forward_table);
	
	/* Determine the number of nodes	*/
	num_sources = op_prg_list_size (keys_lptr);
		
	/* Determine the least recently used entry by	*/
	/* going through each node and each request		*/
	/* The least recently used entry would be the	*/
	/* min(max (recently_used))						*/
	for (count = 0; count < num_sources; count++)
		{
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Get each node entry table	*/
		src_requests_forward_table = (PrgT_String_Hash_Table*) prg_string_hash_table_item_get 
											(route_request_table_ptr->route_request_forward_table, key_str);
		
		if (src_requests_forward_table == OPC_NIL)
			continue;
		
		/* Get the list of keys	*/
		request_keys_lptr = prg_string_hash_table_keys_get (src_requests_forward_table);
			
		/* Get the number of requests for each node	*/
		num_requests = op_prg_list_size (request_keys_lptr);
		
		for (size = 0; size < num_requests; size++)
			{
			req_id_str = (char*) op_prg_list_access (request_keys_lptr, size);
			
			/* Get each request entry	*/
			route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_get 
											(src_requests_forward_table, req_id_str);
			
			if (route_request_forward_entry == OPC_NIL)
				continue;
			
			/* The get MAX(recently_used)	*/
			if (route_request_forward_entry->last_used_time > most_recently_used_time)
				{
				most_recently_used_time = route_request_forward_entry->last_used_time;
				most_recently_used_source_key = key_str;
				}
			}
		
		/* Get the MIN (MAX(recently_used))	*/
		if (most_recently_used_time < least_recently_used_time)
			{
			least_recently_used_time = most_recently_used_time;
			least_recently_used_source_key = most_recently_used_source_key;
			}
		
		/* Free the keys list	*/
		op_prg_list_free (request_keys_lptr);
		op_prg_mem_free (request_keys_lptr);
		}
	
	/* Delete the entry which is the least recently used	*/
	src_requests_forward_table = (PrgT_String_Hash_Table*) prg_string_hash_table_item_remove 
										(route_request_table_ptr->route_request_forward_table, least_recently_used_source_key);
	
	/* Get the number of keys	*/
	request_keys_lptr = prg_string_hash_table_keys_get (src_requests_forward_table);
	
	/* Get the number of requests for this node	*/
	num_requests = op_prg_list_size (request_keys_lptr);
	
	for (count = 0; count < num_requests; count++)
		{
		/* Get each key	*/
		req_id_str = (char*) op_prg_list_access (request_keys_lptr, count);
		
		/* Remove the request entry	*/
		route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_remove 
																	(src_requests_forward_table, req_id_str);
		
		/* Free the request entry	*/
		dsr_route_request_forward_entry_mem_free (route_request_forward_entry);
		}
	
	/* Free the keys list	*/
	op_prg_list_free (request_keys_lptr);
	op_prg_mem_free (request_keys_lptr);
	
	/* Free the source hash table	*/
	prg_string_hash_table_free (src_requests_forward_table);
	
	/* Decrement the size of the table	*/
	route_request_table_ptr->current_table_size--;
	
	/* Update the request table size statistic	*/
	dsr_request_table_size_stat_update (route_request_table_ptr);
	
	/* Free the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);

	FOUT;
	}


static void
dsr_route_request_max_request_entries_check (PrgT_String_Hash_Table* src_requests_forward_table, int max_request_table_entries)
	{
	DsrT_Request_Forward_Entry*		route_request_forward_entry = OPC_NIL;
	List*							keys_lptr = OPC_NIL;
	int								num_entries, count;
	char*							key_str = OPC_NIL;
	double							least_recently_used_time = OPC_DBL_INFINITY;
	char*							least_recently_used_request_id_str = OPC_NIL;
	
	/** Checks if the maximum number of request	**/
	/** identifiers have been reached from a	**/
	/** single source. If so, it deletes the	**/
	/** least recently used request entry		**/
	FIN (dsr_route_request_max_request_entries_check (<args>));
	
	/* Get the number of keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (src_requests_forward_table);
	num_entries = op_prg_list_size (keys_lptr);
	
	/* Determine is the maximum number of request	*/
	/* identifiers have been reached				*/
	if (num_entries < max_request_table_entries)
		{
		/* Free the keys list	*/
		op_prg_list_free (keys_lptr);
		op_prg_mem_free (keys_lptr);
		
		/* The maximum entries have not been reached	*/
		FOUT;
		}
	
	/* The maximum number of entries have been reached	*/
	/* Delete the least recently used entry				*/
	for (count = 0; count < num_entries; count++)
		{
		/* Get each entry	*/
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Get the request entry	*/
		route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_get 
											(src_requests_forward_table, key_str);
		
		/* Determine the least recently used entry	*/
		if (route_request_forward_entry->last_used_time < least_recently_used_time)
			{
			least_recently_used_time = route_request_forward_entry->last_used_time;
			least_recently_used_request_id_str = key_str;
			}
		}
	
	/* Remove the least recently used request ID	*/
	route_request_forward_entry  = (DsrT_Request_Forward_Entry*) prg_string_hash_table_item_remove
											(src_requests_forward_table, least_recently_used_request_id_str);
	
	/* Free the request entry	*/
	dsr_route_request_forward_entry_mem_free (route_request_forward_entry);
	
	/* Free the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


static void
dsr_request_table_size_stat_update (DsrT_Route_Request_Table* route_request_table_ptr)
	{
	/** Updates the request table size statistic	**/
	FIN (dsr_request_table_size_stat_update (<args>));
	
	/* Update the size of the route request table statistic	*/
	op_stat_write (route_request_table_ptr->dsr_stat_ptr->request_table_size_shandle, route_request_table_ptr->current_table_size);
	
	FOUT;
	}
	

static DsrT_Route_Request_Table*
dsr_request_table_mem_alloc (void)
	{
	DsrT_Route_Request_Table* 	route_request_table_ptr = OPC_NIL;
	
	/** Allocates memory for the send	**/
	/** and forward route request table	**/
	FIN (dsr_request_table_mem_alloc (void));
	
	route_request_table_ptr = (DsrT_Route_Request_Table*) op_prg_mem_alloc (sizeof (DsrT_Route_Request_Table));
	route_request_table_ptr->route_request_send_table = prg_string_hash_table_create (10, 10);
	route_request_table_ptr->route_request_forward_table = prg_string_hash_table_create (10, 10);
	
	FRET (route_request_table_ptr);
	}


static DsrT_Request_Orig_Entry*
dsr_route_request_orig_entry_mem_alloc (void)
	{
	static Pmohandle				route_request_orig_entry_pmh;
	DsrT_Request_Orig_Entry*		route_request_orig_entry_ptr = OPC_NIL;
	static Boolean					route_request_orig_entry_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route	**/
	/** request originating entry				**/
	FIN (dsr_route_request_orig_entry_mem_alloc (void));
	
	if (route_request_orig_entry_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route request	*/
		/* option in the DSR packets if not already defined	*/
		route_request_orig_entry_pmh = op_prg_pmo_define ("Route Request Originating Entry", sizeof (DsrT_Request_Orig_Entry), 32);
		route_request_orig_entry_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route request options from the pooled memory	*/
	route_request_orig_entry_ptr = (DsrT_Request_Orig_Entry*) op_prg_pmo_alloc (route_request_orig_entry_pmh);
	route_request_orig_entry_ptr->num_route_discoveries = 0;
	route_request_orig_entry_ptr->last_route_req_time = 0.0;
	
	FRET (route_request_orig_entry_ptr);
	}


static void
dsr_route_request_orig_entry_mem_free (DsrT_Request_Orig_Entry* route_request_orig_entry)
	{
	/** Frees the route request originating entry	**/
	FIN (dsr_route_request_orig_entry_mem_free (<args>));
	
	/* Cancel the retry timer	*/
	if (op_ev_valid (route_request_orig_entry->retry_timer) && op_ev_pending (route_request_orig_entry->retry_timer))
		op_ev_cancel (route_request_orig_entry->retry_timer);
	
	/* Free the rout request originating entry	*/
	op_prg_mem_free (route_request_orig_entry);
	
	FOUT;
	}


static DsrT_Request_Forward_Entry*
dsr_route_request_forward_entry_mem_alloc (void)
	{
	static Pmohandle				route_request_forward_entry_pmh;
	DsrT_Request_Forward_Entry*		route_request_forward_entry_ptr = OPC_NIL;
	static Boolean					route_request_forward_entry_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route	**/
	/** request forward entry					**/
	FIN (dsr_route_request_forward_entry_mem_alloc (void));
	
	if (route_request_forward_entry_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route request	*/
		/* option in the DSR packets if not already defined	*/
		route_request_forward_entry_pmh = op_prg_pmo_define ("Route Request Forward Entry", sizeof (DsrT_Request_Forward_Entry), 32);
		route_request_forward_entry_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route request options from the pooled memory	*/
	route_request_forward_entry_ptr = (DsrT_Request_Forward_Entry*) op_prg_pmo_alloc (route_request_forward_entry_pmh);
	route_request_forward_entry_ptr->route_request_id = OPC_INT_UNDEF;
	route_request_forward_entry_ptr->last_used_time = OPC_DBL_INFINITY;
	
	FRET (route_request_forward_entry_ptr);
	}


static void
dsr_route_request_forward_entry_mem_free (DsrT_Request_Forward_Entry* route_request_forward_entry)
	{
	/** Frees the route request forward entry	**/
	FIN (dsr_route_request_forward_entry_mem_free (<args>));
	
	/* Free the route request entry	*/
	op_prg_mem_free (route_request_forward_entry);
	
	FOUT;
	}


/**********************************************************/
/*********** "GRATUITOUS" ROUTE REPLY TABLE ***************/
/**********************************************************/

DsrT_Route_Reply_Table*
dsr_grat_reply_table_create (double grat_reply_holdoff_time)
	{
	DsrT_Route_Reply_Table*		route_reply_ptr = OPC_NIL;
	
	/** Creates the gratuitous route reply table	**/
	FIN (dsr_grat_reply_table_create (<args>));
	
	route_reply_ptr = dsr_grat_reply_table_mem_alloc ();
	route_reply_ptr->grat_reply_holdoff_time = grat_reply_holdoff_time;
	
	FRET (route_reply_ptr);
	}


void
dsr_grat_reply_table_insert (DsrT_Route_Reply_Table* route_reply_ptr, 
						InetT_Address dest_address, InetT_Address overheard_node_address)
	{
	char						dest_str [INETC_ADDR_STR_LEN];
	List*						node_lptr = OPC_NIL;
	DsrT_Route_Reply_Entry*		route_reply_entry_ptr = OPC_NIL;
	void*						old_contents_ptr = OPC_NIL;
	
	/** Inserts an entry into the gratuitous	**/
	/** route reply table for a specific 		**/
	/** destination address						**/
	FIN (dsr_grat_reply_table_insert (<args>));
	
	/* Delete all expired entries	*/
	dsr_expired_grat_reply_entries_delete (route_reply_ptr);
	
	/* Get the destination address as a */
	/* string to index the hash table	*/
	inet_address_print (dest_str, dest_address);
	
	/* Check if there already exists an entry for the destination	*/
	node_lptr = (List*) prg_string_hash_table_item_get (route_reply_ptr->grat_route_reply_table, dest_str);
	
	if (route_reply_entry_ptr == OPC_NIL)
		{
		/* There does not exist an entry for this destination	*/
		/* Create a list to store all entries from different	*/
		/* overheard nodes to the same destination				*/
		node_lptr = op_prg_list_create ();
		
		/* Set the list in the gratuitous route reply table	*/
		prg_string_hash_table_item_insert (route_reply_ptr->grat_route_reply_table, dest_str, node_lptr, &old_contents_ptr);
		}
	
	/* Create the memory to set overheard node address	*/
	route_reply_entry_ptr = dsr_grat_reply_entry_mem_alloc ();
	route_reply_entry_ptr->overheard_node_address = inet_address_copy (overheard_node_address);
	route_reply_entry_ptr->insert_time = op_sim_time ();
	
	/* Insert this entry into the list	*/
	op_prg_list_insert (node_lptr, route_reply_entry_ptr, OPC_LISTPOS_TAIL);
	
	FOUT;
	}


Boolean
dsr_grat_reply_entry_exists (DsrT_Route_Reply_Table* route_reply_ptr, InetT_Address dest_address, InetT_Address overheard_address)
	{
	char						dest_str [INETC_ADDR_STR_LEN];
	List*						node_lptr = OPC_NIL;
	int							count, num_entries;
	DsrT_Route_Reply_Entry*		route_reply_entry_ptr = OPC_NIL;
	
	/** Checks if there exists an entry for the	**/
	/** specified destination address in the	**/
	/** gratuitous route reply table			**/
	FIN (dsr_grat_reply_entry_exists (<args>));
	
	/* Delete all expired entries	*/
	dsr_expired_grat_reply_entries_delete (route_reply_ptr);
	
	/* Get the destination address as a */
	/* string to index the hash table	*/
	inet_address_print (dest_str, dest_address);
	
	/* Check if there already exists an entry for the destination	*/
	node_lptr = (List*) prg_string_hash_table_item_get (route_reply_ptr->grat_route_reply_table, dest_str);
	
	if (node_lptr == OPC_NIL)
		{
		/* No entry exists for the destination	*/
		FRET (OPC_FALSE);
		}
	
	/* Get the number of overheard entries	*/
	num_entries = op_prg_list_size (node_lptr);
	
	for (count = 0; count < num_entries; count++)
		{
		/* Get each element in the list and	*/
		/* check if the entry exists		*/
		route_reply_entry_ptr = (DsrT_Route_Reply_Entry*) op_prg_list_access (node_lptr, count);
		
		if (inet_address_equal (route_reply_entry_ptr->overheard_node_address, overheard_address) == OPC_TRUE)
			{
			/* A match is found. An entry exists	*/
			FRET (OPC_TRUE);
			}
		}
	
    FRET (OPC_FALSE);
	}


static void
dsr_expired_grat_reply_entries_delete (DsrT_Route_Reply_Table* route_reply_ptr)
	{
	double						current_time;
	int							num_entries;
	int							count;
	int							num_nodes, size;
	char*						key_str = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
	List*						node_lptr = OPC_NIL;
	DsrT_Route_Reply_Entry*		route_reply_entry_ptr = OPC_NIL;
	
	/** Deletes all expired entries from the	**/
	/** gratuitous route reply table			**/
	FIN (dsr_expired_grat_reply_entries_delete (<args>));
	
	/* Get the current time	*/
	current_time = op_sim_time ();
	
	/* Get the list of entries in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (route_reply_ptr->grat_route_reply_table);
	
	/* Get the number of entries	*/
	num_entries = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_entries; count ++)
		{
		/* Get each key and access the value from the	*/
		/* gratuitous route reply table					*/
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		node_lptr = (List*) prg_string_hash_table_item_get (route_reply_ptr->grat_route_reply_table, key_str);
		
		num_nodes = op_prg_list_size (node_lptr);
		
		for (size = 0; size < num_nodes; size++)
			{
			route_reply_entry_ptr = (DsrT_Route_Reply_Entry*) op_prg_list_access (node_lptr, size);
		
			/* Check if the entry has expired	*/
			if ((current_time - route_reply_entry_ptr->insert_time) >= route_reply_ptr->grat_reply_holdoff_time)
				{
				/* This entry has expired. Remove the	*/
				/* entry from the hash table			*/
				route_reply_entry_ptr = (DsrT_Route_Reply_Entry*) op_prg_list_remove (node_lptr, size);;
			
				/* Free the memory for this entry	*/
				dsr_grat_reply_entry_mem_free (route_reply_entry_ptr);
				
				size--;
				num_nodes--;
				}
			}
		
		if (op_prg_list_size (node_lptr) == 0)
			{
			/* No more entries for this destination	*/
			/* Remove it from the table				*/
			node_lptr = (List*) prg_string_hash_table_item_remove (route_reply_ptr->grat_route_reply_table, key_str);
			op_prg_mem_free (node_lptr);
			}
		}
	
	/* Free the key list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}

static DsrT_Route_Reply_Table*
dsr_grat_reply_table_mem_alloc (void)
	{
	DsrT_Route_Reply_Table*		route_reply_table_ptr = OPC_NIL;
	
	/** Allocates memory for the gratuitous route reply table	**/
	FIN (dsr_grat_reply_table_mem_alloc (void));
	
	route_reply_table_ptr = (DsrT_Route_Reply_Table*) op_prg_mem_alloc (sizeof (DsrT_Route_Reply_Table));
	route_reply_table_ptr->grat_route_reply_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	route_reply_table_ptr->grat_reply_holdoff_time = 0.0;
	
	FRET (route_reply_table_ptr);
	}


static DsrT_Route_Reply_Entry*
dsr_grat_reply_entry_mem_alloc (void)
	{
	DsrT_Route_Reply_Entry*			route_reply_entry_ptr = OPC_NIL;
	
	/** Allocates memory for an entry in the	**/
	/** gratuitous route reply table			**/
	FIN (dsr_grat_reply_entry_mem_alloc (void));
	
	route_reply_entry_ptr = (DsrT_Route_Reply_Entry*) op_prg_mem_alloc (sizeof (DsrT_Route_Reply_Entry));
	route_reply_entry_ptr->insert_time = 0.0;
	
	FRET (route_reply_entry_ptr);
	}


static void
dsr_grat_reply_entry_mem_free (DsrT_Route_Reply_Entry* route_reply_entry_ptr)
	{
	/** Frees the memory allocated to an entry	**/
	/** in the gratuitous route reply table		**/
	FIN (dsr_grat_reply_entry_mem_free (<args>));
	
	inet_address_destroy (route_reply_entry_ptr->overheard_node_address);
	op_prg_mem_free (route_reply_entry_ptr);
	
	FOUT;
	}
