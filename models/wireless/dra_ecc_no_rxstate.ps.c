/* dra_ecc_no_rxstate.ps.c */
/* Alternative error correction model for radio link	   	*/
/* Transceiver Pipeline. In contrast to the default model,	*/
/* this model uses the built-in "signal lock" attribute		*/
/* intstead of the receiver channel state information to	*/
/* update the signal lock status of the channel. This model	*/
/* should be used with "*_no_rxstate" versions of the		*/
/* rxgroup and power pipeline stage models.					*/

/****************************************/
/*		 Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		 (A Delaware Corporation)		*/
/*	   7255 Woodmont Av., Suite 250  	*/
/*      Bethesda, MD 20814, U.S.A.      */
/*		   All Rights Reserved.			*/
/****************************************/

#include "opnet.h"


#if defined (__cplusplus)
extern "C"
#endif
void
dra_ecc_no_rxstate_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet* pkptr)
    {
	int				num_errs, accept;
	OpT_Packet_Size	pklen;
	Objid			rx_ch_obid; 
	double			ecc_thresh;

	/** Determine acceptability of given packet at receiver. **/
	FIN_MT (dra_ecc_no_rxstate (pkptr));

	/* Do not accept packets that were received	*/
	/* when the node was disabled.				*/
	if (op_td_is_set (pkptr, OPC_TDA_RA_ND_FAIL))
		accept = OPC_FALSE;
	else
		{
		/* Obtain the error correction threshold of the receiver. */
		ecc_thresh = op_td_get_dbl (pkptr, OPC_TDA_RA_ECC_THRESH);

		/* Obtain length of packet. */
		pklen = op_pk_total_size_get (pkptr);

		/* Obtain number of errors in packet. */
		num_errs = op_td_get_int (pkptr, OPC_TDA_RA_NUM_ERRORS);
	
		/* Test if bit errors exceed threshold. */
		if (pklen == 0)
			accept = OPC_TRUE;
		else
			accept = ((((double) num_errs) / pklen) <= ecc_thresh) ? OPC_TRUE : OPC_FALSE;
		}

	/* Place flag indicating accept/reject in transmission data block. */
	op_td_set_int (pkptr, OPC_TDA_RA_PK_ACCEPT, accept);

	/* In either case the receiver channel is no longer locked. */
	rx_ch_obid = op_td_get_int (pkptr, OPC_TDA_RA_RX_CH_OBJID);
	op_ima_obj_attr_set (rx_ch_obid, "signal lock", OPC_BOOLINT_DISABLED);

	FOUT
	}
