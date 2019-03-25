/* Wireless cell system model C form file: mobility_domain_adv.wdomain.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
static const char mobility_domain_adv_wdomain_c [] = "MIL_3_Tfile_Hdr_ 145A 85A op_mkso 7F 47C5041D 47C5041D 1 WTN09149 opnet 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcb 1                                                                                                                                                                                                                                                                                                                                                                                                          ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>


#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */


/* No Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif

/* Init Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
void *   /* data passed to other callbacks as wdomain_state_ptr argument */
mobility_domain_adv_init(OP_SIM_CONTEXT_ARG_OPT_COMMA Objid wdomain_id) /* ID of the instance */
	{
	FIN_MT (mobility_domain_adv_init(wdomain_id));
	FRET (NULL)
	}

/* Member Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
int
mobility_domain_adv_member(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr,
	Objid channel_id,          /* ID of the radio channel to check */
	Objid node_id,             /* ID of the node where the channel resides */
	SimT_Wcluster_Id * id_ptr) /* cluster id if membership succeeded */
	{
	FIN_MT (mobility_domain_adv_member(wdomain_state_ptr, channel_id, node_id, id_ptr));
	FRET (0)
	}

/* Info_Get Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
mobility_domain_adv_info_get(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr,
	unsigned int request_flags,       /* bit pattern indicating requested data values */
	SimT_Wcluster_Id xmit_cluster_id, /* cluster id of transmitter channel */
	SimT_Wcluster_Id rcvr_cluster_id, /* cluster id of receiver channel */
	SimT_Wcluster_Info * info_ptr,    /* to place requested data */
	void ** continuation_ptr)         /* will be passed to info_set if invoked, can be used for faster access to info */
	{
	FIN_MT (mobility_domain_adv_info_get(wdomain_state_ptr, request_flags, xmit_cluster_id,
		rcvr_cluster_id, info_ptr, continuation_ptr));
	FRET (OPC_COMPCODE_FAILURE)
	}

/* Info_Set Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
mobility_domain_adv_info_set(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr, 
	SimT_Wcluster_Id xmit_cluster_id, /* cluster id of transmitter channel */
	SimT_Wcluster_Id rcvr_cluster_id, /* cluster id of receiver channel */
	SimT_Wcluster_Info * info_ptr,    /* data to store */
	void ** continuation_ptr)         /* from previous info_get, can be used for faster access to info */
	{
	FIN_MT (mobility_domain_adv_info_set(wdomain_state_ptr, xmit_cluster_id
		rcvr_cluster_id, info_ptr, continuation_ptr));
	FRET (OPC_COMPCODE_FAILURE)
	}

/* Auxilliary Procedure Callback */
/* Please leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
mobility_domain_adv_auxproc(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr,
	OpT_Int64 code,  /* arbitrary code value */
	va_list args)    /* variable argument list, use va_arg to access elements */
	{
	FIN_MT (mobility_domain_adv_auxproc(wdomain_state_ptr, code, args));
	FRET (OPC_COMPCODE_SUCCESS);
	}
