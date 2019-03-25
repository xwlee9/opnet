/* dra_txdel.ps.c */                                                       
/* Default transmission delay model for radio link Transceiver Pipeline */

/****************************************/
/*		  Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		(A Delaware Corporation)		*/
/*	7255 Woodmont Av., Suite 250  		*/
/*     Bethesda, MD 20814, U.S.A.       */
/*			All Rights Reserved.		*/
/****************************************/

#include "opnet.h"


#if defined (__cplusplus)
extern "C"
#endif
void
dra_txdel_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
	{
	OpT_Packet_Size	pklen;
	double			tx_drate, tx_delay;

	/** Compute the transmission delay associated with the	**/
	/** transmission of a packet over a radio link.			**/
	FIN_MT (dra_txdel (pkptr));

	/* Obtain the transmission rate of that channel. */
	tx_drate = op_td_get_dbl (pkptr, OPC_TDA_RA_TX_DRATE);

	/* Obtain length of packet. */
	pklen = op_pk_total_size_get (pkptr);

	/* Compute time required to complete transmission of packet. */
	tx_delay = pklen / tx_drate;

	/* Place transmission delay result in packet's */
	/* reserved transmission data attribute. */
	op_td_set_dbl (pkptr, OPC_TDA_RA_TX_DELAY, tx_delay);

	FOUT
	}
