/* dra_closure_all.ps.c */                                                       
/* "Null" closure model for radio link Transceiver Pipeline */

/****************************************/
/*		  Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		(A Delaware Corporation)		*/
/*	7255 Woodmont Av., Suite 250  		*/
/*     Bethesda, MD 20814, U.S.A.       */
/*			All Rights Reserved.		*/
/****************************************/

#include "opnet.h"


/***** pipeline procedure *****/

#if defined (__cplusplus)
extern "C"
#endif
void
dra_closure_all_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet* pkptr)
	{
	/** Guarantee closure between transmitter and receiver. **/
	FIN_MT (dra_closure_all (pkptr));

	/* Place closure status in packet transmission data block. */
	op_td_set_int (pkptr, OPC_TDA_RA_CLOSURE, OPC_TRUE);

	FOUT
	}                
