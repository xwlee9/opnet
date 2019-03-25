/* grp_dest_table.ex.c */
/* C support file for destination table in GRP */


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
#include <ip_support.h>
#include <oms_tan.h>

/**** Local function  *****/
static GrpT_Dest_Info*			grp_dest_table_dest_mem_alloc (void);


void
grp_dest_table_update (PrgT_String_Hash_Table* destination_table, InetT_Address dest_addr, 
						double dest_lat, double dest_long, double timestamp, double quad_size, 
						GrpT_Destination_Type dest_type)
	{
	char					dest_addr_str [128];
	GrpT_Dest_Info*			dest_ptr;
	void*					ignore;
	
	/** Updates the information in the	**/
	/** destination table				**/
	FIN (grp_dest_table_update (<args>));
	
	/* Get the destination address as a string	*/
	/* This string is the key to the table		*/
	inet_address_print (dest_addr_str, dest_addr);
	
	/* Check if the destination exists in the	*/
	/* hash table. 							*/
	dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, dest_addr_str);
	
	if (dest_ptr == PRGC_NIL)
		{
		/* This destination does not exist 	*/
		/* in the hash table. Create a new	*/
		/* entry in the hash table.			*/
		dest_ptr = grp_dest_table_dest_mem_alloc ();
		dest_ptr->dest_addr = inet_address_copy (dest_addr);
		dest_ptr->dest_lat = dest_lat;
		dest_ptr->dest_long = dest_long;
		dest_ptr->timestamp = timestamp;
		dest_ptr->quadrant_size = quad_size;
		dest_ptr->destination_type = dest_type;
		dest_ptr->destination_state = GrpC_Destination_Valid;
		
		/* Insert the entry into the hash table	*/
		prg_string_hash_table_item_insert (destination_table, dest_addr_str, dest_ptr, &ignore);
		}
	else
		{
		/* The destination entry does exist in	*/
		/* the hash table. If this information	*/
		/* is newer, update the table entry		*/
		if ((timestamp >= dest_ptr->timestamp) || (dest_ptr->destination_state == GrpC_Destination_Invalid))
			{
			/* The information received	is newer. Update the entry	*/
			dest_ptr->dest_lat = dest_lat;
			dest_ptr->dest_long = dest_long;
			dest_ptr->timestamp = timestamp;
			dest_ptr->quadrant_size = quad_size;
			dest_ptr->destination_type = dest_type;
			dest_ptr->destination_state = GrpC_Destination_Valid;
			}
		else
			{
			/* The info received is older than	*/
			/* the information in the dest 		*/
			/* table. Ignore the message		*/
			}
		}
	
	FOUT;
	}


GrpT_Dest_Info*
grp_dest_table_entry_exists (PrgT_String_Hash_Table* destination_table, InetT_Address dest_addr)
	{
	char				dest_addr_str [128];
	GrpT_Dest_Info*		dest_ptr;
	
	/** Returns the destination information	**/
	/** stored in the table. If the dest	**/
	/** does not exist, OPC_NIL is returned	**/
	FIN (grp_dest_table_entry_exists (<args>));
	
	/* Get the destination address as a string	*/
	/* This string is the key to the table		*/
	inet_address_print (dest_addr_str, dest_addr);
	
	/* Check if the destination exists in the	*/
	/* hash table. 							*/
	dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, dest_addr_str);
	
	if ((dest_ptr != OPC_NIL) && (dest_ptr->destination_state == GrpC_Destination_Valid))
		{
		FRET (dest_ptr);
		}
	else
		{
		FRET (OPC_NIL);
		}
	}


GrpT_Dest_Info*
grp_dest_table_entry_exists_ptr (PrgT_String_Hash_Table* destination_table, InetT_Address* dest_addr_ptr)
	{
	char				dest_addr_str [128];
	GrpT_Dest_Info*		dest_ptr;
	
	/** Returns the destination information	**/
	/** stored in the table. If the dest	**/
	/** does not exist, OPC_NIL is returned	**/
	FIN (grp_dest_table_entry_exists_ptr (<args>));
	
	/* Get the destination address as a string	*/
	/* This string is the key to the table		*/
	inet_address_ptr_print (dest_addr_str, dest_addr_ptr);
	
	/* Check if the destination exists in the	*/
	/* hash table. 							*/
	dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, dest_addr_str);
	
	if ((dest_ptr != OPC_NIL) && (dest_ptr->destination_state == GrpC_Destination_Valid))
		{
		FRET (dest_ptr);
		}
	else
		{
		FRET (OPC_NIL);
		}
	}


List*
grp_dest_table_dest_info_request_list_get (PrgT_String_Hash_Table* destination_table, double lat_dist, double long_dist, 
												double neighborhood_size)
	{
	List*				dest_lptr;
	List*				keys_lptr;
	int					num_keys, count;
	char*				key_ptr;
	GrpT_Dest_Info*		dest_ptr;
	InetT_Address*		addr_ptr;
	double				low_lat, high_lat, low_long, high_long;
	double				quad_size, quad_lat_m, quad_long_m;
	
	/** Finds the list of destination nodes for which new	**/
	/** location information needs to be obtained			**/
	FIN (grp_dest_table_dest_info_request_list_get (<args>));
	
	/* Create a list to store the elements	*/
	dest_lptr = op_prg_list_create ();
	
	/* Get the list of keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (destination_table);
	
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, key_ptr);
		
		if (dest_ptr->destination_state == GrpC_Destination_Invalid)
			{
			/* If the destination is invalid, we need to get new	*/
			/* information about this destination.					*/
			addr_ptr = inet_address_create_dynamic (dest_ptr->dest_addr);
			op_prg_list_insert (dest_lptr, addr_ptr, OPC_LISTPOS_TAIL);
			
			continue;
			}
		
		if (dest_ptr->destination_type == GrpC_Destination_Node)
			{
			/* This destination has exact location information.	*/
			
			/* Calculate the coordinates of the neighborhood in metres	*/
			grp_support_neighborhood_determine (dest_ptr->dest_lat, dest_ptr->dest_long, dest_ptr->quadrant_size, &low_lat, 
													&low_long, &high_lat, &high_long);
			
			/* Check if the node lies within this quadrant	*/
			if ((low_lat <= lat_dist) && (lat_dist < high_lat) && 
				(low_long <= long_dist) && (long_dist < high_long))
				{
				/* Since the node lies within this quadrant, the informtion	*/
				/* about the destination is correct.						*/
				}
			else
				{
				/* The node does not lie within the quadrant. Calculate the	*/
				/* new destination information for this node				*/
				quad_size = grp_support_highest_neighbor_quadrant_determine (lat_dist, long_dist, dest_ptr->dest_lat, dest_ptr->dest_long, 
																			neighborhood_size, &quad_lat_m, &quad_long_m);
				
				/* Set the new quadrant information for this destination	*/
				dest_ptr->dest_lat = quad_lat_m;
				dest_ptr->dest_long = quad_long_m;
				dest_ptr->destination_type = GrpC_Destination_Quad;
				dest_ptr->quadrant_size = quad_size;
				}
			}
		else
			{
			/* The destination location information is a quadrant. 		*/
			/* Check if this node lies within the destination quadrant	*/
			low_lat = dest_ptr->dest_lat - dest_ptr->quadrant_size / 2.0;
			high_lat = dest_ptr->dest_lat + dest_ptr->quadrant_size / 2.0;
			low_long = dest_ptr->dest_long - dest_ptr->quadrant_size / 2.0;
			high_long = dest_ptr->dest_long + dest_ptr->quadrant_size / 2.0;
			
			if ((low_lat <= lat_dist) && (lat_dist < high_lat) && 
				(low_long <= long_dist) && (long_dist < high_long))
				{
				/* The node does lie within the destination node's		*/
				/* quadrant information. We need to get new destination	*/
				/* location information for this destination.			*/
				addr_ptr = inet_address_create_dynamic (dest_ptr->dest_addr);
				op_prg_list_insert (dest_lptr, addr_ptr, OPC_LISTPOS_TAIL);
				
				/* Set the destination as invalid	*/
				dest_ptr->destination_state = GrpC_Destination_Invalid;
				}
			else
				{
				/* The node does not lie within the destination node's	*/
				/* quadrant. Check if the quadrant information is		*/
				/* correct or calculate the new destination information	*/
				quad_size = grp_support_highest_neighbor_quadrant_determine (lat_dist, long_dist, dest_ptr->dest_lat, dest_ptr->dest_long, 
																				neighborhood_size, &quad_lat_m, &quad_long_m);
				
				/* Set the new quadrant information for this destination	*/
				dest_ptr->dest_lat = quad_lat_m;
				dest_ptr->dest_long = quad_long_m;
				dest_ptr->destination_type = GrpC_Destination_Quad;
				dest_ptr->quadrant_size = quad_size;
				}
			}
		}
	
	/* Free the keys	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FRET (dest_lptr);
	}


List*
grp_dest_table_invalid_dest_list_get (PrgT_String_Hash_Table* destination_table)
	{
	List*				dest_lptr;
	List*				keys_lptr;
	int					num_keys, count;
	char*				key_ptr;
	GrpT_Dest_Info*		dest_ptr;
	InetT_Address*		addr_ptr;
	
	/** Gets the list of invalid destinations in	**/
	/** the destination table.						**/
	FIN (grp_dest_table_invalid_dest_list_get (<args>));
	
	/* Create a list to store the elements	*/
	dest_lptr = op_prg_list_create ();
	
	/* Get the list of keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (destination_table);
	
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, key_ptr);
		
		if (dest_ptr->destination_state == GrpC_Destination_Invalid)
			{
			/* This destination is invalid. Insert into list	*/
			addr_ptr = inet_address_create_dynamic (dest_ptr->dest_addr);
			op_prg_list_insert (dest_lptr, addr_ptr, OPC_LISTPOS_TAIL);
			}
		}
	
	/* Free the keys	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FRET (dest_lptr);
	}


Boolean
grp_dest_table_invalid_dest_exists (PrgT_String_Hash_Table* destination_table)
	{
	List*				keys_lptr;
	int					num_keys, count;
	char*				key_ptr;
	GrpT_Dest_Info*		dest_ptr;
	
	FIN (grp_dest_table_invalid_dest_exists (<args>));
	
	/* Get the list of keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (destination_table);
	
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, key_ptr);
		
		if (dest_ptr->destination_state == GrpC_Destination_Invalid)
			{
			/* Free the list	*/
			op_prg_list_free (keys_lptr);
			op_prg_mem_free (keys_lptr);
			
			FRET (OPC_TRUE);
			}
		}
	
	/* Free the list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FRET (OPC_FALSE);
	}


void
grp_dest_table_print (PrgT_String_Hash_Table* destination_table)
	{
	List*				keys_lptr;
	int					num_keys, count;
	char*				key_ptr;
	GrpT_Dest_Info*		dest_ptr;
	char				node_name [OMSC_HNAME_MAX_LEN];
	char				dest_type_str [128];
	char				dest_state_str [128];
	char				temp_str [10000];
	
	/** Prints the destination table to the standard output	**/
	FIN (grp_dest_table_print (<args>));
	
	op_prg_odb_print_major ("-----------------------------------------------------------------------", OPC_NIL);
	op_prg_odb_print_major ("                      DESTINATION TABLE                                ", OPC_NIL);
	op_prg_odb_print_major ("-----------------------------------------------------------------------", OPC_NIL);
	
	keys_lptr = prg_string_hash_table_keys_get (destination_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_keys; count++)
		{
		key_ptr = (char*) op_prg_list_access (keys_lptr, count);
		dest_ptr = (GrpT_Dest_Info*) prg_string_hash_table_item_get (destination_table, key_ptr);
		inet_address_to_hname (dest_ptr->dest_addr, node_name);
		
		if (dest_ptr->destination_type == GrpC_Destination_Quad)
			sprintf (dest_type_str, "Quadrant Infomation");
		else
			sprintf (dest_type_str, "Exact Position Information");
		
		if (dest_ptr->destination_state == GrpC_Destination_Valid)
			sprintf (dest_state_str, "Destination Valid");
		else
			sprintf (dest_state_str, "Destination Invalid");
		
		sprintf (temp_str, "Node : %s, Lat : %f, Long : %f, Quad Size = %f, Dest Info : %s, Dest State : %s, Time : %f", node_name, 
						dest_ptr->dest_lat, dest_ptr->dest_long, dest_ptr->quadrant_size, dest_type_str, dest_state_str, dest_ptr->timestamp);
		
		op_prg_odb_print_major (temp_str, OPC_NIL);
		}
	
	op_prg_list_free  (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


static GrpT_Dest_Info*
grp_dest_table_dest_mem_alloc (void)
	{
	static Pmohandle		dest_pmh;
	GrpT_Dest_Info*			dest_ptr = OPC_NIL;
	static Boolean			dest_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the	**/
	/** destination table entry			**/
	FIN (grp_dest_table_dest_mem_alloc (void));
	
	if (dest_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for destination*/
		/* table entries if not already defined			*/
		dest_pmh = op_prg_pmo_define ("GRP Destination Info", sizeof (GrpT_Dest_Info), 32);
		dest_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the flooding options from the pooled memory	*/
	dest_ptr = (GrpT_Dest_Info*) op_prg_pmo_alloc (dest_pmh);
	
	FRET (dest_ptr);
	}
