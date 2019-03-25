/* dsr_route_cache.ex.c */
/* C file for DSR Route Cache APIs */


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
#include <ip_support.h>
#include <manet.h>
#include <oms_ot_support.h>
#include <oms_tan.h>

/* These arrays hold the names of the different columns					*/
/* in the respective table. The arrays will be used for column names	*/
/* while creating the templates, and will also be used as column tags	*/
/* while wrting into the tables											*/
int			routes_cache_columns_name_array_size = 6;
const char*	routes_cache_columns_name_array [] 	= {"Destination Node Name", "Time Installed", "First Hop External", 
														"Last Hop External", "Hop Count", "Route(s)"};

/**** Prototypes ****/
static DsrT_Path_Info*				dsr_route_cache_entry_create (List* path_lptr, Boolean first_hop_external, 
																	Boolean last_hop_external);
static Boolean						dsr_route_cache_route_exists (List* dest_routes_lptr, List* new_path_lptr);
static Boolean						dsr_route_cache_size_exceed (DsrT_Route_Cache* route_cache_ptr);
static void							dsr_route_cache_insert_route_sorted (List* paths_lptr, DsrT_Path_Info* path_info_ptr);
static void							dsr_route_cache_insert_route_priority (DsrT_Route_Cache* route_cache_ptr, 
																		List* paths_lptr, DsrT_Path_Info* path_info_ptr);
static void							dsr_route_cache_all_expired_routes_delete (DsrT_Route_Cache* route_cache_ptr);
static void							dsr_route_cache_size_stat_update (DsrT_Route_Cache*);
static void							dsr_route_cache_num_hops_stat_update (DsrT_Route_Cache*, List*);
static void							dsr_route_cache_hit_success_stat_update (DsrT_Route_Cache*);
static void							dsr_route_cache_hit_failure_stat_update (DsrT_Route_Cache*);

static DsrT_Route_Cache*			dsr_route_cache_mem_alloc (void);
static DsrT_Path_Info*				dsr_route_cache_entry_mem_alloc (void);
static void							dsr_route_cache_entry_mem_free (DsrT_Path_Info* path_ptr);


DsrT_Route_Cache*
dsr_route_cache_create (int max_cache_size, double path_expiry_time, DsrT_Stathandles* stat_handle_ptr)
	{
	DsrT_Route_Cache*		route_cache_ptr = OPC_NIL;
	
	/** Creates the route cache for storing	**/
	/** the source routes discovered		**/
	FIN (dsr_route_cache_create ());
	
	/* Allocate memory for the route cache	*/
	route_cache_ptr = dsr_route_cache_mem_alloc ();
	
	/* Set the maximum size of the cache	*/
	route_cache_ptr->max_cache_size = max_cache_size;
	
	/* Set the time for paths in the route	*/
	/* cache to expire after which the 		*/
	/* paths will be deleted				*/
	route_cache_ptr->path_expiry_time = path_expiry_time;
	
	/* Set the statistic handles	*/
	route_cache_ptr->dsr_stat_ptr = stat_handle_ptr;
	
	FRET (route_cache_ptr);
	}


void
dsr_route_cache_entry_add (DsrT_Route_Cache* route_cache_ptr, List* hops_lptr, 
						Boolean first_hop_external, Boolean last_hop_external)
	{
	char				dest_addr_str [INETC_ADDR_STR_LEN];
	List*				dest_routes_lptr = OPC_NIL;
	void*				old_contents_ptr = OPC_NIL;
	Boolean				temp_last_hop_external = OPC_FALSE;
	int					num_hops, count;
	List*				temp_lptr = OPC_NIL;
	InetT_Address*		hop_address_ptr;
	DsrT_Path_Info*		path_info_ptr = OPC_NIL;
	
	/** Adds a new route to a specific 	**/
	/** destination to the route cache	**/
	FIN (dsr_route_cache_entry_add (<args>));
	
	/* Delete all routes that have expired	*/
	dsr_route_cache_all_expired_routes_delete (route_cache_ptr);
	
	/* For each hop, enter it as a reachable	*/
	/* destination in the route cache			*/
	num_hops = op_prg_list_size (hops_lptr);
	
	/* Create a temporary list to store the hops	*/
	/* to each destination that is to be entered	*/
	temp_lptr = op_prg_list_create ();
	
	for (count = 0; count < num_hops; count++)
		{
		/* Get each hop in the path	*/
		hop_address_ptr = (InetT_Address *) op_prg_list_access (hops_lptr, count);
		
		/* Insert it into the temporary list	*/
		op_prg_list_insert (temp_lptr, hop_address_ptr, count);
		
		/* The first hop is this node itself	*/
		/* Do not consider the first hop		*/
		if (count == 0)
			continue;
		
		/* The last hop will be external only	*/
		/* for the last hop in this path		*/
		if (count == (num_hops - 1))
			temp_last_hop_external = last_hop_external;
		
		/* Get the destination address as a 	*/
		/* string to index the hash table		*/
		inet_address_ptr_print (dest_addr_str, hop_address_ptr);
		
		/* Check if this destination exists in the hash table	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_addr_str);
		
		if (dest_routes_lptr != OPC_NIL)
			{
			/* Check if there is a matching route already	*/
			/* to the same destination. Do not insert		*/
			/* matching routes								*/
			if (dsr_route_cache_route_exists (dest_routes_lptr, temp_lptr) == OPC_TRUE)
				continue;
			}
		else
			{
			/* No route currently exists to this destination	*/
			/* Create a new entry and add it to the route cache	*/
			dest_routes_lptr = op_prg_list_create ();
		
			/* Set the route list in the route cache hash table	*/
			prg_string_hash_table_item_insert (route_cache_ptr->route_cache_table, dest_addr_str, dest_routes_lptr, &old_contents_ptr);
			}
		
		/* The route does not exist in the route cache	*/
		/* Create an entry for the route				*/
		path_info_ptr = dsr_route_cache_entry_create (temp_lptr, first_hop_external, temp_last_hop_external);
		
		/* Update the number of hops per route in the	*/
		/* route cache statictic						*/
		dsr_route_cache_num_hops_stat_update (route_cache_ptr, temp_lptr);
	
		/* Check if there is any space in the 	*/
		/* route cache to insert a new route	*/
		if (dsr_route_cache_size_exceed (route_cache_ptr) == OPC_FALSE)
			{
			/* Insert the new route to the route list to that destination	*/
			/* The route is inserted in the list of routes to that 			*/
			/* destination based on priorities with the highest priority	*/
			/* route at the head of the list and the lowest priority route	*/
			/* at the tail of the list										*/
			dsr_route_cache_insert_route_sorted (dest_routes_lptr, path_info_ptr);
			route_cache_ptr->current_cache_size += 1;
			dsr_route_cache_size_stat_update (route_cache_ptr);
			}
		else
			{
			/* The route cache size is full and there is no space	*/
			/* in the route cache to hold any more routes			*/
			/* Insert the route based on priority (number of hops,	*/
			/* no external flag, etc) and a "least recently used"	*/
			/* cache replacement policy.							*/
			
			/* Determine whether to insert this route into the		*/
			/* route cache. If a success is returned, the route		*/
			/* has been inserted and another route was deleted		*/
			/* based on the priority of the routes.					*/
			dsr_route_cache_insert_route_priority (route_cache_ptr, dest_routes_lptr, path_info_ptr);
			}
		}
	
	while (op_prg_list_size (temp_lptr) > 0)
		op_prg_list_remove (temp_lptr, OPC_LISTPOS_HEAD);

	/* Free the temporary list	*/
	op_prg_mem_free (temp_lptr);
	
	
	FOUT;
	}
		
		
DsrT_Path_Info*
dsr_route_cache_entry_access (DsrT_Route_Cache* route_cache_ptr, InetT_Address dest_address, Boolean discovery_performed)
	{
	char				dest_addr_str [INETC_ADDR_STR_LEN];
	List*				dest_routes_lptr = OPC_NIL;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	
	/** Accesses and returns a route to the	**/
	/** destination address specified. If 	**/
	/** no route exists, a NIL is returned	**/
	FIN (dsr_route_cache_entry_access (<args>));

	/* Get the destination string key	*/
	inet_address_print (dest_addr_str, dest_address);
	
	/* Delete all routes that have expired	*/
	dsr_route_cache_all_expired_routes_delete (route_cache_ptr);
	
	/* Get all routes to the destination	*/
	dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_addr_str);
	
	if (dest_routes_lptr == OPC_NIL)
		{
		/* Update the hit failure statistic to indicate	*/
		/* that the route cache did not have a route	*/
		dsr_route_cache_hit_failure_stat_update (route_cache_ptr);
		
		/* No routes exist in the route cache	*/
		/* to the destination. Return a NIL		*/
		FRET (OPC_NIL);
		}
	
	/* No routes exist to the destination	*/
	if (op_prg_list_size (dest_routes_lptr) == 0)
		FRET (OPC_NIL);
	
	/* The best route to the destination is always the first route	*/
	path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, OPC_LISTPOS_HEAD);
	
	/* Update the access time	*/
	path_ptr->last_access_time = op_sim_time ();
	
	/* Update the statistic for a route cache hit	*/
	if (discovery_performed == OPC_FALSE)
		{
		/* A route discovery was not performed to	*/
		/* obtain a route to this destination for 	*/
		/* this packet. This means that this is a 	*/
		/* route cache hit							*/
		dsr_route_cache_hit_success_stat_update (route_cache_ptr);
		}
	
	FRET (path_ptr);
	}


List*
dsr_route_cache_all_routes_to_dest_access (DsrT_Route_Cache* route_cache_ptr, InetT_Address dest_address)
	{
	char				dest_addr_str [INETC_ADDR_STR_LEN];
	List*				dest_routes_lptr = OPC_NIL;
	
	/** Accesses and returns a route to the	**/
	/** destination address specified. If 	**/
	/** no route exists, a NIL is returned	**/
	FIN (dsr_route_cache_all_routes_to_dest_access (<args>));

	/* Get the destination string key	*/
	inet_address_print (dest_addr_str, dest_address);
	
	/* Delete all routes that have expired	*/
	dsr_route_cache_all_expired_routes_delete (route_cache_ptr);
	
	/* Get all routes to the destination	*/
	dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_addr_str);
	
	if (dest_routes_lptr == OPC_NIL)
		{
		/* No routes exist in the route cache	*/
		/* to the destination. Return a NIL		*/
		FRET (OPC_NIL);
		}
	
	FRET (dest_routes_lptr);
	}


void
dsr_route_cache_all_routes_with_link_delete (DsrT_Route_Cache* route_cache_ptr, InetT_Address address_A, InetT_Address address_B)
	{
	List*				keys_lptr = OPC_NIL;
	int					count, size, index;
	int					num_paths, num_nodes, num_hops;
	List*				dest_routes_lptr = OPC_NIL;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	InetT_Address*		first_addr_ptr;
	InetT_Address*		next_addr_ptr;
	Boolean				route_deleted = OPC_FALSE;
	char*				key_str = OPC_NIL;
	
	/** Deletes all routes that have the link in the hop	**/
	FIN (dsr_route_cache_all_routes_with_link_delete (<args>));
	
	/* Get all the keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	/* Get the size of the table	*/
	num_nodes = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_nodes; count++)
		{
		/* Reinitialize the variables	*/
		size = 0;
		num_paths = 0;
		
		/* Get each destination key	*/
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Access the routes to each destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, key_str);
		
		if (dest_routes_lptr == OPC_NIL)
			{
			/* No routes exist to this destination	*/
			continue;
			}
		
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		while (size < num_paths)	
			{
			/* Initailization	*/
			route_deleted = OPC_FALSE;
			
			/* Get each path and check if the link exists	*/
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, size);
			
			/* Get the size of the path	*/
			num_hops = op_prg_list_size (path_ptr->path_hops_lptr);
			
			for (index = 0; index < (num_hops - 1); index++)
				{
				first_addr_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, index);
				next_addr_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, (index + 1));
				
				if (((inet_address_equal (*first_addr_ptr, address_A)) && (inet_address_equal (*next_addr_ptr, address_B))) ||
					((inet_address_equal (*first_addr_ptr, address_B)) && (inet_address_equal (*next_addr_ptr, address_A))))
					{
					/* The route has this hop	*/
					/* Delete the route			*/
					path_ptr = (DsrT_Path_Info*) op_prg_list_remove (dest_routes_lptr, size);
					dsr_route_cache_entry_mem_free (path_ptr);
					route_cache_ptr->current_cache_size -= 1;
					num_paths--;
					route_deleted = OPC_TRUE;
					
					break;
					}
				} /* For each hop in the route */
			
			if (!route_deleted)
				size++;
			} /* For each route to a destination */
		
		/* Get the number of remaining paths to the destination	*/
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		if (num_paths == 0)
			{
			/* All paths to the destination node were deleted	*/
			/* Remove this node entry from the route cache		*/
			dest_routes_lptr = (List*) prg_string_hash_table_item_remove (route_cache_ptr->route_cache_table, key_str);
			op_prg_list_free (dest_routes_lptr);
			op_prg_mem_free (dest_routes_lptr);
			}
		} /* For each destination node */
	
	/* Update the size of the route cache_statistic	*/
	dsr_route_cache_size_stat_update (route_cache_ptr);
	
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


void
dsr_route_cache_src_node_with_next_hop_routes_delete (DsrT_Route_Cache* route_cache_ptr, InetT_Address next_hop_address)
	{
	List*				keys_lptr = OPC_NIL;
	int					count, size;
	int					num_paths, num_nodes;
	List*				dest_routes_lptr = OPC_NIL;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	InetT_Address*		first_hop_addr_ptr;
	Boolean				route_deleted = OPC_FALSE;
	char*				key_str = OPC_NIL;
	
	/** Deletes all routes that have the next hop	**/
	/** address specified. This function should 	**/
	/** only be called at the source node			**/
	FIN (dsr_route_cache_src_node_with_next_hop_routes_delete (<args>));
	
	/* Get all the keys in the hash table	*/
	keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	/* Get the size of the table	*/
	num_nodes = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_nodes; count++)
		{
		/* Reinitialize the variables	*/
		size = 0;
		num_paths = 0;
		
		/* Get each destination key	*/
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Access the routes to each destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, key_str);
		
		if (dest_routes_lptr == OPC_NIL)
			{
			/* No routes exist to this destination	*/
			continue;
			}
		
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		while (size < num_paths)	
			{
			/* Initailization	*/
			route_deleted = OPC_FALSE;
			
			/* Get each path and check if the link exists	*/
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, size);
			
			/* Access the first hop and check if it is the	*/
			/* next hop node since we are at the source		*/
			first_hop_addr_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, 1);
			
			if (inet_address_equal (*first_hop_addr_ptr, next_hop_address))
				{
				/* The route has this hop	*/
				/* Delete the route			*/
				path_ptr = (DsrT_Path_Info*) op_prg_list_remove (dest_routes_lptr, size);
				dsr_route_cache_entry_mem_free (path_ptr);
				route_cache_ptr->current_cache_size -= 1;
				num_paths--;
				route_deleted = OPC_TRUE;
				}
			
			if (!route_deleted)
				size++;
			} /* For each route to a destination */
		
		/* Get the number of remaining paths to the destination	*/
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		if (num_paths == 0)
			{
			/* All paths to the destination node were deleted	*/
			/* Remove this node entry from the route cache		*/
			dest_routes_lptr = (List*) prg_string_hash_table_item_remove (route_cache_ptr->route_cache_table, key_str);
			op_prg_mem_free (dest_routes_lptr);
			}
		} /* For each destination node */
	
	/* Update the size of the route cache_statistic	*/
	dsr_route_cache_size_stat_update (route_cache_ptr);
	
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


void
dsr_route_cache_export (DsrT_Route_Cache* route_cache_ptr, int PRG_ARG_UNUSED (code))
	{
	static Boolean			log_handle_initialized = OPC_FALSE;
	static Log_Handle		route_cache_log_handle;
	List*					dest_keys_lptr = OPC_NIL;
	int						num_dest, count, size, num_routes;
	char*					dest_key_ptr;
	List*					dest_routes_lptr = OPC_NIL;
	char					message_str [10000] = "";
	char					final_str [100000] = "";
	DsrT_Path_Info*			path_ptr = OPC_NIL;
	char					first_hop_str [8];
	char					last_hop_str [8];
	char					temp_str [1000] = "";
	char*					route_str;
	
	/** This function exports the route cache	**/
	/** to a simulation log message				**/
	FIN (dsr_route_cache_export (<args>));
	
	if (log_handle_initialized == OPC_FALSE)
		{
		/* Initailize the log handle	*/
		route_cache_log_handle = op_prg_log_handle_create (OpC_Log_Category_Results, "DSR", "Route Cache", 100);
		log_handle_initialized = OPC_TRUE;
		}
	
	/* Get the number of destinations in the route cache	*/
	dest_keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	num_dest = op_prg_list_size (dest_keys_lptr);
	
	for (count = 0; count < num_dest; count++)
		{
		dest_key_ptr = (char*) op_prg_list_access (dest_keys_lptr, count);
		
		/* Get the list of route to this destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_key_ptr);
		
		if (dest_routes_lptr == OPC_NIL)
			continue;
		
		/* Get the number of routes	*/
		num_routes = op_prg_list_size (dest_routes_lptr);
		
		for (size = 0; size < num_routes; size++)
			{
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, size);
			
			if (path_ptr->first_hop_external == OPC_TRUE)
				sprintf (first_hop_str, "TRUE");
			else
				sprintf (first_hop_str, "FALSE");
			
			if (path_ptr->last_hop_external == OPC_TRUE)
				sprintf (last_hop_str, "TRUE");
			else
				sprintf (last_hop_str, "FALSE");
			
			route_str = dsr_support_route_print (path_ptr->path_hops_lptr);
			
			if (size == 0)
				sprintf (temp_str, "\t\t\t%-18.2f%-22s%-21s%s\n", path_ptr->installed_time, first_hop_str, last_hop_str, route_str);
			else
				sprintf (temp_str, "\t\t\t\t\t%-18.2f%-22s%-21s%s\n", path_ptr->installed_time, first_hop_str, last_hop_str, route_str);
			
			strcat (message_str, temp_str);
			
			/* Free the memory	*/
			op_prg_mem_free (route_str);
			}
		
		strcpy (temp_str, "-----------------------------------------------------------------------------------------------------\n");
		strcat (message_str, temp_str);
		strcat (final_str , message_str);
		}
	
	op_prg_log_entry_write (route_cache_log_handle, 
		"ROUTE CACHE INFORMATION at time %f \n"
		"\n"
		"========================================================================================================================\n"
		"-                                                   ROUTE CACHE                                                        -\n"
		"========================================================================================================================\n"
		"Destination Node    Time Installed    First Hop External    Last Hop External    Route(s)\n"
		"----------------    --------------    ------------------    -----------------    --------\n"
		"%s\n", op_sim_time (), final_str);
	
	/* Free the keys	*/
	op_prg_list_free (dest_keys_lptr);
	op_prg_mem_free (dest_keys_lptr);
		
	FOUT;
	}
	

void
dsr_route_cache_export_to_ot_initialize (void)
	{
	/** Initialize the OT package	**/	
	FIN (dsr_route_cache_export_to_ot_initialize (void));
	
	/* Open the OT file	*/
	Oms_Ot_File_Open ();
	
	FOUT;
	}


void
dsr_route_cache_export_to_ot_close (void* route_cache_ptr1, int PRG_ARG_UNUSED (code))
	{
	DsrT_Route_Cache* 	route_cache_ptr;
	
	/** Close the OT package	**/
	FIN (dsr_route_cache_export_to_ot_close (void));
	
	route_cache_ptr = (DsrT_Route_Cache*) route_cache_ptr1;
	
	/* Close the file	*/
	Oms_Ot_File_Close ();
	
	FOUT;
	}


void
dsr_route_cache_export_to_ot (void* route_cache_ptr1, int PRG_ARG_UNUSED (code))
	{
	OmaT_Ot_Writer* 			ot_writer_ptr = OPC_NIL;
	static OmaT_Ot_Template 	*ot_route_cache_table_template = OPC_NIL;
	OmaT_Ot_Table       		*ot_route_cache_table = OPC_NIL;
	char						table_name [512] = "";
	List*						dest_keys_lptr;
	int							num_dest, count, size, num_routes, hop_index;
	int							num_hops, hop_count;
	char*						dest_key_ptr;
	List*						dest_routes_lptr = OPC_NIL;
	DsrT_Path_Info*				path_ptr = OPC_NIL;
	char						first_hop_str [8];
	char						last_hop_str [8];
	char						temp_str [1000] = "";
	char						node_name [OMSC_HNAME_MAX_LEN];
	char						hop_addr_str [INETC_ADDR_STR_LEN];
	InetT_Address*				hop_address_ptr;
	InetT_Address				dest_addr;
	DsrT_Route_Cache* 			route_cache_ptr;
	
	/** Export the route cache on a node to the	**/
	/** OT package at various times				**/
	FIN (dsr_route_cache_export_to_ot (<args>));
	
	route_cache_ptr = (DsrT_Route_Cache* ) route_cache_ptr1;
	
	/* Get the number of destinations in the route cache	*/
	dest_keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	num_dest = op_prg_list_size (dest_keys_lptr);
	
	if (num_dest == 0)
		FOUT;
	
	/* Get the source node name	*/
	for (count = 0; count < num_dest; count++)
		{
		/* Access the first route	*/
		dest_key_ptr = (char*) op_prg_list_access (dest_keys_lptr, OPC_LISTPOS_HEAD);
		
		/* Get the list of route to this destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_key_ptr);
		
		if (dest_routes_lptr == OPC_NIL)
			continue;
		
		if (op_prg_list_size (dest_routes_lptr) > 0)
			{
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (
				dest_routes_lptr, OPC_LISTPOS_HEAD);
		
			/* Get the first node address	*/
			if (op_prg_list_size (path_ptr->path_hops_lptr) > 0)	
				{
				hop_address_ptr = (InetT_Address*) op_prg_list_access (
					path_ptr->path_hops_lptr, OPC_LISTPOS_HEAD);
			
				break;
				}
			}
		}
	
	/* Get the hierarchical name of the node	*/
	inet_address_to_hname (*hop_address_ptr, node_name);
	
	/* Get the OT file to write into	*/
	ot_writer_ptr =	Oms_Ot_File_Get ();
	
	/* If the OT writer couild not be obtained then exit the function	*/
	if (ot_writer_ptr == OPC_NIL)
		FOUT;
	
	/* If do not yet have the tamplate then	*/
	/* get the template for this table		*/		
	if (ot_route_cache_table_template == OPC_NIL)
		ot_route_cache_table_template = Oms_Ot_Template_Create (routes_cache_columns_name_array, routes_cache_columns_name_array_size);
	
	/* Create the table name */
	sprintf (table_name, "DSR.Route Cache at %d seconds", (int) op_sim_time ());
	
	/* Create the Route Cache table for this node	*/
	ot_route_cache_table = Oma_Ot_Writer_Table_Add (ot_writer_ptr, OmaC_Table_Type_Site,
								"", node_name, table_name, ot_route_cache_table_template);
	
	for (count = 0; count < num_dest; count++)
		{
		dest_key_ptr = (char*) op_prg_list_access (dest_keys_lptr, count);
		
		/* Get the list of route to this destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_key_ptr);
		
		if (dest_routes_lptr == OPC_NIL)
			continue;
		
		/* Create the IP address from the string	*/
		dest_addr = inet_address_create (dest_key_ptr, InetC_Addr_Family_Unknown);
		
		/* Get the node name	*/
		inet_address_to_hname (dest_addr, node_name);
		
		/* Append the two	*/
		sprintf (temp_str, "%s (%s)", dest_key_ptr, node_name);
		
		/* Free the address	*/
		inet_address_destroy (dest_addr);
		
		/* Write out the destination address */
		Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [0], temp_str);
		
		/* Get the number of routes	*/
		num_routes = op_prg_list_size (dest_routes_lptr);
		
		for (size = 0; size < num_routes; size++)
			{
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, size);
			
			/* Write out the installed time */
			sprintf (temp_str, "%0.2f", path_ptr->installed_time);
			Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [1], temp_str);
			
			if (path_ptr->first_hop_external == OPC_TRUE)
				sprintf (first_hop_str, "TRUE");
			else
				sprintf (first_hop_str, "FALSE");
			
			if (path_ptr->last_hop_external == OPC_TRUE)
				sprintf (last_hop_str, "TRUE");
			else
				sprintf (last_hop_str, "FALSE");
			
			/* Write out the first and last hop external */
			Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [2], first_hop_str);
			Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [3], last_hop_str);
			
			/* Get the number of hops	*/
			num_hops = op_prg_list_size (path_ptr->path_hops_lptr);
			hop_count = num_hops - 1;
			
			/* Write out the hop count	*/
			sprintf (temp_str, "%d", hop_count);
			Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [4], temp_str);
	
			for (hop_index = 0; hop_index < num_hops; hop_index++)
				{
				/* Get each hop from the route	*/
				hop_address_ptr = (InetT_Address*) op_prg_list_access (path_ptr->path_hops_lptr, hop_index);
		
				/* Get the IP address string	*/
				inet_address_ptr_print (hop_addr_str, hop_address_ptr);
		
				/* Get the name of the node	*/
				inet_address_to_hname (*hop_address_ptr, node_name);
		
				sprintf (temp_str, "%s (%s)", hop_addr_str, node_name);
				
				/* Write out each hop	*/
				Oma_Ot_Table_Cell_Value_Set (ot_route_cache_table, routes_cache_columns_name_array [5], temp_str);
				
				/* Write out the rows constructed to each corresponding table		*/
				Oma_Ot_Table_Row_Write (ot_route_cache_table);
				}
			
			/* Write out a blank row */
			Oma_Ot_Table_Row_Write (ot_route_cache_table);
			}
		
		/* Write out a blank row */
		Oma_Ot_Table_Row_Write (ot_route_cache_table);
		}

	/* Free the keys	*/
	op_prg_list_free (dest_keys_lptr);
	op_prg_mem_free (dest_keys_lptr);
	
	/* Destroy the table	*/
	if (ot_route_cache_table != OPC_NIL)
		Oma_Ot_Table_Destroy (ot_route_cache_table);
	
	FOUT;
	}
	

/******************* INTERNAL FUNCTIONS *******************/

static DsrT_Path_Info*
dsr_route_cache_entry_create (List* path_lptr, Boolean first_hop_external, Boolean last_hop_external)
	{
	DsrT_Path_Info*			route_entry_ptr = OPC_NIL;
	InetT_Address*			hop_address_ptr;
	InetT_Address*			copy_address_ptr;
	int						num_hops;
	int						hop_count;
	
	/** Creates a route entry to insert into the route	**/
	/** cache for a specific route from a source node	**/
	/** to a destination node.							**/
	FIN (dsr_route_cache_entry_create (<args>));
	
	/* Allocate memory for the route entry	*/
	route_entry_ptr = dsr_route_cache_entry_mem_alloc ();
	
	/* Get the number of hops	*/
	num_hops = op_prg_list_size (path_lptr);
	
	/* Copy each hop into the array	*/
	for (hop_count = 0; hop_count < num_hops; hop_count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (path_lptr, hop_count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (route_entry_ptr->path_hops_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Set the rest of the variables associated with a route entry	*/
	route_entry_ptr->first_hop_external = first_hop_external;
	route_entry_ptr->last_hop_external = last_hop_external;
	route_entry_ptr->num_hops = num_hops;
	route_entry_ptr->last_access_time = op_sim_time ();
	route_entry_ptr->installed_time = op_sim_time ();
	
	FRET (route_entry_ptr);
	}


static Boolean
dsr_route_cache_route_exists (List* dest_routes_lptr, List* new_path_lptr)
	{
	int					num_routes, count, size;
	int					exist_num_hops, new_num_hops;
	DsrT_Path_Info*		path_info_ptr = OPC_NIL;
	InetT_Address*		exist_hop_address_ptr;
	InetT_Address*		new_hop_address_ptr;
	
	/** Goes through all routes to a destination	**/
	/** and checks if the route to be inserted into	**/
	/** the route cache exists already				**/
	FIN (dsr_route_cache_route_exists (<args>));
	
	/* Get the number of routes to the destination	*/
	num_routes = op_prg_list_size (dest_routes_lptr);
	new_num_hops = op_prg_list_size (new_path_lptr);
	
	for (count = 0; count < num_routes; count++)
		{
		/* Get each route that exists	*/
		path_info_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, count);
		
		/* If the size of the two paths are not	*/
		/* the same, then they are different	*/
		exist_num_hops = op_prg_list_size (path_info_ptr->path_hops_lptr);
			
		if (exist_num_hops != new_num_hops)
			{
			/* This route does not match	*/
			/* the new incoming route		*/
			continue;
			}
		
		for (size = 0; size < exist_num_hops; size++)
			{
			/* Access each hop and compare	*/
			/* that they are the same		*/
			exist_hop_address_ptr = (InetT_Address*) op_prg_list_access (path_info_ptr->path_hops_lptr, size);
			new_hop_address_ptr = (InetT_Address*) op_prg_list_access (new_path_lptr, size);
			
			if (inet_address_ptr_equal (exist_hop_address_ptr, new_hop_address_ptr) == OPC_FALSE)
				{
				/* The hops do not match	*/
				break;
				}
			}
		
		if (size == exist_num_hops)
			{
			/* A matching route is found	*/
			/* in the route cache			*/
			FRET (OPC_TRUE);
			}
		}
	
	FRET (OPC_FALSE);
	}


static Boolean
dsr_route_cache_size_exceed (DsrT_Route_Cache* route_cache_ptr)
	{
	/** This function checks if there is any more	**/
	/** space in the route cache to insert any more	**/
	/** routes. Returns a true if there is no space	**/
	FIN (dsr_route_cache_size_exceed (<args>));
	
	if (route_cache_ptr->max_cache_size == -1)
		{
		/* The size of the route cache is infinity	*/
		FRET (OPC_FALSE);
		}
	
	if (route_cache_ptr->current_cache_size >= route_cache_ptr->max_cache_size)
		{
		/* There is no more space in the route cache	*/
		/* to insert any more routes.					*/
		FRET (OPC_TRUE);
		}

	FRET (OPC_FALSE);
	}


static void
dsr_route_cache_insert_route_sorted (List* paths_lptr, DsrT_Path_Info* path_info_ptr)
	{
	int					num_routes;
	int					count;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	
	/** Inserts the route into the list of routes	**/
	/** based on the sort criteria					**/
	FIN (dsr_route_cache_insert_route_sorted (<args>));
	
	/* Get the number of routes to the destination	*/
	num_routes = op_prg_list_size (paths_lptr);
	
	if (num_routes == 0)
		{
		/* No routes exists to this destination.	*/
		/* Insert the route and exit				*/
		op_prg_list_insert (paths_lptr, path_info_ptr, OPC_LISTPOS_TAIL);
		
		FOUT;
		}
	
	/* The sort criteria is the number of hops.		*/
	/* Place the route with the least number of at	*/
	/* the head of the list							*/
	for (count = 0; count < num_routes; count++)
		{
		path_ptr = (DsrT_Path_Info*) op_prg_list_access (paths_lptr, count);
		
		/* The list of routes is an already sorted	*/
		/* list, so if we find any route that has	*/
		/* more hops, then just insert before that	*/
		/* route and finish the search				*/
		if (path_ptr->num_hops > path_info_ptr->num_hops)
			{
			/* This path has equal or more number 	*/
			/* of hops than the one just about to   */
			/* beinserted. Insert the new path at	*/
			/* this location in the list			*/
			op_prg_list_insert (paths_lptr, path_info_ptr, count);
			
			break;
			}
		}
	
	if (count == num_routes)
		{
		/* This new route has the most number of	*/
		/* hops. Insert it at the tail of the list	*/
		op_prg_list_insert (paths_lptr, path_info_ptr, OPC_LISTPOS_TAIL);
		}
	
	FOUT;
	}
	

static void
dsr_route_cache_insert_route_priority (DsrT_Route_Cache* route_cache_ptr, List* paths_lptr, DsrT_Path_Info* path_info_ptr)
	{
	int					num_routes, num_nodes, num_paths;
	int					count;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	List*				keys_lptr = OPC_NIL;
	char*				key_str = OPC_NIL;
	List*				dest_routes_lptr = OPC_NIL;
	Boolean				multiple_paths_exist = OPC_FALSE;
	int					max_hops = 0;
	double				max_lru_time = 0.0;
	List*				path_delete_lptr = OPC_NIL;
	char*				dest_key_str = OPC_NIL;
	
	/** Determines whether to insert a route into	**/
	/** the route cache as there is no space in the	**/
	/** route cache. Based on various criteria, it 	**/
	/** determines which route to hold and which	**/
	/** one to delete from the route cache			**/
	FIN (dsr_route_cache_insert_route_priority (<args));
	
	/****************************************************************************************/
	/* The order of determining the priority of routes in the route cache are as follows :	*/
	/* 1. MULTIPLE ROUTES TO SAME DESTINATION												*/
	/*		a. If there are multiple routes to the same destination, then delete the route	*/
	/*		   with the maximum number of hops.												*/
	/* 		b. If there are multiple routes with the same number of hops to the same		*/
	/*		   destination, then delete the one which was least recently used				*/
	/* 2. NO ROUTE TO DESTINATION															*/
	/*		a. There exists no route to the destination that needs to be inserted, 			*/
	/* 		   determine the destiantions that have multiple routes and delete the one		*/
	/*		   which has the most number of hops and is least recently used.				*/
	/*		b. If all destinations have only one route, delete the route which is least		*/
	/*		   recently used among all the destinations										*/
	/****************************************************************************************/
	
	/* If the route cache size if zero, then no route can	*/
	/* be inserted into the route cache						*/
	if (route_cache_ptr->max_cache_size == 0)
		FOUT;
	
	/* Determine the number of routes to the destination	*/
	num_routes = op_prg_list_size (paths_lptr);
	
	if (num_routes > 0)
		{
		/* There already exists routes to the destination	*/
		/* The route at the bottom of the list will be the	*/
		/* route with the maximum number of hops and least	*/
		/* recently used.									*/
		path_ptr = (DsrT_Path_Info*) op_prg_list_remove (paths_lptr, OPC_LISTPOS_TAIL);
		dsr_route_cache_entry_mem_free (path_ptr);
		
		/* Insert the new route at the correct location		*/
		dsr_route_cache_insert_route_sorted (paths_lptr, path_info_ptr);
			
		FOUT;
		}
		
	/* There is no route to the destination. Determine		*/
	/* criteria 2 to discard the appropriate route			*/
	
	/* Get all the keys to all destinations in the table	*/
	keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	/* Get the size of the table	*/
	num_nodes = op_prg_list_size (keys_lptr);
	
	for (count = 0; count < num_nodes; count++)
		{
		/* Get each destination key	*/
		key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Access the routes to each destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, key_str);
		
		if (dest_routes_lptr == OPC_NIL)
			continue;
		
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		if (num_paths == 0)
			continue;

		if ((multiple_paths_exist == OPC_TRUE) && (num_paths == 1))
			continue;
		
		if (num_paths > 1)
			multiple_paths_exist = OPC_TRUE;
		
		/* Determine the maximum hops and the least	*/
		/* recently used time for all the paths		*/
		path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, OPC_LISTPOS_TAIL);
			
		if (path_ptr->num_hops < max_hops)
			continue;
		
		if (path_ptr->last_access_time <= max_lru_time)
			continue;
		
		/* Set the values	*/
		max_hops = path_ptr->num_hops;
		max_lru_time = path_ptr->last_access_time;
		path_delete_lptr = dest_routes_lptr;
		dest_key_str = key_str;
		}
	
	path_ptr = (DsrT_Path_Info*) op_prg_list_remove (path_delete_lptr, OPC_LISTPOS_TAIL);
	dsr_route_cache_entry_mem_free (path_ptr);
	
	/* Insert the new route at the correct location		*/
	dsr_route_cache_insert_route_sorted (paths_lptr, path_info_ptr);
	
	/* If the exists no route to the destination just 	*/
	/* delete, remove the list form the hash table		*/
	if (op_prg_list_size (path_delete_lptr) == 0)
		{
		/* All paths to the destination node were deleted	*/
		/* Remove this node entry from the route cache		*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_remove (route_cache_ptr->route_cache_table, dest_key_str);
		op_prg_list_free (dest_routes_lptr);
		op_prg_mem_free (dest_routes_lptr);
		}
	
	/* Free the hash table keys	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	/* The size of the cache table will not change since	*/
	/* a route is deleted before adding another route		*/
	FOUT;
	}
	

static void
dsr_route_cache_all_expired_routes_delete (DsrT_Route_Cache* route_cache_ptr)
	{
	double				current_time;
	double				update_time;
	List*				keys_lptr = OPC_NIL;
	int					num_dest, num_paths;
	int					count, size;
	char*				dest_key_str = OPC_NIL;
	List*				dest_routes_lptr = OPC_NIL;
	DsrT_Path_Info*		path_ptr = OPC_NIL;
	
	/** Deletes all routes in the route cache that have expired	**/
	/** This function should be called every time a route is	**/
	/** either inserted or accessed								**/
	FIN (dsr_route_cache_all_expired_routes_delete (<args>));
	
	/* Get the current time	*/
	current_time = op_sim_time ();
	
	/* Get all the keys to all destinations in the table	*/
	keys_lptr = prg_string_hash_table_keys_get (route_cache_ptr->route_cache_table);
	
	/* Get the number of destinations	*/
	num_dest = op_prg_list_size (keys_lptr);
	
	/* Delete all routes to each destination that have expired	*/
	for (count = 0; count < num_dest; count++)
		{
		/* Initailize the variables	*/
		size = 0;
		
		/* Get the destination key	*/
		dest_key_str = (char*) op_prg_list_access (keys_lptr, count);
		
		/* Get the list of routes to this destination	*/
		dest_routes_lptr = (List*) prg_string_hash_table_item_get (route_cache_ptr->route_cache_table, dest_key_str);
		
		/* Get the number of paths to this destination	*/
		num_paths = op_prg_list_size (dest_routes_lptr);
		
		while (size < num_paths)
			{
			/* Get each path to that destination	*/
			path_ptr = (DsrT_Path_Info*) op_prg_list_access (dest_routes_lptr, size);
			
			/* Determine if this route has expired	*/
			if ((current_time - path_ptr->last_access_time) >= route_cache_ptr->path_expiry_time)
				{
				/* This route has expired. Delete this entry	*/
				path_ptr = (DsrT_Path_Info*) op_prg_list_remove (dest_routes_lptr, size);
				route_cache_ptr->current_cache_size -= 1;
				
				/* Update the route cache size statistic at the	*/
				/* time this route was to be deleted			*/
				update_time = path_ptr->installed_time + route_cache_ptr->path_expiry_time;
				op_stat_write_t (route_cache_ptr->dsr_stat_ptr->route_cache_size_shandle, 
									(double) route_cache_ptr->current_cache_size, update_time);
				
				dsr_route_cache_entry_mem_free (path_ptr);
				num_paths--;
				
				continue;
				}
			
			size++;
			}
		
		/* If there are no routes to this destination	*/
		/* free its memory								*/
		if (op_prg_list_size (dest_routes_lptr) == 0)
			{
			dest_routes_lptr = (List*) prg_string_hash_table_item_remove (route_cache_ptr->route_cache_table, dest_key_str);
			op_prg_list_free (dest_routes_lptr);
			op_prg_mem_free (dest_routes_lptr);
			}
		}
	
	/* Free the keys list	*/
	op_prg_list_free (keys_lptr);
	op_prg_mem_free (keys_lptr);
	
	FOUT;
	}


static void
dsr_route_cache_size_stat_update (DsrT_Route_Cache* route_cache_ptr)
	{
	/** Updates the route cache size statistic	*/
	FIN (dsr_route_cache_size_stat_update (<args>));
	
	/* Update the size of the route cache statistic	*/
	op_stat_write (route_cache_ptr->dsr_stat_ptr->route_cache_size_shandle, (double) route_cache_ptr->current_cache_size);
	
	FOUT;
	}

static void
dsr_route_cache_num_hops_stat_update (DsrT_Route_Cache* route_cache_ptr, List* temp_lptr)
	{
	int							num_hops;
	DsrT_Global_Stathandles*	global_stathandle_ptr = OPC_NIL;
	
	/** Updates the statistic for the number of hops	**/
	/** per route in the route cache					**/
	FIN (dsr_route_cache_num_hops_stat_update (<args>));
	
	/* Get the number of hops	*/
	num_hops = op_prg_list_size (temp_lptr);
	
	/* Get a handle to the global statistics	*/
	global_stathandle_ptr = dsr_support_global_stat_handles_obtain ();
	
	/* The number of hops is one less that the total	*/
	/* number of nodes in the route						*/
	op_stat_write (route_cache_ptr->dsr_stat_ptr->num_hops_shandle, (double) (num_hops - 1));
	
	/* Update the global number of hops per route statistic	*/
	op_stat_write (global_stathandle_ptr->num_hops_global_shandle, (double) (num_hops - 1));
	
	FOUT;
	}

static void
dsr_route_cache_hit_success_stat_update (DsrT_Route_Cache* route_cache_ptr)
	{
	/** Updates the route cache hit statistic on	**/
	/** the number of hits in the route cache when	**/
	/** trying to find a route to the destination	**/
	FIN (dsr_route_cache_hit_success_stat_update (<args>));
	
	op_stat_write (route_cache_ptr->dsr_stat_ptr->route_cache_hit_success_shandle, 1.0);
	
	FOUT;
	}

static void
dsr_route_cache_hit_failure_stat_update (DsrT_Route_Cache* route_cache_ptr)
	{
	/** Updates the route cache hit failure statistic 	**/
	/** when the route cache does not have a route to a **/
	/** particular destination when trying to find a 	**/
	/** route to the destination						**/
	FIN (dsr_route_cache_hit_failure_stat_update (<args>));
	
	op_stat_write (route_cache_ptr->dsr_stat_ptr->route_cache_hit_failure_shandle, 1.0);
	
	FOUT;
	}



/*********** MEMORY ALLOCATION AND DEALLOCATION ***********/
static DsrT_Route_Cache*
dsr_route_cache_mem_alloc (void)
	{
	DsrT_Route_Cache*		route_cache = OPC_NIL;
	
	/** Allocate memory for the route cache table	**/
	FIN (dsr_route_cache_mem_alloc (void));
	
	route_cache = (DsrT_Route_Cache*) op_prg_mem_alloc (sizeof (DsrT_Route_Cache));
	route_cache->route_cache_table = (PrgT_String_Hash_Table*) prg_string_hash_table_create (10, 10);
	route_cache->current_cache_size = 0;
	route_cache->max_cache_size = 0;
	
	FRET (route_cache);
	}

static DsrT_Path_Info*
dsr_route_cache_entry_mem_alloc (void)
	{
	DsrT_Path_Info*		route_cache_entry = OPC_NIL;
	
	/** Allocates memory for a specific path	**/
	FIN (dsr_route_cache_entry_mem_alloc (void));
	
	route_cache_entry = (DsrT_Path_Info*) op_prg_mem_alloc (sizeof (DsrT_Path_Info));
	route_cache_entry->path_hops_lptr = op_prg_list_create ();
	route_cache_entry->first_hop_external = OPC_FALSE;
	route_cache_entry->last_hop_external = OPC_FALSE;
	route_cache_entry->num_hops = 0;
	route_cache_entry->last_access_time = OPC_DBL_UNDEF;
	route_cache_entry->installed_time = OPC_DBL_UNDEF;
	
	FRET (route_cache_entry);
	}


static void
dsr_route_cache_entry_mem_free (DsrT_Path_Info* path_ptr)
	{
	int				count, num_hops;
	InetT_Address*	hop_address;
	
	/** Frees the route cache entry for a specific path	**/
	FIN (dsr_route_cache_entry_mem_free (<args>));
	
	num_hops = op_prg_list_size (path_ptr->path_hops_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		hop_address = (InetT_Address*) op_prg_list_remove (path_ptr->path_hops_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address);
		}
	
	op_prg_mem_free (path_ptr->path_hops_lptr);
	op_prg_mem_free (path_ptr);
	
	FOUT;
	}
