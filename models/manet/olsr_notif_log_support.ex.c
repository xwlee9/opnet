/** olsr_notif_log_support.ex.c: Support functions for recording */
/** simulation log messages for OLSR error handling. 		     */

/****************************************/
/*      Copyright (c) 2005-2008         */
/*     by OPNET Technologies, Inc.      */
/*      (A Delaware Corporation)        */
/*   7255 Woodmont Av., Suite 250       */
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.           */
/****************************************/

#include <opnet.h>
#include <olsr_notif_log_support.h>

/* Log handles for model configuration messages */
static Log_Handle			olsr_config_log_handle; 

/***** Procedure Definitions *****/
void 
olsr_notif_log_init (void)
	{
	static Boolean			olsr_notif_log_init = OPC_FALSE; 
	
	/** This function initializes Log_Handle global variables and as **/
	/** a result sets up <Category, Class, Subclass> tuple instances.**/
	/** These handles will be used by OLSR to output messages to the **/
	/** simulation log.  											 **/
	FIN (olsr_notif_log_support_pkt_init (void)); 
	
	if (olsr_notif_log_init == OPC_FALSE)
		{
		/* Set flag to prevent re-initialization */
		olsr_notif_log_init = OPC_TRUE; 
		
		/* Configuration related notification handle */
		olsr_config_log_handle = op_prg_log_handle_create (OpC_Log_Category_Configuration, 
			"OLSR", "Setup", 32); 
		}
	
	FOUT; 
	}

void
olsrnl_main_address_duplicate_log_write (char* address_str)
	{
	/* Write a simulation warning that the main address */
	/* of the OLSR process is not usable, therefore the */
	/* OLSR process will be killed. 					*/
	FIN (olsrnl_main_address_duplicate_log_write (address_str)); 
		
	/* Make sure the log handles are initialized */	
	olsr_notif_log_init (); 
	
	op_prg_log_entry_write (olsr_config_log_handle, 
	/*	123456789_123456789_123456789_123456789_12	*/
		"WARNING:\n"
		"  The address %s chosen as main address \n"
		"  by the OLSR process in this node has  \n"
		"  a duplicate somewhere in the network. \n"
		"  As a result, no OLSR process will be  \n"
		"  running in this node. 				 \n"	
		"\n"	
		"POSSIBLE CAUSE(s):\n"
		"  1. Some addresses were manually set   \n"
		"     on the interfaces and the address  \n"
		"     %s was assigned twice. 			 \n"
		"\n"	
		"SUGGESTIONS:\n"
		"  1. Check and make sure there are no   \n"
		"     duplicate addresses. 				 \n"
		"\n"	
		"\n" , address_str, address_str);
	
	FOUT; 
	}

void
olsrnl_no_active_interfaces_log_write()
	{
	/* Write a simulation warning that this OLSR process */
	/* has no interfaces to use, so it will be killed.   */
	FIN (olsrnl_no_active_interfaces_log_write()); 
	 
	/* Make sure the log handles are initialized */	
	olsr_notif_log_init (); 
	
	op_prg_log_entry_write (olsr_config_log_handle, 
	/*	123456789_123456789_123456789_123456789_123	*/
		"WARNING:\n"
		"  The OLSR process running on this node  \n"
		"  has no interfaces it can use. An inter-\n"	
		"  face is unusable if it is shutdown, or \n"
		"  if OLSR addressing mode is IPv6, but   \n"
		"  but the Link-Local Address is set to   \n"
		"  Not Active. 							  \n"	
		"\n"	
		"POSSIBLE CAUSE(s):\n"
		"  1. Physical interfaces are shutdown.  \n"
		"  2. Interfaces assigned IPv6 addresses \n"
		"     have the Link-Local Address attri- \n"
		"     bute set to Not Active. 			 \n"	
		"\n"	
		"SUGGESTIONS:\n"
		"  1. Check that not all physical inter- \n"
		"     faces are shutdown.				 \n"
		"  2. If OLSR on this node is of IPv6    \n"
		"     address family, then in addition to\n"
		"     the physical interface not being   \n"
		"     shut down, check that the Link-    \n"
		"	  local Address on the interface is  \n"
		"	  is set to something other than Not \n"
		"	  Active. 							 \n"
		"\n"	
		"\n");
	
	FOUT; 
	}
