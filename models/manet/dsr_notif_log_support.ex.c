/** dsr_notif_log_support.ex.c **/
/** for simulation notification logging.	**/

/****************************************/
/*		Copyright (c) 1987-2008		*/
/*      by OPNET Technologies, Inc.		*/
/*       (A Delaware Corporation)      	*/
/*    7255 Woodmont Av., Suite 250     	*/
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.          	*/
/****************************************/

/** Include directives.	**/
#include <opnet.h>
#include <dsr_ptypes.h>


/** Global data. **/
Log_Handle		dsr_conf_log_handle;

/* ---- Externally Callable Procedures ---- */
void
dsr_notification_log_init (void)
	{
	static Boolean	dsr_notif_log_initialized = OPC_FALSE;
	
	/**	Initializes handles used for simulation notification	**/
    /**	logging.  Creates a log handle for each distinct tuple	**/
    /**	(i.e., <Category, Class, Subclass>) -- these handles	**/
    /**	are later used by the DSR  models to log messages.		**/
	FIN (dsr_notification_log_init (void));
	
	if (dsr_notif_log_initialized == OPC_FALSE)
		{
		/* Set flag to prevent re-initialization	*/
		dsr_notif_log_initialized = OPC_TRUE;
		
		/* Configuration related notification handles.			*/
		dsr_conf_log_handle = op_prg_log_handle_create (OpC_Log_Category_Configuration, 
	    							"DSR", "Setup", 32);
		}
	
	FOUT;
	}
