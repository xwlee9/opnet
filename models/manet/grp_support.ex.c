/* grp_support.ex.c */
/* C support file for GRP support functions */


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
#include <grp.h>
#include <grp_ptypes.h>
#include <ip_addr_v4.h>
#include <ip_support.h>
#include <oms_tan.h>
#include <oms_ot_support.h>


/* These arrays hold the names of the different columns					*/
/* in the respective table. The arrays will be used for column names	*/
/* while creating the templates, and will also be used as column tags	*/
/* while wrting into the tables											*/
int			grp_routes_export_columns_name_array_size = 5;
const char*	grp_routes_export_columns_name_array [] 	= {"Time",		"Source Node Name", 	"Destination Node Name", 
														"Hop Count", 	"Route Taken"};

/* Global static variables	*/
static OmaT_Ot_Table       		*ot_routes_report_table = OPC_NIL;



double
grp_support_nbrhood_size_sim_attr_get (void)
	{
	static Boolean	attr_read = OPC_FALSE;
	static double	grp_neighborhood_size;
	
	/** Reads the neighborhood size simulation	**/
	/** attribute and retains its value			**/
	FIN (grp_support_nbrhood_size_sim_attr_get (void));
	
	if (attr_read == OPC_FALSE)
		{
		/* Read the global attribute	*/
		op_ima_sim_attr_get (OPC_IMA_DOUBLE, "GRP Quadrant Size", &grp_neighborhood_size);
		
		/* Set the flag so that	*/
		/* it is not read again	*/
		attr_read = OPC_TRUE;
		}
	
	FRET (grp_neighborhood_size);
	}


Boolean
grp_support_routes_export_sim_attr_get (void)
	{
	static Boolean	attr_read = OPC_FALSE;
	static Boolean	grp_routes_export;
	
	/** Reads the routes export simulation	**/
	/** attribute and retains its value		**/
	FIN (grp_support_routes_export_sim_attr_get (void));
	
	if (attr_read == OPC_FALSE)
		{
		/* Read the global attribute	*/
		op_ima_sim_attr_get (OPC_IMA_TOGGLE, "GRP Routes Export", &grp_routes_export);
		
		/* Set the flag so that	*/
		/* it is not read again	*/
		attr_read = OPC_TRUE;
		}
	
	FRET (grp_routes_export);
	}


Boolean
grp_support_record_routes_sim_attr_get (void)
	{
	static Boolean	attr_read = OPC_FALSE;
	static Boolean	grp_record_routes;
	
	/** Reads the record routes simulation	**/
	/** attribute and retains its value		**/
	FIN (grp_support_record_routes_sim_attr_get (void));
	
	if (attr_read == OPC_FALSE)
		{
		/* Read the global attribute	*/
		op_ima_sim_attr_get (OPC_IMA_TOGGLE, "GRP Record Routes", &grp_record_routes);
		
		/* Set the flag so that	*/
		/* it is not read again	*/
		attr_read = OPC_TRUE;
		}
	
	FRET (grp_record_routes);
	}


int
grp_support_total_num_nodes_get (void)
	{
	static int 		total_nodes = 0;
	static Boolean	attr_read = OPC_FALSE;
	int				num_fixed_nodes;
	int				num_mobile_nodes;
	
	/** Gets the total number of nodes in the network	**/
	FIN (grp_support_total_num_nodes_get (void));
	
	if (attr_read == OPC_FALSE)
		{
		/* Get the number of nodes in the network	*/
		num_fixed_nodes = op_topo_object_count (OPC_OBJTYPE_NODE_FIX);
		num_mobile_nodes = op_topo_object_count (OPC_OBJTYPE_NODE_MOB);
		total_nodes = num_fixed_nodes + num_mobile_nodes;
		
		attr_read = OPC_TRUE;
		}
	
	FRET (total_nodes);
	}
		

GrpT_Global_Stathandles*
grp_support_global_stat_handles_obtain (void)
	{
	static Boolean		   					stat_handles_registered = OPC_FALSE;
	static GrpT_Global_Stathandles*			stat_handle_ptr = OPC_NIL;
	
	/** Registers the global statistics and returns a	**/
	/** handle to the global statistics					**/
	FIN (grp_support_global_stat_handles_obtain (void));
	
	if (stat_handles_registered == OPC_FALSE)
		{
		/* The statistic handles have not yet been registered	*/
		/* Register the global statistic handles				*/
		stat_handle_ptr = (GrpT_Global_Stathandles*) op_prg_mem_alloc (sizeof (GrpT_Global_Stathandles));
		
		stat_handle_ptr->num_hops_global_shandle = op_stat_reg ("GRP.Number of Hops per Route", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->pkts_discard_total_bps_global_shandle = op_stat_reg ("GRP.Packets Dropped (Total) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->pkts_discard_ttl_bps_global_shandle = op_stat_reg ("GRP.Packets Dropped (TTL expiry) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->pkts_discard_no_nbr_bps_global_shandle = op_stat_reg ("GRP.Packets Dropped (No available neighbor) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->pkts_discard_no_dest_bps_global_shandle = op_stat_reg ("GRP.Packets Dropped (Destination unknown) (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_rcvd_bps_global_shandle = op_stat_reg ("GRP.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_rcvd_pps_global_shandle = op_stat_reg ("GRP.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_sent_bps_global_shandle = op_stat_reg ("GRP.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->rte_traf_sent_pps_global_shandle = op_stat_reg ("GRP.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_rcvd_bps_global_shandle = op_stat_reg ("GRP.Total Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_rcvd_pps_global_shandle = op_stat_reg ("GRP.Total Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_sent_bps_global_shandle = op_stat_reg ("GRP.Total Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_traf_sent_pps_global_shandle = op_stat_reg ("GRP.Total Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_num_backtracks_global_shandle = op_stat_reg ("GRP.Total Number of Backtracks", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		stat_handle_ptr->total_num_quadrant_change_global_shandle = op_stat_reg ("GRP.Total Number of Quadrant Changes", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
		/* Set the flag to indicate that the statistics	*/
		/* have been registered							*/
		stat_handles_registered = OPC_TRUE;
		}
	
	FRET (stat_handle_ptr);
	}



void
grp_support_routing_traffic_sent_stats_update (GrpT_Local_Stathandles* stat_handle_ptr, GrpT_Global_Stathandles* global_stathandle_ptr, OpT_Packet_Size pkt_size)
	{
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (grp_support_routing_traffic_sent_stats_update (<args>));
	
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
grp_support_routing_traffic_received_stats_update (GrpT_Local_Stathandles* stat_handle_ptr, GrpT_Global_Stathandles* global_stathandle_ptr, OpT_Packet_Size pkt_size)
	{
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (grp_support_routing_traffic_received_stats_update (<args>));
	
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
grp_support_total_traffic_sent_stats_update (GrpT_Local_Stathandles* stat_handle_ptr, GrpT_Global_Stathandles* global_stathandle_ptr, OpT_Packet_Size pkt_size)
	{
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (grp_support_total_traffic_sent_stats_update (<args>));
	
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
grp_support_total_traffic_received_stats_update (GrpT_Local_Stathandles* stat_handle_ptr, GrpT_Global_Stathandles* global_stathandle_ptr, OpT_Packet_Size pkt_size)
	{
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (grp_support_total_traffic_received_stats_update (<args>));
	
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
grp_support_traffic_dropped_stats_update (GrpT_Local_Stathandles* stat_handle_ptr, GrpT_Global_Stathandles* global_stathandle_ptr, OpT_Packet_Size pkt_size,
											GrpT_Dropped_Type dropped_type)
	{
	/** Updates the application traffic dropped statistic	**/
	/** in bits/sec. 										**/
	FIN (grp_support_traffic_dropped_stats_update (<args>));
	
	/* Update the local total traffic dropped stat in bps	*/
	op_stat_write (stat_handle_ptr->pkts_discard_total_bps_shandle, pkt_size);
	op_stat_write (stat_handle_ptr->pkts_discard_total_bps_shandle, 0.0);
	
	/* Update the global total traffic dropped stat in bps	*/
	op_stat_write (global_stathandle_ptr->pkts_discard_total_bps_global_shandle, pkt_size);
	op_stat_write (global_stathandle_ptr->pkts_discard_total_bps_global_shandle, 0.0);
	
	switch (dropped_type)
		{
		case (GrpC_TTL_Drop):
			{
			/* Update the local traffic dropped stat due to TTL expiry in bps	*/
			op_stat_write (stat_handle_ptr->pkts_discard_ttl_bps_shandle, pkt_size);
			op_stat_write (stat_handle_ptr->pkts_discard_ttl_bps_shandle, 0.0);
	
			/* Update the global traffic dropped stat due to TTL expiry in bps	*/
			op_stat_write (global_stathandle_ptr->pkts_discard_ttl_bps_global_shandle, pkt_size);
			op_stat_write (global_stathandle_ptr->pkts_discard_ttl_bps_global_shandle, 0.0);
			
			break;
			}
			
		case (GrpC_No_Nbr_Drop):
			{
			/* Update the local traffic dropped stat due no neighbor node in bps	*/
			op_stat_write (stat_handle_ptr->pkts_discard_no_nbr_bps_shandle, pkt_size);
			op_stat_write (stat_handle_ptr->pkts_discard_no_nbr_bps_shandle, 0.0);
	
			/* Update the global traffic dropped stat due no neighbor node in bps	*/
			op_stat_write (global_stathandle_ptr->pkts_discard_no_nbr_bps_global_shandle, pkt_size);
			op_stat_write (global_stathandle_ptr->pkts_discard_no_nbr_bps_global_shandle, 0.0);
			
			break;
			}
			
		case (GrpC_No_Dest_Drop):
			{
			/* Update the local traffic dropped stat due no destination found in bps	*/
			op_stat_write (stat_handle_ptr->pkts_discard_no_dest_bps_shandle, pkt_size);
			op_stat_write (stat_handle_ptr->pkts_discard_no_dest_bps_shandle, 0.0);
	
			/* Update the global traffic dropped stat due no destination found in bps	*/
			op_stat_write (global_stathandle_ptr->pkts_discard_no_dest_bps_global_shandle, pkt_size);
			op_stat_write (global_stathandle_ptr->pkts_discard_no_dest_bps_global_shandle, 0.0);
			
			break;
			}
		
		default:
			{
			break;
			}
		}
	
	FOUT;
	}
	
			

void
grp_support_route_print_to_ot_initialize (void)
	{
	OmaT_Ot_Writer* 		ot_writer_ptr = OPC_NIL;
	static Boolean			initialized_flag = OPC_FALSE;
	OmaT_Ot_Template 		*ot_routes_report_table_template = OPC_NIL;
	
	/** Initializes the OT package for routes export	*/
	FIN (grp_support_route_print_to_ot (<args>));
	
	if (initialized_flag == OPC_FALSE)
		{
		/* Open the OT file	*/
		ot_writer_ptr =	Oms_Ot_File_Open ();
	
		/* If do not yet have the tamplate then */
		/* get the template for this table		*/
		ot_routes_report_table_template = Oms_Ot_Template_Create (grp_routes_export_columns_name_array, grp_routes_export_columns_name_array_size);
		
		/* Create the Routes Report Table for this node	*/
		ot_routes_report_table = Oma_Ot_Writer_Table_Add (ot_writer_ptr, OmaC_Table_Type_Global, "", "", 
											"GRP.GRP Routes Report", ot_routes_report_table_template);
		
		/* Set the flag to true	*/
		initialized_flag = OPC_TRUE;
		}
	
	FOUT;
	}


void
grp_support_route_print_to_ot_close (void* PRG_ARG_UNUSED (ptr), int PRG_ARG_UNUSED (code))
	{
	static Boolean		closed_flag = OPC_FALSE;
	
	/** Closes the OT file for a route print	**/
	FIN (grp_support_route_print_to_ot_close (void));
	
	if (closed_flag == OPC_FALSE)
		{
		/* Destroy the table	*/
		/* if (ot_routes_report_table != OPC_NIL)
			Oma_Ot_Table_Destroy (ot_routes_report_table); */
	
		/* Close the file	*/
		Oms_Ot_File_Close ();
		
		/* Set the flag to true	*/
		closed_flag = OPC_TRUE;
		}
	
	FOUT;
	}


void
grp_support_route_print_to_ot (List* route_lptr, double creation_time)
	{
	InetT_Address*					hop_address_ptr;
	InetT_Address*					src_addr_ptr;
	InetT_Address*					dest_addr_ptr;
	int								total_nodes; 
	int								total_hops, count;
	char							temp_str [128];
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
	FIN (grp_support_route_print_to_ot (<args>));
	
	/* Get the time string	*/
	sprintf (temp_str, "%f", creation_time);
	
	/* Write out column value */
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [0], temp_str);
	
	src_addr_ptr = (InetT_Address*) op_prg_list_access (route_lptr, OPC_LISTPOS_HEAD);
	
	/* Get the IP address string	*/
	inet_address_ptr_print (src_addr_str, src_addr_ptr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (*src_addr_ptr, src_node_name);
	
	/* Append the two	*/
	sprintf (src_str, "%s (%s)", src_addr_str, src_node_name);
	
	/* Write out the source node information	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [1], src_str);
	
	dest_addr_ptr = (InetT_Address*) op_prg_list_access (route_lptr, OPC_LISTPOS_TAIL);
	
	/* Get the IP address string	*/
	inet_address_ptr_print (dest_addr_str, dest_addr_ptr);
		
	/* Get the name of the node	*/
	inet_address_to_hname (*dest_addr_ptr, dest_node_name);
	
	/* Append the two	*/
	sprintf (dest_str, "%s (%s)", dest_addr_str, dest_node_name);
	
	/* Write out the destination node information	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [2], dest_str);
	
	/* Determine the total number of hops in the route	*/
	total_nodes = op_prg_list_size (route_lptr);
	total_hops = total_nodes - 1;
	
	/* Write out the number of hops	*/
	sprintf (temp_str, "%d", total_hops);
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [3], temp_str);
	
	/* Write out the first hop as the source node	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [4], src_str);
	
	/* Write out the rows constructed to each corresponding table		*/
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	for (count = 1; count < total_nodes - 1; count++)
		{
		/* Access each hop	*/
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, count);
		
		/* Get the IP address string	*/
		inet_address_ptr_print (hop_addr_str, hop_address_ptr);
		
		/* Get the name of the node	*/
		inet_address_to_hname (*hop_address_ptr, hop_node_name);
	
		/* Append the two	*/
		sprintf (hop_str, "%s (%s)", hop_addr_str, hop_node_name);
		
		/* Write out each intermediate hop	*/
		Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [4], hop_str);
		
		/* Write out the rows constructed to each corresponding table		*/
		Oma_Ot_Table_Row_Write (ot_routes_report_table);
		}
	
	/* Add the destination	*/
	Oma_Ot_Table_Cell_Value_Set (ot_routes_report_table, grp_routes_export_columns_name_array [4], dest_str);
	
	/* Write out the rows constructed to each corresponding table		*/
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	/* Write out a blank row */
	Oma_Ot_Table_Row_Write (ot_routes_report_table);
	
	FOUT;
	}


/* Neighborhood calculation functions	*/


void
grp_support_neighborhood_determine (double lat_dist_m, double long_dist_m, double quadrant_size, double* low_lat_m, 
								double* low_long_m, double* high_lat_m, double* high_long_m)
	{
	/** Determines the node's current neighborhood	**/
	FIN (grp_support_neighborhood_determine (<args>));
	
	/* Calculate the low co-ordinates of the neighborhood in metres	*/
	*low_lat_m = (floor (lat_dist_m/quadrant_size)) * quadrant_size;
	*low_long_m = (floor (long_dist_m/quadrant_size)) * quadrant_size;
	
	/* Calculate the high co-ordinates of the neighborhood in metres*/
	*high_lat_m = *low_lat_m + quadrant_size;
	*high_long_m = *low_long_m + quadrant_size;
	
	FOUT;
	}


void
grp_support_neighborhood_centre_determine (double lat_dist_m, double long_dist_m, double quadrant_size, 
												double* quad_lat_m, double* quad_long_m)
	{
	double		low_lat_m;
	double		low_long_m;
	
	/** Determines the centre of the quadrant	**/
	FIN (grp_support_neighborhood_centre_determine (<args>));
	
	/* Calculate the low co-ordinates of the neighborhood in metres	*/
	low_lat_m = (floor (lat_dist_m/quadrant_size)) * quadrant_size;
	low_long_m = (floor (long_dist_m/quadrant_size)) * quadrant_size;
	
	/* Calculate the centre coordinates of the quadrant	*/
	*quad_lat_m = low_lat_m + (quadrant_size/2.0);
	*quad_long_m = low_long_m + (quadrant_size/2.0);
	
	FOUT;
	}


void
grp_support_neighborhood_to_flood_determine (double old_lat_m, double old_long_m, double new_lat_m, 
										double new_long_m, double neighborhood_size, double* flood_low_lat_m, double* flood_low_long_m, 
										double* flood_high_lat_m, double* flood_high_long_m)
	{
	double		common_size;
	
	/** Determines the block of neighborhood to flood based	**/
	/** on the old and new co-ordinates of the node. It		**/
	/** returns the lat, long values of the quadrant 2 flood**/
	FIN (grp_support_neighborhood_to_flood_determine (<args>));

	/* First determine the common level where the two	*/
	/* positions fall into the same quadrant.			*/
	common_size = grp_support_common_quadrant_determine (old_lat_m, old_long_m, new_lat_m, new_long_m, neighborhood_size);
	
	grp_support_neighborhood_determine (old_lat_m, old_long_m, common_size, flood_low_lat_m, flood_low_long_m,
										flood_high_lat_m, flood_high_long_m);
	
	FOUT;
	}


double
grp_support_common_quadrant_determine (double old_lat_m, double old_long_m, double new_lat_m, double new_long_m, double neighborhood_size)
	{
	double		common_size;
	int			level = 0;
	double		x1, y1, x2, y2;
	
	/** Determines the common quadrant for both	**/
	/** input positions on the node				**/
	FIN (grp_support_common_quadrant_determine (<args>));
	
	/* Initialize the common neighborhood size	*/
	common_size = neighborhood_size;
	
	/* First determine the common level where the two	*/
	/* positions fall into the same quadrant.			*/
	
	do {
	/* Increment the level for each loop	*/
	/* to find the common level.			*/
	level = level + 1;
	
	/* Double the neighborhood size for		*/
	/* each higher level.					*/
	if (level > 1)
		common_size = common_size * 2.0;
	
	x1 = floor (old_lat_m/common_size);
	y1 = floor (old_long_m/common_size);
	
	x2 = floor (new_lat_m/common_size);
	y2 = floor (new_long_m/common_size);
	
	} while ((x1 != x2) || (y1 != y2));
	
	FRET (common_size);
	}


double
grp_support_highest_neighbor_quadrant_determine (double node_lat_m, double node_long_m, double dest_lat_m, double dest_long_m, 
													double neighborhood_size, double* quad_lat_m_ptr, double* quad_long_m_ptr)
	{
	double			common_level_quad_size;
	double			highest_nbr_quad_size;
	
	/** Determines the highest neighborhood quadrant	**/
	/** that a destination node lies in.				**/
	FIN (grp_support_highest_neighbor_quadrant_determine (<args>));
	
	/* Determine the highest common level	*/
	common_level_quad_size = grp_support_common_quadrant_determine (node_lat_m, node_long_m, dest_lat_m, dest_long_m, neighborhood_size);
	
	/* If the node is in the same lowest level quadrant	*/
	/* then this function is not required to be called	*/
	if (common_level_quad_size == neighborhood_size)
		op_sim_end ("GRP Routing : Nodes are in the same quadrant", OPC_NIL, OPC_NIL, OPC_NIL);
	
	/* The highest neighbor quadrant would	*/
	/* one less than the common quadrant	*/
	highest_nbr_quad_size = common_level_quad_size/2.0;
	
	/* Determine the centre coordinates of the quadrant	*/
	grp_support_neighborhood_centre_determine (dest_lat_m, dest_long_m, highest_nbr_quad_size, quad_lat_m_ptr, quad_long_m_ptr);
	
	FRET (highest_nbr_quad_size);
	}
