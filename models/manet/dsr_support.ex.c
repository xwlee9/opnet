/* dsr_support.ex.c */
/* C support file for DSR support functions */


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
#include <string.h>
#include <dsr.h>
#include <dsr_pkt_support.h>
#include <dsr_ptypes.h>
#include <ip_addr_v4.h>
#include <ip_support.h>
#include <manet.h>
#include <oms_ot_support.h>

/* These arrays hold the names of the different columns					*/
/* in the respective table. The arrays will be used for column names	*/
/* while creating the templates, and will also be used as column tags	*/
/* while wrting into the tables											*/
int			routes_export_columns_name_array_size = 5;
const char*	routes_export_columns_name_array [] 	= {"Time",		"Source Node Name", 	"Destination Node Name", 
														"Hop Count", 	"Route Taken"};

/* Global static variables	*/
static OmaT_Ot_Table       		*ot_routes_report_table = OPC_NIL;

char*
dsr_support_route_print (List* route_lptr)
	{
	char			return_str [100000] = "";
	char*			final_return_str = OPC_NIL;
	char			node_name [OMSC_HNAME_MAX_LEN];
	char			hop_addr_str [INETC_ADDR_STR_LEN];
	char			temp_str [512];
	InetT_Address*	hop_address_ptr;
	int				num_hops, count;
	
	/** Returns a print of the route list passed	**/
	FIN (dsr_support_route_print (<args>));
	
	/* Get the number of hops	*/
	num_hops = op_prg_list_size (route_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		/* Get each hop from the route	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, count);
		
		/* Get the IP address string	*/
		inet_address_ptr_print (hop_addr_str, hop_address_ptr);
		
		/* Get the name of the node	*/
		inet_address_to_hname (*hop_address_ptr, node_name);
		
		sprintf (temp_str, "%s (%s)", hop_addr_str, node_name);
		
		/* Set the return string	*/
		strcat (return_str, temp_str);
		
		if (count != (num_hops - 1))
			strcat (return_str, "--> ");
		}
	
	/* Allocate memory for the return string	*/
	final_return_str = (char*) op_prg_mem_alloc (sizeof (char) * (strlen (return_str) + 1));
	strcpy (final_return_str, return_str);
	
	FRET (final_return_str);
	}


char*
dsr_support_option_route_print (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Request_Option*		route_request_option_ptr;
	DsrT_Route_Reply_Option*		route_reply_option_ptr;
	DsrT_Source_Route_Option*		source_route_option_ptr;
	char*							return_str = OPC_NIL;
	
	/** Prints the route in an option	**/
	FIN (dsr_support_option_route_print (<args>));
	
	switch (dsr_tlv_ptr->option_type)
		{
		case (DSRC_ROUTE_REQUEST):
			{
			/* Print the route request option route	*/
			route_request_option_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
			return_str = dsr_support_route_print (route_request_option_ptr->route_lptr);
			break;
			}
		case (DSRC_ROUTE_REPLY):
			{
			/* Print the route reply option route	*/
			route_reply_option_ptr = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
			return_str = dsr_support_route_print (route_reply_option_ptr->route_lptr);
			break;
			}
		case (DSRC_SOURCE_ROUTE):
			{
			/* Print the source route option route	*/
			source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
			return_str = dsr_support_route_print (source_route_option_ptr->route_lptr);
			break;
			}
		}
	
	FRET (return_str);
	}


char*
dsr_support_route_cache_print_to_string (DsrT_Route_Cache* route_cache_ptr)
	{
	List*					dest_keys_lptr 		= OPC_NIL;
	int						num_dest, count, size, num_routes;
	char*					dest_key_ptr		= OPC_NIL;
	List*					dest_routes_lptr 	= OPC_NIL;
	char					message_str [10000] = "";
	char*					final_str			= OPC_NIL;
	DsrT_Path_Info*			path_ptr 			= OPC_NIL;
	char					first_hop_str [8];
	char					last_hop_str [8];
	char					temp_str [1000] 	= "";
	char*					route_str			= OPC_NIL;
	int						current_elements, add_elements;
	
	/** Prints the route cache on a node to a string	**/
	FIN (dsr_support_route_cache_print_to_string (<args>));
	
	sprintf (message_str, 
		"ROUTE CACHE INFORMATION at time %f \n"
		"\n"
		"========================================================================================================================\n"
		"-                                                   ROUTE CACHE                                                        -\n"
		"========================================================================================================================\n"
		"Destination Node    Time Installed    First Hop External    Last Hop External    Route(s)\n"
		"----------------    --------------    ------------------    -----------------    --------\n", op_sim_time ());
	
	/* Get the size of each string	*/
	add_elements = strlen (message_str);
		
	/* Append the new string to the return string	*/
	final_str = (char*) dsr_array_elements_add (final_str, 0, (add_elements + 1), sizeof (char));
	strcat (final_str, message_str);
	
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
		
		sprintf (message_str, "%s", dest_key_ptr);
		
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
		
		/* Get the size of each string	*/
		current_elements = strlen (final_str);
		add_elements = strlen (message_str);
		
		/* Append the new string to the return string	*/
		final_str = (char*) dsr_array_elements_add (final_str, current_elements, (add_elements + 1), sizeof (char));
		strcat (final_str, message_str);
		}
	
	/* Free the keys	*/
	op_prg_list_free (dest_keys_lptr);
	op_prg_mem_free (dest_keys_lptr);
		
	FRET (final_str);
	}


void
dsr_support_route_print_to_file (IpT_Dgram_Fields* ip_dgram_fd_ptr, List* intermediate_route_lptr)
	{
	static OmsT_Ext_File_Handle		ext_file_handle = OPC_NIL;
	InetT_Address*					hop_address_ptr;
	int								num_intermediate_nodes; 
	int								total_intermediate_hops, total_hops, count;
	char							temp_str [10000];
	char							src_addr_str [INETC_ADDR_STR_LEN];
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							hop_addr_str [INETC_ADDR_STR_LEN];
	char							hop_node_name [OMSC_HNAME_MAX_LEN];
	char							src_str [512];
	char							dest_str [512];
	char							hop_str [512];

	/** Prints the route taken by each source	**/
	/** route packet to a file					**/
	FIN (dsr_support_route_print_to_file (<args>));
	
	/* Determine the total number of hops in the route	*/
	num_intermediate_nodes = op_prg_list_size (intermediate_route_lptr);
	total_intermediate_hops = num_intermediate_nodes - 1;
	
	/* Add the source and destination node hops	*/
	total_hops = total_intermediate_hops + 2;
	
	/* Check if we have file in which to write data	*/
	if (ext_file_handle == OPC_NIL)
		{
		ext_file_handle = Oms_Ext_File_Handle_Get (OMSC_EXT_FILE_PROJ_SCEN_NAME, "dsr_routes", OMSC_EXT_FILE_GDF);
		
		/* Add the header to the file	*/
		strcpy (temp_str, "DSR Routes Report\n");
		Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
		strcpy (temp_str, "-----------------\n\n\n");
		Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
		strcpy (temp_str, "TIME        SOURCE NODE NAME                        DESTINATION NODE NAME                       HOP COUNT       ROUTE TAKEN\n");
		Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
		strcpy (temp_str, "----        ----------------                        ---------------------                       ----------      -----------\n");
		Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
		}
	
	/* Get the source node and destination node names	*/
	
	/* If the source address is invalid, do not export	*/
	if (!inet_address_valid (ip_dgram_fd_ptr->src_addr))
		FOUT;
	
	/* Get the IP address string	*/
	inet_address_print (src_addr_str, ip_dgram_fd_ptr->src_addr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
	
	/* Append the two	*/
	sprintf (src_str, "%s (%s)", src_addr_str, src_node_name);
	
	/* Get the IP address string	*/
	inet_address_print (dest_addr_str, ip_dgram_fd_ptr->dest_addr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
	
	/* Append the two	*/
	sprintf (dest_str, "%s (%s)", dest_addr_str, dest_node_name);
	
	/* Add the first line	*/
	sprintf (temp_str, "%-12.2f%-38s\t%-40s\t%-15d\t%s\n", op_sim_time (), src_str, dest_str, total_hops, src_str);
	Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
	
	for (count = 0; count < num_intermediate_nodes; count++)
		{
		/* Access each hop	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (intermediate_route_lptr, count);
		
		/* Get the IP address string	*/
		inet_address_ptr_print (hop_addr_str, hop_address_ptr);
		
		/* Get the name of the node	*/
		inet_address_to_hname (*hop_address_ptr, hop_node_name);
	
		/* Append the two	*/
		sprintf (hop_str, "%s (%s)", hop_addr_str, hop_node_name);
		
		sprintf (temp_str, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t%s\n", hop_str);
		Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
		}
	
	/* Add the destination	*/
	sprintf (temp_str, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t%s\n", dest_str);
	Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
	
	/* Add a separator	*/
	strcpy (temp_str, "----------------------------------------------------------------------------------------------------------------------------------------\n");
	Oms_Ext_File_Info_Append (ext_file_handle, temp_str);
	
	FOUT;
	}


void
dsr_support_route_print_to_ot_initialize (void)
	{
	OmaT_Ot_Writer* 		ot_writer_ptr = OPC_NIL;
	static Boolean			initialized_flag = OPC_FALSE;
	OmaT_Ot_Template 		*ot_routes_report_table_template = OPC_NIL;
	
	/** Initializes the OT package for routes export	*/
	FIN (dsr_support_route_print_to_ot (<args>));
	
	if (initialized_flag == OPC_FALSE)
		{
		/* Open the OT file	*/
		ot_writer_ptr =	Oms_Ot_File_Open ();
	
		/* If do not yet have the tamplate then */
		/* get the template for this table		*/
		ot_routes_report_table_template = Oms_Ot_Template_Create (routes_export_columns_name_array, routes_export_columns_name_array_size);
		
		/* Create the Routes Report Table for this node	*/
		ot_routes_report_table = Oma_Ot_Writer_Table_Add (ot_writer_ptr, OmaC_Table_Type_Global, "", "", 
											"DSR.DSR Routes Report", ot_routes_report_table_template);
		
		/* Set the flag to true	*/
		initialized_flag = OPC_TRUE;
		}
	
	FOUT;
	}


void
dsr_support_route_print_to_ot_close (void* PRG_ARG_UNUSED (ptr), int PRG_ARG_UNUSED (code))
	{
	static Boolean		closed_flag = OPC_FALSE;
	
	/** Closes the OT file for a route print	**/
	FIN (dsr_support_route_print_to_ot_close (void));
	
	if (closed_flag == OPC_FALSE)
		{
		/* Destroy the table	*/
		if (ot_routes_report_table != OPC_NIL)
			Oma_Ot_Table_Destroy (ot_routes_report_table);
	
		/* Close the file	*/
		Oms_Ot_File_Close ();
		
		/* Set the flag to true	*/
		closed_flag = OPC_TRUE;
		}
	
	FOUT;
	}


void
dsr_support_route_print_to_ot (IpT_Dgram_Fields* ip_dgram_fd_ptr, List* intermediate_route_lptr)
	{
	InetT_Address*					hop_address_ptr;
	int								num_intermediate_nodes; 
	int								total_hops, count;
	char							temp_str [128] = "";
	char							src_addr_str [INETC_ADDR_STR_LEN];
	char							src_node_name [OMSC_HNAME_MAX_LEN];
	char							dest_addr_str [INETC_ADDR_STR_LEN];
	char							dest_node_name [OMSC_HNAME_MAX_LEN];
	char							hop_addr_str [INETC_ADDR_STR_LEN];
	char							hop_node_name [OMSC_HNAME_MAX_LEN];
	char							src_str [512];
	char							dest_str [512];
	char							hop_str [512];
	
	/** Export the routes taken to an output table file	**/	
	FIN (dsr_support_route_print_to_ot (<args>));
	
	/* If the source address is invalid, do not export	*/
	if (!inet_address_valid (ip_dgram_fd_ptr->src_addr))
		FOUT;
	
	/* Get the time string	*/
	sprintf (temp_str, "%f", op_sim_time ());
	
	/* Write out column value */
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [0], temp_str);
	
	/* Get the IP address string	*/
	inet_address_print (src_addr_str, ip_dgram_fd_ptr->src_addr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (ip_dgram_fd_ptr->src_addr, src_node_name);
	
	/* Append the two	*/
	sprintf (src_str, "%s (%s)", src_addr_str, src_node_name);
	
	/* Write out the source node information	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [1], src_str);
	
	/* Get the IP address string	*/
	inet_address_print (dest_addr_str, ip_dgram_fd_ptr->dest_addr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (ip_dgram_fd_ptr->dest_addr, dest_node_name);
	
	/* Append the two	*/
	sprintf (dest_str, "%s (%s)", dest_addr_str, dest_node_name);
	
	/* Write out the destination node information	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [2], dest_str);
	
	/* Determine the total number of hops in the route	*/
	num_intermediate_nodes = op_prg_list_size (intermediate_route_lptr);
	total_hops = num_intermediate_nodes + 1;
	
	/* Write out the number of hops	*/
	sprintf (temp_str, "%d", total_hops);
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [3], temp_str);
	
	/* Write out the first hop as the source node	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [4], src_str);
	
	/* Write out the rows constructed to each corresponding table		*/
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	for (count = 0; count < num_intermediate_nodes; count++)
		{
		/* Access each hop	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (intermediate_route_lptr, count);
		
		/* Get the IP address string	*/
		inet_address_ptr_print (hop_addr_str, hop_address_ptr);
		
		/* Get the name of the node	*/
		inet_address_to_hname (*hop_address_ptr, hop_node_name);
	
		/* Append the two	*/
		sprintf (hop_str, "%s (%s)", hop_addr_str, hop_node_name);
		
		/* Write out each intermediate hop	*/
		Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [4], hop_str);
		
		/* Write out the rows constructed to each corresponding table		*/
		Oma_Ot_Table_Row_Write (ot_routes_report_table);
		}
	
	/* Add the destination	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, routes_export_columns_name_array [4], dest_str);
	
	/* Write out the rows constructed to each corresponding table		*/
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	/* Write out a blank row */
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	FOUT;
	}


Boolean
dsr_support_route_export_sim_attr_get (void)
	{
	static Boolean		val_obtained = OPC_FALSE;
	static Boolean		sim_attr_val = OPC_FALSE;
	
	/** Obtains the simulation attribute	**/
	FIN (dsr_support_route_export_sim_attr_get (void));
	
	if (!val_obtained)
		{
		/* Set the flag	*/
		val_obtained = OPC_TRUE;
		
		if (!op_ima_sim_attr_exists ("DSR Routes Export") ||
			!op_ima_sim_attr_get (OPC_IMA_TOGGLE, "DSR Routes Export", &sim_attr_val))
			{
			/* This simulation attribute does not exist	*/
			/* Do not export the routes to a file		*/
			sim_attr_val = OPC_FALSE;
			}
		}
	
	/* Initialize the file handles */
	Oms_Ext_File_Handle_Get (OMSC_EXT_FILE_PROJ_SCEN_NAME, "dsr_routes", OMSC_EXT_FILE_GDF);
	Oms_Ext_File_Handle_Get (OMSC_EXT_FILE_PROJ_SCEN_NAME, "manet_routes_dump", OMSC_EXT_FILE_GDF);
	
	FRET (sim_attr_val);
	}


Boolean
dsr_support_route_record_sim_attr_get (void)
	{
	static Boolean		val_obtained = OPC_FALSE;
	static Boolean		sim_attr_val = OPC_FALSE;
	
	/** Obtains the simulation attribute	**/
	FIN (dsr_support_route_record_sim_attr_get (void));
	
	if (!val_obtained)
		{
		/* Set the flag	*/
		val_obtained = OPC_TRUE;
		
		if (!op_ima_sim_attr_exists ("DSR Record Routes") ||
			!op_ima_sim_attr_get (OPC_IMA_TOGGLE, "DSR Record Routes", &sim_attr_val))
			{
			/* This simulation attribute does not exist	*/
			/* Do not export the routes to a file		*/
			sim_attr_val = OPC_FALSE;
			}
		}
	
	FRET (sim_attr_val);
	}


DsrT_Global_Stathandles*
dsr_support_global_stat_handles_obtain (void)
	{
	static Boolean		   					stat_handles_registered = OPC_FALSE;
	static DsrT_Global_Stathandles*			stat_handle_ptr = OPC_NIL;
	
	/** Registers the global statistics and returns a	**/
	/** handle to the global statistics					**/
	FIN (dsr_support_global_stat_handles_obtain (void));
	
	if (stat_handles_registered == OPC_FALSE)
		{
		/* The statistic handles have not yet been registered	*/
		/* Register the global statistic handles				*/
		stat_handle_ptr = (DsrT_Global_Stathandles*) op_prg_mem_alloc (sizeof (DsrT_Global_Stathandles));
		
		stat_handle_ptr->route_discovery_time_global_shandle = op_stat_reg ("DSR.Route Discovery Time", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->num_hops_global_shandle = op_stat_reg ("DSR.Number of Hops per Route", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->num_pkts_discard_global_shandle = op_stat_reg ("DSR.Total Packets Dropped", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_rcvd_bps_global_shandle = op_stat_reg ("DSR.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_rcvd_pps_global_shandle = op_stat_reg ("DSR.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_sent_bps_global_shandle = op_stat_reg ("DSR.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_sent_pps_global_shandle = op_stat_reg ("DSR.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_rcvd_bps_global_shandle = op_stat_reg ("DSR.Total Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_rcvd_pps_global_shandle = op_stat_reg ("DSR.Total Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_sent_bps_global_shandle = op_stat_reg ("DSR.Total Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_sent_pps_global_shandle = op_stat_reg ("DSR.Total Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		
		stat_handle_ptr->total_requests_sent_global_shandle = op_stat_reg ("DSR.Total Route Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_non_prop_requests_sent_global_shandle = op_stat_reg ("DSR.Total Non Propagating Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_prop_requests_sent_global_shandle = op_stat_reg ("DSR.Total Propagating Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_replies_sent_global_shandle = op_stat_reg ("DSR.Total Route Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_replies_sent_from_dest_global_shandle = op_stat_reg ("DSR.Total Replies Sent from Destination", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_cached_replies_sent_global_shandle = op_stat_reg ("DSR.Total Cached Replies Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_ack_requests_sent_global_shandle = op_stat_reg ("DSR.Total Acknowledgement Requests Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_acks_sent_global_shandle = op_stat_reg ("DSR.Total Acknowledgements Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_route_errors_sent_global_shandle = op_stat_reg ("DSR.Total Route Errors Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_pkts_salvaged_global_shandle = op_stat_reg ("DSR.Total Packets Salvaged", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
		/* Set the flag to indicate that the statistics	*/
		/* have been registered							*/
		stat_handles_registered = OPC_TRUE;
		}
	
	FRET (stat_handle_ptr);
	}


void*
dsr_array_elements_add (void* array, int count, int add_count, int elem_size)
	{
	void*		new_array;

	/** This procedure allocates a new array of elements of	**/
	/** the specified size and copies the elements of the	**/
	/** original array into the new array.  The old array	**/
	/** is deallocated and the new array is returned.		**/
	FIN (dsr_array_elements_add (<args>));

	/* Allocate the array. If this fails, the called 	*/
	/* procedure will handle the error.					*/
	new_array = op_prg_mem_alloc ((count + add_count) * elem_size);

	/* If there were any elements previously, copy 		*/
	/* them into the new array.							*/
	if (count != 0)
		{
		/* Copy the old array into the new array. */
		op_prg_mem_copy (array, new_array, count * elem_size);

		/* Deallocate the old array. */
		op_prg_mem_free (array);
		}

	/* Return the new array pointer. */
	FRET (new_array);
	}


void
dsr_support_routing_traffic_sent_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (dsr_support_routing_traffic_sent_stats_update (<args>));
	
	/* Get the size of the packet	*/
	pkt_size = op_pk_total_size_get (pkptr);
	
	/* Update the local routing traffic sent stat in bps 	*/
	op_stat_write (stat_handle_ptr->rte_traf_sent_bps_shandle, pkt_size);
	op_stat_write (stat_handle_ptr->rte_traf_sent_bps_shandle, 0.0);
	
	/* Update the local routing traffic sent stat in pps 	*/
	op_stat_write (stat_handle_ptr->rte_traf_sent_pps_shandle, 1.0);
	op_stat_write (stat_handle_ptr->rte_traf_sent_pps_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in bps 	*/
	op_stat_write (global_stathandle_ptr->rte_traf_sent_bps_global_shandle, pkt_size);
	op_stat_write (global_stathandle_ptr->rte_traf_sent_bps_global_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in pps 	*/
	op_stat_write (global_stathandle_ptr->rte_traf_sent_pps_global_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->rte_traf_sent_pps_global_shandle, 0.0);
	
	FOUT;
	}


void
dsr_support_routing_traffic_received_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (dsr_support_routing_traffic_received_stats_update (<args>));
	
	/* Get the size of the packet	*/
	pkt_size = op_pk_total_size_get (pkptr);
	
	/* Update the local routing traffic sent stat in bps 	*/
	op_stat_write (stat_handle_ptr->rte_traf_rcvd_bps_shandle, pkt_size);
	op_stat_write (stat_handle_ptr->rte_traf_rcvd_bps_shandle, 0.0);
	
	/* Update the local routing traffic sent stat in pps 	*/
	op_stat_write (stat_handle_ptr->rte_traf_rcvd_pps_shandle, 1.0);
	op_stat_write (stat_handle_ptr->rte_traf_rcvd_pps_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in bps 	*/
	op_stat_write (global_stathandle_ptr->rte_traf_rcvd_bps_global_shandle, pkt_size);
	op_stat_write (global_stathandle_ptr->rte_traf_rcvd_bps_global_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in pps 	*/
	op_stat_write (global_stathandle_ptr->rte_traf_rcvd_pps_global_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->rte_traf_rcvd_pps_global_shandle, 0.0);
	
	FOUT;
	}


void
dsr_support_total_traffic_sent_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (dsr_support_total_traffic_sent_stats_update (<args>));
	
	/* Get the size of the packet	*/
	pkt_size = op_pk_total_size_get (pkptr);
	
	/* Update the local routing traffic sent stat in bps 	*/
	op_stat_write (stat_handle_ptr->total_traf_sent_bps_shandle, pkt_size);
	op_stat_write (stat_handle_ptr->total_traf_sent_bps_shandle, 0.0);
	
	/* Update the local routing traffic sent stat in pps 	*/
	op_stat_write (stat_handle_ptr->total_traf_sent_pps_shandle, 1.0);
	op_stat_write (stat_handle_ptr->total_traf_sent_pps_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in bps 	*/
	op_stat_write (global_stathandle_ptr->total_traf_sent_bps_global_shandle, pkt_size);
	op_stat_write (global_stathandle_ptr->total_traf_sent_bps_global_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in pps 	*/
	op_stat_write (global_stathandle_ptr->total_traf_sent_pps_global_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_traf_sent_pps_global_shandle, 0.0);
	
	FOUT;
	}


void
dsr_support_total_traffic_received_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (dsr_support_total_traffic_received_stats_update (<args>));
	
	/* Get the size of the packet	*/
	pkt_size = op_pk_total_size_get (pkptr);
	
	/* Update the local routing traffic sent stat in bps 	*/
	op_stat_write (stat_handle_ptr->total_traf_rcvd_bps_shandle, pkt_size);
	op_stat_write (stat_handle_ptr->total_traf_rcvd_bps_shandle, 0.0);
	
	/* Update the local routing traffic sent stat in pps 	*/
	op_stat_write (stat_handle_ptr->total_traf_rcvd_pps_shandle, 1.0);
	op_stat_write (stat_handle_ptr->total_traf_rcvd_pps_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in bps 	*/
	op_stat_write (global_stathandle_ptr->total_traf_rcvd_bps_global_shandle, pkt_size);
	op_stat_write (global_stathandle_ptr->total_traf_rcvd_bps_global_shandle, 0.0);
	
	/* Update the global routing traffic sent stat in pps 	*/
	op_stat_write (global_stathandle_ptr->total_traf_rcvd_pps_global_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_traf_rcvd_pps_global_shandle, 0.0);
	
	FOUT;
	}

void
dsr_support_route_request_sent_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, 
											Boolean non_prop_route_request)
	{
	/** Update the statistics associated with route requests	**/
	FIN (dsr_support_route_request_sent_stats_update (<args>));
	
	/* Based on whether the request is a propagating	*/
	/* or non-propagating request, update the			*/
	/* appropriate statistic							*/
	if (non_prop_route_request)
		{
		/* Update the statistic for non-propagating	*/
		/* route request both locally and globally	*/
		op_stat_write (stat_handle_ptr->total_non_prop_requests_sent_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_non_prop_requests_sent_global_shandle, 1.0);
		}
	else
		{
		/* Update the statistic for propagating		*/
		/* route request both locally and globally	*/
		op_stat_write (stat_handle_ptr->total_prop_requests_sent_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_prop_requests_sent_global_shandle, 1.0);
		}
		
	/* Update the statistics for the total number of	*/
	/* requests sent both locally and globally			*/
	op_stat_write (stat_handle_ptr->total_requests_sent_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_requests_sent_global_shandle, 1.0);
	
	FOUT;
	}
	
void
dsr_support_route_reply_sent_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, 
												Boolean cached_route_reply)
	{
	/** Updates the statistics related to the route replies sent	**/
	FIN (dsr_support_route_reply_sent_stats_update (<args>));
	
	/* Based on whether the route reply is a 	*/
	/* cached route reply or a route reply from	*/
	/* the destination, update the statistic	*/
	if (cached_route_reply)
		{
		/* Update the statistic for the total	*/
		/* number of cached route replies sent	*/
		/* both locally and globally			*/
		op_stat_write (stat_handle_ptr->total_cached_replies_sent_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_cached_replies_sent_global_shandle, 1.0);
		}
	else
		{
		/* Update the statistic for the total	*/
		/* number of route replies sent from 	*/
		/* the destination both local and global*/
		op_stat_write (stat_handle_ptr->total_replies_sent_from_dest_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_replies_sent_from_dest_global_shandle, 1.0);
		}
	
	/* Update the statistic for the total number of	*/
	/* route replies sent both locally and globally	*/
	op_stat_write (stat_handle_ptr->total_replies_sent_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_replies_sent_global_shandle, 1.0);
	
	FOUT;
	}

void
dsr_support_maintenace_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr, 
									Boolean ack_request)
	{
	/** Updates the maintenance related statistics	**/
	FIN (dsr_support_maintenace_stats_update (<args>));
	
	/* Based on whether it is a acknowledgement	*/
	/* request or an acknowledgement, update 	*/
	/* appropriate statistics					*/
	if (ack_request)
		{
		/* Update the statistics for the total	*/
		/* number of acknowledgement requests	*/
		/* sent both locally and globally		*/
		op_stat_write (stat_handle_ptr->total_ack_requests_sent_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_ack_requests_sent_global_shandle, 1.0);
		}
	else
		{
		/* Update the statistics for the total	*/
		/* number of acknowledgementssent both	*/
		/* locally and globally					*/
		op_stat_write (stat_handle_ptr->total_acks_sent_shandle, 1.0);
		op_stat_write (global_stathandle_ptr->total_acks_sent_global_shandle, 1.0);
		}
	
	FOUT;
	}

void
dsr_support_route_error_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr)
	{
	/** Update the statistic for the total number	**/
	/** of route error packets sent 			    **/
	FIN (dsr_support_route_error_stats_update (<args>));
	
	op_stat_write (stat_handle_ptr->total_route_errors_sent_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_route_errors_sent_global_shandle, 1.0);
	
	FOUT;
	}

void
dsr_support_packets_salvaged_stats_update (DsrT_Stathandles* stat_handle_ptr, DsrT_Global_Stathandles* global_stathandle_ptr)
	{
	/** Update the statistic for the total number	**/
	/** of packets salvaged			   			 	**/
	FIN (dsr_support_packets_salvaged_stats_update (<args>));
	
	op_stat_write (stat_handle_ptr->total_pkts_salvaged_shandle, 1.0);
	op_stat_write (global_stathandle_ptr->total_pkts_salvaged_global_shandle, 1.0);
	
	FOUT;
	}
