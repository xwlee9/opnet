/* dra_rxgroup_no_rxstate.ps.c */                                                       
/* Alternative receiver group model for radio link			*/
/* Transceiver Pipeline. In contrast the default rxgroup	*/
/* model, this model does not create and set a state		*/
/* information for receiver channels. Since the default		*/
/* power and ecc pipeline stage models rely on this state	*/
/* information, they should not be used with this model in	*/
/* the same node model. Instead, the "*_no_rxstate"			*/
/* versions of those models should be used.					*/

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
int
dra_rxgroup_no_rxstate_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA 
	Objid PRG_ARG_UNUSED(tx_obid), Objid PRG_ARG_UNUSED(rx_obid))
	{
	/** Determine the potential for communication between	**/
	/** given transmitter and receiver channel objects.		**/
	FIN_MT (dra_rxgroup_no_rxstate (tx_obid, rx_obid));

	/* By default, all receivers are considered	*/
	/* as potential destinations.				*/
	FRET (OPC_TRUE)
	}                
