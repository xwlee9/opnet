/* dra_snr.ps.c */                                                       
/* Default Signal-to-Noise-Ratio (SNR) model for radio link Transceiver Pipeline */

/****************************************/
/*		  Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		(A Delaware Corporation)		*/
/*	7255 Woodmont Av., Suite 250  		*/
/*     Bethesda, MD 20814, U.S.A.       */
/*			All Rights Reserved.		*/
/****************************************/

#include "opnet.h"
#include <math.h>


#if defined (__cplusplus)
extern "C"
#endif
void
dra_snr_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
	{
	double		bkg_noise, accum_noise, rcvd_power;

	/** Compute the signal-to-noise ratio for the given packet. **/
	FIN_MT (dra_snr (pkptr));

	/* Get the packet's received power level. */
	rcvd_power = op_td_get_dbl (pkptr, OPC_TDA_RA_RCVD_POWER);

	/* Get the packet's accumulated noise levels calculated by the */
	/* interference and background noise stages. */
	accum_noise = op_td_get_dbl (pkptr, OPC_TDA_RA_NOISE_ACCUM);
	bkg_noise = op_td_get_dbl (pkptr, OPC_TDA_RA_BKGNOISE);

	/* Assign the SNR in dB. */
	op_td_set_dbl (pkptr, OPC_TDA_RA_SNR, 
		10.0 * log10 (rcvd_power / (accum_noise + bkg_noise)));

	/* Set field indicating the time at which SNR was calculated. */
	op_td_set_dbl (pkptr, OPC_TDA_RA_SNR_CALC_TIME, op_sim_time ());

	FOUT
	}
