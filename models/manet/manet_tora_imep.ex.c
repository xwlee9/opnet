/* manet_tora_imep.ex.c */

/****************************************/
/*      Copyright (c) 1987-2008		*/
/*      by OPNET Technologies, Inc.		*/
/*       (A Delaware Corporation)      	*/
/*    7255 Woodmont Av., Suite 250     	*/
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.          	*/
/****************************************/
#include <opnet.h>

#include <manet_tora_imep.h>
#include "ip_support.h"
#include "ip_rte_support.h"

#define		ToraC_Del_Delay		0.0001

/* Global list variable. */
List*		id_pool_list = OPC_NIL;

/* Animation related global variables */
Pmohandle 	global_imep_header_fields_pmohandle;
Anvid 		Tora_ImepI_Anvid;
Boolean 	tora_animation_requested;
int			tora_animation_refresh_interval;
int			tora_animated_dest_rid;
Boolean 	global_flag_single_ulp;
Objid 		Imep_Subnet_Objid;
List*		global_tora_node_lptr;

/* Structure used for router id auto addressing. */
typedef struct
	{
	int		domain_id;
	int		index;
	Objid	id_pool[ToraC_Max_Domain_Node_Num];
	}
ToraT_Router_ID_Pool;

/* Structure for IP delivery cache. */
typedef struct
	{
	Objid	nd_objid;
	int		last_update_period;
	List*	node_lptr;
	}
ToraT_Ip_Deliver_List_Entry;

/* Local function declaration. */
EXTERN_C_BEGIN
int		tora_imep_sup_status_display (const char* dummy1, void* dummy2);
EXTERN_C_END
	
void 	tora_imep_sup_router_id_auto_assign(ToraT_Router_ID_Pool*, Objid, Objid);


void		
tora_imep_sup_router_id_assign (int domain_id, int node_id, Objid node_objid, Objid module_objid)
	{
	int								pool_num, pool_index, i;
	ToraT_Router_ID_Pool*			pool_ptr = OPC_NIL;
	Boolean							found = OPC_FALSE;
	Objid							tmp_objid;

	/** PURPOSE: Assign a usable node id to each TORA router.		**/
	/** REQUIRES: domain and router id as well as router objid.		**/
	/** EFFECTS: Unique domain and router id combo is assigned. 	**/
	FIN(tora_imep_sup_router_id_assign(domain_id, node_id, node_objid));

	/* See if we have created pool list already. */
	if(id_pool_list == OPC_NIL)
		{
		id_pool_list = op_prg_list_create();
		}

	/* See if we have the id pool struct for the domain. */
	pool_num = op_prg_list_size(id_pool_list);
	for(pool_index=0; pool_index < pool_num; pool_index++)
		{
		pool_ptr = (ToraT_Router_ID_Pool*)op_prg_list_access(id_pool_list, pool_index);

		if(pool_ptr->domain_id == domain_id)
			{
			found = OPC_TRUE;
			break;
			}
		}

	if(!found)
		{
		/* Create an empty pool for the domain. */
		pool_ptr = (ToraT_Router_ID_Pool*)op_prg_mem_alloc(sizeof(ToraT_Router_ID_Pool));

		/* initialize some values. */
		pool_ptr->domain_id = domain_id;
		pool_ptr->index = 0;
		for(i=0; i < ToraC_Max_Domain_Node_Num; i++)
			{
			pool_ptr->id_pool[i] = -1;
			}
		}

	/* The main algorithm to assign id number starts here! */
	if(node_id == ToraC_Router_ID_Auto_Assigned)
		{
		/* Any available node id value from this domain can be assigned. */
		tora_imep_sup_router_id_auto_assign(pool_ptr, node_objid, module_objid);
		}
	else
		{
		/* This node came with a value assigned. */
		/* Sanity check first! */
		if(node_id >= ToraC_Max_Domain_Node_Num)
			{
			op_sim_end("This node has an ID exceeding TORA maximum.  Ending simulation.", 
				OPC_NIL, OPC_NIL, OPC_NIL);
			}

		/* See if some body was auto assigned the value already. */
		if(pool_ptr->id_pool[node_id] != -1)
			{
			/* lets move this to sone other id value. */
			tmp_objid = pool_ptr->id_pool[node_id];
			tora_imep_sup_router_id_auto_assign(pool_ptr, tmp_objid, module_objid);
			}

		/* Take waht is rightfully yours! */
		pool_ptr->id_pool[node_id] = node_objid;
		}

	if(!found)
		{
		/* We have to put this newly created struct into the list. */
		op_prg_list_insert(id_pool_list, pool_ptr, OPC_LISTPOS_TAIL);
		}

	FOUT;
	}


void 
tora_imep_sup_router_id_auto_assign(ToraT_Router_ID_Pool* pool_ptr, Objid node_objid, Objid module_objid)
	{
	Objid				tmp_objid;
	
	/** PURPOSE: Assign any available id to this object.			**/
	/** REQUIRES: Objid and the pool struct.						**/
	/** EFFECTS: Unique domain and router id combo is assigned. 	**/
	FIN(tora_imep_sup_router_id_auto_assign(pool_ptr, node_objid));

	/* Iterate through the pool to find a free id. */
	while((pool_ptr->id_pool[pool_ptr->index] != -1) && (pool_ptr->index < ToraC_Max_Domain_Node_Num))
		{
		pool_ptr->index++;
		}

	if(pool_ptr->index == ToraC_Max_Domain_Node_Num)
		{
		/* There are more than allowed number of nodes in this domain. */
		op_sim_end("Too many nodes with the same domain ID detected.  Ending simulation.", 
			OPC_NIL, OPC_NIL, OPC_NIL);
		}
	else
		{
		/* We found an empty id to give out. */
		pool_ptr->id_pool[pool_ptr->index] = node_objid;
		op_ima_obj_attr_get (module_objid, "manet_mgr.TORA/IMEP Parameters", &tmp_objid);
		tmp_objid = op_topo_child (tmp_objid, OPC_OBJTYPE_GENERIC, 0);
		op_ima_obj_attr_set (tmp_objid, "Router ID", pool_ptr->index);
		}

	FOUT;
	}

Objid
tora_imep_sup_nd_objid_from_rid (int router_id)
	{
	ToraT_Router_ID_Pool*			pool_ptr;
	
	/* Helper function to resolve node objid from assigned router_id. */
	FIN (tora_imep_sup_nd_objid_from_rid (router_id));
	
	/* We expect only single domain for now. */
	pool_ptr = (ToraT_Router_ID_Pool*)op_prg_list_access(id_pool_list, OPC_LISTPOS_HEAD);
	
	FRET (pool_ptr->id_pool[router_id]);
	}
	
void
manet_imep_disp_message (const char* msg0)
	{
	/* Purpose: Print a warning message and possibly continue the simulation. */
	/* Requires: Character string that will be printed out to console 		  */
	/* Effects: None.														  */
	
	FIN (manet_imep_disp_message (const char* msg0, const char* msg1));

	op_sim_message ("\nWarning from Manet-Tora-Imep process:", msg0);

	FOUT;
	}

void
imep_allocate_pooled_memory ()
	{
	static Boolean	init = OPC_FALSE;
	
	/* Purpose: Function allocates the pooled memory handles for the IMEP packet header */
	/*   	    and data fields.														*/
	/* Requires: None.																	*/
	/* Effects: None.																	*/
	FIN (imep_allocate_pooled_memory ())
	
	if (!init)
		{
		/* First define a PMO handle for the header fields */
		global_imep_header_fields_pmohandle = op_prg_pmo_define ("IMEP Header Fields", sizeof (ImepT_Header_Fields_Struct), 256);	
		
		/* Error handle the returned values */
		if ((global_imep_header_fields_pmohandle == OPC_PMO_INVALID))
			{
			manet_imep_disp_message ("Unable to allocate memory for the pooled memory handles.");
			op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
			}
		
		init = OPC_TRUE;
		}
		
	FOUT;	
	}

ImepT_Header_Fields_Struct*
imep_packet_header_field_copy (ImepT_Header_Fields_Struct* input_ptr, int PRG_ARG_UNUSED (size))
	{
	/* Purpose: Function to be used for deep copy of header structure on the imep packet */
	/* Requires: Valid input pointer that needs to be copied 							 */
	/* Effects: Returns a pointer to the newly allocated structure 						 */
	/* Note here that the "size" argument is really not used, its a dummy argument 		 */
	ImepT_Header_Fields_Struct* 			retval = OPC_NIL;
	
	FIN (imep_packet_header_field_copy (ImepT_Header_Fields_Struct* input_ptr, int size))
		
	/* Error check the passed in value */
	if (input_ptr == OPC_NIL)
		FRET (OPC_NIL);
	
	/* Allocate memory for the created structure and error check */
	retval = (ImepT_Header_Fields_Struct*) op_prg_pmo_alloc (global_imep_header_fields_pmohandle);
	if (retval == OPC_NIL)
		{
		manet_imep_disp_message ("Unable to allocate memory for the header structure.");
		op_sim_end ("Terminating simulation in IMEP", OPC_NIL, OPC_NIL, OPC_NIL);
		}

	/* Copy the simple data types first. */
	op_prg_mem_copy (input_ptr, retval, sizeof (ImepT_Header_Fields_Struct));
	
	if (input_ptr->ack_list)
		{
		/* Copy the ack list. */
		retval->ack_list = op_prg_list_create ();
		op_prg_list_elems_copy (input_ptr->ack_list, retval->ack_list);
		}
	
	if (input_ptr->ack_info_ptr)
		{
		/* Copy the ack information. */
		retval->ack_info_ptr = (ImepT_Ack_Elem_Struct*) op_prg_mem_copy_create (input_ptr->ack_info_ptr, sizeof (ImepT_Ack_Elem_Struct));
		}
		
	FRET (retval);	
	}

void
imep_packet_header_field_destroy (ImepT_Header_Fields_Struct* input_ptr)
	{
	/* Purpose: Function required to destroy the header field structure in IMEP packet */
	/* Requires: A valid input structure that needs to be freed up.					   */
	/* Effects: None.																   */

	FIN (imep_packet_header_field_destroy (input_ptr));
	
	if (input_ptr->ack_list)
		{
		/* Destroy the ack list. */
		op_prg_list_free (input_ptr->ack_list);
		op_prg_mem_free (input_ptr->ack_list);
		}
	
	if (input_ptr->ack_info_ptr)
		{
		/* Destroy the ack information. */
		op_prg_mem_free (input_ptr->ack_info_ptr);
		}
	
	/* Destroy the whole thing now. */
	op_prg_mem_free (input_ptr);

	FOUT;	
	}

void
tora_imep_sup_rid_register (Objid node_objid, Objid module_objid, int rid, 
	IpT_Address address, IpT_Interface_Info* lan_intf_info_ptr, int mgr_pro_id, int imep_pro_id, int strm_from_tora_interface)
	{
	static	Boolean					init = OPC_FALSE;
	ToraT_Topo_Struct*				new_topo_struct_ptr;
	
	/** PURPOSE: Insert each TORA node and their info in the global list.			**/
	/** REQUIRES: Node object id, assigned router id and interface ip address.		**/
	/** EFFECTS: List is created and filled. 										**/
	FIN (tora_imep_sup_rid_register (node_objid, module_objid, rid, address, local_net_addr, mgr_pro_id, imep_pro_id));
	
	if (init == OPC_FALSE)
		{
		global_tora_node_lptr = op_prg_list_create ();
		init = OPC_TRUE;
		}
	
	new_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_mem_alloc (sizeof (ToraT_Topo_Struct));
	
	/* Fill in the values to the struct and insert into the list. */
	new_topo_struct_ptr->node_objid = node_objid;
	new_topo_struct_ptr->module_objid = module_objid;
	new_topo_struct_ptr->rid = rid;
	new_topo_struct_ptr->ip_addr = address;
	new_topo_struct_ptr->lan_intf_info_ptr = lan_intf_info_ptr;
	new_topo_struct_ptr->mgr_pro_id = mgr_pro_id;
	new_topo_struct_ptr->imep_pro_id = imep_pro_id;
	new_topo_struct_ptr->stream_from_tora_interface = strm_from_tora_interface;

	op_prg_list_insert (global_tora_node_lptr, new_topo_struct_ptr, OPC_LISTPOS_TAIL);
		
	FOUT;
	}


void
tora_imep_sup_packet_deliver (Packet* pk_ptr, Objid nd_objid)
	{
	static int							max_dist = -1;
	ToraT_Topo_Struct*					tmp_topo_struct_ptr;
	double								tx_alt, tx_lat, tx_lon, tx_x, tx_y, tx_z, 
										rx_alt, rx_lat, rx_lon, rx_x, rx_y, rx_z, distance;
	int									list_size, index, cache_period;
	static List*						node_cache_lptr = OPC_NIL;
	ToraT_Ip_Deliver_List_Entry			*cache_entry_ptr = OPC_NIL, *tmp_cache_entry_ptr;
	IpT_Dgram_Fields*					pk_fd_ptr;
	Boolean								is_unicast;
	static double						ip_deliver_cache_refresh_rate =	-1.0;

	InetT_Address 						compare_address_A;
	InetT_Address 						compare_address_B;
		
	/** PURPOSE: Deliver the packet to the closeby neighbors bypassing lower layers.**/
	/** REQUIRES: packet to send and distance info to neighbors.					**/
	/** EFFECTS: packets will be copied and delivered. 								**/
	FIN (tora_imep_sup_packet_deliver (pk_ptr, node_objid));
	
	if (max_dist == -1)
		{
		/* Access the max distance information. */
		op_ima_sim_attr_get (OPC_IMA_INTEGER, "TORA Max Communication Distance", &max_dist);
		}
	
	if (ip_deliver_cache_refresh_rate == -1.0)
		{
		/* Access the max distance information. */
		op_ima_sim_attr_get (OPC_IMA_DOUBLE, "TORA IMEP Packet Delivery Cache Update", 
			&ip_deliver_cache_refresh_rate);
		}

	if (node_cache_lptr == OPC_NIL)
		{
		node_cache_lptr = op_prg_list_create ();
		}
	
	/* Get the destination. */
	op_pk_nfd_access (pk_ptr, "fields", &pk_fd_ptr);

	compare_address_A = pk_fd_ptr->dest_addr;
	compare_address_B = inet_address_copy (InetI_Imep_Mcast_Addr);
	if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
		{
		is_unicast = OPC_FALSE;
		}
	else
		{
		is_unicast = OPC_TRUE;
		}

	/* Check the cache for the entry for me. */
	cache_period = (int) (op_sim_time () / ip_deliver_cache_refresh_rate);
	list_size = op_prg_list_size (node_cache_lptr);
	for (index=0; index < list_size; index++)
		{
		tmp_cache_entry_ptr = (ToraT_Ip_Deliver_List_Entry*) op_prg_list_access (node_cache_lptr, index);
		
		if (tmp_cache_entry_ptr->nd_objid == nd_objid)
			{
			/* Found the cache entry.*/
			cache_entry_ptr = tmp_cache_entry_ptr;
			
			if (cache_period == cache_entry_ptr->last_update_period)
				{
				/* We can reuse the cached information. */
				list_size = op_prg_list_size (cache_entry_ptr->node_lptr);
				for (index=0; index < list_size; index++)
					{
					tmp_topo_struct_ptr = (ToraT_Topo_Struct*) 
						op_prg_list_access (cache_entry_ptr->node_lptr, index);
					
					/* This neighbor can hear me. */
					if (is_unicast)
						{
						compare_address_A = pk_fd_ptr->dest_addr;
						compare_address_B = inet_address_from_ipv4_address_create (tmp_topo_struct_ptr->ip_addr);
						if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
							{
							/* Matching unicast address. */
							op_ici_install (OPC_NIL);
							op_pk_deliver_delayed (pk_ptr, tmp_topo_struct_ptr->module_objid, tmp_topo_struct_ptr->stream_from_tora_interface, 
								ToraC_Del_Delay);
							
							FOUT;
							}
						}
					else
						{
						op_ici_install (OPC_NIL);
						op_pk_deliver_delayed (op_pk_copy (pk_ptr), 
							tmp_topo_struct_ptr->module_objid, tmp_topo_struct_ptr->stream_from_tora_interface, ToraC_Del_Delay);
						}
					}
				
				op_pk_destroy (pk_ptr);
				
				FOUT;
				}
			else				
				break;				
			}
		}
	
	if (!cache_entry_ptr)
		{
		/* First time for the entry. */
		cache_entry_ptr = (ToraT_Ip_Deliver_List_Entry*) op_prg_mem_alloc 
			(sizeof (ToraT_Ip_Deliver_List_Entry));
		cache_entry_ptr->node_lptr = op_prg_list_create ();
		}
	
	/* Update the cached node list. */
	cache_entry_ptr->last_update_period = cache_period;
	cache_entry_ptr->nd_objid = nd_objid;
	while (op_prg_list_size (cache_entry_ptr->node_lptr))
		{
		/* Remove old information. */
		op_prg_list_remove (cache_entry_ptr->node_lptr, OPC_LISTPOS_HEAD);
		}
	
	/* Access my current position. */
	op_ima_obj_pos_get (nd_objid, &tx_lat, &tx_lon, &tx_alt, &tx_x, &tx_y, &tx_z);
	
	/* Calculate the distance between me and the neighbors in the list. */
	list_size = op_prg_list_size (global_tora_node_lptr);
	for (index=0; index < list_size; index++)
		{
		tmp_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_list_access (global_tora_node_lptr, index);
		
		if (tmp_topo_struct_ptr->node_objid == nd_objid)
			{
			/* It's me. */
			continue;
			}
		
		/* Find the position of the neighbor. */
		op_ima_obj_pos_get (tmp_topo_struct_ptr->node_objid, 
			&rx_lat, &rx_lon, &rx_alt, &rx_x, &rx_y, &rx_z);

		/* Calculate the distance. */
		distance = prg_geo_lat_long_distance_get 
			(tx_lat, tx_lon, tx_alt, rx_lat, rx_lon, rx_alt);
		
		if ((double) max_dist >= distance)
			{
			if (is_unicast)
				{
				compare_address_A = pk_fd_ptr->dest_addr;
				compare_address_B = inet_address_from_ipv4_address_create (tmp_topo_struct_ptr->ip_addr);
				if (inet_address_equal (compare_address_A, compare_address_B) == OPC_TRUE)
					{
					/* Matching unicast address. */
					op_pk_deliver_delayed (op_pk_copy (pk_ptr), 
						tmp_topo_struct_ptr->module_objid, tmp_topo_struct_ptr->stream_from_tora_interface, ToraC_Del_Delay);
					}
				}
			else
				{
				op_pk_deliver_delayed (op_pk_copy (pk_ptr), 
					tmp_topo_struct_ptr->module_objid, tmp_topo_struct_ptr->stream_from_tora_interface, ToraC_Del_Delay);
				}
			
			/* Cache the information. */
			op_prg_list_insert (cache_entry_ptr->node_lptr, tmp_topo_struct_ptr, OPC_LISTPOS_TAIL);
			}
		}
	
	/* Enter the entry to the global list. */
	op_prg_list_insert (node_cache_lptr, cache_entry_ptr, OPC_LISTPOS_TAIL);

	op_pk_destroy (pk_ptr);
	
	FOUT;
	}


void
tora_imep_sup_rid_to_ip_addr (int neighbor_rid, IpT_Address* neighbor_address)
	{
	int							list_size, index;
	ToraT_Topo_Struct*			tmp_topo_struct_ptr;
	
	/** PURPOSE: Get the ip address from the Router ID.	**/
	/** REQUIRES: Global Neighbor list.					**/
	/** EFFECTS: Address is filled and returned. 		**/
	FIN (tora_imep_sup_rid_to_ip_addr (neighbor_rid, neighbor_address));
	
  	/* Find the entry in the global node list. */
	list_size = op_prg_list_size (global_tora_node_lptr);
	for (index=0; index <  list_size; index++)
		{
		tmp_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_list_access 
			(global_tora_node_lptr, index);
		
		if (tmp_topo_struct_ptr->rid == neighbor_rid)
			{
			/* Found it. */
			*neighbor_address = tmp_topo_struct_ptr->ip_addr;
			
			FOUT;
			}
		}
	
	op_sim_end ("error...", OPC_NIL, OPC_NIL, OPC_NIL);
	}

ToraT_Topo_Struct*
tora_imep_sup_ip_addr_to_rid (int* neighbor_rid, IpT_Address neighbor_address)
	{
	/* Purpose: Function returns the router ID for a given IP Address */
	/* Requires: Global neighbor list 								  */
	/* Effects: The neighbor router ID is filled and returned.		  */
	int list_size, counter;
	ToraT_Topo_Struct*	tmp_topo_struct_ptr;
	
	FIN (tora_imep_sup_ip_addr_to_rid (neighbor_rid, neighbor_address))
	
	list_size = op_prg_list_size (global_tora_node_lptr);
	for (counter = 0; counter < list_size; counter ++)
		{
		tmp_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_list_access (global_tora_node_lptr, counter);
		
		if (ip_address_equal (tmp_topo_struct_ptr->ip_addr, neighbor_address) == OPC_TRUE)
			{
			*neighbor_rid = tmp_topo_struct_ptr->rid;
			FRET (tmp_topo_struct_ptr);
			}
		}
	
	/* For error handling */	
	*neighbor_rid = -1;
	
	FRET (OPC_NIL);	
	}

ToraT_Topo_Struct*
tora_imep_lan_addr_to_router (IpT_Address dest_address)
	{
	/* Purpose: Function returns the router ID for a given LAN address */
	/* Requires: Global neighbor list 								   */
	/* Effects: The neighbor router ID is filled and returned.		   */
	int list_size, counter;
	ToraT_Topo_Struct*	tmp_topo_struct_ptr;
	
	FIN (tora_imep_sup_lan_addr_to_router (dest_address))
	
	list_size = op_prg_list_size (global_tora_node_lptr);
	for (counter = 0; counter < list_size; counter ++)
		{
		tmp_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_list_access (global_tora_node_lptr, counter);
		
		if (tmp_topo_struct_ptr->lan_intf_info_ptr)
			{
			/* Check and see if the dest fall into the range. */
			if (ip_address_range_check (dest_address, ip_rte_intf_addr_range_get 
				(tmp_topo_struct_ptr->lan_intf_info_ptr)) == OPC_TRUE)
				{
				FRET (tmp_topo_struct_ptr);
				}
			}
		}
		
	 FRET (OPC_NIL);
	}

void
tora_imep_sup_debug_comm_reg (void)
	{
	static Boolean	init = OPC_FALSE;
	
	/* Register debugging command ODB. */
	if (!init)
		{
		op_prg_odb_command_register ("manet_tora",
			"manet_tora", "", "", "", tora_imep_sup_status_display,
			OPC_NIL, OPC_NIL);
		
		init = OPC_TRUE;
		}
	}

int
tora_imep_sup_status_display (const char* PRG_ARG_UNUSED (dummy1), void* PRG_ARG_UNUSED (dummy2))
	{
	int 							list_size, index;
	ToraT_Topo_Struct*				tmp_topo_struct_ptr;
	char							ip_addr [32], node_name [256];
	
	/** PURPOSE: Display some info about the tora managers in the net.		**/
	/** REQUIRES: Global topo list.											**/
	/** EFFECTS: Output to the ODB prompt. 									**/
	FIN (tora_imep_sup_status_display (void));
	
	printf ("\n\t*** TORA Node Info Table ***");
	printf ("\n\tNode Name\tMgr PID\tIMEP PID\tRouter ID\tIP Address");
	printf ("\n\t=======================================================");
	
	if (global_tora_node_lptr)
		{
		list_size = op_prg_list_size (global_tora_node_lptr);
	
		for (index=0; index <  list_size; index++)
			{
			tmp_topo_struct_ptr = (ToraT_Topo_Struct*) op_prg_list_access 
				(global_tora_node_lptr, index);
	
			ip_address_print (ip_addr, tmp_topo_struct_ptr->ip_addr);
			op_ima_obj_attr_get (tmp_topo_struct_ptr->node_objid, "name", node_name);
			printf ("\n\t%s\t%d\t%d\t%d\t%s", node_name, tmp_topo_struct_ptr->mgr_pro_id,
				tmp_topo_struct_ptr->imep_pro_id, tmp_topo_struct_ptr->rid, ip_addr);
			}
		}
	
	FRET (0);
	}

void
tora_imep_support_animation_init ()
	{
	char 									net_name [128];
	char 									subnet_name [128];
	static Boolean 							animation_init = OPC_FALSE;

	/* Purpose: Initialization function for the animation of Tora and Imep */
	/* Requires: Custom animation probe created.						   */
	/* Effects: Gets the viewer ID for animation in Tora and Imep.		   */
	FIN (tora_imep_support_animation_init ());
	
	if (!animation_init)
		{		
		/* Done with the initializations - just go ahead and reset the bool variable*/
		animation_init = OPC_TRUE;

		Tora_ImepI_Anvid = op_anim_lprobe_anvid ("MANET");
		
		if ((OPC_ANIM_ANVID_NONE == Tora_ImepI_Anvid) ||
			(OPC_ANIM_ID_NIL == Tora_ImepI_Anvid))	
			{
			tora_animation_requested = OPC_FALSE;
			FOUT;
			}
		else
			{
			/* Correction: get net_name from sim attribute rather than sim info		*/
			/*op_sim_info_get (OPC_STRING, OPC_SIM_INFO_OUTPUT_FILE_NAME, net_name);*/
			op_ima_sim_attr_get (OPC_IMA_STRING, "net_name", net_name);
			
			Imep_Subnet_Objid = op_topo_child (0, OPC_OBJMTYPE_SITE, 0);
			op_ima_obj_attr_get (Imep_Subnet_Objid, "name", &subnet_name);
			op_anim_ime_nmod_draw (Tora_ImepI_Anvid, OPC_ANIM_MODTYPE_NETWORK, net_name, subnet_name,
				OPC_ANIM_MOD_OPTION_NONE, OPC_ANIM_DEFPROPS);
			
			/* Refresh rate and TORA destination. */
			op_ima_sim_attr_get (OPC_IMA_INTEGER, "TORA IMEP Animation Refresh Interval",
				&tora_animation_refresh_interval);
			op_ima_sim_attr_get (OPC_IMA_INTEGER, "TORA Animated Destination RID",
				&tora_animated_dest_rid);
			
			tora_animation_requested = OPC_TRUE;
			}		
		}
	
	FOUT;
	}

void
tora_imep_sup_draw_arrow_by_position (int src_vx, int src_vy, int dest_vx, int dest_vy, 
	int props, int arrow_width, int	offset, Boolean double_thickness, Andid did_array[], Andid did_array_2[])
	{
	double										slope;
	int											x_min, y_min, x_max, y_max;
	int											start_line_x, start_line_y, end_line_x, end_line_y;
	int											upper_arrow_x, upper_arrow_y, lower_arrow_x;
	int											lower_arrow_y, src_remainder, dest_remainder;
	int											quarter_x, quarter_y, quarter_line_x;
	int											quarter_line_y, quarter_remainder;
	int											eighth_x, eighth_y, eighth_line_x;
	int											eighth_line_y, eighth_remainder;
	int											mid_x, mid_y, mid_line_x, mid_line_y, mid_remainder;
	int											start_line_y_d, start_line_x_d;
	int											eighth_line_y_d, eighth_line_x_d;
	int											upper_arrow_y_d, upper_arrow_x_d;
	int											lower_arrow_y_d, lower_arrow_x_d, eighth_line_y_d_l, eighth_line_x_d_l;

	Boolean										x_axis = OPC_FALSE;
	Boolean										vertical_slope = OPC_FALSE;
	
	/* Function to draw pipe based on the parameters passed to it. */
	FIN (tora_imep_sup_draw_arrow_by_position  (params..));
	
	/* Draw intelligently. */
	if (src_vx < dest_vx)
		{
		x_min = src_vx; x_max = dest_vx;
		/* src is on the left of dest. */
		if (src_vy < dest_vy)
			{
			/* src is nw of dest. */
			y_min = src_vy; y_max = dest_vy;
			slope = (double)(y_max - y_min) / (double)(x_max - x_min);
			x_axis = (slope > 1.0) ? OPC_FALSE : OPC_TRUE;
			
			quarter_y = dest_vy - (dest_vy - src_vy) / 4;
			eighth_y = dest_vy - (dest_vy - src_vy) / 8;
			}
			
		if (src_vy == dest_vy)
			{
			/* src and dest on the same horizontal line. */
			y_min = dest_vy; y_max = src_vy;
			slope = 0.0;
			x_axis = OPC_TRUE;
			
			eighth_y = quarter_y = src_vy;
			}
	
		if (src_vy > dest_vy)
			{
			/* src is sw of dest. */
			y_min = dest_vy; y_max = src_vy;
			slope = -(double)(y_max - y_min) / (double)(x_max - x_min);
			x_axis = (slope < - 1.0) ? OPC_FALSE : OPC_TRUE;
					
			quarter_y = (src_vy - dest_vy) / 4 + dest_vy;
			eighth_y = (src_vy - dest_vy) / 8 + dest_vy;
			}
		
		quarter_x = dest_vx - (dest_vx - src_vx) / 4;
		eighth_x = dest_vx - (dest_vx - src_vx) / 8;
		}
		
	if (src_vx == dest_vx)
		{
		vertical_slope = OPC_TRUE;
		x_axis = OPC_FALSE;
		x_min = src_vx; x_max = dest_vx;
		/* Src and dest on the same vertical line. */
		if (src_vy == dest_vy)
			{
			/* src and dest on the same position */
			FOUT;
			}
			
		if (src_vy > dest_vy)
			{
			y_min = dest_vy; y_max = src_vy;
				
			quarter_y = (src_vy - dest_vy) / 4 + dest_vy;
			eighth_y = (src_vy - dest_vy) / 8 + dest_vy;
			}
		else
			{
			y_max = dest_vy; y_min = src_vy;
				
			quarter_y = dest_vy - (dest_vy - src_vy) / 4;
			eighth_y = dest_vy - (dest_vy - src_vy) / 8;
			}

		eighth_x = quarter_x = src_vx;
		}

	if (src_vx > dest_vx)
		{
		x_min = dest_vx; x_max = src_vx;
		/* Src is on the right of dest. */
		if (src_vy < dest_vy)
			{
			/* src is ne of dest. */
			y_min = src_vy; y_max = dest_vy;
			slope = - (double)(y_max - y_min) / (double)(x_max - x_min);
			x_axis = (slope < -1.0) ? OPC_FALSE : OPC_TRUE;
						
			quarter_y = dest_vy - (dest_vy - src_vy) / 4;
			eighth_y = dest_vy - (dest_vy - src_vy) / 8;
			}
			
		if (src_vy == dest_vy)
			{
			/* src and dest on the same horizontal line. */
			y_min = src_vy; y_max = dest_vy;
			slope = 0.0;
			x_axis = OPC_TRUE;
			
			eighth_y = quarter_y = src_vy;
			}
	
		if (src_vy > dest_vy)
			{
			/* src is se of dest. */
			y_min = dest_vy; y_max = src_vy;
			slope = (double)(y_max - y_min) / (double)(x_max - x_min);
			x_axis = (slope > 1.0) ? OPC_FALSE : OPC_TRUE;
			
			quarter_y = (src_vy - dest_vy) / 4 + dest_vy;
			eighth_y = (src_vy - dest_vy) / 8 + dest_vy;
			}
				
		quarter_x = (src_vx - dest_vx) / 4 + dest_vx;
		eighth_x = (src_vx - dest_vx) / 8 + dest_vx;
		}
			
	/* Caluclate the midpoints. */
	mid_x = (x_max - x_min) / 2 + x_min;
	mid_y = (y_max - y_min) / 2 + y_min;
	
	/* Calculate the endpoints offset amount from the original between the center points. */
	if (x_axis)
		{
		if (slope != 0.0)
			{
			/* Calculate the diagonal line equation at the source point. */
			src_remainder = src_vy + src_vx / slope;

			/* Calculate the line start point. */
			start_line_y = src_vy + offset;
			start_line_x = slope * (src_remainder - start_line_y);
			
			/* Calculate the diagonal line equation at the dest point. */
			dest_remainder = dest_vy + dest_vx / slope;

			/* Now the end point x, y. */
			end_line_y = dest_vy + offset;
			end_line_x = slope * (dest_remainder - end_line_y);
			
			/* Find the mid point on the line. */
			mid_remainder = mid_y +mid_x / slope;
			
			/* Now the quarter point x, y. */
			mid_line_y = mid_y + offset;
			mid_line_x = slope * (mid_remainder - mid_line_y);
		
			/* Find the quarter point on the line. */
			quarter_remainder = quarter_y + quarter_x / slope;
			
			/* Now the quarter point x, y. */
			quarter_line_y = quarter_y + offset;
			quarter_line_x = slope * (quarter_remainder - quarter_line_y);
				
			/* Find the eighth point on the line. */
			eighth_remainder = eighth_y + eighth_x / slope;
			
			/* Now the quarter point x, y. */
			eighth_line_y = eighth_y + offset;
			eighth_line_x = slope * (eighth_remainder - eighth_line_y);

			/* To find the tip of the arrows... */
			upper_arrow_y = quarter_line_y + arrow_width;
			upper_arrow_x = slope * (quarter_remainder - upper_arrow_y);
			lower_arrow_y = quarter_line_y - arrow_width;
			lower_arrow_x = slope * (quarter_remainder - lower_arrow_y);
			
			}
		else
			{
			/* Slope is zero. */
			start_line_y = src_vy + offset;
			start_line_x = src_vx;
			end_line_y = dest_vy + offset;
			end_line_x = dest_vx;
			quarter_line_y = quarter_y + offset;
			quarter_line_x = quarter_x;
			mid_line_y = mid_y + offset;
			mid_line_x = mid_x;
			eighth_line_y = eighth_y + offset;
			eighth_line_x = eighth_x;
			upper_arrow_y = quarter_line_y + arrow_width;
			upper_arrow_x = quarter_x;
			lower_arrow_y = quarter_line_y - arrow_width;
			lower_arrow_x = quarter_x;
			}
					
		/* Optional double thickness calculation. */
		start_line_y_d = start_line_y + 1;
		start_line_x_d = start_line_x;
		eighth_line_y_d = eighth_line_y + 1;
		eighth_line_x_d = eighth_line_x;
		eighth_line_y_d_l = eighth_line_y - 1;
		eighth_line_x_d_l = eighth_line_x;
		upper_arrow_y_d = upper_arrow_y + 1;
		upper_arrow_x_d = upper_arrow_x;
		lower_arrow_y_d = lower_arrow_y - 1;
		lower_arrow_x_d = lower_arrow_x;
		}
	else 
		{
		if (!vertical_slope)
			{
			/* Calculate the diagonal line equation at the source point. */
			src_remainder = src_vy + src_vx / slope;
		
			/* Calculate the line start point. */
			start_line_x = src_vx + offset;
			start_line_y = - start_line_x / slope + src_remainder;
		
			/* Calculate the diagonal line equation at the dest point. */
			dest_remainder = dest_vy + dest_vx / slope;

			/* Now the end point x, y. */
			end_line_x = dest_vx + offset;
			end_line_y = - end_line_x / slope + dest_remainder;
		
			/* Find the mid point on the line. */
			mid_remainder = mid_y + mid_x / slope;

			/* Now the mid point x, y. */
			mid_line_x = mid_x + offset;
			mid_line_y = - mid_line_x / slope + mid_remainder;
		
			/* Find the quarter point on the line. */
			quarter_remainder = quarter_y + quarter_x / slope;

			/* Now the end point x, y. */
			quarter_line_x = quarter_x + offset;
			quarter_line_y = - quarter_line_x / slope + quarter_remainder;
		
			/* Find the eighth point on the line. */
			eighth_remainder = eighth_y + eighth_x / slope;

			/* Now the eighth end point x, y. */
			eighth_line_x = eighth_x + offset;
			eighth_line_y = - eighth_line_x / slope + eighth_remainder;

			/* To find the tip of the arrows... */
			upper_arrow_x = quarter_line_x + arrow_width;
			upper_arrow_y = - upper_arrow_x / slope + quarter_remainder;
			lower_arrow_x = quarter_line_x - arrow_width;
			lower_arrow_y = - lower_arrow_x / slope + quarter_remainder;
			}
		else
			{
			/* Vertical slope. */
			start_line_x = src_vx + offset;
			start_line_y = src_vy;
			
			end_line_x = dest_vx + offset;
			end_line_y = dest_vy;
		
			/* Now the mid point x, y. */
			mid_line_x = mid_x + offset;
			mid_line_y = mid_y;
		
			/* Now the quarter point x, y. */
			quarter_line_x = quarter_x + offset;
			quarter_line_y = quarter_y;
			
			/* Now the eighth point x, y. */
			eighth_line_x = eighth_x + offset;
			eighth_line_y = eighth_y;
		
			/* To find the tip of the arrows... */
			upper_arrow_x = quarter_line_x + arrow_width;
			upper_arrow_y = quarter_line_y;
			lower_arrow_x = quarter_line_x - arrow_width;
			lower_arrow_y = quarter_line_y;
			}
				
		/* Optional double thickness calculation. */
		start_line_y_d = start_line_y;
		start_line_x_d = start_line_x + 1;
		eighth_line_y_d = eighth_line_y;
		eighth_line_x_d = eighth_line_x + 1;
		eighth_line_y_d_l = eighth_line_y;
		eighth_line_x_d_l = eighth_line_x - 1;
		upper_arrow_y_d = upper_arrow_y;
		upper_arrow_x_d = upper_arrow_x + 1;
		lower_arrow_y_d = lower_arrow_y;
		lower_arrow_x_d = lower_arrow_x - 1;
		}
		
	/* Draw and store the id. */
	did_array [0] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, start_line_x, 
		start_line_y, eighth_line_x, eighth_line_y);
	did_array [1] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, upper_arrow_x, 
		upper_arrow_y, eighth_line_x, eighth_line_y);
	did_array [2] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, lower_arrow_x, 
		lower_arrow_y, eighth_line_x, eighth_line_y);
	
	/* Optional double thickness. */
	if (double_thickness)
		{
		did_array_2 [0] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, start_line_x_d, 
			start_line_y_d, eighth_line_x_d, eighth_line_y_d);
		did_array_2 [1] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, upper_arrow_x_d, 
			upper_arrow_y_d, eighth_line_x_d, eighth_line_y_d);
		did_array_2 [2] = op_anim_igp_line_draw (Tora_ImepI_Anvid, props, lower_arrow_x_d, 
			lower_arrow_y_d, eighth_line_x_d_l, eighth_line_y_d_l);
		}
	
	FOUT;
	}
   



