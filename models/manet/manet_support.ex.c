/* manet_support_api.ex.c */
/* C file for MANET APIs to interface to IP */

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
#include <ip_addr_v4.h>
#include <ip_rte_support.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <manet.h>
#include <oms_rr.h>

/*----- (SECTION - GLOBAL VARIABLE DECLARATIONS)------------------*/
/* Following variables are visible to functions in this file only */
/* An external function needs to call xxx_get () functions in this*/
/* file to obtain reference to following variables.				  */
/*----------------------------------------------------------------*/ 

/* Global IP address list maintained to obtain a random	*/
/* IP address destination for a flow					*/
static List*		global_ip_address_lptr = OPC_NIL;

/* Pool memory handler for external routes.				*/
/* Categorized memory for SMART MAC buffer elements.*/
Pmohandle external_route_cache_entry_pmh;

/* Flag for memory initialization.					*/
Boolean	 external_route_cache_entry_ready_flag = OPC_FALSE;


/***** Prototypes *****/
static Manet_Rte_Invoke_Data*		manet_rte_invoke_data_mem_alloc (void);
static void							manet_rte_invoke_data_mem_free (Manet_Rte_Invoke_Data*);
static void							manet_rte_error (const char* str1, const char* str2, const char* str3);



/***** APIs *****/

void
manet_rte_to_higher_layer_pkt_send_schedule (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
										Packet* ip_dgram_pkptr)
	{
	Manet_Rte_Invoke_Data* 			manet_data_ptr = OPC_NIL;
	
	/** Schedules an event for the current simulation time	**/
	/** to invoke IP to send the reply packet. The event at	**/
	/** this functions gets called is a process invocation	**/
	/** for this process from IP. The new event will allow	**/
	/** invocation of the IP process as it will not be the 	**/
	/** same event that IP was invoked						**/
	FIN (manet_rte_to_higher_layer_pkt_send_schedule(<args>));
	
	/** Allocate memory to store information to	**/
	/** invoke the IP process					**/
	manet_data_ptr = manet_rte_invoke_data_mem_alloc ();
	manet_data_ptr->ip_module_data_ptr = module_data_ptr;
	manet_data_ptr->manet_mgr_phandle = manet_mgr_prohandle;
	manet_data_ptr->ip_pkptr = ip_dgram_pkptr;
	
	/* Use procedure interrupt mechanism to call the IP	*/
	/* routing process invoker function.				*/
	op_intrpt_schedule_call (op_sim_time (), PKT_TO_HIGHER_LAYER, 
		manet_rte_ip_process_invoker, manet_data_ptr);
	
	FOUT;
	}


void
manet_rte_to_cpu_pkt_send_schedule (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
										int manet_mgr_process_id, Packet* ip_dgram_pkptr)
	{
	Manet_Rte_Invoke_Data* 			manet_data_ptr = OPC_NIL;
	Ici*							pkt_info_ici_ptr;
	
	/** Schedules an event for the current simulation time	**/
	/** to invoke IP to send the reply packet. The event at	**/
	/** this functions gets called is a process invocation	**/
	/** for this process from IP. The new event will allow	**/
	/** invocation of the IP process as it will not be the 	**/
	/** same event that IP was invoked						**/
	FIN (manet_rte_to_cpu_pkt_send_schedule (<args>));
	
	/* Destroy the ICI associated with the packet	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_dgram_pkptr);
	
	if (pkt_info_ici_ptr != OPC_NIL)
		{
		/* Destroy the ICI	*/
		ip_rte_intf_ici_destroy (pkt_info_ici_ptr);
		}
	
	/** Allocate memory to store information to	**/
	/** invoke the IP process					**/
	manet_data_ptr = manet_rte_invoke_data_mem_alloc ();
	manet_data_ptr->ip_module_data_ptr = module_data_ptr;
	manet_data_ptr->manet_mgr_phandle = manet_mgr_prohandle;
	manet_data_ptr->manet_mgr_pro_id = manet_mgr_process_id;
	manet_data_ptr->ip_pkptr = ip_dgram_pkptr;
	
	/* Use procedure intruupt mechanism to call the IP	*/
	/* routing process invoker function.				*/
	op_intrpt_schedule_call (op_sim_time (), PKT_TO_CPU, manet_rte_ip_process_invoker, manet_data_ptr);
	
	FOUT;
	}

void
manet_rte_to_cpu_pkt_send_schedule_with_jitter (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
										int manet_mgr_process_id, Packet* ip_dgram_pkptr)
	{
	Manet_Rte_Invoke_Data* 			manet_data_ptr = OPC_NIL;
	double							jitter;
	Ici*							pkt_info_ici_ptr;
	
	/** Schedules an event for the current simulation time	**/
	/** to invoke IP to send the reply packet. The event at	**/
	/** this functions gets called is a process invocation	**/
	/** for this process from IP. The new event will allow	**/
	/** invocation of the IP process as it will not be the 	**/
	/** same event that IP was invoked						**/
	FIN (manet_rte_to_cpu_pkt_send_schedule (<args>));
	
	/* Destroy the ICI associated with the packet	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_dgram_pkptr);
	
	if (pkt_info_ici_ptr != OPC_NIL)
		{
		/* Destroy the ICI	*/
		ip_rte_intf_ici_destroy (pkt_info_ici_ptr);
		}
	
	/* Adding jitter between 0 and 0.01 before sending out packets */
	jitter = op_dist_uniform (0.01);
	
	/** Allocate memory to store information to	**/
	/** invoke the IP process					**/
	manet_data_ptr = manet_rte_invoke_data_mem_alloc ();
	manet_data_ptr->ip_module_data_ptr = module_data_ptr;
	manet_data_ptr->manet_mgr_phandle = manet_mgr_prohandle;
	manet_data_ptr->manet_mgr_pro_id = manet_mgr_process_id;
	manet_data_ptr->ip_pkptr = ip_dgram_pkptr;
	
	/* Use procedure intruupt mechanism to call the IP	*/
	/* routing process invoker function.				*/
	op_intrpt_schedule_call (op_sim_time () + jitter, PKT_TO_CPU, manet_rte_ip_process_invoker, manet_data_ptr);
	
	FOUT;
	}


void
manet_rte_to_cpu_pkt_send_schedule_with_high_jitter (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
										int manet_mgr_process_id, Packet* ip_dgram_pkptr)
	{
	Manet_Rte_Invoke_Data* 			manet_data_ptr = OPC_NIL;
	double							jitter;
	Ici*							pkt_info_ici_ptr;
	
	/** Schedules an event for the current simulation time	**/
	/** to invoke IP to send the reply packet. The event at	**/
	/** this functions gets called is a process invocation	**/
	/** for this process from IP. The new event will allow	**/
	/** invocation of the IP process as it will not be the 	**/
	/** same event that IP was invoked						**/
	FIN (manet_rte_to_cpu_pkt_send_schedule (<args>));
	
	/* Destroy the ICI associated with the packet	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_dgram_pkptr);
	
	if (pkt_info_ici_ptr != OPC_NIL)
		{
		/* Destroy the ICI	*/
		ip_rte_intf_ici_destroy (pkt_info_ici_ptr);
		}
	
	/* Adding jitter between 0 and 0.1 before sending out packets */
	jitter = op_dist_uniform (0.1);
	
	/** Allocate memory to store information to	**/
	/** invoke the IP process					**/
	manet_data_ptr = manet_rte_invoke_data_mem_alloc ();
	manet_data_ptr->ip_module_data_ptr = module_data_ptr;
	manet_data_ptr->manet_mgr_phandle = manet_mgr_prohandle;
	manet_data_ptr->manet_mgr_pro_id = manet_mgr_process_id;
	manet_data_ptr->ip_pkptr = ip_dgram_pkptr;
	
	/* Use procedure intruupt mechanism to call the IP	*/
	/* routing process invoker function.				*/
	op_intrpt_schedule_call (op_sim_time () + jitter, PKT_TO_CPU, manet_rte_ip_process_invoker, manet_data_ptr);
	
	FOUT;
	}


void
manet_rte_ip_process_invoker (void* generic_manet_data_ptr, int code)
	{
	Manet_Rte_Invoke_Data* manet_data_ptr = OPC_NIL; 
	/** Cover function to call the IP process invocation	**/
	/** function. This function is called as a result of	**/
	/** a procedure interrupt.								**/
	FIN (manet_rte_ip_process_invoker (<args>));
	
	manet_data_ptr = (Manet_Rte_Invoke_Data*) generic_manet_data_ptr;
	
	if (code == PKT_TO_CPU)
		{
		/* Pass the IP datagram to IP routing process for	*/
		/* further processing.								*/
		manet_rte_to_cpu_pkt_send (manet_data_ptr->ip_module_data_ptr, manet_data_ptr->manet_mgr_phandle, 
								manet_data_ptr->manet_mgr_pro_id, manet_data_ptr->ip_pkptr);
		}
	else if (code == PKT_TO_HIGHER_LAYER)
		{
		manet_rte_to_higher_layer_pkt_send (manet_data_ptr->ip_module_data_ptr, manet_data_ptr->manet_mgr_phandle, 
											manet_data_ptr->ip_pkptr);
		}
	else
		{
		manet_rte_error ("Invalid code received in procedure interrupt", OPC_NIL, OPC_NIL);
		}
	
	/* Free the memory allocated for this procedure interrupt	*/
	manet_rte_invoke_data_mem_free (manet_data_ptr);

	FOUT;
	}


void
manet_rte_to_cpu_pkt_send (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
							int manet_mgr_process_id, Packet* ip_dgram_pkptr)
	{
	/** Send the packet to the IP CPU process	**/
	/** as this packet needs to be processed	**/
	/** before being sent out to the MAC layer	**/
	FIN (manet_rte_to_cpu_pkt_send (<args>));
	
	/* Install the IP packet in the memory being shared with IP	*/
	/* (note that IP gets packets from its child processes		*/
	/* using this approach.)									*/
	module_data_ptr->ip_ptc_mem.child_pkptr = ip_dgram_pkptr;
	module_data_ptr->ip_ptc_mem.child_process_id = manet_mgr_process_id;

	/* Invoke IP process model, which routes the IP datagram	*/
	/* containing the DSR packet to the destination address.	*/
	op_pro_invoke (manet_mgr_prohandle, OPC_NIL);

	FOUT;
	}


void
manet_rte_to_higher_layer_pkt_send (IpT_Rte_Module_Data* module_data_ptr, Prohandle manet_mgr_prohandle, 
									Packet* ip_dgram_pkptr)
	{
	/** Send the IP datagram to the higher layer **/
	FIN (manet_rte_to_higher_layer_pkt_send (<args>));
	
	/* Set the packet in the argument memory to be	*/
	/* sent to the higher layer from this process	*/
	module_data_ptr->ip_ptc_mem.child_pkptr = OPC_NIL;
	op_pro_invoke (manet_mgr_prohandle, ip_dgram_pkptr);
	
	FOUT;
	}


void
manet_rte_to_mac_pkt_send_options (IpT_Rte_Module_Data* module_data_ptr, Packet* ip_pkptr, InetT_Address next_hop_addr, 
			IpT_Dgram_Fields* ip_dgram_fd_ptr, IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, int output_intf_index)
	{
	IpT_Interface_Info*		iface_info_ptr;
	int						outstrm;
	InetT_Address_Range* 	inet_addr_range_ptr;
	InetT_Addr_Family		addr_family;
	int						slot_index;
	short					intf_index;
	
	/** Send the packet out directly to the	**/
	/** outgoing interface as this packet 	**/
	/** has already been processed by the 	**/
	/** IP CPU process and no further 		**/
	/** processing is needed on this packet	**/
	FIN (manet_rte_to_mac_pkt_send_options (<args>));
	
	if (output_intf_index != IPC_INTF_INDEX_INVALID)
		{
		/* A valid outgoing interface has been provided by 	*/
		/* the calling process. Get the interface_info_ptr	*/
		/* AODV protocol usually provides the outgoing intf	*/
		iface_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, output_intf_index);
		}
	else
		{
		/* Determine the outgoing interface index */
		if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &intf_index,
										 &iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_FAILURE)
			{
			/* Unable to determine the outgoing interface	*/
			/* as there is no matching interface with the 	*/
			/* same local network							*/
			manet_rte_error ("manet_rte_to_mac_pkt_send. Unable to determine the outgoing interface", OPC_NIL, OPC_NIL);
			}
		output_intf_index = (int) intf_index;
		}
	
	/* Set the interface info in the interface ICI	*/
	intf_ici_fdstruct_ptr->output_intf_index = output_intf_index;
	intf_ici_fdstruct_ptr->output_subintf_index = ip_rte_minor_port_from_intf_table_index_get
													(module_data_ptr, output_intf_index);
	intf_ici_fdstruct_ptr->interface_type 	 = ip_rte_intf_type_get (iface_info_ptr);
	intf_ici_fdstruct_ptr->outstrm 		  	 = ip_rte_intf_outstrm_get (iface_info_ptr, ip_pkptr, &slot_index);
	intf_ici_fdstruct_ptr->outslot 		  	 = ip_rte_intf_slot_index_get (iface_info_ptr);
	intf_ici_fdstruct_ptr->iface_speed 	  	 = ip_rte_intf_link_bandwidth_get (iface_info_ptr);
	intf_ici_fdstruct_ptr->next_addr 		 = next_hop_addr;

	/* Get the family the IP address next hop belongs to	*/
	addr_family = inet_address_family_get (&next_hop_addr);
	
	/* Get the interface MTU size	*/
	intf_ici_fdstruct_ptr->output_mtu 		 = inet_rte_intf_mtu_get (iface_info_ptr, addr_family);
	
	/* Send the packet to the next hop	*/
	ip_rte_datagram_interface_forward_direct (module_data_ptr,  
										ip_pkptr, intf_ici_fdstruct_ptr);
	
	FOUT;
	}


int
manet_rte_interface_mtu_size_get (IpT_Rte_Module_Data* module_data_ptr, InetT_Address next_hop_addr)
	{
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	InetT_Address_Range* 	inet_addr_range_ptr;
	int						outstrm;
	short					output_intf_index;
	int						intf_mtu;
	InetT_Addr_Family		addr_family;
	
	/** Returns the MTU size for the interface	**/
	/** to which this next hop node is connected**/
	FIN (manet_rte_interface_mtu_size_get (<args>));
	
	/* Determine the outgoing interface index	*/
	if (ip_rte_destination_local_network (module_data_ptr, next_hop_addr, &output_intf_index,
								&iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_FAILURE)
		{
		/* Unable to determine the outgoing interface	*/
		/* as there is no matching interface with the 	*/
		/* same local network							*/
		manet_rte_error ("Unable to determine the outgoing interface", OPC_NIL, OPC_NIL);
		}
	
	/* Get the family the IP address next hop belongs to	*/
	addr_family = inet_address_family_get (&next_hop_addr);
	
	/* Get the interface MTU size	*/
	intf_mtu = inet_rte_intf_mtu_get (iface_info_ptr, addr_family);
	
	FRET (intf_mtu);
	}


void
manet_rte_datagram_broadcast (IpT_Rte_Module_Data* module_data_ptr, Packet* ip_pkptr)
	{
	Ici*						pkt_info_ici_ptr;
	IpT_Rte_Ind_Ici_Fields* 	intf_ici_fdstruct_ptr;
	
	/** Broadcasts the IP datagram on all	**/
	/** interfaces on this node				**/
	FIN (manet_rte_datagram_broadcast (<args>));
	
	/* Get the ICI associated with the packet	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_pkptr);
	
	/* Get the incoming packet information	*/
	op_ici_attr_get (pkt_info_ici_ptr, "rte_info_fields", &intf_ici_fdstruct_ptr);
	
	/* Set the packet as if it was being generated	*/
	/* by this child process of IP to re-broadcast	*/
	intf_ici_fdstruct_ptr->instrm = IpC_Pk_Instrm_Child;
	
	/* Set the incoming packet information	*/
	op_ici_attr_set (pkt_info_ici_ptr, "rte_info_fields", intf_ici_fdstruct_ptr);
	
	/* Set the ICI in the packet	*/
	op_pk_ici_set (ip_pkptr, pkt_info_ici_ptr);
	
	/* Broadcast the datagram on all interfaces	*/
	/* on this node								*/
	intf_ici_fdstruct_ptr->output_intf_index = IP_BROADCAST_ALL_INTERFACES;
	inet_rte_datagram_broadcast (module_data_ptr, 
				ip_pkptr,  
				intf_ici_fdstruct_ptr);
	
	FOUT;
	}


InetT_Address
manet_rte_rcvd_interface_address_get (IpT_Rte_Module_Data* module_data_ptr,
	IpT_Rte_Ind_Ici_Fields* intf_ici_fdstruct_ptr, InetT_Addr_Family addr_family)
	{
	IpT_Interface_Info *	iface_elem_ptr = OPC_NIL;
	InetT_Address			rcvd_intf_addr;
	
	/** Determines the address of the interface	**/
	/** on which the packet was received		**/
	FIN (manet_rte_rcvd_interface_address_get (<args>));
	
	/* Get the interface_info of the interface specified.					*/
	iface_elem_ptr = inet_rte_intf_tbl_access (module_data_ptr, intf_ici_fdstruct_ptr->intf_recvd_index);
	
	/* Get the received interface address	*/
	rcvd_intf_addr = inet_rte_intf_addr_get (iface_elem_ptr, addr_family);
	
	FRET (rcvd_intf_addr);
	}


Boolean
manet_rte_address_belongs_to_node (IpT_Rte_Module_Data* module_data_ptr, InetT_Address node_address)
	{
	IpT_Interface_Info*		iface_info_ptr = OPC_NIL;
	int						outstrm;
	short					output_intf_index;
	InetT_Address_Range* 	inet_addr_range_ptr;
	
	/** Checks if the incoming packet has	**/
	/** reached its destination				**/
	FIN (manet_rte_address_belongs_to_node (<args>));
	
	/* Check if the destination address belongs to one	*/
	/* of the directly connected networks of this node	*/
	if (ip_rte_destination_local_network (module_data_ptr, node_address, &output_intf_index, 
							 &iface_info_ptr, &outstrm, &inet_addr_range_ptr) == OPC_COMPCODE_SUCCESS)
		{
		/* Packet belongs to a directly connected network	*/
		/* Check if the packet is destined for this node	*/
		if (inet_address_range_address_equal (inet_addr_range_ptr, &node_address))
			{
			/* The packet is destined for this node	*/
			FRET (OPC_TRUE);
			}
		}
	
	FRET (OPC_FALSE);
	}


void
manet_rte_ip_pkt_info_access (Packet* ip_pkptr, IpT_Dgram_Fields** ip_dgram_fd_pptr,
							IpT_Rte_Ind_Ici_Fields** intf_ici_fdstruct_pptr)
	{
	Ici*			pkt_info_ici_ptr = OPC_NIL;
	
	/** Access the information from the IP packet	**/
	FIN (manet_rte_ip_pkt_info_access (<args>));
	
	/* Get the ICI associated with the packet	*/
	/* Get the pointer to access the contents	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_pkptr);
	
	/* Get the incoming packet information	*/
	op_ici_attr_get (pkt_info_ici_ptr, "rte_info_fields", intf_ici_fdstruct_pptr);
	
	/* Set the incoming packet information	*/
	op_ici_attr_set (pkt_info_ici_ptr, "rte_info_fields", *intf_ici_fdstruct_pptr);
	
	/* Set the ICI back in the packet	*/
	op_pk_ici_set (ip_pkptr, pkt_info_ici_ptr);
	
	/* Access the fields of the IP datagram	*/
	op_pk_nfd_access (ip_pkptr, "fields", ip_dgram_fd_pptr);
	
	FOUT;
	}
	


Packet*
manet_rte_ip_pkt_copy (Packet* ip_pkptr)
	{
	Ici*						pkt_info_ici_ptr;
	Ici*						copy_pkt_info_ici_ptr;
	IpT_Rte_Ind_Ici_Fields* 	intf_ici_fdstruct_ptr; 
	IpT_Rte_Ind_Ici_Fields*		copy_intf_ici_fdstruct_ptr;
	Packet*						copy_pkptr;
	
	/** Makes a copy of the IP packet	**/
	/** This can only be used for calls	**/
	/** in the IP module				**/
	FIN (manet_rte_ip_pkt_copy (<args>));
	
	/* Make a copy of the packet	*/
	copy_pkptr = op_pk_copy (ip_pkptr);
	
	/* Get the ICI associated with the packet	*/
	/* Get the pointer to access the contents	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_pkptr);
	
	/* Set the ICI back in the packet	*/
	op_pk_ici_set (ip_pkptr, pkt_info_ici_ptr);
	
	if ((pkt_info_ici_ptr != OPC_NIL) && (op_ici_attr_exists (pkt_info_ici_ptr, "rte_info_fields") == OPC_TRUE))
		{
		/* Get the incoming packet information	*/
		op_ici_attr_get (pkt_info_ici_ptr, "rte_info_fields", &intf_ici_fdstruct_ptr);
	
		/* Set the incoming packet information	*/
		op_ici_attr_set (pkt_info_ici_ptr, "rte_info_fields", intf_ici_fdstruct_ptr);
	
		/* Make a copy of the routing info fields	*/
		copy_intf_ici_fdstruct_ptr = ip_rte_ind_ici_fdstruct_copy (intf_ici_fdstruct_ptr);
	
		/* Create a new ICI and set the information in that ICI	*/
		copy_pkt_info_ici_ptr = op_ici_create ("ip_rte_ind_v4");
	
		/* Set the information	*/
		op_ici_attr_set (copy_pkt_info_ici_ptr, "rte_info_fields", copy_intf_ici_fdstruct_ptr);
	
		/* Associate the new ICI with the packet	*/
		op_pk_ici_set (copy_pkptr, copy_pkt_info_ici_ptr);
		}
	
	FRET (copy_pkptr);
	}
	
	

void
manet_rte_ip_pkt_destroy (Packet* ip_pkptr)
	{
	Ici*		pkt_info_ici_ptr = OPC_NIL;
	
	/** Destroys the IP packet and the ICI	**/
	/** associated with the packet if any	**/
	FIN (manet_rte_ip_pkt_destroy (<args>));
	
	/* Get the ICI associated with the packet	*/
	pkt_info_ici_ptr = op_pk_ici_get (ip_pkptr);
	
	if (pkt_info_ici_ptr != OPC_NIL)
		{
		/* Destroy the ICI	*/
		ip_rte_intf_ici_destroy (pkt_info_ici_ptr);
		}
	
	/* Destroy the IP datagram	*/
	op_pk_destroy (ip_pkptr);
	
	FOUT;
	}


void
manet_rte_dgram_discard_record (IpT_Rte_Module_Data* iprmd_ptr, Packet* ip_pkptr, 
								const char* discard_reason)
	{
	char				reason_str [256];
	
	/** Record the discard reson for the IP datagram	**/
	/** so that it is displayed during IP route display	**/
	FIN (manet_rte_dgram_discard_record (<args>));
	
	if (discard_reason != OPC_NIL)
		{
		/* Append "Incomplete to the reason to indicate	*/
		/* that this is an incomplete route.			*/
		sprintf (reason_str, "Incomplete (%s)", discard_reason);
		
		/* Update the "record route" option with the	*/
		/* appropriate hop and reason for discard of	*/
		/* the IP datagram if this is a tracer packet	*/
		oms_rr_info_update (ip_pkptr, iprmd_ptr->node_name);
		oms_rr_info_update (ip_pkptr, reason_str);
		}
	
	FOUT;
	}


void
manet_rte_path_display_dump (List* record_route_lptr)
	{
	static OmsT_Ext_File_Handle		ext_file_handle = OPC_NIL;
	InetT_Address*					src_addr_ptr;
	InetT_Address*					dest_addr_ptr;
	InetT_Address*					hop_addr_ptr;
	char							file_line_entry [8192] = "";
	char							temp_str [128] = "";
	char							src_node_name [OMSC_HNAME_MAX_LEN] = "";
	char							dest_node_name [OMSC_HNAME_MAX_LEN] = "";
	char							hop_node_name [OMSC_HNAME_MAX_LEN] = "";
	char							conn_name [512] = "";
	int								num_hops, count;
	
	/** Dumps the list of hops to a file to display	**/
	/** in the route browser. The list should 		**/
	/** contain the entire path of nodes from the	**/
	/** source to the destination with the source	**/
	/** node at the head of the list and the		**/
	/** destination node at the tail. The input 	**/
	/** list needs to be freed by the user			**/
	FIN (manet_rte_path_display_dump (<args>));
	
	/* Check if we have file in which to write data	*/
	if (ext_file_handle == OPC_NIL)
		{
		ext_file_handle = Oms_Ext_File_Handle_Get (OMSC_EXT_FILE_PROJ_SCEN_NAME, "manet_routes_dump", OMSC_EXT_FILE_GDF);
		}
	
	/* There should be atleast 2 nodes (source and destination)	*/
	/* for it to be a valid route								*/
	num_hops = op_prg_list_size (record_route_lptr);
	
	/* Set the source name, destiation name, start 	*/
	/* time and call reference number in the file	*/
	src_addr_ptr = (InetT_Address*) op_prg_list_access (record_route_lptr, OPC_LISTPOS_HEAD);
	inet_address_to_hname (*src_addr_ptr, src_node_name);
	strcat (file_line_entry, src_node_name);
	strcat (file_line_entry, ",");
	dest_addr_ptr = (InetT_Address*) op_prg_list_access (record_route_lptr, OPC_LISTPOS_TAIL);
	inet_address_to_hname (*dest_addr_ptr, dest_node_name);
	strcat (file_line_entry, dest_node_name);
	strcat (file_line_entry, ",");
	sprintf (temp_str, "%0.2f", op_sim_time ());
	strcat (file_line_entry, temp_str);
	strcat (file_line_entry, ",");
	sprintf (temp_str, "0");
	strcat (file_line_entry, temp_str);
	strcat (file_line_entry, ",");
	
	/* Set the connection name to be (src_node_name)<->(dest_node_name)	*/
	strcat (conn_name, src_node_name);
	strcat (conn_name, "<->");
	strcat (conn_name, dest_node_name);
	strcat (file_line_entry, conn_name);
	strcat (file_line_entry, ",");
	
	/* Read the source node and intermediate nodes in the route	*/
	for (count = 0; count < (num_hops - 1); count++)
		{
		strcat (file_line_entry, ",");
		
		hop_addr_ptr = (InetT_Address*) op_prg_list_access (record_route_lptr, count);
		inet_address_to_hname (*hop_addr_ptr, hop_node_name);
		strcat (file_line_entry, hop_node_name);
		strcat (file_line_entry, ",");
				
		/* Append "None" for the link	*/
		strcat (file_line_entry, "None");
		}
	
	/* Go to the next line	*/
	strcat (file_line_entry, "\n");
	
	/* Write this into the file.	*/
	Oms_Ext_File_Info_Append (ext_file_handle, file_line_entry);

	FOUT;
	}
	

void
manet_rte_ip_address_register (IpT_Interface_Info* own_iface_info_ptr)
	{
	static Boolean		global_list_created = OPC_FALSE;
	
	/** This function is called to register the	**/
	/** IP address of every node to select a 	**/
	/** random destination by the manet RPG		**/
	FIN (manet_rte_ip_address_register (<args>));

	if (global_list_created == OPC_FALSE)
		{
		/* Create a list to hold the IP addresses	*/
		global_ip_address_lptr = op_prg_list_create ();
		
		global_list_created = OPC_TRUE;
		}
	
	op_prg_list_insert (global_ip_address_lptr, own_iface_info_ptr, OPC_LISTPOS_TAIL);
	
	FOUT;
	}


InetT_Address*
manet_rte_dest_ip_address_obtain (IpT_Interface_Info* own_iface_info_ptr)
	{
	int						num_dest;
	int						num_attempts = 0;
	int						dest_index;
	Boolean					ipv6_enabled = OPC_FALSE;
	Boolean					ipv4_enabled = OPC_FALSE;
	InetT_Address			dest_address;
	InetT_Address*			dest_address_ptr;
	IpT_Interface_Info*		dest_iface_info_ptr;
	Boolean					dest_ipv4_enabled = OPC_FALSE;
	Boolean					dest_ipv6_enabled = OPC_FALSE;	
	static int				global_ip_version;
	static Boolean			global_ip_version_initialized = OPC_FALSE;
	int						i, num_max_attempts;
	
	/** This function picks a random destination	**/
	/** IP address for the MANET RPG				**/
	FIN (manet_rte_dest_ip_address_obtain (void));
	
	/* Get the global IP version preference				*/
	if (!global_ip_version_initialized)
		{
		op_ima_sim_attr_get (OPC_IMA_INTEGER, "IP Version Preference", &global_ip_version);
		global_ip_version_initialized = OPC_TRUE;
		}
	
	/* Check if this node trying to get a destination	*/
	/* has valid IPv6 and IPv4 addresses				*/
	ipv6_enabled = ip_rte_intf_ip_version_active (own_iface_info_ptr, InetC_Addr_Family_v6);

	ipv4_enabled = ip_rte_intf_ip_version_active (own_iface_info_ptr, InetC_Addr_Family_v4);
	
	/* Get the number of possible destinations	*/
	num_dest = op_prg_list_size (global_ip_address_lptr);
	
	/* The number of destination have to be greater */
	/* than only this one node calling the function	*/
	if (num_dest <= 1)
		FRET (OPC_NIL);

	/* Remove the source interface from the list of interfaces so that we will have only eligible 		*/
	/* destinations.																					*/
	for (i = 0; i < num_dest; i++)
		{
		if (op_prg_list_access (global_ip_address_lptr, i) == own_iface_info_ptr)
			{
			op_prg_list_remove (global_ip_address_lptr, i);
			num_dest--;
			break;
			}
		}

	/* We will try as many times as there are destinations. No particular reason for this limit.	*/				
	num_max_attempts = num_dest;

	while (num_attempts <= num_max_attempts)
		{
		/* Pick a destination uniformly	*/
		dest_index = (int) op_dist_uniform (num_dest);
		
		/* Get the randomly picked destination address	*/
		dest_iface_info_ptr = (IpT_Interface_Info*) op_prg_list_access (global_ip_address_lptr, dest_index);
		
		/* Determine if an IPv4 or IPv6 address should be generated for destination	*/
		/* Criterion is that this address should be taken from *intersection* of source and destination	*/
		/* In case of tie between both v4 and v6, it is broken by global simulation attribute			*/
		/* IP Version Preference																		*/
		dest_ipv4_enabled = ip_rte_intf_ip_version_active (dest_iface_info_ptr, InetC_Addr_Family_v4);
		dest_ipv6_enabled = ip_rte_intf_ip_version_active (dest_iface_info_ptr, InetC_Addr_Family_v6);
		
		if ((ipv6_enabled) && (!ipv4_enabled))
			{
			/* If destination has an IPv6 address, use it.	*/
			if (dest_ipv6_enabled)
				{
				/* Get the destination IPv6 address							*/
				dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v6);
				break;
				}
			}
		
		else if ((ipv4_enabled) && (!ipv6_enabled))
			{
			/* If destination has an IPv4 address, use it.	*/
			if (dest_ipv4_enabled)
				{
				/* Get the destination IPv4 address							*/
				dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v4);
				break;
				}
			}
		
		else if ((ipv4_enabled) && (ipv6_enabled))
			{
			/* If destination has IPv6 only, choose v6						*/
			if ((dest_ipv6_enabled) && (!dest_ipv4_enabled))
				{
				/* Get the destination IPv6 address							*/
				dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v6);
				break;
				}
			
			/* If destination has Ipv4 only, choose v4						*/
			if ((dest_ipv4_enabled) && (!dest_ipv6_enabled))
				{
				/* Get the destination IPv4 address							*/
				dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v4);
				break;
				}
			
			/* If destination has both, break the tie by reading the global attribute IP Version Preference	*/
			if ((dest_ipv4_enabled) && (dest_ipv6_enabled))
				{
				if (global_ip_version == 1)
					{
					/* Generate IPv4 address					*/
					dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v4);
					break;
					}
				else
					{
					/* Generate IPv6 address			*/
					dest_address = inet_rte_intf_addr_get (dest_iface_info_ptr, InetC_Addr_Family_v6);
					break;
					}
				
				}
			}
		
		num_attempts++;
		}

	/* Add source interface back to the list.	*/
	op_prg_list_insert (global_ip_address_lptr, own_iface_info_ptr, OPC_LISTPOS_TAIL);
	
	/* If num_attempts became more than num_dest, no valid destination was found		*/
	if (num_attempts > num_max_attempts)
		FRET (OPC_NIL);
	
	/* Return a copy of the address	*/
	dest_address_ptr = inet_address_create_dynamic (dest_address);
	
	FRET (dest_address_ptr);
	}
			

static Manet_Rte_Invoke_Data*
manet_rte_invoke_data_mem_alloc (void)
	{
	Manet_Rte_Invoke_Data*		manet_data_ptr = OPC_NIL;
	
	/** Allocates memory for invoking ip_dispatch	**/
	/** in the same event that ip_dispatch invoked	**/
	/** this process.								**/
	FIN (manet_rte_invoke_data_mem_alloc (void));
	
	manet_data_ptr = (Manet_Rte_Invoke_Data*) op_prg_mem_alloc (sizeof (Manet_Rte_Invoke_Data));
	manet_data_ptr->ip_module_data_ptr = OPC_NIL;
	manet_data_ptr->ip_pkptr = OPC_NIL;
	
	FRET (manet_data_ptr);
	}

static void
manet_rte_invoke_data_mem_free (Manet_Rte_Invoke_Data* manet_data_ptr)
	{
	/** Frees the memory allocated for the manet	**/
	/** procedure interrupt							**/
	FIN (manet_rte_invoke_data_mem_free (<args>));
	
	op_prg_mem_free (manet_data_ptr);
	
	FOUT;
	}


static void
manet_rte_error (const char* str1, const char* str2, const char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (dsr_rte_error <args>);
	
	op_sim_end ("MANET Routing Process : ", str1, str2, str3);
	
	FOUT;
	}


Manet_Rte_Ext_Cache_Table*
manet_ext_rte_cache_table_init (void)
	{
	Manet_Rte_Ext_Cache_Table* external_rte_cache_hashtbl_ptr;	

	/** Initializes the cache table used to store all external 	**/
	/** routes established between MANET nodes and non-MANET 	**/
	/** nodes.													**/
	FIN (manet_ext_rte_cache_table_init ());
	
	/* The number of elements in the hash table is 64.			*/
	external_rte_cache_hashtbl_ptr = inet_addr_hash_table_create (64, 64);

	/* Initialize the pool memory for the entries in the table.		*/
	if (!external_route_cache_entry_ready_flag)
		{
		external_route_cache_entry_pmh = op_prg_pmo_define ("MANET Connectivity: External routes cache elements", sizeof (Manet_Rte_Ext_Cache_entry), 32);
		external_route_cache_entry_ready_flag = OPC_TRUE;
		}
	
	FRET (external_rte_cache_hashtbl_ptr);
	}

Boolean
manet_ext_rte_cache_table_entry_available (Manet_Rte_Ext_Cache_Table* ext_cache_table_ptr, InetT_Address dest_addr, Manet_Rte_Ext_Cache_entry** ext_cache_entry_pptr)
	{
	/** Searches for dest_addr in the MANET external routes	**/
	/** cache, and returns OPC_TRUE if the entry is found.	**/
	/** The a reference to the entry is stored in 			**/
	/** ext_cache_entry_pptr.								**/ 
	FIN (manet_ext_rte_cache_table_entry_available ());

	/* ODB trace.										*/
	if (op_prg_odb_ltrace_active ("manet_ext_rte"))
		{
		char	msg[512];
		char	dest_addr_str [INETC_ADDR_STR_LEN];
				
		inet_address_print (dest_addr_str, dest_addr);
		sprintf (msg, "   Searching for external destination [%s]", dest_addr_str);
		op_prg_odb_print_minor ("MANET Connectivity: cache table lookup.", msg, OPC_NIL);
		
		manet_ext_rte_cache_table_print (ext_cache_table_ptr, inet_address_family_get (&dest_addr));
		}
	
	/* Search for an entry.					*/
	(*ext_cache_entry_pptr) = (Manet_Rte_Ext_Cache_entry*) inet_addr_hash_table_item_get (ext_cache_table_ptr, &dest_addr);

	/* Entries may exist in the table but the can be tag as	*/
	/* not available.										*/
	if ((*ext_cache_entry_pptr) != OPC_NIL && (*ext_cache_entry_pptr)->available == OPC_TRUE)
		{
		/* Entry exists and it is marked as available.		*/
		
		/* ODB trace.										*/
		if (op_prg_odb_ltrace_active ("manet_ext_rte"))
			{
			op_prg_odb_print_minor ("Available entry.", OPC_NIL);
			}
		FRET (OPC_TRUE);
		}

	/* Destination is not available. Either the entry does	*/
	/* not exist or it is marked as non-available.			*/
	
	/* ODB trace.											*/
	if (op_prg_odb_ltrace_active ("manet_ext_rte"))
		{
		if ((*ext_cache_entry_pptr) == OPC_NIL)
			op_prg_odb_print_minor ("    - Entry does not exists.", OPC_NIL);
		else
			op_prg_odb_print_minor ("    - Entry exists but is disabled.", OPC_NIL);
		}

	FRET (OPC_FALSE);
	}

void
manet_ext_rte_cache_table_print (Manet_Rte_Ext_Cache_Table* ext_cache_table_ptr, InetT_Addr_Family manet_addressing_mode)
	{
	PrgT_List*					entries_list_ptr;
	Manet_Rte_Ext_Cache_entry* 	cache_entry_ptr = OPC_NIL;
	int							i, list_size;	
	char						dest_addr_str [INETC_ADDR_STR_LEN];
	
	/** Prints the contents of the external cache table.		**/
	FIN (manet_ext_rte_cache_table_lookup ());

	/* Access the correct bin hash based on the IP address		*/
	/* family.													*/
	if (manet_addressing_mode == InetC_Addr_Family_v4)
		{
		if (ext_cache_table_ptr->ipv4_hash_table == OPC_NIL)
			FOUT;
		
		/* Get a list of the entries in the hash table.			*/
		entries_list_ptr = prg_bin_hash_table_item_list_get (ext_cache_table_ptr->ipv4_hash_table);
		}
	else
		{
		if (ext_cache_table_ptr->ipv6_hash_table == OPC_NIL)
			FOUT;
		
		/* Get a list of the entries in the hash table.			*/
		entries_list_ptr = prg_bin_hash_table_item_list_get (ext_cache_table_ptr->ipv6_hash_table);
		}
	
	/* Verify that a list is obtained.							*/
	if (entries_list_ptr == OPC_NIL)
		FOUT;

	/* Print the table header.									*/
	printf ("----------------------------------------------\n");		
	printf ("|  Destination        |    Status     | Seq  |\n");
	printf ("----------------------------------------------\n");	

	/* Obtain the number of elements in the list.				*/	
	if ((list_size = prg_list_size (entries_list_ptr)) == 0)
		FOUT;
	
	/* Loop through the list printing each entry.				*/
	for (i = 0 ; i < list_size ; i++)
		{
		/* Access an element in the list.						*/
		cache_entry_ptr = (Manet_Rte_Ext_Cache_entry*) prg_list_access (entries_list_ptr, i);
		
		/* Make sure it is not nil.								*/
		if (cache_entry_ptr != OPC_NIL)
			{
			inet_address_print (dest_addr_str, cache_entry_ptr->dest_addr);
			printf ("| %19s | %13s | %4d |\n", dest_addr_str, 
				cache_entry_ptr->available == OPC_TRUE ? "Available" : "Not Available", 
				cache_entry_ptr->dest_seq_num);
			}
		}
		
	printf ("-------------------------------------------------------\n");	
		
	FOUT;
	}

void
manet_ext_rte_cache_table_entry_activate (Manet_Rte_Ext_Cache_Table* ext_cache_table_ptr, InetT_Address dest_addr, int dest_seq_num)
	{
	Manet_Rte_Ext_Cache_entry* 	ext_cache_entry_ptr = OPC_NIL;

	/** Activates an external destination in the external route	**/
	/** cache table. If the destination does not have an entry 	**/
	/** in the table, a new entry is created for it. If the 	**/
	/** destination exists its sets its "available" fields to	**/
	/** OPC_TRUE.												**/
	FIN (manet_ext_rte_cache_table_entry_activate ());
	
	/* ODB trace.													*/
	if (op_prg_odb_ltrace_active ("manet_ext_rte"))
		{
		char	msg[512];
		char	dest_addr_str [INETC_ADDR_STR_LEN];
				
		inet_address_print (dest_addr_str, dest_addr);

		sprintf (msg, "Trying to insert external IP address [%s]", dest_addr_str);
		
		op_prg_odb_print_major ("MANET Connectivity:", msg, OPC_NIL);
		
		manet_ext_rte_cache_table_print (ext_cache_table_ptr, inet_address_family_get (&dest_addr));
		}
	
	/* Search for an entry.					*/
	ext_cache_entry_ptr = (Manet_Rte_Ext_Cache_entry*) inet_addr_hash_table_item_get (ext_cache_table_ptr, &dest_addr);
	
	if (ext_cache_entry_ptr != OPC_NIL)
		{
		/* If the entry exists, only change its state to available.	*/
		ext_cache_entry_ptr->available = OPC_TRUE;

		/* ODB trace.													*/
		if (op_prg_odb_ltrace_active ("manet_ext_rte"))
			{
			op_prg_odb_print_minor ("Entry already existed, re-activating entry", OPC_NIL);
			}
			
		FOUT;
		}

	/* The entry does not exist in the cache table. Add a new entry.*/
	
	/* Create the entry.											*/
	ext_cache_entry_ptr = manet_ext_rte_cache_entry_create (dest_addr, dest_seq_num);

	/* Insert the new entry.										*/
	inet_addr_hash_table_item_insert (ext_cache_table_ptr,
					&dest_addr, ext_cache_entry_ptr, (void **) OPC_NIL);

	/* ODB trace.													*/
	if (op_prg_odb_ltrace_active ("manet_ext_rte"))
		{
		op_prg_odb_print_minor ("Adding new entry for the external route",  OPC_NIL);
		}
	
	FOUT;
	}

Manet_Rte_Ext_Cache_entry* 
manet_ext_rte_cache_entry_create (InetT_Address dest_addr, int dest_seq_num)
	{
	Manet_Rte_Ext_Cache_entry* ext_cache_entry_ptr;
		
	/** Creates and intializes an entry for the external**/
	/** route cache table.								**/
	FIN (manet_ext_rte_cache_entry_create ());
	
	/* Allocate memory for the entry.					*/
	ext_cache_entry_ptr = (Manet_Rte_Ext_Cache_entry *) op_prg_pmo_alloc (external_route_cache_entry_pmh);
	
	/* All new entries are considered available.		*/
	ext_cache_entry_ptr->available = OPC_TRUE;

	/* Assign the destination sequence number indicated */
	/* by the caller.									*/
	ext_cache_entry_ptr->dest_seq_num = dest_seq_num;	
	ext_cache_entry_ptr->src_count = 1;
	
	/* Store the destination address that corresponds	*/
	/* to this entry.									*/
	ext_cache_entry_ptr->dest_addr = *inet_address_copy_dynamic (&dest_addr);
	
	/* Return a reference to the entry.					*/
	FRET (ext_cache_entry_ptr);
	}
