/* dra_ber.ps.c */                                                       
/* Default Bit-Error-Rate (BER) model for radio link Transceiver Pipeline */

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
dra_ber_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
	{
	double		ber, snr, proc_gain, eff_snr;
	Vartype		modulation_table;

	/** Calculate the average bit error rate affecting given packet. **/
	FIN_MT (dra_ber (pkptr));

	/* Determine current value of Signal-to-Noise-Ratio (SNR). */
	snr = op_td_get_dbl (pkptr, OPC_TDA_RA_SNR);

	/* Determine address of modulation table. */
	modulation_table = op_td_get_ptr (pkptr, OPC_TDA_RA_RX_MOD);

	/* Determine processing gain on channel. */
	proc_gain = op_td_get_dbl (pkptr, OPC_TDA_RA_PROC_GAIN);

	/* Calculate effective SNR incorporating processing gain. */
	eff_snr = snr + proc_gain;

	/* Derive expected BER from effective SNR. */
	ber = op_tbl_mod_ber (modulation_table, eff_snr);

	/* Place the BER in the packet's transmission data. */
	op_td_set_dbl (pkptr, OPC_TDA_RA_BER, ber);

	FOUT
	}
