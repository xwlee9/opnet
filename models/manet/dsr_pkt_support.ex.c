/* dsr_pkt_support.ex.c */
/* C support file for creation and deletion of DSR packets */


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
#include <dsr_pkt_support.h>
#include <dsr_ptypes.h>
#include <ip_addr_v4.h>

/* Macro definition for string handling.	*/
#define DSR_PKPRINT_STRING_INSERT(str, contents, list)					\
	{										\
	str = (char*) op_prg_mem_alloc ((strlen (contents) + 1) * sizeof (char));	\
	strcpy (str, contents);								\
	op_prg_list_insert (list, str, OPC_LISTPOS_TAIL);				\
	}

/* File Globals	*/
static int 			fd_options_index = OPC_FIELD_INDEX_INVALID;

/**** Prototypes ****/
static List*							dsr_pkt_support_tlv_options_mem_copy (List* tlv_options_lptr);
static DsrT_Packet_Option*				dsr_pkt_support_route_request_mem_copy (DsrT_Packet_Option*);
static DsrT_Packet_Option*				dsr_pkt_support_route_reply_mem_copy (DsrT_Packet_Option*);
static DsrT_Packet_Option*				dsr_pkt_support_route_error_mem_copy (DsrT_Packet_Option*);
static DsrT_Packet_Option*				dsr_pkt_support_ack_request_mem_copy (DsrT_Packet_Option*);
static DsrT_Packet_Option*				dsr_pkt_support_acknowledgement_mem_copy (DsrT_Packet_Option*);
static DsrT_Packet_Option*				dsr_pkt_support_source_route_mem_copy (DsrT_Packet_Option*);

static void								dsr_pkt_support_tlv_options_mem_free (List* tlv_options_lptr);
static void								dsr_pkt_support_route_request_mem_free (DsrT_Packet_Option*);
static void								dsr_pkt_support_route_reply_mem_free (DsrT_Packet_Option*);
static void								dsr_pkt_support_route_error_mem_free (DsrT_Packet_Option*);
static void								dsr_pkt_support_ack_request_mem_free (DsrT_Packet_Option*);
static void								dsr_pkt_support_acknowledgement_mem_free (DsrT_Packet_Option*);
static void								dsr_pkt_support_source_route_mem_free (DsrT_Packet_Option*);

static DsrT_Packet_Option*				dsr_pkt_support_option_mem_alloc (void);
static DsrT_Route_Request_Option*		dsr_pkt_support_route_request_mem_alloc (void);
static DsrT_Route_Reply_Option*			dsr_pkt_support_route_reply_mem_alloc (void);
static DsrT_Route_Error_Option*			dsr_pkt_support_route_error_mem_alloc (void);
static DsrT_Acknowledgement_Request*	dsr_pkt_support_ack_request_mem_alloc (void);
static DsrT_Acknowledgement*			dsr_pkt_support_acknowledgement_mem_alloc (void);
static DsrT_Source_Route_Option*		dsr_pkt_support_source_route_mem_alloc (void);


/*****************************************************/
/************* PACKET CREATION FUNCTIONS *************/
/*****************************************************/

Packet*
dsr_pkt_support_pkt_create (int next_header)
	{
	Packet*					dsr_pkptr = OPC_NIL;
	List*					tlv_options_lptr = OPC_NIL;
	
	/** Create the DSR packet with the list of	**/
	/** options that are passed in. A single	**/
	/** packet can have multiple options		**/
	FIN (dsr_pkt_support_pkt_create (<args>));
	
	/* Create the DSR packet	*/
	dsr_pkptr = op_pk_create_fmt ("dsr");
	
	/* Create a list to store the options	*/
	tlv_options_lptr = op_prg_list_create ();
	
	/* Set the options in the packet	*/
	op_pk_nfd_set (dsr_pkptr, "Options", tlv_options_lptr, dsr_pkt_support_tlv_options_mem_copy, 
						dsr_pkt_support_tlv_options_mem_free, 0);
	
	/* Set the next header in the packet	*/
	/* The next header field of the DSR 	*/
	/* packet is set to the protocol field	*/
	/* of the IP datagram					*/
	op_pk_nfd_set (dsr_pkptr, "Next Header", next_header);
	
	FRET (dsr_pkptr);
	}


Packet*
dsr_pkt_support_pkt_create_with_options (int next_header, List* tlv_options_lptr)
	{
	Packet*					dsr_pkptr = OPC_NIL;
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	int						total_length = 0;
	int						num_options, count;
	
	/** Create the DSR packet with the list of	**/
	/** options that are passed in. A single	**/
	/** packet can have multiple options		**/
	FIN (dsr_pkt_support_pkt_create_with_options (<args>));
	
	/* Create the DSR packet	*/
	dsr_pkptr = op_pk_create_fmt ("dsr");
	
	/* Get the field index for the options field	*/
	if (fd_options_index == OPC_FIELD_INDEX_INVALID)
		fd_options_index = op_pk_nfd_name_to_index (dsr_pkptr, "Options");
	
	/* Determine the number of options in the list	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access each TLV in the list	*/
		/* to determine its size		*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		/* Get the total size of all the options	*/
		total_length += dsr_tlv_ptr->option_length;
		}
	
	/* Set the options in the packet	*/
	op_pk_fd_set (dsr_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, tlv_options_lptr, total_length,
					dsr_pkt_support_tlv_options_mem_copy, dsr_pkt_support_tlv_options_mem_free, sizeof (List));
	
	/* Set the next header in the packet	*/
	/* The next header field of the DSR 	*/
	/* packet is set to the protocol field	*/
	/* of the IP datagram					*/
	op_pk_nfd_set (dsr_pkptr, "Next Header", next_header);
	
	FRET (dsr_pkptr);
	}


void
dsr_pkt_support_option_add (Packet* dsr_pkptr, DsrT_Packet_Option* dsr_tlv_ptr)
	{
	int						total_length = 0;
	OpT_Packet_Size			field_size = 0;
	List*					tlv_options_lptr = OPC_NIL;
	int						num_options, count;
	DsrT_Packet_Option*		exist_dsr_tlv_ptr = OPC_NIL;
	Boolean					option_inserted = OPC_FALSE;
	
	/** Add an option to the DSR packet	**/
	FIN (dsr_pkt_support_option_add (<args>));

	/* Get the field index for the options field	*/
	if (fd_options_index == OPC_FIELD_INDEX_INVALID)
		fd_options_index = op_pk_nfd_name_to_index (dsr_pkptr, "Options");
		
	/* Determine the size of the field, before we strip it 	*/
	/* from the packet.										*/
	field_size = (int) op_pk_fd_size (dsr_pkptr, fd_options_index);
	
	/* Get the list of options	*/
	op_pk_fd_get (dsr_pkptr, fd_options_index, &tlv_options_lptr);

	/* Determine the number of options in the list	*/
	num_options = op_prg_list_size (tlv_options_lptr);
   	
	/* Determine the total length of the options field	*/
	/* with the new option added						*/
	total_length = field_size + dsr_tlv_ptr->option_length;
	
	/* Insert the new option into the list	*/
	/* Always insert all options before the	*/
	/* source route option					*/
	for (count = 0; count < num_options; count++)
		{
		/* Access each TLV in the list	*/
		exist_dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		if (exist_dsr_tlv_ptr->option_type == DSRC_SOURCE_ROUTE)
			{
			/* Insert before the source route option	*/
			op_prg_list_insert (tlv_options_lptr, dsr_tlv_ptr, count);
			
			option_inserted = OPC_TRUE;
			
			break;
			}
		}
	
	if (option_inserted == OPC_FALSE)
		{
		/* There is no source route in the packet	*/
		/* Insert the option at the end of the list	*/
		op_prg_list_insert (tlv_options_lptr, dsr_tlv_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Set the options in the packet	*/
	op_pk_fd_set (dsr_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, tlv_options_lptr, total_length,
					dsr_pkt_support_tlv_options_mem_copy, dsr_pkt_support_tlv_options_mem_free, sizeof (List));
	
	FOUT;
	}


void
dsr_pkt_support_option_remove (Packet* dsr_pkptr, int option_type)
	{
	int						num_options, count;
	int						total_length = 0;
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	List*					tlv_options_lptr = OPC_NIL;
	List*					temp_lptr = OPC_NIL;
	
	/** Removes a specific option from the DSR	**/
	/** packet and sets its size accordingly	**/
	/** There should only be one option of that	**/
	/** type in the DSR packet header			**/
	FIN (dsr_pkt_support_option_remove (<args>));
	
	/* Get the field index for the options field	*/
	if (fd_options_index == OPC_FIELD_INDEX_INVALID)
		fd_options_index = op_pk_nfd_name_to_index (dsr_pkptr, "Options");
	
	/* Get the list of options	*/
	op_pk_nfd_get (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Determine the number of options in the list	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access each TLV in the list	*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		if (dsr_tlv_ptr->option_type == option_type)
			{
			/* Remove the matching option from the	*/
			/* list of options in the packet		*/
			dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_remove (tlv_options_lptr, count);
			
			/* Free the memory	*/
			temp_lptr = op_prg_list_create ();
			op_prg_list_insert (temp_lptr, dsr_tlv_ptr, OPC_LISTPOS_TAIL);
			dsr_pkt_support_tlv_options_mem_free (temp_lptr);
			
			break;
			}
		}
	
	/* Reset the size of the packet based on the options remaining	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Access each TLV in the list	*/
		/* to determine its size		*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		/* Get the total size of all the options	*/
		total_length += dsr_tlv_ptr->option_length;
		}
	
	/* Set the options in the packet	*/
	op_pk_fd_set (dsr_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, tlv_options_lptr, total_length,
					dsr_pkt_support_tlv_options_mem_copy, dsr_pkt_support_tlv_options_mem_free, sizeof (List));
	
	FOUT;
	}


/*****************************************************/
/************* PACKET SUPPORT FUNCTIONS **************/
/*****************************************************/

void
dsr_pkt_support_route_request_hop_insert (Packet* ip_pkptr, InetT_Address node_address)
	{
	List*								tlv_options_lptr = OPC_NIL;
	Packet*								dsr_pkptr = OPC_NIL;
	DsrT_Packet_Option* 				dsr_tlv_ptr = OPC_NIL;
	DsrT_Route_Request_Option*			route_request_ptr = OPC_NIL;
	int									num_hops, count, num_options;
	int									total_length = 0;
	int									address_length;
	InetT_Address*						copy_address_ptr = OPC_NIL;
	OpT_Packet_Size						dsr_pkt_size;
		
	/** Inserts the hop address into the list of hops	**/
	/** traversed by this route request					**/
	FIN (dsr_pkt_support_route_request_hop_insert (<args>));
	
	/* Get the DSR packet from the IP datagram	*/
	op_pk_nfd_get (ip_pkptr, "data", &dsr_pkptr);
	
	/* Get the list of options	*/
	op_pk_nfd_get (dsr_pkptr, "Options", &tlv_options_lptr);
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		if (dsr_tlv_ptr->option_type == DSRC_ROUTE_REQUEST)
			break;
		}

	/* Get a handle to route request structure	*/
	route_request_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Set the node address in the hop list	*/
	copy_address_ptr = inet_address_create_dynamic (node_address);
	op_prg_list_insert (route_request_ptr->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
	
	/* Determine the size of the options field	*/
	address_length = (inet_address_family_get (copy_address_ptr) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	num_hops = op_prg_list_size (route_request_ptr->route_lptr);
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + (num_hops + 1) * address_length;
	
	/* Calculate the length of the options field   */
	for (count = 0; count < num_options; count++)
		{
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		total_length += dsr_tlv_ptr->option_length;
		}
	
	/* Set the options in the packet	*/
	op_pk_fd_set (dsr_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, tlv_options_lptr, total_length,
					dsr_pkt_support_tlv_options_mem_copy, dsr_pkt_support_tlv_options_mem_free, sizeof (List));
	
	/* Get new dsr packet size 							*/
	dsr_pkt_size = op_pk_total_size_get (dsr_pkptr);

	/* Set the DSR packet in the IP datagram			*/
	op_pk_nfd_set (ip_pkptr, "data", dsr_pkptr);

	/* Set the bulk size of the IP packet to model		*/
	/* the space occupied by the encapsulated data. 	*/
	op_pk_bulk_size_set (ip_pkptr, dsr_pkt_size);
	
	FOUT;
	}


InetT_Address
dsr_pkt_support_source_route_hop_obtain (DsrT_Source_Route_Option* source_route_ptr, DsrT_Hop_Type hop_type, InetT_Address src_addr, InetT_Address dest_addr)
	{
	InetT_Address*					next_hop_address_ptr;
	int								num_hops;
	int								access_number;
	
	/** Obtains the current or next hop in the 		**/
	/** source route and decrements the segments  	**/
	/** left in the source route option of the TLV  **/
	/** if the next hop is accessed					**/
	FIN (dsr_pkt_support_source_route_hop_obtain (<args>));
	
	/* Total number of hops	not including the source	*/
	/* and destination node (only intermediate nodes)	*/
	num_hops = op_prg_list_size (source_route_ptr->route_lptr);
	
	if (hop_type == DsrC_Previous_Hop)
		{
		/* Get the previous hop node from which	*/
		/* this node received the packet		*/
		access_number = num_hops - source_route_ptr->segments_left - 1;
		}
	else if (hop_type == DsrC_Current_Hop)
		{
		/* Get the current node interface address	*/
		access_number = num_hops - source_route_ptr->segments_left;
		}
	else if (hop_type == DsrC_Next_Hop)
		{
		/* Decrement the segments left to visit	*/
		/* if we are accessing the next hop		*/
		source_route_ptr->segments_left--;
		access_number = num_hops - source_route_ptr->segments_left;
		}
	
	if (access_number < 0)
		{
		/* Return the source address	*/
		FRET (src_addr);
		}
	else if (access_number > (num_hops - 1))
		{
		/* Return the destination address	*/
		FRET (dest_addr);
		}
	else
		{
		next_hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_ptr->route_lptr, access_number);
		FRET (*next_hop_address_ptr);
		}
	}

/*****************************************************/
/************** TLV CREATION FUNCTIONS ***************/
/*****************************************************/

DsrT_Packet_Option*
dsr_pkt_support_route_request_tlv_create (long int request_id, InetT_Address dest_address)
	{
	DsrT_Route_Request_Option*			route_request_ptr = OPC_NIL;
	DsrT_Packet_Option*					dsr_tlv_ptr = OPC_NIL;
	int 								address_length;
	
	/** Creates the route request option in the DSR packet	**/
	FIN (dsr_pkt_support_route_request_tlv_create (<args>));
	
	address_length = (inet_address_family_get (&dest_address) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	route_request_ptr = dsr_pkt_support_route_request_mem_alloc ();
	route_request_ptr->identification = request_id;
	route_request_ptr->target_address = inet_address_copy (dest_address);
	
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_ROUTE_REQUEST;
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + address_length;
	dsr_tlv_ptr->dsr_option_ptr = (void*) route_request_ptr;
	
	FRET (dsr_tlv_ptr);
	}


DsrT_Packet_Option*
dsr_pkt_support_route_reply_tlv_create (InetT_Address PRG_ARG_UNUSED (src_address), InetT_Address dest_address, 
											List* intermediate_hops_lptr, Boolean last_hop_external)
	{
	DsrT_Route_Reply_Option*			route_reply_ptr = OPC_NIL;
	DsrT_Packet_Option*					dsr_tlv_ptr = OPC_NIL;
	int									count, num_hops;
	int									address_length;
	InetT_Address*						hop_address_ptr;
	InetT_Address*						copy_address_ptr = OPC_NIL;
	
	/** Creates the route reply TLV in the DSR packet	**/
	FIN (dsr_pkt_support_route_reply_tlv_create (<args>));
	
	route_reply_ptr = dsr_pkt_support_route_reply_mem_alloc ();
	route_reply_ptr->last_hop_external = last_hop_external;

	num_hops = op_prg_list_size (intermediate_hops_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (intermediate_hops_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (route_reply_ptr->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Insert the destination node address at the tail of the list	*/
	copy_address_ptr = inet_address_create_dynamic (dest_address);
	op_prg_list_insert (route_reply_ptr->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
	
	/* Allocate the main DSR header	*/
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_ROUTE_REPLY;
	
	/* The number of addresses are the number of 	*/
	/* intermediate hops and the source and			*/
	/* destination addresses						*/
	address_length = (inet_address_family_get (copy_address_ptr) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + address_length * (num_hops + 2);
	dsr_tlv_ptr->dsr_option_ptr = (void*) route_reply_ptr;
	
	FRET (dsr_tlv_ptr);
	}


DsrT_Packet_Option*
dsr_pkt_support_route_error_tlv_create (int error_type, int salvage, InetT_Address src_address, InetT_Address dest_address,
											InetT_Address unreachable_node_address)
	{
	DsrT_Route_Error_Option*		route_error_ptr = OPC_NIL;
	DsrT_Packet_Option*				dsr_tlv_ptr = OPC_NIL;
	int								address_length;
	
	/** Creates the route error TLV in the DSR packet	**/
	FIN (dsr_pkt_support_route_error_tlv_create (<args>));
	
	/* Allocate the route error option	*/
	route_error_ptr = dsr_pkt_support_route_error_mem_alloc ();
	route_error_ptr->error_type = error_type;
	route_error_ptr->salvage = salvage;
	
	/* Copy the relavent IP addresses	*/
	route_error_ptr->error_source_address = inet_address_copy (src_address);
	route_error_ptr->error_dest_address = inet_address_copy (dest_address);
	route_error_ptr->unreachable_node_address = inet_address_copy (unreachable_node_address);
	
	/* Allocate the main DSR header	*/
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_ROUTE_ERROR;
	
	/* There are three IP addresses in this packet	*/
	address_length = (inet_address_family_get (&src_address) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + address_length * 3;
	dsr_tlv_ptr->dsr_option_ptr = (void*) route_error_ptr;
	
	FRET (dsr_tlv_ptr);
	}


DsrT_Packet_Option*
dsr_pkt_support_ack_request_tlv_create (int ack_request_id)
	{
	DsrT_Acknowledgement_Request*	ack_request_ptr = OPC_NIL;
	DsrT_Packet_Option*				dsr_tlv_ptr = OPC_NIL;
	
	/** Creates the acknowledgement request	**/
	/** TLV in the DSR packet				**/
	FIN (dsr_pkt_support_ack_request_tlv_create (<args>));
	
	/* Allocate the acknowledgement request option	*/
	ack_request_ptr = dsr_pkt_support_ack_request_mem_alloc ();
	ack_request_ptr->identification = ack_request_id;
	
	/* Allocate the main DSR header	*/
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_ACK_REQUEST;
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS;
	dsr_tlv_ptr->dsr_option_ptr = (void*) ack_request_ptr;
	
	FRET (dsr_tlv_ptr);
	}


DsrT_Packet_Option*
dsr_pkt_support_acknowledgement_tlv_create (int ack_request_id, InetT_Address source_address, InetT_Address dest_address)
	{
	DsrT_Acknowledgement*		acknowledgement_ptr = OPC_NIL;
	DsrT_Packet_Option*			dsr_tlv_ptr = OPC_NIL;
	int							address_length;
	
	/** Creates the acknowledgement TLV in the DSR packet **/
	FIN (dsr_pkt_support_acknowledgement_tlv_create (<args>));
	
	/* Allocate the acknowledgement option	*/
	acknowledgement_ptr = dsr_pkt_support_acknowledgement_mem_alloc ();
	acknowledgement_ptr->identification = ack_request_id;
	acknowledgement_ptr->ack_source_address = inet_address_copy (source_address);
	acknowledgement_ptr->ack_dest_address = inet_address_copy (dest_address);
	
	/* Allocate the main DSR header	*/
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_ACKNOWLEDGEMENT;
	
	/* There are two IP addresses in the packet	*/
	address_length = (inet_address_family_get (&source_address) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + address_length * 2;
	dsr_tlv_ptr->dsr_option_ptr = (void*) acknowledgement_ptr;
	
	FRET (dsr_tlv_ptr);
	}


DsrT_Packet_Option*
dsr_pkt_support_source_route_tlv_create (List* route_lptr, Boolean first_hop_external, 
							Boolean last_hop_external, Boolean route_export, Boolean packet_salvaging)
	{
	DsrT_Source_Route_Option*	source_route_ptr = OPC_NIL;
	DsrT_Packet_Option*			dsr_tlv_ptr = OPC_NIL;
	InetT_Address*				hop_address_ptr;
	InetT_Address*				copy_address_ptr = OPC_NIL;
	int							num_hops, count;
	int							start_count;
	int							address_length;
	
	/** Creates the source route TLV in the DSR packet **/
	FIN (dsr_pkt_support_source_route_tlv_create (<args>));
	
	/* Allocate the source route option	*/
	source_route_ptr = dsr_pkt_support_source_route_mem_alloc ();
	source_route_ptr->first_hop_external = first_hop_external;
	source_route_ptr->last_hop_external = last_hop_external;
	source_route_ptr->export_route = route_export;
	
	/* Initailize the salvage to zero	*/
	source_route_ptr->salvage = 0;
	
	/* Get the number of hops	*/
	num_hops = op_prg_list_size (route_lptr);
	
	/* The source node needs to be included for	*/
	/* packet salvaging							*/
	if (packet_salvaging == OPC_FALSE)
		{
		start_count = 1;
		}
	else
		{
		/* Packet salvaging	*/
		start_count = 0;
		}
	
	/* Set the number of hops remaining	*/
	source_route_ptr->segments_left = num_hops - 2;
			
	/* Copy each hop except the first and last hop	*/
	/* since the source route contains all the		*/
	/* intermediate hops except the source and 		*/
	/* destination address							*/
	for (count = start_count; count < (num_hops - 1); count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (source_route_ptr->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Get the number of hops in the source route	*/
	num_hops = op_prg_list_size (source_route_ptr->route_lptr);
	
	/* Allocate the main DSR header	*/
	dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	dsr_tlv_ptr->option_type = DSRC_SOURCE_ROUTE;
	
	/* Set the option length based on the number	*/
	/* of IP addresses present in the option		*/
	if (copy_address_ptr != OPC_NIL)
		address_length = (inet_address_family_get (copy_address_ptr) == InetC_Addr_Family_v4 ? IP_V4_ADDRESS_LENGTH : IP_V6_ADDRESS_LENGTH);
	else
		address_length = 0;
	
	dsr_tlv_ptr->option_length = DSR_HEADER_OPTIONS + address_length * num_hops;
	dsr_tlv_ptr->dsr_option_ptr = (void*) source_route_ptr;
	
	FRET (dsr_tlv_ptr);
	}	
	

void
dsr_pkt_support_options_print (void* tlv_options_lptr1, VosT_Ll_Desc* output_list1)
	{
	char							temp_str [256];
	char							ip_addr_str [INETC_ADDR_STR_LEN];
	char*							alloc_str;
	char*							route_str;
	int								num_options, count;
	DsrT_Packet_Option*				dsr_tlv_ptr;
	DsrT_Route_Request_Option*		route_request_option_ptr;
	DsrT_Route_Reply_Option*		route_reply_option_ptr;
	DsrT_Route_Error_Option*		route_error_option_ptr;
	DsrT_Acknowledgement_Request*	ack_request_option_ptr;
	DsrT_Acknowledgement*			ack_option_ptr;
	DsrT_Source_Route_Option*		source_route_option_ptr;
	List* 							tlv_options_lptr;
	Prg_List* 						output_list;
	
	/** Prints the options set in the DSR packet	**/
	/** to an output list which is passd into the	**/
	/** print procedure for the packet print		**/
	FIN (dsr_pkt_support_options_print (<args>));
	
	/* Initialize the parameters. */
	tlv_options_lptr = (List*) tlv_options_lptr1;
	output_list = (Prg_List*) output_list1;
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Get each option and print its contents	*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		switch (dsr_tlv_ptr->option_type)
			{
			case (DSRC_ROUTE_REQUEST):
				{
				route_request_option_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      ROUTE REQUEST OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				sprintf (temp_str, "        Route Request ID                   %ld", route_request_option_ptr->identification);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				inet_address_print(ip_addr_str, route_request_option_ptr->target_address);
				sprintf (temp_str, "        Route Request Target Address        %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				route_str = dsr_support_route_print (route_request_option_ptr->route_lptr);
				strcpy (temp_str, "        Route Traversed");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				DSR_PKPRINT_STRING_INSERT (alloc_str, route_str, output_list);
				
				break;
				}
			
			case (DSRC_ROUTE_REPLY):
				{
				route_reply_option_ptr = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      ROUTE REPLY OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				route_str = dsr_support_route_print (route_reply_option_ptr->route_lptr);
				strcpy (temp_str, "        Reply Packet Route");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				DSR_PKPRINT_STRING_INSERT (alloc_str, route_str, output_list);
				
				break;
				}
			
			case (DSRC_ROUTE_ERROR):
				{
				route_error_option_ptr = (DsrT_Route_Error_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      ROUTE ERROR OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				if (route_error_option_ptr->error_type == 1)
					{
					strcpy (temp_str, "        Route Error Cause                   Unreachable Node");
					DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
					}
				
				inet_address_print(ip_addr_str, route_error_option_ptr->error_source_address);
				sprintf (temp_str, "        Route Error Source Address        %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				inet_address_print(ip_addr_str, route_error_option_ptr->error_dest_address);
				sprintf (temp_str, "        Route Error Destination Address   %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				inet_address_print(ip_addr_str, route_error_option_ptr->unreachable_node_address);
				sprintf (temp_str, "        Unreachable Node Address          %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				sprintf (temp_str, "        Salvage Count                     %d", route_error_option_ptr->salvage);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				break;
				}
				
			case (DSRC_ACK_REQUEST):
				{
				ack_request_option_ptr = (DsrT_Acknowledgement_Request*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      ACKNOWLEDGEMENT REQUEST OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				sprintf (temp_str, "        Acknowledgement Request ID        %ld", ack_request_option_ptr->identification);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				break;
				}
			
			case (DSRC_ACKNOWLEDGEMENT):
				{
				ack_option_ptr = (DsrT_Acknowledgement*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      ACKNOWLEDGEMENT OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				sprintf (temp_str, "        Acknowledgement ID                    %ld", ack_option_ptr->identification);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				inet_address_print(ip_addr_str, ack_option_ptr->ack_source_address);
				sprintf (temp_str, "        Acknowledgement Source Address        %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				inet_address_print(ip_addr_str, ack_option_ptr->ack_dest_address);
				sprintf (temp_str, "        Acknowledgement Destination Address   %s", ip_addr_str);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				break;
				}
				
			case (DSRC_SOURCE_ROUTE):
				{
				source_route_option_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
				
				strcpy (temp_str, "      SOURCE ROUTE OPTION");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				route_str = dsr_support_route_print (source_route_option_ptr->route_lptr);
				strcpy (temp_str, "        Source Route");
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				DSR_PKPRINT_STRING_INSERT (alloc_str, route_str, output_list);
				
				sprintf (temp_str, "        Number of Segments Left           %d", source_route_option_ptr->segments_left);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				sprintf (temp_str, "        Salvage Count                     %d", source_route_option_ptr->salvage);
				DSR_PKPRINT_STRING_INSERT (alloc_str, temp_str, output_list);
				
				break;
				}
			}
		}
	
	FOUT;
	}
				
				


/*****************************************************/
/*** MEMORY ALLOCATION AND DEALLOCATION FUNCTIONS ****/
/*****************************************************/

static List*
dsr_pkt_support_tlv_options_mem_copy (List* tlv_options_lptr)
	{
	List*					copy_tlv_options_lptr = OPC_NIL;
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	DsrT_Packet_Option*		copy_dsr_tlv_ptr = OPC_NIL;
	int						num_options, count;
	
	/** Copies the list of options in the DSR packet	**/
	FIN (dsr_pkt_support_tlv_options_mem_copy (<args>));
	
	/* Create a new list to place the copies of the options	*/
	copy_tlv_options_lptr = op_prg_list_create ();
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Get each option and copy the option	*/
		/* based on its option type				*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_access (tlv_options_lptr, count);
		
		switch (dsr_tlv_ptr->option_type)
			{
			case (DSRC_ROUTE_REQUEST):
				{
				/* Route Request Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_route_request_mem_copy (dsr_tlv_ptr);
				break;
				}
			
			case (DSRC_ROUTE_REPLY):
				{
				/* Route Reply Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_route_reply_mem_copy (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ROUTE_ERROR):
				{
				/* Route Error Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_route_error_mem_copy (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ACK_REQUEST):
				{
				/* Ack Request Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_ack_request_mem_copy (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ACKNOWLEDGEMENT):
				{
				/* Acknowledgement Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_acknowledgement_mem_copy (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_SOURCE_ROUTE):
				{
				/* DSR Source Route Option	*/
				copy_dsr_tlv_ptr = dsr_pkt_support_source_route_mem_copy (dsr_tlv_ptr);
				break;
				}
			
			default:
				{
				/* Invalid option type	*/
				op_sim_end ("Invalid Option Type in DSR packet", OPC_NIL, OPC_NIL, OPC_NIL);
				break;
				}
			}
		
		/* Insert the option into the options list	*/
		op_prg_list_insert (copy_tlv_options_lptr, copy_dsr_tlv_ptr, OPC_LISTPOS_TAIL);
		}
	
	FRET (copy_tlv_options_lptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_route_request_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Request_Option*		route_request_ptr = OPC_NIL;
	DsrT_Route_Request_Option*		copy_route_request_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					copy_address_ptr;
	DsrT_Packet_Option*				copy_dsr_tlv_ptr = OPC_NIL;
	
	/** Makes a copy of the route request options	**/
	FIN (dsr_pkt_support_route_request_mem_copy (<args>));
	
	/* Get the original route request options	*/
	route_request_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
		
	/* Allocate memory for the copy of the route request options	*/
	copy_route_request_ptr = dsr_pkt_support_route_request_mem_alloc ();
	copy_route_request_ptr->identification = route_request_ptr->identification;
	copy_route_request_ptr->target_address = inet_address_copy (route_request_ptr->target_address);
	
	/* Insert each hop that has already been traversed	*/
	/* into the route request option copy				*/
	if (route_request_ptr->route_lptr != OPC_NIL)
		{
		num_hops = op_prg_list_size (route_request_ptr->route_lptr);
		for (count = 0; count < num_hops; count++)
			{
			hop_address_ptr = (InetT_Address*) op_prg_list_access (route_request_ptr->route_lptr, count);
			copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
			op_prg_list_insert (copy_route_request_ptr->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
			}
		}
	
	/* Allocate memory for the copy of the DSR option header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_route_request_ptr;
	
	FRET (copy_dsr_tlv_ptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_route_reply_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Reply_Option*		route_reply_option = OPC_NIL;
	DsrT_Route_Reply_Option*		copy_route_reply_option = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	InetT_Address*					copy_address_ptr;
	DsrT_Packet_Option*				copy_dsr_tlv_ptr = OPC_NIL;
	
	/** Makes a copy of the route reply TLV field	**/
	FIN (dsr_pkt_support_route_reply_mem_copy (<args>));
	
	/* Get the original route reply option	*/
	route_reply_option = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Allocate memory for the copy of the route reply option	*/
	copy_route_reply_option = dsr_pkt_support_route_reply_mem_alloc ();
	copy_route_reply_option->last_hop_external = route_reply_option->last_hop_external;
	
	/* Get the number of hops which includes	*/
	/* the source and destination hops 			*/
	num_hops = op_prg_list_size (route_reply_option->route_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (route_reply_option->route_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (copy_route_reply_option->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Allocate the main DSR header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_route_reply_option;
	
	FRET (copy_dsr_tlv_ptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_route_error_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Error_Option*		route_error_option = OPC_NIL;
	DsrT_Route_Error_Option*		copy_route_error_option = OPC_NIL;
	DsrT_Packet_Option*				copy_dsr_tlv_ptr = OPC_NIL;
	
	/** Makes a copy of the route error TLV field	**/
	FIN (dsr_pkt_support_route_error_mem_copy (<args>));
	
	/* Get the original route error option	*/
	route_error_option = (DsrT_Route_Error_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Allocate memory for the copy	of the route error option */
	copy_route_error_option = dsr_pkt_support_route_error_mem_alloc ();
	copy_route_error_option->error_type = route_error_option->error_type;
	copy_route_error_option->salvage = route_error_option->salvage;
	
	/* Copy the relavent IP addresses	*/
	copy_route_error_option->error_source_address = inet_address_copy (route_error_option->error_source_address);
	copy_route_error_option->error_dest_address = inet_address_copy (route_error_option->error_dest_address);
	copy_route_error_option->unreachable_node_address = inet_address_copy (route_error_option->unreachable_node_address);
	
	/* Allocate the main DSR header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_route_error_option;
	
	FRET (copy_dsr_tlv_ptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_ack_request_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Acknowledgement_Request*	ack_request_option = OPC_NIL;
	DsrT_Acknowledgement_Request*	copy_ack_request_option = OPC_NIL;
	DsrT_Packet_Option*				copy_dsr_tlv_ptr = OPC_NIL;
	
	/** Makes a copy of the acknowledgement request TLV field	**/
	FIN (dsr_pkt_support_ack_request_mem_copy (<args>));
	
	/* Get the original ack request option	*/
	ack_request_option = (DsrT_Acknowledgement_Request*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Allocate memory for the copy of the ack request option	*/
	copy_ack_request_option = dsr_pkt_support_ack_request_mem_alloc ();
	copy_ack_request_option->identification = ack_request_option->identification;
	
	/* Allocate the main DSR header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_ack_request_option;
	
	FRET (copy_dsr_tlv_ptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_acknowledgement_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Acknowledgement*		acknowledgement_option = OPC_NIL;
	DsrT_Acknowledgement*		copy_acknowledgement_option = OPC_NIL;
	DsrT_Packet_Option*			copy_dsr_tlv_ptr = OPC_NIL;
	
	/** Makes a copy of the acknowledgement TLV field	**/
	FIN (dsr_pkt_support_acknowledgement_mem_copy (<args>));
	
	/* Get the original acknowledgement option	*/
	acknowledgement_option = (DsrT_Acknowledgement*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Allocate memory for the copy of the acknowledgement option	*/
	copy_acknowledgement_option = dsr_pkt_support_acknowledgement_mem_alloc ();
	copy_acknowledgement_option->identification = acknowledgement_option->identification;
	copy_acknowledgement_option->ack_source_address = inet_address_copy (acknowledgement_option->ack_source_address);
	copy_acknowledgement_option->ack_dest_address = inet_address_copy (acknowledgement_option->ack_dest_address);
	
	/* Allocate the main DSR header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_acknowledgement_option;
	
	FRET (copy_dsr_tlv_ptr);
	}


static DsrT_Packet_Option*
dsr_pkt_support_source_route_mem_copy (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Source_Route_Option*	source_route_option = OPC_NIL;
	DsrT_Source_Route_Option*	copy_source_route_option = OPC_NIL;
	DsrT_Packet_Option*			copy_dsr_tlv_ptr = OPC_NIL;
	int							count, num_hops;
	InetT_Address*				hop_address_ptr;
	InetT_Address*				copy_address_ptr;
	
	/** Makes a copy of the DSR Source Route TLV field	**/
	FIN (dsr_pkt_support_source_route_mem_copy (<args>));
	
	/* Get the original source route option	*/
	source_route_option = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Allocate memory for the copy of the acknowledgement option	*/
	copy_source_route_option = dsr_pkt_support_source_route_mem_alloc ();
	copy_source_route_option->first_hop_external = source_route_option->first_hop_external;
	copy_source_route_option->last_hop_external = source_route_option->last_hop_external;
	copy_source_route_option->export_route = source_route_option->export_route;
	copy_source_route_option->salvage = source_route_option->salvage;
	copy_source_route_option->segments_left = source_route_option->segments_left;
	
	/* Copy all the hops	*/
	num_hops = op_prg_list_size (source_route_option->route_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_access (source_route_option->route_lptr, count);
		copy_address_ptr = inet_address_copy_dynamic (hop_address_ptr);
		op_prg_list_insert (copy_source_route_option->route_lptr, copy_address_ptr, OPC_LISTPOS_TAIL);
		}
	
	/* Allocate the main DSR header	*/
	copy_dsr_tlv_ptr = dsr_pkt_support_option_mem_alloc ();
	copy_dsr_tlv_ptr->option_type = dsr_tlv_ptr->option_type;
	copy_dsr_tlv_ptr->option_length = dsr_tlv_ptr->option_length;
	copy_dsr_tlv_ptr->dsr_option_ptr = (void*) copy_source_route_option;
	
	FRET (copy_dsr_tlv_ptr);
	}


static void
dsr_pkt_support_tlv_options_mem_free (List* tlv_options_lptr)
	{
	DsrT_Packet_Option*		dsr_tlv_ptr = OPC_NIL;
	int						num_options, count;
	
	/** Frees the list of options in the DSR packet	**/
	FIN (dsr_pkt_support_tlv_options_mem_free (<args>));
	
	/* Get the number of options	*/
	num_options = op_prg_list_size (tlv_options_lptr);
	
	for (count = 0; count < num_options; count++)
		{
		/* Get each option and free the option	*/
		/* based on its option type				*/
		dsr_tlv_ptr = (DsrT_Packet_Option*) op_prg_list_remove (tlv_options_lptr, OPC_LISTPOS_HEAD);
		
		switch (dsr_tlv_ptr->option_type)
			{
			case (DSRC_ROUTE_REQUEST):
				{
				/* Route Request Option	*/
				dsr_pkt_support_route_request_mem_free (dsr_tlv_ptr);
				break;
				}
			
			case (DSRC_ROUTE_REPLY):
				{
				/* Route Reply Option	*/
				dsr_pkt_support_route_reply_mem_free (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ROUTE_ERROR):
				{
				/* Route Error Option	*/
				dsr_pkt_support_route_error_mem_free (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ACK_REQUEST):
				{
				/* Ack Request Option	*/
				dsr_pkt_support_ack_request_mem_free (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_ACKNOWLEDGEMENT):
				{
				/* Acknowledgement Option	*/
				dsr_pkt_support_acknowledgement_mem_free (dsr_tlv_ptr);
				break;
				}
				
			case (DSRC_SOURCE_ROUTE):
				{
				/* DSR Source Route Option	*/
				dsr_pkt_support_source_route_mem_free (dsr_tlv_ptr);
				break;
				}
			
			default:
				{
				/* Invalid option type	*/
				op_sim_end ("Invalid Option Type in DSR packet", OPC_NIL, OPC_NIL, OPC_NIL);
				break;
				}
			}
		}
	
	/* Free the list	*/
	op_prg_mem_free (tlv_options_lptr);
	
	FOUT;
	}


static void
dsr_pkt_support_route_request_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Request_Option*		route_request_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	
	/** Frees the memory associated with a route request	**/
	/** option in the DSR packet header						**/
	FIN (dsr_pkt_support_route_request_mem_free (<args>));
	
	/* Get the route request options	*/
	route_request_ptr = (DsrT_Route_Request_Option*) dsr_tlv_ptr->dsr_option_ptr;
	inet_address_destroy (route_request_ptr->target_address);
	
	/* Free each hop in the route	*/
	if (route_request_ptr->route_lptr != OPC_NIL)
		{
		num_hops = op_prg_list_size (route_request_ptr->route_lptr);
		for (count = 0; count < num_hops; count++)
			{
			hop_address_ptr = (InetT_Address*) op_prg_list_remove (route_request_ptr->route_lptr, OPC_LISTPOS_HEAD);
			inet_address_destroy_dynamic (hop_address_ptr);
			}
		}
	
	op_prg_mem_free (route_request_ptr->route_lptr);
	op_prg_mem_free (route_request_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


static void
dsr_pkt_support_route_reply_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Reply_Option*		route_reply_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	
	/** Frees the route reply TLV	**/
	FIN (dsr_pkt_support_route_reply_mem_free (<args>));
	
	/* Get the route reply options	*/
	route_reply_ptr = (DsrT_Route_Reply_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Get the number of hops and destroy each hop	*/
	num_hops = op_prg_list_size (route_reply_ptr->route_lptr);
	
	for (count = 0; count < num_hops; count ++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (route_reply_ptr->route_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		}
	
	/* Free the list and the TLV	*/
	op_prg_mem_free (route_reply_ptr->route_lptr);
	op_prg_mem_free (route_reply_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


static void
dsr_pkt_support_route_error_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Route_Error_Option*		route_error_ptr = OPC_NIL;
	
	/** Frees the Route Error TLV	**/
	FIN (dsr_pkt_support_route_error_mem_free (<args>));
	
	/* Get the route error options	*/
	route_error_ptr = (DsrT_Route_Error_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Free the IP addresses	*/
	inet_address_destroy (route_error_ptr->error_source_address);
	inet_address_destroy (route_error_ptr->error_dest_address);
	inet_address_destroy (route_error_ptr->unreachable_node_address);
	
	/* Free the memory	*/
	op_prg_mem_free (route_error_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


static void
dsr_pkt_support_ack_request_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Acknowledgement_Request*	ack_request_ptr = OPC_NIL;
	
	/** Frees the Acknowledgement Request TLV	**/
	FIN (dsr_pkt_support_ack_request_mem_free (<args>));
	
	/* Get the acknowledgement request options	*/
	ack_request_ptr = (DsrT_Acknowledgement_Request*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Free the memory	*/
	op_prg_mem_free (ack_request_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


static void
dsr_pkt_support_acknowledgement_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Acknowledgement*		acknowledgement_ptr = OPC_NIL;
	
	/** Frees the Acknowledgement TLV	**/
	FIN (dsr_pkt_support_acknowledgement_mem_free (<void>));
	
	/* Get the acknowledgement option	*/
	acknowledgement_ptr = (DsrT_Acknowledgement*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Destroy the IP addresses	*/
	inet_address_destroy (acknowledgement_ptr->ack_source_address);
	inet_address_destroy (acknowledgement_ptr->ack_dest_address);
	
	/* Free the memory	*/
	op_prg_mem_free (acknowledgement_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


static void
dsr_pkt_support_source_route_mem_free (DsrT_Packet_Option* dsr_tlv_ptr)
	{
	DsrT_Source_Route_Option*		source_route_ptr = OPC_NIL;
	int								num_hops, count;
	InetT_Address*					hop_address_ptr;
	
	/** Frees the DSR Source Route TLV	**/
	FIN (dsr_pkt_support_source_route_mem_free (<args>));
	
	/* Get the source route option	*/
	source_route_ptr = (DsrT_Source_Route_Option*) dsr_tlv_ptr->dsr_option_ptr;
	
	/* Free the IP addresses for each hop	*/
	num_hops = op_prg_list_size (source_route_ptr->route_lptr);
	
	for (count = 0; count < num_hops; count++)
		{
		hop_address_ptr = (InetT_Address*) op_prg_list_remove (source_route_ptr->route_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (hop_address_ptr);
		}
	
	/* Free the memory allocated	*/
	op_prg_mem_free (source_route_ptr->route_lptr);
	op_prg_mem_free (source_route_ptr);
	op_prg_mem_free (dsr_tlv_ptr);
	
	FOUT;
	}


/*****************************************************/
/******** POOLED MEMORY ALLOCATION FUNCTIONS *********/
/*****************************************************/

static DsrT_Packet_Option*
dsr_pkt_support_option_mem_alloc (void)
	{
	static Pmohandle		dsr_option_pmh;
	static Boolean			dsr_option_pmh_defined = OPC_FALSE;
	DsrT_Packet_Option*		dsr_header_ptr = OPC_NIL;
	
	/** Allocates pooled memory for the DSR header options	**/
	FIN (dsr_pkt_support_option_mem_alloc (<args>));
	
	if (dsr_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory for the DSR option header	*/
		dsr_option_pmh = op_prg_pmo_define ("DSR TLV Option", sizeof (DsrT_Packet_Option), 32);
		dsr_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the DSR header from the pooled memory	*/
	dsr_header_ptr = (DsrT_Packet_Option*) op_prg_pmo_alloc (dsr_option_pmh);
	
	FRET (dsr_header_ptr);
	}


static DsrT_Route_Request_Option*
dsr_pkt_support_route_request_mem_alloc (void)
	{
	static Pmohandle				route_request_pmh;
	DsrT_Route_Request_Option*		route_request_ptr = OPC_NIL;
	static Boolean					route_request_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route request	**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_route_request_mem_alloc (<args>));
	
	if (route_request_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route request	*/
		/* option in the DSR packets if not already defined	*/
		route_request_pmh = op_prg_pmo_define ("Route Request Option", sizeof (DsrT_Route_Request_Option), 32);
		route_request_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route request options from the pooled memory	*/
	route_request_ptr = (DsrT_Route_Request_Option*) op_prg_pmo_alloc (route_request_pmh);
	route_request_ptr->route_lptr = op_prg_list_create ();
	
	FRET (route_request_ptr);
	}


static DsrT_Route_Reply_Option*
dsr_pkt_support_route_reply_mem_alloc (void)
	{
	static Pmohandle				route_reply_pmh;
	DsrT_Route_Reply_Option*		route_reply_ptr = OPC_NIL;
	static Boolean					route_reply_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route reply		**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_route_reply_mem_alloc (void));
	
	if (route_reply_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route reply	*/
		/* option in the DSR packets if not already defined	*/
		route_reply_pmh = op_prg_pmo_define ("Route Reply Option", sizeof (DsrT_Route_Reply_Option), 32);
		route_reply_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route reply options from the pooled memory	*/
	route_reply_ptr = (DsrT_Route_Reply_Option*) op_prg_pmo_alloc (route_reply_pmh);
	route_reply_ptr->route_lptr = op_prg_list_create ();
	
	FRET (route_reply_ptr);
	}


static DsrT_Route_Error_Option*
dsr_pkt_support_route_error_mem_alloc (void)
	{
	static Pmohandle				route_error_pmh;
	DsrT_Route_Error_Option*		route_error_ptr = OPC_NIL;
	static Boolean					route_error_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route error		**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_route_error_mem_alloc (void));
	
	if (route_error_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route error	*/
		/* option in the DSR packets if not already defined	*/
		route_error_pmh = op_prg_pmo_define ("Route Error Option", sizeof (DsrT_Route_Error_Option), 32);
		route_error_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route error options from the pooled memory	*/
	route_error_ptr = (DsrT_Route_Error_Option*) op_prg_pmo_alloc (route_error_pmh);
	
	FRET (route_error_ptr);
	}


static DsrT_Acknowledgement_Request*
dsr_pkt_support_ack_request_mem_alloc (void)
	{
	static Pmohandle				ack_request_pmh;
	DsrT_Acknowledgement_Request*	ack_request_ptr = OPC_NIL;
	static Boolean					ack_request_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the ack request		**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_ack_request_mem_alloc (void));
	
	if (ack_request_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for ack request	*/
		/* option in the DSR packets if not already defined	*/
		ack_request_pmh = op_prg_pmo_define ("Acknowledgement Request Option", sizeof (DsrT_Acknowledgement_Request), 32);
		ack_request_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the acknowledgement request option from the pooled memory	*/
	ack_request_ptr = (DsrT_Acknowledgement_Request*) op_prg_pmo_alloc (ack_request_pmh);
	
	FRET (ack_request_ptr);
	}


static DsrT_Acknowledgement*
dsr_pkt_support_acknowledgement_mem_alloc (void)
	{
	static Pmohandle				acknowledgement_pmh;
	DsrT_Acknowledgement*			acknowledgement_ptr = OPC_NIL;
	static Boolean					acknowledgement_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the acknowledgement	**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_acknowledgement_mem_alloc (void));
	
	if (acknowledgement_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for acknowledgement	*/
		/* option in the DSR packets if not already defined	*/
		acknowledgement_pmh = op_prg_pmo_define ("Acknowledgement Option", sizeof (DsrT_Acknowledgement), 32);
		acknowledgement_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the acknowledgement option from the pooled memory	*/
	acknowledgement_ptr = (DsrT_Acknowledgement*) op_prg_pmo_alloc (acknowledgement_pmh);
	
	FRET (acknowledgement_ptr);
	}


static DsrT_Source_Route_Option*
dsr_pkt_support_source_route_mem_alloc (void)
	{
	static Pmohandle				source_route_pmh;
	DsrT_Source_Route_Option*		source_route_ptr = OPC_NIL;
	static Boolean					source_route_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the source route	**/
	/** options in the DSR header						**/
	FIN (dsr_pkt_support_source_route_mem_alloc (void));
	
	if (source_route_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for source route	*/
		/* option in the DSR packets if not already defined	*/
		source_route_pmh = op_prg_pmo_define ("Source Route Option", sizeof (DsrT_Source_Route_Option), 32);
		source_route_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the source route option from the pooled memory	*/
	source_route_ptr = (DsrT_Source_Route_Option*) op_prg_pmo_alloc (source_route_pmh);
	source_route_ptr->first_hop_external = OPC_FALSE;
	source_route_ptr->last_hop_external = OPC_FALSE;
	source_route_ptr->export_route = OPC_FALSE;
	source_route_ptr->route_lptr = op_prg_list_create ();
	
	FRET (source_route_ptr);
	}
