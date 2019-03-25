/* aodv_pkt_support.ex.c */
/* C file for AODV Packet Support APIs */


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
#include <aodv.h>
#include <aodv_pkt_support.h>
#include <aodv_ptypes.h>
#include <ip_addr_v4.h>

/***** Prototypes *****/
static AodvT_Packet_Option*			aodv_pkt_support_option_mem_copy (AodvT_Packet_Option* option_ptr);
static void							aodv_pkt_support_option_mem_free (AodvT_Packet_Option* option_ptr);
static List*						aodv_pkt_support_unreachable_nodes_copy (List* unreachable_nodes_lptr);
static AodvT_Packet_Option*			aodv_pkt_support_option_mem_alloc (void);
static AodvT_Rreq*					aodv_pkt_support_rreq_option_mem_alloc (void);
static AodvT_Rrep*					aodv_pkt_support_rrep_option_mem_alloc (void);
static AodvT_Rerr*					aodv_pkt_support_rerr_option_mem_alloc (void);
static void							aodv_pkt_support_rreq_option_mem_free (AodvT_Rreq* rreq_option_ptr);
static void							aodv_pkt_support_rrep_option_mem_free (AodvT_Rrep* rrep_option_ptr);
static void							aodv_pkt_support_rerr_option_mem_free (AodvT_Rerr* rerr_option_ptr);


Packet*
aodv_pkt_support_pkt_create (AodvT_Packet_Option* pkt_option_ptr, int option_size)
	{
	static int		fd_options_index = OPC_FIELD_INDEX_INVALID;
	Packet*			aodv_pkptr;
	
	/** Creates and sets the information	**/
	/** in the AODV packet					**/
	FIN (aodv_pkt_support_pkt_create (<args>));
	
	/* Create the AODV packet	*/
	aodv_pkptr = op_pk_create_fmt ("aodv");
	
	/* Get the field index for the options field	*/
	if (fd_options_index == OPC_FIELD_INDEX_INVALID)
		fd_options_index = op_pk_nfd_name_to_index (aodv_pkptr, "Options");
	
	/* Set the options in the packet	*/
	op_pk_fd_set (aodv_pkptr, fd_options_index, OPC_FIELD_TYPE_STRUCT, pkt_option_ptr, option_size,
		aodv_pkt_support_option_mem_copy, aodv_pkt_support_option_mem_free, sizeof (AodvT_Packet_Option));
	
	FRET (aodv_pkptr);
	}


AodvT_Packet_Option*
aodv_pkt_support_rreq_option_create (Boolean join, Boolean repair, Boolean grat_rrep, Boolean dest_only,
	Boolean unknown_seq_num, int hop_count, int rreq_id, InetT_Address dest_addr, int dest_seq_num,
	InetT_Address src_addr, int src_seq_num)
	{
    AodvT_Rreq*					rreq_option_ptr;
	AodvT_Packet_Option*		aodv_pkt_option_ptr;
	
	/** Creates the route request option	**/
	FIN (aodv_pkt_support_rreq_option_create (<args>));
	
	/* Allocate memory for the Route Request option	**/
	rreq_option_ptr = aodv_pkt_support_rreq_option_mem_alloc ();
	
	/* Set the variables of the option	*/
	rreq_option_ptr->join_flag = join;
	rreq_option_ptr->repair_flag = repair;
	rreq_option_ptr->grat_rrep_flag = grat_rrep;
	rreq_option_ptr->dest_only = dest_only;
	rreq_option_ptr->unknown_seq_num_flag = unknown_seq_num;
	rreq_option_ptr->hop_count = hop_count;
	rreq_option_ptr->rreq_id = rreq_id;
	rreq_option_ptr->dest_seq_num = dest_seq_num;
	rreq_option_ptr->src_seq_num = src_seq_num;
	rreq_option_ptr->dest_addr = inet_address_copy (dest_addr);
	rreq_option_ptr->src_addr = inet_address_copy (src_addr);
	
	/* Allocate memory to set into the AODV packet option	*/
	aodv_pkt_option_ptr = aodv_pkt_support_option_mem_alloc ();
	aodv_pkt_option_ptr->type = AODVC_ROUTE_REQUEST;
	aodv_pkt_option_ptr->value_ptr = (void*) rreq_option_ptr;
	
	FRET (aodv_pkt_option_ptr);
	}


AodvT_Packet_Option*
aodv_pkt_support_rrep_option_create (Boolean repair, Boolean ack_required, int hop_count, 
			InetT_Address dest_addr, int dest_seq_num, InetT_Address src_addr, double lifetime, int type)
	{
	AodvT_Rrep*					rrep_option_ptr;
	AodvT_Packet_Option*		aodv_pkt_option_ptr;
	
	/** Creates the route reply option	**/
	FIN (aodv_pkt_support_rrep_option_create (<args>));
	
	/* Allocate memory for the route reply option	**/
	rrep_option_ptr = aodv_pkt_support_rrep_option_mem_alloc ();
	
	/* Set the variables of the option	*/
	rrep_option_ptr->repair_flag = repair;
	rrep_option_ptr->ack_required_flag = ack_required;
	rrep_option_ptr->hop_count = hop_count;
	rrep_option_ptr->dest_seq_num = dest_seq_num;
	rrep_option_ptr->lifetime = lifetime;
	rrep_option_ptr->dest_addr = inet_address_copy (dest_addr);
	rrep_option_ptr->src_addr = inet_address_copy (src_addr);
	
	/* Allocate memory to set into the AODV packet option	*/
	aodv_pkt_option_ptr = aodv_pkt_support_option_mem_alloc ();
	aodv_pkt_option_ptr->type = type;
	aodv_pkt_option_ptr->value_ptr = (void*) rrep_option_ptr;
	
	FRET (aodv_pkt_option_ptr);
	}


AodvT_Packet_Option*
aodv_pkt_support_rerr_option_create (Boolean no_delete, int num_unreachable_dest, 
										List* unreachable_dest_lptr)
	{
	AodvT_Rerr*					rerr_option_ptr;
	AodvT_Packet_Option*		aodv_pkt_option_ptr;

	
	/** Creates the route error option	**/
	FIN (aodv_pkt_support_rerr_option_create (<args>));
	
	/* Allocate memory for the route error option	**/
	rerr_option_ptr = aodv_pkt_support_rerr_option_mem_alloc ();
	
	/* Set the variables of the option	*/
	rerr_option_ptr->no_delete_flag = no_delete;
	rerr_option_ptr->num_unreachable_dest = num_unreachable_dest;
	rerr_option_ptr->unreachable_dest_lptr = unreachable_dest_lptr;
	
	/* Allocate memory to set into the AODV packet option	*/
	aodv_pkt_option_ptr = aodv_pkt_support_option_mem_alloc ();
	aodv_pkt_option_ptr->type = AODVC_ROUTE_ERROR;
	aodv_pkt_option_ptr->value_ptr = (void*) rerr_option_ptr;
	
	FRET (aodv_pkt_option_ptr);
	}


AodvT_Packet_Option*
aodv_pkt_support_rrep_ack_option_create (void)
	{
	AodvT_Packet_Option*		aodv_pkt_option_ptr;
	
	/** Creates the route reply acknowledgement option	**/
	FIN (aodv_pkt_support_rrep_ack_option_create (void));
	
	/* Allocate memory to set into the AODV packet option	*/
	aodv_pkt_option_ptr = aodv_pkt_support_option_mem_alloc ();
	aodv_pkt_option_ptr->type = AODVC_RREP_ACK;
	
	FRET (aodv_pkt_option_ptr);
	}


AodvT_Unreachable_Node*
aodv_pkt_support_unreachable_nodes_mem_alloc (void)
	{
	static Pmohandle			unreachable_node_option_pmh;
	AodvT_Unreachable_Node*		unreachable_node_ptr = OPC_NIL;
	static Boolean				unreachable_node_option_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the unreachable nodes	**/
	/** in the route error option.							**/
	FIN (aodv_pkt_support_unreachable_nodes_mem_alloc (void));
	
	if (unreachable_node_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for the unreachable node */
		unreachable_node_option_pmh = op_prg_pmo_define ("AODV Unreachable Node", sizeof (AodvT_Unreachable_Node), 32);
		unreachable_node_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the unreachable node from the pooled memory	*/
	unreachable_node_ptr = (AodvT_Unreachable_Node*) op_prg_pmo_alloc (unreachable_node_option_pmh);
	
	FRET (unreachable_node_ptr);
	}


/*** Internally callable functions	***/

static AodvT_Packet_Option*
aodv_pkt_support_option_mem_copy (AodvT_Packet_Option* option_ptr)
	{
	AodvT_Packet_Option*		copy_option_ptr;
	AodvT_Rreq*					rreq_option_ptr;
	AodvT_Rrep*					rrep_option_ptr;
	AodvT_Rerr*					rerr_option_ptr;
	List*						copy_unreachable_lptr;
	
	/** Makes a copy of the options field	**/
	/** in the AODV packet					**/
	FIN (aodv_pkt_support_option_mem_copy (<args>));
	
	/* Based on the type of option, copy it	*/
	switch (option_ptr->type)
		{
		case (AODVC_ROUTE_REQUEST):
			{
			/* This is a route request option	*/
			rreq_option_ptr = (AodvT_Rreq*) option_ptr->value_ptr;
			copy_option_ptr = aodv_pkt_support_rreq_option_create (rreq_option_ptr->join_flag, 
				rreq_option_ptr->repair_flag, rreq_option_ptr->grat_rrep_flag, rreq_option_ptr->dest_only,
				rreq_option_ptr->unknown_seq_num_flag, rreq_option_ptr->hop_count, rreq_option_ptr->rreq_id,
				rreq_option_ptr->dest_addr, rreq_option_ptr->dest_seq_num, rreq_option_ptr->src_addr,
				rreq_option_ptr->src_seq_num);
			
			break;
			}
			
		case (AODVC_ROUTE_REPLY):
			{
			/* This is a route reply option	*/
			rrep_option_ptr = (AodvT_Rrep*) option_ptr->value_ptr;
			copy_option_ptr = aodv_pkt_support_rrep_option_create (rrep_option_ptr->repair_flag,
				rrep_option_ptr->ack_required_flag, rrep_option_ptr->hop_count, rrep_option_ptr->dest_addr, 
				rrep_option_ptr->dest_seq_num, rrep_option_ptr->src_addr, rrep_option_ptr->lifetime, AODVC_ROUTE_REPLY);
			
			break;
			}
			
		case (AODVC_ROUTE_ERROR):
			{
			/* This is a route error option	*/
			rerr_option_ptr = (AodvT_Rerr*) option_ptr->value_ptr;
			copy_unreachable_lptr = aodv_pkt_support_unreachable_nodes_copy (rerr_option_ptr->unreachable_dest_lptr);
			copy_option_ptr = aodv_pkt_support_rerr_option_create (rerr_option_ptr->no_delete_flag,
				rerr_option_ptr->num_unreachable_dest, copy_unreachable_lptr);
			
			break;
			}
			
		case (AODVC_RREP_ACK):
			{
			/* This is a route reply acknowledgement option	*/
			copy_option_ptr = aodv_pkt_support_rrep_ack_option_create ();
			
			break;
			}
			
		case (AODVC_HELLO):
			{
			/* This is a route reply option	*/
			rrep_option_ptr = (AodvT_Rrep*) option_ptr->value_ptr;
			copy_option_ptr = aodv_pkt_support_rrep_option_create (rrep_option_ptr->repair_flag,
				rrep_option_ptr->ack_required_flag, rrep_option_ptr->hop_count, rrep_option_ptr->dest_addr, 
				rrep_option_ptr->dest_seq_num, rrep_option_ptr->src_addr, rrep_option_ptr->lifetime, AODVC_HELLO);
			
			break;
			}
		}
	
	FRET (copy_option_ptr);
	}


static void
aodv_pkt_support_option_mem_free (AodvT_Packet_Option* option_ptr)
	{
	AodvT_Rreq*					rreq_option_ptr;
	AodvT_Rrep*					rrep_option_ptr;
	AodvT_Rerr*					rerr_option_ptr;
	
	/** Free the memory for the packet option	*/
	FIN (aodv_pkt_support_option_mem_free (<args>));
	
	/* Based on the type of option free it	*/
	switch (option_ptr->type)
		{
		case (AODVC_ROUTE_REQUEST):
			{
			/* This is a route request option	*/
			rreq_option_ptr = (AodvT_Rreq*) option_ptr->value_ptr;
			aodv_pkt_support_rreq_option_mem_free (rreq_option_ptr);
			
			break;
			}
			
		case (AODVC_ROUTE_REPLY):
		case (AODVC_HELLO):
			{
			/* This is a route reply option	*/
			rrep_option_ptr = (AodvT_Rrep*) option_ptr->value_ptr;
			aodv_pkt_support_rrep_option_mem_free (rrep_option_ptr);
			
			break;
			}
			
		case (AODVC_ROUTE_ERROR):
			{
			/* This is a route error option	*/
			rerr_option_ptr = (AodvT_Rerr*) option_ptr->value_ptr;
			aodv_pkt_support_rerr_option_mem_free (rerr_option_ptr);
			
			break;
			}
		}	
			
	/* Free the memory allocated for the TLV	*/
	op_prg_mem_free (option_ptr);
	
	FOUT;
	}


static List*
aodv_pkt_support_unreachable_nodes_copy (List* unreachable_nodes_lptr)
	{
	List*						copy_lptr;
	int							num_nodes, count;
	AodvT_Unreachable_Node*		unreachable_node_ptr;
	AodvT_Unreachable_Node*		copy_unreachable_ptr;
	
	/** Makes a copy of the unreachable nodes list	**/
	FIN (aodv_pkt_support_unreachable_nodes_copy (<args>));
	
	/* Create a list for the copy	*/
	copy_lptr = op_prg_list_create ();	
	
	/* Get the number of unreachable nodes	*/
	num_nodes = op_prg_list_size (unreachable_nodes_lptr);
	
	for (count = 0; count < num_nodes; count++)
		{
		/* Get each unreachable node	*/
		unreachable_node_ptr = (AodvT_Unreachable_Node*) op_prg_list_access (unreachable_nodes_lptr, count);
		
		/* Make a copy of the node	*/
		copy_unreachable_ptr = aodv_pkt_support_unreachable_nodes_mem_alloc ();
		copy_unreachable_ptr->unreachable_dest = inet_address_copy (unreachable_node_ptr->unreachable_dest);
		copy_unreachable_ptr->unreachable_dest_seq_num = unreachable_node_ptr->unreachable_dest_seq_num;
		
		/* Insert into the list of unreachable nodes	*/
		op_prg_list_insert (copy_lptr, copy_unreachable_ptr, OPC_LISTPOS_TAIL);
		}
	
	FRET (copy_lptr);
	}


static AodvT_Packet_Option*
aodv_pkt_support_option_mem_alloc (void)
	{
	static Pmohandle		aodv_option_pmh;
	AodvT_Packet_Option*	aodv_option_ptr = OPC_NIL;
	static Boolean			aodv_option_pmh_defined = OPC_FALSE;
	
	/** Allocates memory for the TLV in the option	**/
	FIN (aodv_pkt_support_option_mem_alloc (void));
	
	if (aodv_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route request option	*/
		aodv_option_pmh = op_prg_pmo_define ("AODV Option TLV", sizeof (AodvT_Packet_Option), 32);
		aodv_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route request option from the pooled memory	*/
	aodv_option_ptr = (AodvT_Packet_Option*) op_prg_pmo_alloc (aodv_option_pmh);
	
	FRET (aodv_option_ptr);
	}
	

static AodvT_Rreq*
aodv_pkt_support_rreq_option_mem_alloc (void)
	{
	static Pmohandle		rreq_option_pmh;
	AodvT_Rreq*				rreq_ptr = OPC_NIL;
	static Boolean			rreq_option_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route request option	**/
	FIN (aodv_pkt_support_rreq_option_mem_alloc (void));
	
	if (rreq_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route request option	*/
		rreq_option_pmh = op_prg_pmo_define ("AODV Route Request Option", sizeof (AodvT_Rreq), 32);
		rreq_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route request option from the pooled memory	*/
	rreq_ptr = (AodvT_Rreq*) op_prg_pmo_alloc (rreq_option_pmh);
	
	FRET (rreq_ptr);
	}


static AodvT_Rrep*
aodv_pkt_support_rrep_option_mem_alloc (void)
	{
	static Pmohandle		rrep_option_pmh;
	AodvT_Rrep*				rrep_ptr = OPC_NIL;
	static Boolean			rrep_option_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route reply option	**/
	FIN (aodv_pkt_support_rrep_option_mem_alloc (void));
	
	if (rrep_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route reply option	*/
		rrep_option_pmh = op_prg_pmo_define ("AODV Route Reply Option", sizeof (AodvT_Rrep), 32);
		rrep_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route reply option from the pooled memory	*/
	rrep_ptr = (AodvT_Rrep*) op_prg_pmo_alloc (rrep_option_pmh);
	
	FRET (rrep_ptr);
	}


static AodvT_Rerr*
aodv_pkt_support_rerr_option_mem_alloc (void)
	{
	static Pmohandle		rerr_option_pmh;
	AodvT_Rerr*				rerr_ptr = OPC_NIL;
	static Boolean			rerr_option_pmh_defined = OPC_FALSE;
	
	/** Allocates pooled memory for the route error option	**/
	FIN (aodv_pkt_support_rerr_option_mem_alloc (void));
	
	if (rerr_option_pmh_defined == OPC_FALSE)
		{
		/* Define the pool memory handle for route erroroption	*/
		rerr_option_pmh = op_prg_pmo_define ("AODV Route Error Option", sizeof (AodvT_Rerr), 32);
		rerr_option_pmh_defined = OPC_TRUE;
		}
	
	/* Allocate the route error option from the pooled memory	*/
	rerr_ptr = (AodvT_Rerr*) op_prg_pmo_alloc (rerr_option_pmh);
	
	FRET (rerr_ptr);
	}


static void
aodv_pkt_support_rreq_option_mem_free (AodvT_Rreq* rreq_option_ptr)
	{
	/** Frees the memory allocated to the route request option	**/
	FIN (aodv_pkt_support_rreq_option_mem_free (<args>));
	
	/* Destroy the IP addresses	*/
	inet_address_destroy (rreq_option_ptr->dest_addr);
	inet_address_destroy (rreq_option_ptr->src_addr);
	
	/* Free the option	*/
	op_prg_mem_free (rreq_option_ptr);
	
	FOUT;
	}


static void
aodv_pkt_support_rrep_option_mem_free (AodvT_Rrep* rrep_option_ptr)
	{
	FIN (aodv_pkt_support_rrep_option_mem_free (<args>));
	
	/* Destroy the IP addresses	*/
	inet_address_destroy (rrep_option_ptr->dest_addr);
	inet_address_destroy (rrep_option_ptr->src_addr);
	
	/* Free the option	*/
	op_prg_mem_free (rrep_option_ptr);
	
	FOUT;
	}


static void
aodv_pkt_support_rerr_option_mem_free (AodvT_Rerr* rerr_option_ptr)
	{
	AodvT_Unreachable_Node*		unreachable_dest_ptr;
	int							num_dest;
	
	/** Frees the memory allocated for the route error option	**/
	FIN (aodv_pkt_support_rerr_option_mem_free (<args>));
	
	/* Get the number of unreachable destinations	*/
	num_dest = op_prg_list_size (rerr_option_ptr->unreachable_dest_lptr);
	
	while (num_dest > 0)
		{
		/* Access each unreachable destination	*/
		unreachable_dest_ptr = (AodvT_Unreachable_Node*) op_prg_list_remove (rerr_option_ptr->unreachable_dest_lptr, OPC_LISTPOS_HEAD);
		
		/* Free the memory	*/
		inet_address_destroy (unreachable_dest_ptr->unreachable_dest);
		op_prg_mem_free (unreachable_dest_ptr);
		
		num_dest--;
		}
	
	/* Free the list	*/
	op_prg_mem_free (rerr_option_ptr->unreachable_dest_lptr);
	
	/* Free the route error option	*/
	op_prg_mem_free (rerr_option_ptr);
	
	FOUT;
	}
