/* dra_no_rxgroup.ps.c */                                                       
/* Special receiver group model for radio link Transceiver Pipeline */
/* When specified for a transmitter, will initialize each of its	*/
/* channels to have no receiver channels in its receiver group.		*/
/* Like any other receiver groups, such initially empty groups can	*/
/* later be modified using the op_radio kernel procedures. This		*/
/* model also populates the state information of the receiver		*/
/* channels to be used in power and ecc pipeline stage models. If	*/
/* you don't want the receiver channel state information to be set	*/
/* and used by radio pipeline stage models, then use "*_no_rxstate"	*/
/* version of rxgroup, power and ecc stage models in your node		*/
/* models, and change the rxgroup model in order to return			*/
/* OPC_FALSE instead of OPC_TRUE.									*/

/****************************************/
/*		 Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		 (A Delaware Corporation)		*/
/*	   7255 Woodmont Av., Suite 250  	*/
/*      Bethesda, MD 20814, U.S.A.      */
/*		   All Rights Reserved.			*/
/****************************************/

#include "opnet.h"
#include "dra.h"


#if defined (__cplusplus)
extern "C"
#endif
int
dra_no_rxgroup_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Objid PRG_ARG_UNUSED(tx_obid), Objid rx_obid)	
	{
	DraT_Rxch_State_Info*	rxch_state_ptr;

	/** Determine the potential for communication between	**/
	/** given transmitter and receiver channel objects.		**/
	/** Also create and initialize the receiver channel's	**/
	/** state information to be used by other pipeline		**/
	/** stages during the simulation. This pipeline stage	**/
	/** will not be invoked. It is only defined to provide	**/
	/** the appropriate name in the list of choices for the	**/
	/** 'rxgroup model' attribute of a radio transmitter.	**/ 
	FIN_MT (dra_no_rxgroup (tx_obid, rx_obid));

	/* Unless it is already done, initialize the receiver	*/
	/* channel's state information.							*/
	if (op_ima_obj_state_get (rx_obid) == OPC_NIL)
		{
		/* Channel state information doesn't exist. Lock	*/
		/* the global mutex before continuing.				*/
		op_prg_mt_global_lock ();
		
		/* Check again since another thread may have		*/
		/* already set up the state information.			*/
		if (op_ima_obj_state_get (rx_obid) == OPC_NIL)
			{
			/* Create and set the initial state information	*/
			/* for the receiver channel. State information	*/
			/* is used by other pipeline stages to			*/
			/* access/update channel specific data			*/
			/* efficiently.									*/
			rxch_state_ptr = (DraT_Rxch_State_Info *) op_prg_mem_alloc (sizeof (DraT_Rxch_State_Info));
			rxch_state_ptr->signal_lock = OPC_FALSE;
			op_ima_obj_state_set (rx_obid, rxch_state_ptr);
			}
		
		/* Unlock the global mutex.							*/
		op_prg_mt_global_unlock ();
		}
	
	/* Initially none of the receiver channels is a			*/
	/* potential receiver.									*/
	FRET (OPC_FALSE)
	}                
