/* grp_nbr_table.ex.c */
/* C support file for neighbor table in GRP */


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
#include <math.h>
#include <grp.h>
#include <grp_pkt_support.h>
#include <grp_ptypes.h>
#include <ip_addr_v4.h>

/**** Local function  *****/
static double			grp_nbr_table_nbr_dist_to_dest_quad_calculate (double, double, double, double, double);
static void				grp_nbr_table_expired_entries_delete (PrgT_String_Hash_Table*);
static GrpT_Nbr_Info*	grp_nbr_table_nbr_mem_alloc (void);
static void				grp_nbr_table_nbr_mem_free (GrpT_Nbr_Info*);


void
grp_nbr_table_update (PrgT_String_Hash_Table* neighbor_table, InetT_Address nbr_addr, double node_lat, double node_long, 
						double timestamp, double nbr_expiry_time)
	{
	char					nbr_addr_str [128];
	GrpT_Nbr_Info*			nbr_ptr;
	void*					ignore;
	
	/** Updates the information in the neighbor	**/
	/** table upon reception of a Hello message	**/
	FIN (grp_nbr_table_update (<args>));
	
	/* Get the neighbor address as a string	*/
	/* This string is the key to the table	*/
	inet_address_print (nbr_addr_str, nbr_addr);
	
	/* Check if the neighbor exists in the	*/
	/* hash table. 							*/
	nbr_ptr = (GrpT_Nbr_Info*) prg_string_hash_table_item_get (neighbor_table, nbr_addr_str);
	if (nbr_ptr == PRGC_NIL)
		{
		/* This neighbor does not exist in	*/
		/* the hash table. Create a new		*/
		/* entry in the hash table.			*/
		nbr_ptr = grp_nbr_table_nbr_mem_alloc ();
		nbr_ptr->nbr_addr = inet_address_copy (nbr_addr);
		nbr_ptr->nbr_lat = node_lat;
		nbr_ptr->nbr_long = node_long;
		nbr_ptr->timestamp = timestamp;
		nbr_ptr->timeout = op_sim_time () + nbr_expiry_time;
		
		/* Insert the entry into the hash table	*/
		prg_string_hash_table_item_insert (neighbor_table, nbr_addr_str, nbr_ptr, &ignore);
		}
	else
		{
		/* The neighbor entry does exist in		*/
		/* the hash table. If this information	*/
		/* is newer, update the table entry		*/
		if (timestamp >= nbr_ptr->timestamp)
			{
			/* The Hello information received	*/
			/* is newer. Update the entry		*/
			nbr_ptr->nbr_lat = node_lat;
			nbr_ptr->nbr_long = node_long;
			nbr_ptr->timestamp = timestamp;
			nbr_ptr->timeout = op_sim_time () + nbr_expiry_time;
			}
		else
			{
			/* The Hello received is older than	*/
			/* the information in the nbr table	*/
			/* Ignore the hello message			*/
			}
		}
	
	FOUT;
	}


InetT_Address
grp_nbr_table_closest_nbr_to_dest_find (PrgT_String_Hash_Table* neighbor_table, PrgT_String_Hash_Table* backtrack_table, 
										PrgT_String_Hash_Table* destination_table, List* nodes_traversed_lptr, 
										GrpT_Dest_Info* dest_ptr, double prev_dist_to_dest, double* new_dist_to_dest)
	{
	double					nbr_dist;
	double					dest_low_lat, dest_high_lat;
	double					dest_low_long, dest_high_long;
	List*					keys_lptr;
	char*					key_ptr;
	int						num_keys, count, num;
	GrpT_Nbr_Info*			nbr_ptr;
	GrpT_Dest_Info*			nbr_dest_ptr;
	InetT_Address			min_nbr_addr = InetI_Invalid_Addr;
	InetT_Address*			traversed_node_addr_ptr;
	Boolean					node_traversed_flag;
	int						nodes_traversed_size;
	GrpT_Backtrack_Info*	backtrack_nbr_ptr;
	char					nbr_addr_str [128];
	
	/** Finds the neighbor node that is closest to the	**/
	/** destination node in distance.					**/
	FIN (grp_nbr_table_closest_nbr_to_dest_find (<args>));
	
	/* Delete all expired entries in the neighbor table	*/
	grp_nbr_table_expired_entries_delete (neighbor_table);
	
	/* Get the size of the nodes traversed list	*/
	if (nodes_traversed_lptr != OPC_NIL)
		nodes_traversed_size = op_prg_list_size (nodes_traversed_lptr);
	
	if (dest_ptr->destination_type == GrpC_Destination_Quad)
		{
		/* Calculate the lowest and highest latitudes the quadrant covers	*/
		dest_low_lat = dest_ptr->dest_lat - (dest_ptr->quadrant_size/2.0);
		dest_high_lat = dest_ptr->dest_lat + (dest_ptr->quadrant_size/2.0);
	
		/* Calculate the lowest and highest longitudes the quadrant covers	*/
		dest_low_long = dest_ptr->dest_long - (dest_ptr->quadrant_size/2.0);
		dest_high_long = dest_ptr->dest_long + (dest_ptr->quadrant_size/2.0);
		}
	else
		{
		/* Determine the quadrant that this node lies in	*/
		grp_support_neighborhood_determine (dest_ptr->dest_lat, dest_ptr->dest_long, dest_ptr->quadrant_size, &dest_low_lat, 
											&dest_low_long, &dest_high_lat, &dest_high_long);
		}
	
	/* Get the list of items in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (neighbor_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		/* Get each item in the list	*/
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		nbr_ptr = (GrpT_Nbr_Info*) prg_string_hash_table_item_get (neighbor_table, key_ptr);
		
		/* Check if the neighbor node is the destination	*/
		if (inet_address_equal (nbr_ptr->nbr_addr, dest_ptr->dest_addr))
			{
			/* The neighbor node is the destination	*/
			/* Free the memory for the keys.		*/
			op_prg_list_free (keys_lptr);
			op_prg_mem_free (keys_lptr);
			
			FRET (nbr_ptr->nbr_addr);
			}
		
		/* If the neighbor is invalid in the destination table,	*/
		/* then do not consider this neighbor node.				*/
		nbr_dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, key_ptr);
		if ((nbr_dest_ptr != OPC_NIL) && (nbr_dest_ptr->destination_state == GrpC_Destination_Invalid))
			{
			/* This neighbor has been marked as invalid in the	*/
			/* destination table. Do not consider this node		*/
			continue;
			}
		
		/* Check if this neighbor node has already been	*/
		/* traversed by the data packet.				*/
		if (nodes_traversed_lptr != OPC_NIL)
			{
			node_traversed_flag = OPC_FALSE;
			
			for (num = 0; num < nodes_traversed_size; num++)
				{
				traversed_node_addr_ptr = (InetT_Address*) op_prg_list_access (nodes_traversed_lptr, num);
				
				if (inet_address_equal (nbr_ptr->nbr_addr, *traversed_node_addr_ptr))
					{
					/* Omit this node as the packet has	*/
					/* already traversed this node		*/
					node_traversed_flag = OPC_TRUE;
					
					break;
					}
				}
			
			if (node_traversed_flag == OPC_TRUE)
				{
				/* This neighbor node has already	*/
				/* been traversed by the data 		*/
				/* packet. Omit this node.			*/
				continue;
				}
			}
		
		/* Check if the neighbor exists in the backtrack table	*/
		inet_address_print (nbr_addr_str, nbr_ptr->nbr_addr);
		backtrack_nbr_ptr = (GrpT_Backtrack_Info*) prg_string_hash_table_item_get (backtrack_table, nbr_addr_str);
		
		if (backtrack_nbr_ptr != PRGC_NIL)
			{
			/* The neighbor does exist in the backtrack table	*/
			/* This neighbor node cannot be considered for path	*/
			/* computation as a backtrack message was previously*/
			/* received by this neighbor node.					*/
			continue;
			}
				
		if (dest_ptr->destination_type == GrpC_Destination_Quad)
			{
			/* Check if the neighbor node lies in the destination quadrant	*/
			if ((dest_low_lat <= nbr_ptr->nbr_lat) && (nbr_ptr->nbr_lat < dest_high_lat) &&
				(dest_low_long <= nbr_ptr->nbr_long) && (nbr_ptr->nbr_long < dest_high_long))
				{
				/* The neighbor node does lie in the destination	*/
				/* quadrant. Simply consider this node as the		*/
				/* closest node to the destination.					*/
				*new_dist_to_dest = -1.0;
				
				/* Free the memory for the keys.		*/
				op_prg_list_free (keys_lptr);
				op_prg_mem_free (keys_lptr);
				
				FRET (nbr_ptr->nbr_addr);
				}
			else
				{
				/* The destination node is in a different quadrant	*/
				/* Calculate the distance between the neighbor node	*/
				/* and the destination quadrant entry point 		*/
				nbr_dist = grp_nbr_table_nbr_dist_to_dest_quad_calculate (nbr_ptr->nbr_lat, nbr_ptr->nbr_long, dest_ptr->dest_lat, 
																			dest_ptr->dest_long, dest_ptr->quadrant_size);
				}
			}
		else
			{
			/* The destination node is in the same quadrant as this node	*/
			/* Check if the neighbor node lies in the destination quadrant	*/
			if ((dest_low_lat <= nbr_ptr->nbr_lat) && (nbr_ptr->nbr_lat < dest_high_lat) &&
				(dest_low_long <= nbr_ptr->nbr_long) && (nbr_ptr->nbr_long < dest_high_long))
				{
				/* The neighbor node does lie in the same quadrant	*/
				/* Calculate the distance between the neighbor node	*/
				/* and the exact destination location.				*/
				nbr_dist = sqrt (((dest_ptr->dest_lat - nbr_ptr->nbr_lat) * (dest_ptr->dest_lat - nbr_ptr->nbr_lat)) + 
							((dest_ptr->dest_long - nbr_ptr->nbr_long) * (dest_ptr->dest_long - nbr_ptr->nbr_long)));
				}
			else
				{
				/* Do not consider neighbor nodes that do not lie in the same	*/
				/* quadrant as the destination node and the current node		*/
				continue;
				}
			}
		
		/* Check if the distance is the least distance	*/
		/* to the destination that has been found		*/
		if ((prev_dist_to_dest == -1.0) || (nbr_dist < prev_dist_to_dest))
			{
			/* This node is closest to the destination	*/
			prev_dist_to_dest = nbr_dist;
			
			if (!inet_address_equal (min_nbr_addr, InetI_Invalid_Addr))
				inet_address_destroy (min_nbr_addr);
			
			min_nbr_addr = inet_address_copy (nbr_ptr->nbr_addr);
			}
		}
	
	/* Set the new distance to the destination	*/
	*new_dist_to_dest = prev_dist_to_dest;
	
	/* Free the list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FRET (min_nbr_addr);
	}


InetT_Address
grp_nbr_table_pos_req_nbr_find (PrgT_String_Hash_Table* neighbor_table, List* nbr_nodes_attempted_lptr, 
									double low_lat, double low_long, double high_lat, double high_long)
	{
	int					nbr_attempted_size;
	List*				keys_lptr;
	int					size, count, num;
	char*				key_ptr;
	GrpT_Nbr_Info*		nbr_ptr;
	Boolean				nbr_found;
	InetT_Address*		att_nbr_node_addr_ptr;
	InetT_Address*		nbr_node_addr_ptr;
	
	/** Finds a neighbor node in the same quadrant	**/
	/** as the current node which has already not	**/
	/** been attempted already.						**/
	FIN (grp_nbr_table_pos_req_nbr_find (<args>));
	
	/* Get the size of the neighbor node attempted list	*/
	nbr_attempted_size = op_prg_list_size (nbr_nodes_attempted_lptr);
	
	/* Get the keys to the neighbor table	*/
	keys_lptr = prg_string_hash_table_keys_get (neighbor_table);
	size = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < size; count++)
		{
		/* Access each neighbor	*/
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		nbr_ptr = (GrpT_Nbr_Info*) prg_string_hash_table_item_get (neighbor_table, key_ptr);
		
		/* Check if the neighbor is in the same quadrant as this node	*/
		if ((low_lat <= nbr_ptr->nbr_lat) && (nbr_ptr->nbr_lat < high_lat) && 
			(low_long <= nbr_ptr->nbr_long) && (nbr_ptr->nbr_long < high_long))
			{
			/* Initialize the variable to keep track of		*/
			/* whether the neighbor has been found 			*/
			nbr_found = OPC_FALSE;
			
			/* This neighbor node is in the same quadrant	*/
			/* as the destination node. Make sure that this	*/
			/* neighbor node has not already been used		*/
			for (num = 0; num < nbr_attempted_size; num++)
				{
				att_nbr_node_addr_ptr = (InetT_Address*) op_prg_list_access (nbr_nodes_attempted_lptr, num);
				
				nbr_node_addr_ptr = inet_address_create_dynamic (nbr_ptr->nbr_addr);
				
				if (inet_address_ptr_equal (att_nbr_node_addr_ptr, nbr_node_addr_ptr) == OPC_TRUE)
					{
					/* This neighbor node has already been attempted	*/
					/* Do not consider this neighbor for position req	*/
					nbr_found = OPC_TRUE;
					}
				
				/* Free the memory	*/
				op_prg_mem_free (nbr_node_addr_ptr);
				
				if (nbr_found == OPC_TRUE)
					{
					break;
					}
				}
			
			if (nbr_found == OPC_FALSE)
				{
				/* Free the keys	*/
				op_prg_list_free (keys_lptr);
				op_prg_mem_free (keys_lptr);
				
				/* This neighbor node has not	*/
				/* yet been attempted. Use it.	*/
				FRET (nbr_ptr->nbr_addr);
				}
			}
		}
	
	/* Free the keys	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	/* There are either no neighbors in this quadrant	*/
	/* or this node has already ateempted all its nbrs	*/
	FRET (InetI_Invalid_Addr);
	}
			
					
				

/****** Internal function  *******/


static double
grp_nbr_table_nbr_dist_to_dest_quad_calculate (double nbr_lat, double nbr_long, double dest_quad_centre_lat, 
													double dest_quad_centre_long, double quadrant_size)
	{
	double		dest_low_lat, dest_high_lat;
	double		dest_low_long, dest_high_long;
	double		nbr_dist;
	
	/** Calculates the closest distance between the neighbor node	**/
	/** and the destination quadrant that the destination node lies	**/
	FIN (grp_nbr_table_nbr_dist_to_dest_quad_calculate (<args>));
	
	/* Calculate the lowest and highest latitudes the quadrant covers	*/
	dest_low_lat = dest_quad_centre_lat - (quadrant_size/2.0);
	dest_high_lat = dest_quad_centre_lat + (quadrant_size/2.0);
	
	/* Calculate the lowest and highest longitudes the quadrant covers	*/
	dest_low_long = dest_quad_centre_long - (quadrant_size/2.0);
	dest_high_long = dest_quad_centre_long + (quadrant_size/2.0);
	
	/* Check if the node lies within the latitudes of the quadrant	*/
	if ((dest_low_lat <= nbr_lat) && (nbr_lat < dest_high_lat))
		{
		/* The node lies within the latitudes of the quadrant	*/
		if (nbr_long < dest_low_long)
			{
			/* The shortest distance to the destination 	*/
			/* quadrant is the difference in longitude		*/
			nbr_dist = dest_low_long - nbr_long;
			}
		else if (nbr_long >= dest_high_long)
			{
			/* The shortest distance to the destination 	*/
			/* quadrant is the difference in longitude		*/
			nbr_dist = nbr_long - dest_high_long;
			}
		else
			{
			/* The node lies in the same quadrant as the	*/
			/* destination. This case should not happen as 	*/
			/* an earlier check is made to make sure the 	*/
			/* neighbor node is not in the same quadrant	*/
			}
		}
	else if ((dest_low_long <= nbr_long) && (nbr_long < dest_high_long))
		{
		/* The node lies within the longitudes of the quadrant	*/
		if (nbr_lat < dest_low_lat)
			{
			/* The shortest distance to the destination 	*/
			/* quadrant is the difference in latitude		*/
			nbr_dist = dest_low_lat - nbr_lat;
			}
		else if (nbr_lat >= dest_high_lat)
			{
			/* The shortest distance to the destination 	*/
			/* quadrant is the difference in latitude		*/
			nbr_dist = nbr_lat - dest_high_lat;
			}
		else
			{
			/* The node lies in the same quadrant as the	*/
			/* destination. This case should not happen as 	*/
			/* an earlier check is made to make sure the 	*/
			/* neighbor node is not in the same quadrant	*/
			}
		}
	else if ((nbr_lat < dest_low_lat) && (nbr_long < dest_low_long))
		{
		/* The closest distance to the destination quadrant	*/
		/* is the vertex of the quadrant.					*/
		nbr_dist = sqrt (((dest_low_lat - nbr_lat) * (dest_low_lat - nbr_lat)) + ((dest_low_long - nbr_long) * (dest_low_long - nbr_long)));
		}
	else if ((nbr_lat < dest_low_lat) && (nbr_long > dest_high_long))
		{
		/* The closest distance to the destination quadrant	*/
		/* is the vertex of the quadrant.					*/
		nbr_dist = sqrt (((dest_low_lat - nbr_lat) * (dest_low_lat - nbr_lat)) + ((nbr_long - dest_high_long) * (nbr_long - dest_high_long)));
		}
	else if ((nbr_lat > dest_high_lat) && (nbr_long < dest_low_long))
		{
		/* The closest distance to the destination quadrant	*/
		/* is the vertex of the quadrant.					*/
		nbr_dist = sqrt (((nbr_lat - dest_high_lat) * (nbr_lat - dest_high_lat)) + ((dest_low_long - nbr_long) * (dest_low_long - nbr_long)));
		}
	else if ((nbr_lat > dest_high_lat) && (nbr_long > dest_high_long))
		{
		/* The closest distance to the destination quadrant	*/
		/* is the vertex of the quadrant.					*/
		nbr_dist = sqrt (((nbr_lat - dest_high_lat) * (nbr_lat - dest_high_lat)) + ((nbr_long - dest_high_long) * (nbr_long - dest_high_long)));
		}
	else
		{
		/* This case is not possible as all options have	*/
		/* already been covered. The node must lie in one	*/
		/* of the options above.							*/
		}
	
	FRET (nbr_dist);
	}


static void
grp_nbr_table_expired_entries_delete (PrgT_String_Hash_Table* neighbor_table)
	{
	double				current_time;
	List*				keys_lptr;
	int					num_keys, count;
	char*				key_ptr;
	GrpT_Nbr_Info*		nbr_ptr;
	
	/** Deletes all the entries in the neighbor	**/
	/** table that are expired.					**/
	FIN (grp_nbr_table_expired_entries_delete (<args>));
	
	current_time = op_sim_time ();
	
	/* Get the list of keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (neighbor_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		nbr_ptr = (GrpT_Nbr_Info*) prg_string_hash_table_item_get (neighbor_table, key_ptr);
		
		if (current_time > nbr_ptr->timeout)
			{
			/* This entry has expired. Remove	*/
			/* it from the neighbor table		*/
			nbr_ptr = (GrpT_Nbr_Info*) prg_string_hash_table_item_remove (neighbor_table, key_ptr);
			grp_nbr_table_nbr_mem_free (nbr_ptr);
			}
		}
	
	/* Free the list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


static GrpT_Nbr_Info*
grp_nbr_table_nbr_mem_alloc (void)
	{
	static Pmohandle		nbr_pmh;
	GrpT_Nbr_Info*			nbr_ptr = OPC_NIL;
	static Boolean			nbr_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the neighbor table entry	**/
	FIN (grp_nbr_table_nbr_mem_alloc (void));
	
	if (nbr_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for neighbor	*/
		/* table entries if not already defined			*/
		nbr_pmh = op_prg_pmo_define ("GRP Neighbor Info", sizeof (GrpT_Nbr_Info), 32);
		nbr_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the flooding options from the pooled memory	*/
	nbr_ptr = (GrpT_Nbr_Info*) op_prg_pmo_alloc (nbr_pmh);
	
	FRET (nbr_ptr);
	}


static void
grp_nbr_table_nbr_mem_free (GrpT_Nbr_Info* nbr_ptr)
	{
	/** Frees the entry in the neighbor table	**/
	FIN (grp_nbr_table_nbr_mem_free (<args>));
	
	inet_address_destroy (nbr_ptr->nbr_addr);
	op_prg_mem_free (nbr_ptr);
	
	FOUT;
	}
