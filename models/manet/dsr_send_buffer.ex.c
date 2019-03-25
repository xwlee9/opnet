/* dsr_send_buffer.ex.c */
/* C support file for the send buffer	*/


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
static DsrT_Packet_Entry*		dsr_send_buffer_pkt_entry_create (Packet*);
static void						dsr_send_buffer_all_expired_pkts_delete (DsrT_Send_Buffer*);
static Boolean					dsr_send_buffer_size_exceeded (DsrT_Send_Buffer*);
static Compcode					dsr_send_buffer_overflow_handle (DsrT_Send_Buffer*);
static void						dsr_send_buffer_size_stat_update (DsrT_Send_Buffer*);
static void						dsr_send_buffer_route_discovery_time_stat_update (DsrT_Send_Buffer*, List*);
static DsrT_Send_Buffer*		dsr_send_buffer_mem_alloc (void);
static DsrT_Packet_Entry*		dsr_send_buffer_pkt_entry_mem_alloc (void);
static void						dsr_send_buffer_pkt_entry_mem_free (DsrT_Packet_Entry*);


DsrT_Send_Buffer*
dsr_send_buffer_create (int max_buffer_size, double expiry_time, DsrT_Stathandles* stat_handle_ptr,
						DsrT_Global_Stathandles* global_stat_handle_ptr)
	{
	DsrT_Send_Buffer*		send_buffer_ptr = OPC_NIL;
	
	/** Creates the send buffer to enqueue packets	**/
	FIN (dsr_send_buffer_create (<args>));
	
	/* Allocate memory and creates the send buffer	*/
	send_buffer_ptr = dsr_send_buffer_mem_alloc ();
	
	/* Set the maximum size of the send buffer	*/
	send_buffer_ptr->max_buffer_size = max_buffer_size;
	
	/* Set the time for packet expiry in this buffer	*/
	send_buffer_ptr->pkt_expiry_time = expiry_time;
	
	/* Set the statistic handles	*/
	send_buffer_ptr->dsr_stat_ptr = stat_handle_ptr;
	send_buffer_ptr->global_dsr_stat_ptr = 	global_stat_handle_ptr;
	
	FRET (send_buffer_ptr);
	}


Compcode
dsr_send_buffer_packet_enqueue (DsrT_Send_Buffer* send_buffer_ptr, Packet* pkptr, InetT_Address dest_address)
	{
	List*					packet_queue_lptr = OPC_NIL;
	DsrT_Packet_Entry*		dsr_pkt_entry_ptr = OPC_NIL;
	char					dest_addr_str [INETC_ADDR_STR_LEN];
	void*					old_contents_ptr = OPC_NIL;
	
	/** Enqueues a packet in the send buffer	**/
	FIN (dsr_send_buffer_packet_enqueue (<args>));
	
	/* Get the destination address as a 	*/
	/* string to index the hash table		*/
	inet_address_print (dest_addr_str, dest_address);
	
	/* Delete all packets that have expired	*/
	dsr_send_buffer_all_expired_pkts_delete (send_buffer_ptr);
	
	/* Check if there already exists packets waiting	*/
	/* to be sent to the destination of this packet		*/
	packet_queue_lptr = (List*) prg_string_hash_table_item_get (send_buffer_ptr->send_buffer_table, dest_addr_str);
	
	if (packet_queue_lptr == OPC_NIL)
		{
		/* No packets exist to the destination	*/
		/* Create a packet queue and insert the	*/
		/* packet into the queue				*/
		packet_queue_lptr = op_prg_list_create ();
		
		/* Set the packet queue in the send	buffer table */
		prg_string_hash_table_item_insert (send_buffer_ptr->send_buffer_table, dest_addr_str, packet_queue_lptr, &old_contents_ptr);
		}
	
	/* Check if there is any space in the send buffer	*/
	/* to enqueue the packet							*/
	if (dsr_send_buffer_size_exceeded (send_buffer_ptr) == OPC_TRUE)
		{
		/* There is no space in the send buffer	*/
		/* Based on the type of priority scheme	*/
		/* for packets to be discarded, this 	*/
		/* packet may or may not be enqueued.	*/
		/* Typically a FIFO scheme is used to 	*/
		/* discard packets from this buffer		*/
		if (dsr_send_buffer_overflow_handle (send_buffer_ptr) == OPC_COMPCODE_FAILURE)
			{
			FRET (OPC_COMPCODE_FAILURE);
			}
		}
	
	/* Enqueue the packet in the send buffer	*/
	dsr_pkt_entry_ptr = dsr_send_buffer_pkt_entry_create (pkptr);
	op_prg_list_insert (packet_queue_lptr, dsr_pkt_entry_ptr, OPC_LISTPOS_TAIL);
	
	/* Increment the size of the send buffer	*/
	send_buffer_ptr->current_buffer_size++;
	
	/* Update the size of the send buffer statistic	*/
	dsr_send_buffer_size_stat_update (send_buffer_ptr);

	/* The packet was inserted successfully	*/
	FRET (OPC_COMPCODE_SUCCESS);
	}
		

List*
dsr_send_buffer_pkt_list_get (DsrT_Send_Buffer* send_buffer_ptr, InetT_Address dest_address, Boolean remove)
	{
	DsrT_Packet_Entry*		pkt_entry_ptr = OPC_NIL;
	List*					pkt_queue_lptr = OPC_NIL;
	List*					pkt_lptr = OPC_NIL;
	char					dest_addr_str [INETC_ADDR_STR_LEN];
	int						num_packets, count;
	int						num_pkts_deleted;
	
	/** Returns the list of of packets enqueued for	**/
	/** a particular destination. If the remove 	**/
	/** flag is set, all the packets to that 		**/
	/** destination will be removed from the buffer	**/
	/** An OPC_NIL is returned if no packets exist	**/
	FIN (dsr_send_buffer_pkt_list_get (<args>));
	
	/* Get the destination address as a 	*/
	/* string to index the hash table		*/
	inet_address_print (dest_addr_str, dest_address);
	
	/* Delete all packets that have expired	*/
	dsr_send_buffer_all_expired_pkts_delete (send_buffer_ptr);
	
	/* Get the list of packets to the particular destination	*/
	if (remove)
		{
		pkt_queue_lptr = (List*) prg_string_hash_table_item_remove (send_buffer_ptr->send_buffer_table, dest_addr_str);
		
		if (pkt_queue_lptr == OPC_NIL)
			FRET (pkt_queue_lptr);
		
		/* Update the route discovery time statistics	*/
		dsr_send_buffer_route_discovery_time_stat_update (send_buffer_ptr, pkt_queue_lptr);
		
		/* Get the number of packets removed	*/
		num_packets = op_prg_list_size (pkt_queue_lptr);
		num_pkts_deleted = num_packets;
		
		/* Create a list to return the packets	*/
		pkt_lptr = op_prg_list_create ();
		
		while (num_packets > 0)
			{
			/* Get each packet	*/
			pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_remove (pkt_queue_lptr, OPC_LISTPOS_HEAD);
			
			/* Place each packet in the queue	*/
			op_prg_list_insert (pkt_lptr, pkt_entry_ptr->pkptr, OPC_LISTPOS_TAIL);
			
			/* Free each entry	*/
			op_prg_mem_free (pkt_entry_ptr);
			
			num_packets--;
			}			
		
		/* Free the list	*/
		op_prg_mem_free (pkt_queue_lptr);
				
		/* Decrement the number of entries in the send buffer	*/
		send_buffer_ptr->current_buffer_size -= num_pkts_deleted;
		
		/* Update the size of the send buffer statistic	*/
		dsr_send_buffer_size_stat_update (send_buffer_ptr);
		}
	else
		{
		pkt_queue_lptr = (List*) prg_string_hash_table_item_get (send_buffer_ptr->send_buffer_table, dest_addr_str);
		
		if (pkt_queue_lptr == OPC_NIL)
			FRET (pkt_queue_lptr);
		
		/* Get the number of packets accessed	*/
		num_packets = op_prg_list_size (pkt_queue_lptr);
		
		/* Create a list to return the packets	*/
		pkt_lptr = op_prg_list_create ();
		
		for (count = 0; count < num_packets; count++)
			{
			/* Get each packet	*/
			pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_access (pkt_queue_lptr, count);
			
			/* Place each packet in the queue	*/
			op_prg_list_insert (pkt_lptr, pkt_entry_ptr->pkptr, OPC_LISTPOS_TAIL);
			}			
		}
	
	FRET (pkt_lptr);
	}


static DsrT_Packet_Entry*
dsr_send_buffer_pkt_entry_create (Packet* pkptr)
	{
	DsrT_Packet_Entry*			pkt_entry_ptr = OPC_NIL;
	
	/** Create an entry for a packet to a specific	**/
	/** destination, setting its timout value		**/
	FIN (dsr_send_buffer_pkt_entry_create (<args>));
	
	/* Create the packet entry	*/
	pkt_entry_ptr = dsr_send_buffer_pkt_entry_mem_alloc ();
	pkt_entry_ptr->pkptr = pkptr;
	pkt_entry_ptr->insert_time = op_sim_time ();
	
	FRET (pkt_entry_ptr);
	}


static void
dsr_send_buffer_all_expired_pkts_delete (DsrT_Send_Buffer* send_buffer_ptr)
	{
	double					current_time;
	double					update_time;
	int						num_dest, num_pkts;
	int						count, size;
	char*					key_ptr = OPC_NIL;
	List*					keys_lptr = OPC_NIL;
	List*					pkt_queue_lptr = OPC_NIL;
	DsrT_Packet_Entry*		pkt_entry_ptr = OPC_NIL;
	
	/** Deletes all packets that have expired in the send buffer	**/
	/** This function should be called every time a packet is		**/
	/** either inserted or accessed									**/
	FIN (dsr_send_buffer_all_expired_pkts_delete (<args>));
	
	/* Get the current time	*/
	current_time = op_sim_time ();
	
	/* Get all the keys in the hash table. This represents	*/
	/* the number of destinations that packets need to be	*/
	/* sent out to											*/
	keys_lptr = prg_string_hash_table_keys_get (send_buffer_ptr->send_buffer_table);
	
	/* Get the size of the keys	*/
	num_dest = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_dest; count++)
		{
		/* Initialize the variables	*/
		size = 0;
		
		/* Access the destination key	*/
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Access the packet queue for that destination	*/
		pkt_queue_lptr = (List*) prg_string_hash_table_item_get (send_buffer_ptr->send_buffer_table, key_ptr);
		
		/* Get the number of packets in the queue	*/
		num_pkts = op_prg_list_size (pkt_queue_lptr);
		
		while (size < num_pkts)
			{
			/* Get each packet entry */
			pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_access (pkt_queue_lptr, size);
		
			/* Determine if the entry has expired	*/
			if ((current_time - pkt_entry_ptr->insert_time) >= send_buffer_ptr->pkt_expiry_time)
				{
				/* Remove the packet from the queue	*/
				pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_remove (pkt_queue_lptr, size);
				
				/* Decrement the size of the send buffer	*/
				send_buffer_ptr->current_buffer_size--;
				
				/* Update the send buffer size statistic at the	*/
				/* time this packet was to be dequeued			*/
				update_time = pkt_entry_ptr->insert_time + send_buffer_ptr->pkt_expiry_time;
				op_stat_write_t (send_buffer_ptr->dsr_stat_ptr->send_buffer_size_shandle, 
									(double) send_buffer_ptr->current_buffer_size, update_time);
				
				/* Update the send buffer packets discarded statistic	*/
				/* at the time this packet was to be dequeued			*/
				op_stat_write_t (send_buffer_ptr->dsr_stat_ptr->num_pkts_discard_shandle, 1.0, update_time);
				op_stat_write_t (send_buffer_ptr->global_dsr_stat_ptr->num_pkts_discard_global_shandle, 1.0,
									update_time);
				
				/* The packet has expired. Delete the packet	*/
				dsr_send_buffer_pkt_entry_mem_free (pkt_entry_ptr);
				
				num_pkts--;
				
				continue;
				}
			
			size++;
			}
		
		if (op_prg_list_size (pkt_queue_lptr) == 0)
			{
			/* There are no more packets in the	*/
			/* queue to this destination. Free 	*/
			/* the memory for this queue		*/
			pkt_queue_lptr = (List*) prg_string_hash_table_item_remove (send_buffer_ptr->send_buffer_table, key_ptr);
			op_prg_mem_free (pkt_queue_lptr);
			}
		}
	
	/* Free the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


static Boolean
dsr_send_buffer_size_exceeded (DsrT_Send_Buffer* send_buffer_ptr)
	{
	/** Checks if there is space in the send buffer	**/
	/** to enqueue another packet					**/
	FIN (dsr_send_buffer_size_exceeded (<args>));

	if (send_buffer_ptr->max_buffer_size == -1)
		{
		/* The send buffer is of infinite size	*/
		FRET (OPC_FALSE);
		}
	
	if (send_buffer_ptr->current_buffer_size >= send_buffer_ptr->max_buffer_size)
		{
		/* There is no more space in the send buffer	*/
		FRET (OPC_TRUE);
		}
	else
		{
		/* There is space in the send buffer	*/
		FRET (OPC_FALSE);
		}
	}
	

static Compcode
dsr_send_buffer_overflow_handle (DsrT_Send_Buffer* send_buffer_ptr)
	{
	List*					keys_lptr = OPC_NIL;
	int						num_dest, count, num_pkts;
	char*					key_ptr = OPC_NIL;
	List*					pkt_queue_lptr = OPC_NIL;
	DsrT_Packet_Entry*		pkt_entry_ptr = OPC_NIL;
	double					first_insert_time = OPC_DBL_INFINITY;
	char*					first_dest_node_str = OPC_NIL;
	DsrT_Packet_Entry*		first_pkptr_entry_ptr = OPC_NIL;
	
	/** Determines whether to insert a packet into the	**/
	/** send buffer as the buffer is full. The packets	**/
	/** are discarded in a FIFO order to avoid buffer	**/
	/** overflow. If a packet is removed successfully	**/
	/** a success if returned							**/
	FIN (dsr_send_buffer_overflow_handle (<args>));
	
	/* If the size of the send buffer is zero	*/
	/* return failure to enqueue the packet		*/
	if (send_buffer_ptr->max_buffer_size == 0)
		FRET (OPC_COMPCODE_FAILURE);
	
	/* Get all the keys in the hash table. This represents	*/
	/* the number of destinations that packets need to be	*/
	/* sent out to											*/
	keys_lptr = prg_string_hash_table_keys_get (send_buffer_ptr->send_buffer_table);
	
	/* Get the size of the keys	*/
	num_dest = op_prg_list_size (keys_lptr);
	
	if (num_dest == 0)
		op_sim_end ("Invalid Condition : No entries in the send buffer although determined as overflowing", OPC_NIL, OPC_NIL, OPC_NIL);
	
	for (count = 0; count < num_dest; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Access the packet queue for that destination	*/
		pkt_queue_lptr = (List*) prg_string_hash_table_item_get (send_buffer_ptr->send_buffer_table, key_ptr);
		
		/* Get the number of packets in the queue	*/
		num_pkts = op_prg_list_size (pkt_queue_lptr);
		
		/* If there are no packets, go to the next destination	*/
		if (num_pkts == 0)
			continue;
		
		/* The packet at the head of the queue will		*/
		/* always be the oldest packet destined to that	*/
		/* particular destination						*/
		pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_access (pkt_queue_lptr, OPC_LISTPOS_HEAD);
		
		/* Check if the packet insert time is the oldest	*/
		if (pkt_entry_ptr->insert_time < first_insert_time)
			{
			first_insert_time = pkt_entry_ptr->insert_time;
			first_dest_node_str = key_ptr;
			}
		}
	
	/* Remove the oldest packet in the send buffer	*/
	/* Access the packet queue for that destination	*/
	pkt_queue_lptr = (List*) prg_string_hash_table_item_get (send_buffer_ptr->send_buffer_table, first_dest_node_str);
	first_pkptr_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_remove (pkt_queue_lptr, OPC_LISTPOS_HEAD);
	
	/* Update the send buffer packets discarded statistic	*/
	/* at the time this packet was to be dequeued			*/
	op_stat_write_t (send_buffer_ptr->dsr_stat_ptr->num_pkts_discard_shandle, 1.0, op_sim_time ());
	op_stat_write_t (send_buffer_ptr->global_dsr_stat_ptr->num_pkts_discard_global_shandle, 1.0,
									op_sim_time ());
	
	dsr_send_buffer_pkt_entry_mem_free (first_pkptr_entry_ptr);
	
	/* Decrement the size of the send buffer	*/
	send_buffer_ptr->current_buffer_size--;
	
	/* Destroy the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FRET (OPC_COMPCODE_SUCCESS);
	}


static void
dsr_send_buffer_size_stat_update (DsrT_Send_Buffer* send_buffer_ptr)
	{
	/** Updates the send buffer statistic	*/
	FIN (dsr_send_buffer_size_stat_update (<args>));
	
	/* Update the size of the send buffer local statistic	*/
	op_stat_write (send_buffer_ptr->dsr_stat_ptr->send_buffer_size_shandle, (double) send_buffer_ptr->current_buffer_size);
	
	/* Update the size of the send buffer global statistic	*/
	op_stat_write (send_buffer_ptr->dsr_stat_ptr->send_buffer_size_shandle, (double) send_buffer_ptr->current_buffer_size);
	
	FOUT;
	}


static void
dsr_send_buffer_route_discovery_time_stat_update (DsrT_Send_Buffer* send_buffer_ptr, List* pkt_queue_lptr)
	{
	int							num_pkts, count;
	DsrT_Packet_Entry*			pkt_entry_ptr = OPC_NIL;
	DsrT_Global_Stathandles*	global_stathandle_ptr = OPC_NIL;
	double						first_enqueue_time = OPC_DBL_INFINITY;
	double						route_discovery_time;
	
	/** Updates the statistic for the route discovery time	**/
	FIN (dsr_send_buffer_route_discovery_time_stat_update (<args>));
	
	/* Get the number of packets for the	*/
	/* destination whose route has just 	*/
	/* been discovered						*/
	num_pkts = op_prg_list_size (pkt_queue_lptr);
	
	for (count = 0; count < num_pkts; count++)
		{
		/* Access each packet information	*/
		/* and determine the first packet	*/
		/* that was enqueued.				*/
		pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_list_access (pkt_queue_lptr, count);
		
		if (pkt_entry_ptr->insert_time < first_enqueue_time)
			{
			/* This entry is earlier than 	*/
			/* any other so far				*/
			first_enqueue_time = pkt_entry_ptr->insert_time;
		    }
		}
	
	/* The route discovery time is the time	*/
	/* difference between the first packet 	*/
	/* enqueued to this destination and the	*/
	/* current simulation time when the 	*/
	/* route was discovered					*/
	route_discovery_time = op_sim_time () - first_enqueue_time;
	
	/* Get a handle to the global statistics	*/
	global_stathandle_ptr = dsr_support_global_stat_handles_obtain ();
	
	/* Update the route discovery time for	*/
	/* a specific destination				*/
	op_stat_write (send_buffer_ptr->dsr_stat_ptr->route_discovery_time_shandle, route_discovery_time);
	
	/* Update the global route discovery time statistic	*/
	op_stat_write (global_stathandle_ptr->route_discovery_time_global_shandle, route_discovery_time);
	
	FOUT;
	}


static DsrT_Send_Buffer*
dsr_send_buffer_mem_alloc (void)
	{
	DsrT_Send_Buffer*		send_buffer_ptr = OPC_NIL;
	
	/** Allocates memory for the send buffer	**/
	FIN (dsr_send_buffer_mem_alloc (void));
	
	send_buffer_ptr = (DsrT_Send_Buffer*) op_prg_mem_alloc (sizeof (DsrT_Send_Buffer));
	send_buffer_ptr->send_buffer_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	send_buffer_ptr->current_buffer_size = 0;
	send_buffer_ptr->max_buffer_size = 0;
	
	FRET (send_buffer_ptr);
	}


static DsrT_Packet_Entry*
dsr_send_buffer_pkt_entry_mem_alloc (void)
	{
	static Pmohandle		send_buffer_pkt_entry_pmh;
	DsrT_Packet_Entry*		pkt_entry_ptr = OPC_NIL;
	static Boolean			send_buffer_pkt_entry_pmh_defined = OPC_FALSE;
	
	/** Allocates memory for the packet entry	**/	
	FIN (dsr_send_buffer_pkt_entry_mem_alloc (void));
	
	if (send_buffer_pkt_entry_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for the packet	*/
		/* entry in the send buffer if not already done	*/
		send_buffer_pkt_entry_pmh = op_prg_pmo_define ("Send Buffer Packet Entry", sizeof (DsrT_Packet_Entry), 32);
		send_buffer_pkt_entry_pmh_defined = OPC_TRUE;
		}
	
	pkt_entry_ptr = (DsrT_Packet_Entry*) op_prg_pmo_alloc (send_buffer_pkt_entry_pmh);
	pkt_entry_ptr->pkptr = OPC_NIL;
	
	FRET (pkt_entry_ptr);
	}


static void
dsr_send_buffer_pkt_entry_mem_free (DsrT_Packet_Entry* pkt_entry_ptr)
	{
	/** Frees the packet entry in the send buffer	**/
	FIN (dsr_send_buffer_pkt_entry_mem_free (<args>));
	
	/* Destroy the packet	*/
	manet_rte_ip_pkt_destroy (pkt_entry_ptr->pkptr);
	
	/* Free the entry	*/
	op_prg_mem_free (pkt_entry_ptr);
	
	FOUT;
	}
