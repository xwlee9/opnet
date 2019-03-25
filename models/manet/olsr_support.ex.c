/* olsr_support.ex.c */

/****************************************/
/*      Copyright (c) 2004-2008         */
/*     by OPNET Technologies, Inc.      */
/*      (A Delaware Corporation)        */
/*   7255 Woodmont Av., Suite 250       */
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.           */
/****************************************/

#include <olsr_support.h>

/* Declaes the strcture for interface set table and interface set entry */
/* with corresponding functions to register and access the values from */
/* the table */

static PrgT_Bin_Hash_Table* 	interface_set_table = OPC_NIL;

static OlsrT_Interface_Set_Entry*	olsr_support_interface_set_entry_mem_alloc (void);

void
olsr_support_multiple_interface_register (int intf_addr, void* addr, int type)
	{
	OlsrT_Interface_Set_Entry*		intf_set_entry_ptr = OPC_NIL;
	int							key_addr;
	void*							old_contents;
	int								main_addr;
	List*							intf_addr_lptr = OPC_NIL;
	
	FIN (olsr_support_multiple_interface_register (<args>));
	
	if (type == 1)
		{
		/* Second argument is main addr of the node */
		main_addr = *((int*) addr);
		key_addr = intf_addr;
		}
	else
		{
		/* Second argument is list of intf addr and first argument is main address */
		intf_addr_lptr = (List*) addr;
		main_addr = intf_addr;
		key_addr = main_addr;
		}
	
	if (interface_set_table == OPC_NIL)
		{
		/* Create the hash table */
		/* This should happen only for the first register function call */
		interface_set_table = (PrgT_Bin_Hash_Table*) prg_bin_hash_table_create (9, 4);
		}
	
	/* check if there's already an entry for this interface */
	intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) prg_bin_hash_table_item_get (interface_set_table, &key_addr);
	
	if (intf_set_entry_ptr == OPC_NIL)
		{
		/* Create a new entry */
		intf_set_entry_ptr = olsr_support_interface_set_entry_mem_alloc ();
		}
			
	intf_set_entry_ptr->intf_addr = intf_addr;
	intf_set_entry_ptr->main_addr = main_addr;
	intf_set_entry_ptr->intf_addr_lptr = intf_addr_lptr;
	intf_set_entry_ptr->type = type;
		
	prg_bin_hash_table_item_insert (interface_set_table, &key_addr, intf_set_entry_ptr, &old_contents);
	
	FOUT;
	}
	

int
olsr_support_main_addr_get (int intf_addr)
	{
	OlsrT_Interface_Set_Entry*		intf_set_entry_ptr;

	/** Gets the main address corresponding to this intf addr	**/
	FIN (olsr_support_main_addr_get (int));
				
	/* Get the entry for this intf_addr	*/
	intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) prg_bin_hash_table_item_get (interface_set_table, &intf_addr);

	/* if node has only one interface return the same address */
	FRET (intf_set_entry_ptr ? intf_set_entry_ptr->main_addr : intf_addr);
	}


PrgT_Bin_Hash_Table*
olsr_support_interface_table_ptr_get (void)
	{
	/* Returns the handle to interface set table */
	FIN (olsr_support_interface_table_ptr_get (void));
	
	FRET (interface_set_table);
	}

void
olsr_support_routing_traffic_sent_stats_update 
	(OlsrT_Local_Stathandles* stat_handle_ptr, 
		OlsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (olsr_support_routing_traffic_sent_stats_update (<args>));
	
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
olsr_support_routing_traffic_received_stats_update 
	(OlsrT_Local_Stathandles* stat_handle_ptr, 
		OlsrT_Global_Stathandles* global_stathandle_ptr, Packet* pkptr)
	{
	OpT_Packet_Size			pkt_size;
	
	/** Updates the routing traffic sent statistics	**/
	/** in both bits/sec and pkts/sec				**/
	FIN (olsr_support_routing_traffic_received_stats_update (<args>));
	
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


/* Internally callable function */
static OlsrT_Interface_Set_Entry*
olsr_support_interface_set_entry_mem_alloc (void)
	{
	OlsrT_Interface_Set_Entry* 		intf_set_entry_ptr = OPC_NIL;
	static Pmohandle				intf_set_entry_pmh;
	static Boolean					intf_set_entry_pmh_defined = OPC_FALSE;
	
	FIN (olsr_rte_mpr_set_entry_mem_alloc (void));
	
	/** Allocates pooled memory for a interface set entry	**/
	
	if (intf_set_entry_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for intf set entry	*/
		intf_set_entry_pmh = op_prg_pmo_define ("Interface Set Entry", sizeof (OlsrT_Interface_Set_Entry), 32);
		intf_set_entry_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the mpr set entry from the pooled memory	*/
	intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) op_prg_pmo_alloc (intf_set_entry_pmh);
	
	FRET (intf_set_entry_ptr);
	
	}


