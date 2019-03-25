/* This file contains implementations for functions that are called	*/
/* by various process of the SMART MAC model.						*/

/****************************************/
/*      Copyright (c) 2005-2008       */
/*      by OPNET Technologies, Inc.     */
/*       (A Delaware Corporation)       */
/*    7255 Woodmont Av., Suite 250      */
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.           */
/****************************************/

#include "smart_mac_support.h"

/* Global statistic handlers.						*/
/* Statistic handle for SMART MAC throughput . 		*/
Stathandle	global_throughput_handle;

/* Statistic handle for global SMART MAC load. 		*/
Stathandle	global_load_handle;

/* Statistic handle for SMART MAC dropped data. 	*/
Stathandle	global_dropped_data_handle;

/* Statistic handle for SMART MAC delay. 			*/
Stathandle	global_delay_handle;

/* Statistic handle for SMART MAC number of 		*/
/* reachable nodes. 								*/
Stathandle	global_reachable_set_size_handle;

/* Statistic handle for  SMART MAC contenders count.*/
Stathandle	global_contenders_count_handle;

/* Statistic handle for SMART MAC data dropped due 	*/
/* to hidden node (in bits per second).				*/
Stathandle	global_hidden_node_bps_handle;

/* Statistic handle for SMART MAC data dropped due 	*/
/* to hidden node (in packets per second).			*/
Stathandle	global_hidden_node_ps_handle;

/* Statistic handle for SMART MAC retransmissions. 	*/
Stathandle	global_retx_handle;


/* Categorized memory for SMART MAC buffer elements.*/
Pmohandle higher_layer_buffer_entries_pmh;
	
/* Flag for memory initialization.					*/
Boolean	 higher_layer_buffer_entries_ready_flag = OPC_FALSE;

/* Simulation performance log handler.				*/
Log_Handle	sim_perfomance_log_handle;

/* Flag to initialize log handler.					*/
Boolean    sim_perfomance_log_handle_ready_flag = OPC_FALSE;
	
			
/* Function prototypes.								*/
int smac_list_elem_add_comp (const void* list_elem_ptr1,  const void* list_elem_ptr2)
	{
	SmacT_Contention_Member_Info* 	member_info_ptr1;
	SmacT_Contention_Member_Info* 	member_info_ptr2;
	
	/* This procedure is used in list processing to sort the contenting member list by		*/
	/* the MAC address of the contenders.													*/
	/* It returns 1 if member_info_ptr1 is at a lower address than member_info_ptr2 		*/
	/* 		(closer to list head).															*/
	/* It returns -1 if member_info_ptr1 is at a higher address than member_info_ptr2 		*/
	/* 		(closer to list tail).															*/
	/* It returns 0 if the addresses associated with the two elements are equal.			*/
	FIN (wlan_hld_list_elem_add_comp (list_elem_ptr1, list_elem_ptr2));

	member_info_ptr1 = (SmacT_Contention_Member_Info *) list_elem_ptr1;
	member_info_ptr2 = (SmacT_Contention_Member_Info *) list_elem_ptr2;
	
	if (member_info_ptr1->mac_addr_objid > member_info_ptr2->mac_addr_objid) {FRET (-1);}
	if (member_info_ptr1->mac_addr_objid < member_info_ptr2->mac_addr_objid) {FRET (1);}
	else {FRET (0);}
	}
