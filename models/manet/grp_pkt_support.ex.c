/* grp_pkt_support.ex.c */
/* C support file for creation and deletion of GRP packets */


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
#include <grp.h>
#include <grp_pkt_support.h>
#include <grp_ptypes.h>
#include <ip_addr_v4.h>

/* File Globals	*/
static int 			fd_options_index = OPC_FIELD_INDEX_INVALID;


/***** Prototypes *****/
static GrpT_Packet_Option*				grp_pkt_support_tlv_mem_alloc (void);
static GrpT_Hello_Option*				grp_pkt_support_hello_option_mem_alloc (void);
static GrpT_Flooding_Option*			grp_pkt_support_flooding_option_mem_alloc (void);
static GrpT_Data_Option*				grp_pkt_support_data_option_mem_alloc (void);
static GrpT_Dest_Pos_Req_Option*		grp_pkt_support_pos_req_option_mem_alloc (void);
static GrpT_Dest_Pos_Resp_Option*		grp_pkt_support_pos_resp_option_mem_alloc (void);
static GrpT_Dest_Response*				grp_pkt_support_dest_resp_alloc (void);
static GrpT_Packet_Option*				grp_pkt_support_tlv_options_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static GrpT_Hello_Option*				grp_pkt_support_hello_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static GrpT_Flooding_Option*			grp_pkt_support_flooding_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static GrpT_Data_Option*				grp_pkt_support_data_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static GrpT_Dest_Pos_Req_Option*		grp_pkt_support_pos_req_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static GrpT_Dest_Pos_Resp_Option*		grp_pkt_support_pos_resp_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_tlv_options_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_hello_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_flooding_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_data_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_pos_req_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_pos_resp_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr);
static void								grp_pkt_support_tlv_mem_free (GrpT_Packet_Option* grp_tlv_ptr);


/*****************************************************/
/************* PACKET CREATION FUNCTIONS *************/
/*****************************************************/

Packet*
grp_pkt_support_pkt_create (int next_header)
	{
	Packet*		grp_pkptr;
	
	/** Creates the GRP packet	**/
	FIN (grp_pkt_support_pkt_create (void));
	
	/* Create the GRP packet	*/
	grp_pkptr = op_pk_create_fmt ("grp");
	
	/* Set the next header in the packet	*/
	/* The next header field of the GRP 	*/
	/* packet is set to the protocol field	*/
	/* of the IP datagram					*/
	op_pk_nfd_set (grp_pkptr, "Next Header", next_header);
	
	FRET (grp_pkptr);
	}


void
grp_pkt_support_option_set (Packet* grp_pkptr, GrpT_Packet_Option* grp_tlv_ptr)
	{
	/** Sets the GRP options in the packet	**/
	FIN (grp_pkt_support_option_set (<args>));
	
	/* Get the field index for the options field	*/
	if (fd_options_index == OPC_FIELD_INDEX_INVALID)
		fd_options_index = op_pk_nfd_name_to_index (grp_pkptr, "Options");
	
	/* Set the GRP option in the packet	*/
	op_pk_fd_set (grp_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, grp_tlv_ptr,
					grp_tlv_ptr->option_length, grp_pkt_support_tlv_options_mem_copy, 
					grp_pkt_support_tlv_options_mem_free, sizeof (GrpT_Packet_Option));
	
	FOUT;
	}


/*****************************************************/
/************** TLV CREATION FUNCTIONS ***************/
/*****************************************************/

GrpT_Packet_Option*
grp_pkt_support_hello_tlv_create (InetT_Address node_address, double node_lat, double node_long, int type)
	{
	GrpT_Hello_Option*		hello_option_ptr;
	GrpT_Packet_Option*		grp_tlv_ptr;
	
	/** Creates the TLV option for the Hello packet	**/
	FIN (grp_pkt_support_hello_tlv_create (<args>));
	
	hello_option_ptr = grp_pkt_support_hello_option_mem_alloc ();
	hello_option_ptr->nbr_addr = inet_address_copy (node_address);
	hello_option_ptr->hello_type = type;
	hello_option_ptr->nbr_lat = node_lat;
	hello_option_ptr->nbr_long = node_long;
	hello_option_ptr->timestamp = op_sim_time ();
	
	grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	grp_tlv_ptr->option_type = GRPC_HELLO_OPTION;
	grp_tlv_ptr->option_length = HELLO_PKT_SIZE + GRP_TLV_SIZE;
	grp_tlv_ptr->grp_option_ptr = hello_option_ptr;
	
	FRET (grp_tlv_ptr);
	}


GrpT_Packet_Option*
grp_pkt_support_flooding_tlv_create (InetT_Address node_address, double node_lat, double node_long,
									double low_quad_lat, double low_quad_long, double high_quad_lat,
									double high_quad_long)
	{
	GrpT_Flooding_Option*		flood_option_ptr;
	GrpT_Packet_Option*			grp_tlv_ptr;
	
	/** Creates the TLV option for the Flooding packet	**/
	FIN (grp_pkt_support_flooding_tlv_create (<args>));
	
	flood_option_ptr = grp_pkt_support_flooding_option_mem_alloc ();
	flood_option_ptr->node_addr = inet_address_copy (node_address);
	flood_option_ptr->node_lat = node_lat;
	flood_option_ptr->node_long = node_long;
	flood_option_ptr->timestamp = op_sim_time ();
	flood_option_ptr->low_quad_lat = low_quad_lat;
	flood_option_ptr->low_quad_long = low_quad_long;
	flood_option_ptr->high_quad_lat = high_quad_lat;
	flood_option_ptr->high_quad_long = high_quad_long;
	
	grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	grp_tlv_ptr->option_type = GRPC_FLOODING_OPTION;
	grp_tlv_ptr->option_length = FLOODING_PKT_SIZE + GRP_TLV_SIZE;
	grp_tlv_ptr->grp_option_ptr = flood_option_ptr;
	
	FRET (grp_tlv_ptr);
	}


GrpT_Packet_Option*
grp_pkt_support_data_tlv_create (InetT_Address dest_address, double prev_dist_to_dest, GrpT_Destination_Type dest_type,
								double dest_lat, double dest_long, double quadrant_size, double timestamp, 
								InetT_Address node_traversed)
	{
	GrpT_Data_Option*			data_option_ptr;
	GrpT_Packet_Option*			grp_tlv_ptr;
	int							list_size, total_size = 0;
	InetT_Address*				node_traversed_ptr;
	
	/** Creates the option for the data packet	**/
	FIN (grp_pkt_support_data_tlv_create (<args>));
	
	data_option_ptr = grp_pkt_support_data_option_mem_alloc ();
	data_option_ptr->dest_addr = inet_address_copy (dest_address);
	data_option_ptr->prev_dist_to_dest = prev_dist_to_dest;
	data_option_ptr->dest_type = dest_type;
	data_option_ptr->dest_lat = dest_lat;
	data_option_ptr->dest_long = dest_long;
	data_option_ptr->quadrant_size = quadrant_size;
	data_option_ptr->timestamp = timestamp;
	
	node_traversed_ptr = inet_address_create_dynamic (node_traversed);
	op_prg_list_insert (data_option_ptr->nodes_traversed_lptr, node_traversed_ptr, OPC_LISTPOS_TAIL);
	
	list_size = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
	total_size = GRP_DATA_HEADER_SIZE + list_size * IPC_V4_ADDR_LEN;
	
	grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	grp_tlv_ptr->option_type = GRPC_DATA_OPTION;
	grp_tlv_ptr->option_length = total_size + GRP_TLV_SIZE;
	grp_tlv_ptr->grp_option_ptr = data_option_ptr;
	
	FRET (grp_tlv_ptr);
	}


GrpT_Packet_Option*
grp_pkt_support_pos_req_tlv_create (InetT_Address nbr_node_addr, List* dest_lptr)
	{
	GrpT_Dest_Pos_Req_Option*		pos_req_option_ptr;
	int								num_elements, list_size, total_size;
	GrpT_Packet_Option*				grp_tlv_ptr;
	
	/** Creates the option for position request packet	**/
	FIN (grp_pkt_support_pos_req_tlv_create (<args>));
	
	pos_req_option_ptr = grp_pkt_support_pos_req_option_mem_alloc ();
	pos_req_option_ptr->nbr_addr = inet_address_copy (nbr_node_addr);
	pos_req_option_ptr->dest_req_lptr = dest_lptr;
	
	num_elements = op_prg_list_size (dest_lptr);
	list_size = IPC_V4_ADDR_LEN * num_elements;
	total_size = GRP_POS_REQ_TLV_SIZE + list_size;
	
	grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	grp_tlv_ptr->option_type = GRPC_POS_REQ_OPTION;
	grp_tlv_ptr->option_length = total_size + GRP_TLV_SIZE;
	grp_tlv_ptr->grp_option_ptr = pos_req_option_ptr;
	
	FRET (grp_tlv_ptr);
	}


GrpT_Dest_Pos_Resp_Option*
grp_pkt_support_pos_resp_option_create (InetT_Address node_addr)
	{
	GrpT_Dest_Pos_Resp_Option*		pos_resp_option_ptr;
	
	/** Creates the position response option	**/
	FIN (grp_pkt_support_pos_resp_tlv_create (<args>));
	
	pos_resp_option_ptr = grp_pkt_support_pos_resp_option_mem_alloc ();
	pos_resp_option_ptr->node_addr = inet_address_copy (node_addr);
	pos_resp_option_ptr->desp_resp_lptr = op_prg_list_create ();
	
	FRET (pos_resp_option_ptr);
	}


void
grp_pkt_support_pos_resp_option_dest_add (GrpT_Dest_Pos_Resp_Option* pos_resp_option_ptr, InetT_Address dest_addr, double dest_lat,
											double dest_long, GrpT_Destination_Type dest_type, double quadrant_size, double timestamp)
	{
	GrpT_Dest_Response*			dest_resp_ptr;
	
	/** Adds the destination information to the position response option	**/
	FIN (grp_pkt_support_pos_resp_option_dest_add (<args>));
	
	dest_resp_ptr = grp_pkt_support_dest_resp_alloc ();
	dest_resp_ptr->dest_addr = inet_address_copy (dest_addr);
	dest_resp_ptr->dest_lat = dest_lat;
	dest_resp_ptr->dest_long = dest_long;
	dest_resp_ptr->dest_type = dest_type;
	dest_resp_ptr->quadrant_size = quadrant_size;
	dest_resp_ptr->timestamp = timestamp;
	
	/* Add the destination info to the response option	*/
	op_prg_list_insert (pos_resp_option_ptr->desp_resp_lptr, dest_resp_ptr, OPC_LISTPOS_TAIL);
	
	FOUT;
	}


GrpT_Packet_Option*
grp_pkt_support_pos_resp_tlv_create (GrpT_Dest_Pos_Resp_Option* pos_resp_option_ptr)
	{
	int						num_elem, total_size;
	GrpT_Packet_Option*		grp_tlv_ptr;
	
	/** Creates the position response TLV	**/
	FIN (grp_pkt_support_pos_resp_tlv_create (<args>));
	
	num_elem = op_prg_list_size (pos_resp_option_ptr->desp_resp_lptr);
	total_size = GRP_POS_REQ_TLV_SIZE + num_elem * GRP_DEST_INFO_SIZE;
	
	grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	grp_tlv_ptr->option_type = GRPC_POS_RESP_OPTION;
	grp_tlv_ptr->option_length = total_size + GRP_TLV_SIZE;
	grp_tlv_ptr->grp_option_ptr = pos_resp_option_ptr;
	
	FRET (grp_tlv_ptr);
	}



/*****************************************************/
/******** POOLED MEMORY ALLOCATION FUNCTIONS *********/
/*****************************************************/


static GrpT_Packet_Option*
grp_pkt_support_tlv_mem_alloc (void)
	{
	static Pmohandle				tlv_pmh;
	GrpT_Packet_Option*				tlv_ptr = OPC_NIL;
	static Boolean					tlv_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the TLV		**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_tlv_mem_alloc (void));
	
	if (tlv_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for tlv			*/
		/* option in the GRP packets if not already defined	*/
		tlv_pmh = op_prg_pmo_define ("GRP TLV Option", sizeof (GrpT_Packet_Option), 32);
		tlv_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the TLV options from the pooled memory	*/
	tlv_ptr = (GrpT_Packet_Option*) op_prg_pmo_alloc (tlv_pmh);
	
	FRET (tlv_ptr);
	}


static GrpT_Hello_Option*
grp_pkt_support_hello_option_mem_alloc (void)
	{
	static Pmohandle				hello_pmh;
	GrpT_Hello_Option*				hello_ptr = OPC_NIL;
	static Boolean					hello_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the hello	**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_hello_option_mem_alloc (void));
	
	if (hello_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for hello			*/
		/* option in the GRP packets if not already defined	*/
		hello_pmh = op_prg_pmo_define ("GRP Hello Option", sizeof (GrpT_Hello_Option), 32);
		hello_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the hello options from the pooled memory	*/
	hello_ptr = (GrpT_Hello_Option*) op_prg_pmo_alloc (hello_pmh);
	
	FRET (hello_ptr);
	}


static GrpT_Flooding_Option*
grp_pkt_support_flooding_option_mem_alloc (void)
	{
	static Pmohandle				flooding_pmh;
	GrpT_Flooding_Option*			flooding_ptr = OPC_NIL;
	static Boolean					flooding_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the flooding	**/
	/** options in the GRP header					**/
	FIN (grp_pkt_support_flooding_option_mem_alloc (void));
	
	if (flooding_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for flooding		*/
		/* option in the GRP packets if not already defined	*/
		flooding_pmh = op_prg_pmo_define ("GRP Flooding Option", sizeof (GrpT_Flooding_Option), 32);
		flooding_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the flooding options from the pooled memory	*/
	flooding_ptr = (GrpT_Flooding_Option*) op_prg_pmo_alloc (flooding_pmh);
	
	FRET (flooding_ptr);
	}


static GrpT_Data_Option*
grp_pkt_support_data_option_mem_alloc (void)
	{
	static Pmohandle				data_pmh;
	GrpT_Data_Option*				data_ptr = OPC_NIL;
	static Boolean					data_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the data	**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_data_option_mem_alloc (void));
	
	if (data_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for data			*/
		/* option in the GRP packets if not already defined	*/
		data_pmh = op_prg_pmo_define ("GRP Data Option", sizeof (GrpT_Data_Option), 32);
		data_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the flooding options from the pooled memory	*/
	data_ptr = (GrpT_Data_Option*) op_prg_pmo_alloc (data_pmh);
	data_ptr->nodes_traversed_lptr = op_prg_list_create ();
	
	FRET (data_ptr);
	}


static GrpT_Dest_Pos_Req_Option*
grp_pkt_support_pos_req_option_mem_alloc (void)
	{
	static Pmohandle				pos_req_pmh;
	GrpT_Dest_Pos_Req_Option*		pos_req_ptr = OPC_NIL;
	static Boolean					pos_req_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the pos req	**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_pos_req_option_mem_alloc (<args>));
	
	if (pos_req_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for position req	*/
		/* option in the GRP packets if not already defined	*/
		pos_req_pmh = op_prg_pmo_define ("GRP Pos Req Option", sizeof (GrpT_Dest_Pos_Req_Option), 32);
		pos_req_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the pos req options from the pooled memory	*/
	pos_req_ptr = (GrpT_Dest_Pos_Req_Option*) op_prg_pmo_alloc (pos_req_pmh);
	
	FRET (pos_req_ptr);
	}


static GrpT_Dest_Pos_Resp_Option*
grp_pkt_support_pos_resp_option_mem_alloc (void)
	{
	static Pmohandle				pos_resp_pmh;
	GrpT_Dest_Pos_Resp_Option*		pos_resp_ptr = OPC_NIL;
	static Boolean					pos_resp_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the pos resp**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_pos_resp_option_mem_alloc (void));
	
	if (pos_resp_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for position resp	*/
		/* option in the GRP packets if not already defined	*/
		pos_resp_pmh = op_prg_pmo_define ("GRP Pos Resp Option", sizeof (GrpT_Dest_Pos_Resp_Option), 32);
		pos_resp_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the pos resp options from the pooled memory	*/
	pos_resp_ptr = (GrpT_Dest_Pos_Resp_Option*) op_prg_pmo_alloc (pos_resp_pmh);
	
	FRET (pos_resp_ptr);
	}


static GrpT_Dest_Response*
grp_pkt_support_dest_resp_alloc (void)
	{
	static Pmohandle				dest_resp_pmh;
	GrpT_Dest_Response*				dest_resp_ptr = OPC_NIL;
	static Boolean					dest_resp_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the dest resp**/
	/** options in the GRP header				**/
	FIN (grp_pkt_support_dest_resp_alloc (void));
	
	if (dest_resp_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for dest resp		*/
		/* option in the GRP packets if not already defined	*/
		dest_resp_pmh = op_prg_pmo_define ("GRP Dest Resp Option", sizeof (GrpT_Dest_Response), 32);
		dest_resp_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the dest resp options from the pooled memory	*/
	dest_resp_ptr = (GrpT_Dest_Response*) op_prg_pmo_alloc (dest_resp_pmh);
	
	FRET (dest_resp_ptr);
	}


static GrpT_Packet_Option*
grp_pkt_support_tlv_options_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Packet_Option*		copy_grp_tlv_ptr;
	void*					copy_option_ptr;
	
	/** Copies the TLV options in the GRP header	**/
	FIN (grp_pkt_support_tlv_options_mem_copy (<args>));
	
	switch (grp_tlv_ptr->option_type)
		{
		case (GRPC_HELLO_OPTION):
			{
			/* Hello option	*/
			copy_option_ptr = (void*) grp_pkt_support_hello_option_mem_copy (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_FLOODING_OPTION):
			{
			/* Flooding Option	*/
			copy_option_ptr = (void*) grp_pkt_support_flooding_option_mem_copy (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_DATA_OPTION):
			{
			/* Data Option	*/
			copy_option_ptr = (void*) grp_pkt_support_data_option_mem_copy (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_POS_REQ_OPTION):
			{
			/* Position Request Option	*/
			copy_option_ptr = (void*) grp_pkt_support_pos_req_option_mem_copy (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_POS_RESP_OPTION):
			{
			/* Position Response Option	*/
			copy_option_ptr = (void*) grp_pkt_support_pos_resp_option_mem_copy (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_BACKTRACK_OPTION):
			{
			copy_option_ptr = (void*) grp_pkt_support_data_option_mem_copy (grp_tlv_ptr);
			break;
			}
		
		default:
			{
			/* Invalid option type	*/
			op_sim_end ("Invalid Option Type in GRP packet", OPC_NIL, OPC_NIL, OPC_NIL);
			break;
			}
		}
	
	copy_grp_tlv_ptr = grp_pkt_support_tlv_mem_alloc ();
	copy_grp_tlv_ptr->option_type = grp_tlv_ptr->option_type;
	copy_grp_tlv_ptr->option_length = grp_tlv_ptr->option_length;
	copy_grp_tlv_ptr->grp_option_ptr = copy_option_ptr;
	
	FRET (copy_grp_tlv_ptr);
	}


static GrpT_Hello_Option*
grp_pkt_support_hello_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Hello_Option*		hello_option_ptr;
	GrpT_Hello_Option*		copy_hello_option_ptr;
	
	/** Copies the Hello option in the GRP TLV	**/
	FIN (grp_pkt_support_hello_option_mem_copy (<args>));
	
	hello_option_ptr = (GrpT_Hello_Option*) grp_tlv_ptr->grp_option_ptr;
	
	copy_hello_option_ptr = grp_pkt_support_hello_option_mem_alloc ();
	copy_hello_option_ptr->nbr_addr = inet_address_copy (hello_option_ptr->nbr_addr);
	copy_hello_option_ptr->hello_type = hello_option_ptr->hello_type;
	copy_hello_option_ptr->nbr_lat = hello_option_ptr->nbr_lat;
	copy_hello_option_ptr->nbr_long = hello_option_ptr->nbr_long;
	copy_hello_option_ptr->timestamp = hello_option_ptr->timestamp;
	
	FRET (copy_hello_option_ptr);
	}


static GrpT_Flooding_Option*
grp_pkt_support_flooding_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Flooding_Option*		flooding_option_ptr;
	GrpT_Flooding_Option*		copy_flooding_option_ptr;
	
	/** Copies the flooding option in the GRP TLV	**/
	FIN (grp_pkt_support_flooding_option_mem_copy (<args>));
	
	flooding_option_ptr = (GrpT_Flooding_Option*) grp_tlv_ptr->grp_option_ptr;
	
	copy_flooding_option_ptr = grp_pkt_support_flooding_option_mem_alloc ();
	copy_flooding_option_ptr->node_addr = inet_address_copy (flooding_option_ptr->node_addr);
	copy_flooding_option_ptr->node_lat = flooding_option_ptr->node_lat;
	copy_flooding_option_ptr->node_long = flooding_option_ptr->node_long;
	copy_flooding_option_ptr->timestamp = flooding_option_ptr->timestamp;
	copy_flooding_option_ptr->low_quad_lat = flooding_option_ptr->low_quad_lat;
	copy_flooding_option_ptr->low_quad_long = flooding_option_ptr->low_quad_long;
	copy_flooding_option_ptr->high_quad_lat = flooding_option_ptr->high_quad_lat;
	copy_flooding_option_ptr->high_quad_long = flooding_option_ptr->high_quad_long;
	
	FRET (copy_flooding_option_ptr);
	}


static GrpT_Data_Option*
grp_pkt_support_data_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Data_Option*		data_option_ptr;
	GrpT_Data_Option*		copy_data_option_ptr;
	int						num_nodes, count;
	InetT_Address*			dest_addr_ptr;
	InetT_Address*			copy_dest_addr_ptr;
	
	/** Copies the data option in the GRP TLV	**/
	FIN (grp_pkt_support_data_option_mem_copy (<args>));
	
	data_option_ptr = (GrpT_Data_Option*) grp_tlv_ptr->grp_option_ptr;
	
	copy_data_option_ptr = grp_pkt_support_data_option_mem_alloc ();
	copy_data_option_ptr->dest_addr = inet_address_copy (data_option_ptr->dest_addr);
	copy_data_option_ptr->prev_dist_to_dest = data_option_ptr->prev_dist_to_dest;
	copy_data_option_ptr->dest_type = data_option_ptr->dest_type;
	copy_data_option_ptr->dest_lat = data_option_ptr->dest_lat;
	copy_data_option_ptr->dest_long = data_option_ptr->dest_long;
	copy_data_option_ptr->timestamp = data_option_ptr->timestamp;
	
	num_nodes = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
	
	for (count = 0; count < num_nodes; count++)
		{
		dest_addr_ptr = (InetT_Address*) op_prg_list_access (data_option_ptr->nodes_traversed_lptr, count);
		copy_dest_addr_ptr = inet_address_copy_dynamic (dest_addr_ptr);
		op_prg_list_insert (copy_data_option_ptr->nodes_traversed_lptr, copy_dest_addr_ptr, OPC_LISTPOS_TAIL);
		}
	
	FRET (copy_data_option_ptr);
	}


static GrpT_Dest_Pos_Req_Option*
grp_pkt_support_pos_req_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Dest_Pos_Req_Option*		pos_req_option_ptr;
	GrpT_Dest_Pos_Req_Option*		copy_pos_req_option_ptr;
	int								num_nodes, count;
	InetT_Address*					dest_addr_ptr;
	InetT_Address*					copy_dest_addr_ptr;
	
	/** Copies the position request option in the GRP TLV	**/
	FIN (grp_pkt_support_pos_req_option_mem_copy (<args>));
	
	pos_req_option_ptr = (GrpT_Dest_Pos_Req_Option*) grp_tlv_ptr->grp_option_ptr;
	
	copy_pos_req_option_ptr = grp_pkt_support_pos_req_option_mem_alloc ();
	copy_pos_req_option_ptr->nbr_addr = inet_address_copy (pos_req_option_ptr->nbr_addr);
	copy_pos_req_option_ptr->dest_req_lptr = op_prg_list_create ();
	
	num_nodes = op_prg_list_size (pos_req_option_ptr->dest_req_lptr);
	for (count = 0; count < num_nodes; count++)
		{
		dest_addr_ptr = (InetT_Address*) op_prg_list_access (pos_req_option_ptr->dest_req_lptr, count);
		copy_dest_addr_ptr = inet_address_copy_dynamic (dest_addr_ptr);
		op_prg_list_insert (copy_pos_req_option_ptr->dest_req_lptr, copy_dest_addr_ptr, OPC_LISTPOS_TAIL);
		}
	
	FRET (copy_pos_req_option_ptr);
	}


static GrpT_Dest_Pos_Resp_Option*
grp_pkt_support_pos_resp_option_mem_copy (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Dest_Pos_Resp_Option*			pos_resp_option_ptr;
	GrpT_Dest_Pos_Resp_Option*			copy_pos_resp_option_ptr;
	GrpT_Dest_Response*					dest_info_ptr;
	GrpT_Dest_Response*					copy_dest_info_ptr;
	int									num_nodes, count;
	
	/** Copies the position response option in the GRP TLV	**/
	FIN (grp_pkt_support_pos_resp_option_mem_copy (<args>));
	
	pos_resp_option_ptr = (GrpT_Dest_Pos_Resp_Option*) grp_tlv_ptr->grp_option_ptr;
	
	copy_pos_resp_option_ptr = grp_pkt_support_pos_resp_option_create (pos_resp_option_ptr->node_addr);
	
	num_nodes = op_prg_list_size (pos_resp_option_ptr->desp_resp_lptr);
	for (count = 0; count < num_nodes; count++)
		{
		dest_info_ptr = (GrpT_Dest_Response*) op_prg_list_access (pos_resp_option_ptr->desp_resp_lptr, count);
		copy_dest_info_ptr = grp_pkt_support_dest_resp_alloc ();
		copy_dest_info_ptr->dest_addr = inet_address_copy (dest_info_ptr->dest_addr);
		copy_dest_info_ptr->dest_lat = dest_info_ptr->dest_lat;
		copy_dest_info_ptr->dest_long = dest_info_ptr->dest_long;
		copy_dest_info_ptr->dest_type = dest_info_ptr->dest_type;
		copy_dest_info_ptr->quadrant_size = dest_info_ptr->quadrant_size;
		copy_dest_info_ptr->timestamp = dest_info_ptr->timestamp;
		
		op_prg_list_insert (copy_pos_resp_option_ptr->desp_resp_lptr, copy_dest_info_ptr, OPC_LISTPOS_TAIL);
		}
	
	FRET (copy_pos_resp_option_ptr);
	}
	

static void
grp_pkt_support_tlv_options_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	/** Frees the TLV options in the GRP header	**/
	FIN (grp_pkt_support_tlv_options_mem_free (<args>));
	
	switch (grp_tlv_ptr->option_type)
		{
		case (GRPC_HELLO_OPTION):
			{
			/* Hello option	*/
			grp_pkt_support_hello_option_mem_free (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_FLOODING_OPTION):
			{
			/* Flooding Option	*/
			grp_pkt_support_flooding_option_mem_free (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_DATA_OPTION):
			{
			/* Data Option	*/
			grp_pkt_support_data_option_mem_free (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_POS_REQ_OPTION):
			{
			grp_pkt_support_pos_req_option_mem_free (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_POS_RESP_OPTION):
			{
			grp_pkt_support_pos_resp_option_mem_free (grp_tlv_ptr);
			break;
			}
			
		case (GRPC_BACKTRACK_OPTION):
			{
			grp_pkt_support_data_option_mem_free (grp_tlv_ptr);
			break;
			}
		
		default:
			{
			/* Invalid option type	*/
			op_sim_end ("Invalid Option Type in GRP packet", OPC_NIL, OPC_NIL, OPC_NIL);
			break;
			}
		}
	
	/* Free the GRP TLV	*/
	grp_pkt_support_tlv_mem_free (grp_tlv_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_hello_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Hello_Option*		hello_option_ptr;
	
	/** Frees the Hello option in the TLV	**/
	FIN (grp_pkt_support_hello_option_mem_free (<args>));
	
	hello_option_ptr = (GrpT_Hello_Option*) grp_tlv_ptr->grp_option_ptr;
	inet_address_destroy (hello_option_ptr->nbr_addr);
	op_prg_mem_free (hello_option_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_flooding_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Flooding_Option*		flooding_option_ptr;
	
	/** Frees the Flooding option in the TLV	**/
	FIN (grp_pkt_support_flooding_option_mem_free (<args>));
	
	flooding_option_ptr = (GrpT_Flooding_Option*) grp_tlv_ptr->grp_option_ptr;
	inet_address_destroy (flooding_option_ptr->node_addr);
	op_prg_mem_free (flooding_option_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_data_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Data_Option*		data_option_ptr;
	int						num_nodes, count;
	InetT_Address*			node_addr_ptr;
	
	/** Frees the Data option in the TLV	**/
	FIN (grp_pkt_support_data_option_mem_free (<args>));
	
	data_option_ptr = (GrpT_Data_Option*) grp_tlv_ptr->grp_option_ptr;
	inet_address_destroy (data_option_ptr->dest_addr);
	
	num_nodes = op_prg_list_size (data_option_ptr->nodes_traversed_lptr);
	for (count = 0; count < num_nodes; count++)
		{
		node_addr_ptr = (InetT_Address*) op_prg_list_remove (data_option_ptr->nodes_traversed_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (node_addr_ptr);
		}
	
	op_prg_mem_free (data_option_ptr->nodes_traversed_lptr);
	op_prg_mem_free (data_option_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_pos_req_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Dest_Pos_Req_Option*		pos_req_option_ptr;
	int								num_nodes, count;
	InetT_Address*					node_addr_ptr;
	
	/** Frees the position request option in the TLV	**/
	FIN (grp_pkt_support_pos_req_option_mem_free (<args>));
	
	pos_req_option_ptr = (GrpT_Dest_Pos_Req_Option*) grp_tlv_ptr->grp_option_ptr;
	inet_address_destroy (pos_req_option_ptr->nbr_addr);
	
	num_nodes = op_prg_list_size (pos_req_option_ptr->dest_req_lptr);
	for (count = 0; count < num_nodes; count++)
		{
		node_addr_ptr = (InetT_Address*) op_prg_list_remove (pos_req_option_ptr->dest_req_lptr, OPC_LISTPOS_HEAD);
		inet_address_destroy_dynamic (node_addr_ptr);
		}
	
	op_prg_mem_free (pos_req_option_ptr->dest_req_lptr);
	op_prg_mem_free (pos_req_option_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_pos_resp_option_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	GrpT_Dest_Pos_Resp_Option*			pos_resp_option_ptr;
	int									num_nodes, count;
	GrpT_Dest_Response*					dest_info_ptr;
	
	/** Frees the position response option in the TLV	**/
	FIN (grp_pkt_support_pos_resp_option_mem_free (<args>));
	
	pos_resp_option_ptr = (GrpT_Dest_Pos_Resp_Option*) grp_tlv_ptr->grp_option_ptr;
	inet_address_destroy (pos_resp_option_ptr->node_addr);
	
	num_nodes = op_prg_list_size (pos_resp_option_ptr->desp_resp_lptr);
	for (count = 0; count < num_nodes; count++)
		{
		dest_info_ptr = (GrpT_Dest_Response*) op_prg_list_remove (pos_resp_option_ptr->desp_resp_lptr, OPC_LISTPOS_HEAD);
		
		inet_address_destroy (dest_info_ptr->dest_addr);
		op_prg_mem_free (dest_info_ptr);
		}
	
	op_prg_mem_free (pos_resp_option_ptr->desp_resp_lptr);
	op_prg_mem_free (pos_resp_option_ptr);
	
	FOUT;
	}


static void
grp_pkt_support_tlv_mem_free (GrpT_Packet_Option* grp_tlv_ptr)
	{
	/** Frees the TLV	**/
	FIN (grp_pkt_support_tlv_mem_free (<args>));
	
	op_prg_mem_free (grp_tlv_ptr);
	
	FOUT;
	}
