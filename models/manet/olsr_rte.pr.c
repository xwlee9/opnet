/* Process model C form file: olsr_rte.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char olsr_rte_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C91EEE7 5C91EEE7 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <vos.h>
#include <opnet.h>
#include <ip_rte_v4.h>
#include <ip_addr_v4.h>
#include <ip_rte_support.h>
#include <udp_api.h>
#include <oms_pr.h>
#include <oms_auto_addr_support.h>
#include <ip_addr_v4.h>
#include <ip_rte_support.h>
#include <ip_rte_v4.h>
#include <ip_dgram_sup.h>
#include <oms_topo_table.h>
#include <ip_cmn_rte_table.h>
#include <ip_mcast_support.h>
#include <olsr.h>
#include <olsr_pkt_support.h>
#include <olsr_support.h>
#include <olsr_notif_log_support.h>

#include <prg_graph.h>
#include <prg_djk.h>

/* Constants identifying the Timer Expiry */
/* Constants to identify the timers	*/
#define OLSRC_HELLO_TIMER_EXPIRY			1
#define	OLSRC_TC_TIMER_EXPIRY				2
#define OLSRC_LINK_SET_ENTRY_EXPIRY			3
#define	OLSRC_NBR_SET_ENTRY_EXPIRY			4
#define OLSRC_TOPO_SET_ENTRY_EXPIRY			5
#define OLSRC_CONN_LOSS_TIMER				6
#define	OLSRC_DUP_ENTRY_EXPIRY				7
#define OLSRC_SYM_LINK_EXPIRY				8 
#define OLSRC_RTE_CALC_TIMER_EXPIRY			9
#define OLSRC_TWO_HOP_EXPIRY				10
#define OLSRC_HELLO_PROCESSING_TIMER_EXPIRY	11

#define CONNECTION_CLASS_1      1
#define OLSRC_UDP_PORT			698
	   
#define HELLO_TIMER_EXPIRY				\
		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == OLSRC_HELLO_TIMER_EXPIRY))
#define TC_TIMER_EXPIRY					\
		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == OLSRC_TC_TIMER_EXPIRY))
#define RTE_CALC_TIMER_EXPIRY			\
		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == OLSRC_RTE_CALC_TIMER_EXPIRY))
#define HELLO_PROCESSING_TIMER_EXPIRY	\
		((invoke_mode == OPC_PROINV_DIRECT) && (intrpt_type == OPC_INTRPT_SELF) && (intrpt_code == OLSRC_HELLO_PROCESSING_TIMER_EXPIRY))

#define OLSRC_INFINITE_VALUE	OPC_DBL_INFINITY
#define LTRACE_ACTIVE			op_prg_odb_ltrace_active ("olsr_rte")
#define DIRECTLY_CONNECTED		-2
#define IDLE_OLSR				-10

/* Private data types */
	   
/* Neighbor Set Entry, Table indexed by nbr_main_addr */
typedef struct {
	int					nbr_addr;  		    /* Neighbor Address				*/
	int					status;				/* Neighbor Status				*/
	int					willingness;		/* Neighbor Willingness			*/
	int					link_entry_count;	/* No of links for this nbr 	*/
	List				two_hop_nbr_list;   /* Two hop neighbor list		*/
	int					reachability;		/* Used for MPR set calculation	*/
	int					degree;				/* Used for MPR set calculation */
	int					sym_link_entry_count; /* Symmetric link count		*/
    Boolean mpr_selector; /* Indicates if this nbr is an MPR selector of the node maintaining this data structure */
	} OlsrT_Neighbor_Set_Entry;

/* 2 Hop Neighbor Set Entry,  Table indexed by two_hop_addr */
typedef struct
{
	int					two_hop_addr;   	/* Two hop nbr address			*/
	List				neighbor_list;     	/* Nbr list through which two	*/
                                            /* hop can be reached 			*/
											/* OlsrT_Nbr_Addr_Two_Hop_Entry	*/
	PrgT_Bin_Hash_Table	*neighbor_hash_ptr;	/* same info as in neighbor_list hashed by neighbor address for speed */
    Boolean 			strict_two_hop;     /* Indicates if this node is a strict two hop neighbor */

	} OlsrT_Two_Hop_Neighbor_Set_Entry;

/* This structure is added as List  element 	*/
/* in the neighbor_lptr vector of Two Hop Nbr	*/
/* table. It contains nbr addr and expiry time	*/
typedef struct
	{
	Evhandle					two_hop_expiry_evhandle;
	OlsrT_Neighbor_Set_Entry	* nbr_ptr;
	int							two_hop_addr; // parent entry address (used by expiry proc)
	} OlsrT_Nbr_Addr_Two_Hop_Entry;

/* MPR set Entry, Table indexed by MPR address */
typedef struct {
	int					mpr_addr;
	} OlsrT_MPR_Set_Entry;

/* MPR Selector Set Entry, Table indexed by selector address */
typedef struct {
	OlsrT_Neighbor_Set_Entry*  			 mpr_selector_ptr;	/* MPR selector node pointer	 	*/
	double 				expiry_time;        /* entry expiry time 			*/
	} OlsrT_MPR_Selector_Set_Entry;

/* Topology Set, */
/* Each node maintains a local TC table indexed by 	*/
/* last_addr (originator addr of TC msg). 			*/
/* For each entry in local TC table, an entry in 	*/
/* in Common TC table (list of dest_addr reachable	*/
/* through this last_addr & seq_num) is maintained	*/
/* Common table entry is shared between all nodes	*/

/* Entry for Common TC table */
typedef struct {
	List 				dest_addr_list;     /* List of dest addr reachable 	*/
											/* through a given last_addr	*/
	int					tc_seq_num;         /* Seq number for this entry	*/
	} OlsrT_TC_Dest_Addr_Entry;

/* Entry for local TC table , indexed by last address */
typedef struct {
	int							last_addr; 	 /* originator addr of TC msg 		*/ 
	int							tc_seq_num;  /* tc seq number					*/
	double 						tc_time;     /* expiry time for the entry		*/
	OlsrT_TC_Dest_Addr_Entry*	state_ptr; 	 /* handle to Common TC table entry	*/
	} OlsrT_TC_Set_Entry;

/* Entry for temporary hash table created for MPR calculation */
typedef struct {
	double	time;
	OlsrT_Two_Hop_Neighbor_Set_Entry* two_hop_ptr;
	} OlsrT_Strict_Two_Hop_Entry;

/* Routing Table entry, table indexed by destination addr */
typedef struct {
	int					dest_addr;			/* Destination address 				*/
	int					next_addr;			/* Next hop to reach dest addr		*/
	int					hops;				/* Num of hops to reach dest addr 	*/
	int					local_iface_addr;	/* local iface to reach next hop	*/
	IpT_Dest_Prefix		dest_prefix; 		/* We need to keep a hold of the    */
	} OlsrT_Routing_Table_Entry;			/* dest prefix OLSR creates and     */
											/* gives to IP in order to prevent  */
											/* prevent a memory leak.           */
typedef struct {
	int			addr;
	int			willingness;
        int local_intf;
	Boolean		mpr_selector;
	} OlsrT_Djk_State;

typedef struct {
	int			local_intf;
	int			neighbor_intf;
	} OlsrT_Djk_Edge_State;

typedef struct OlsrT_Rte_Deferred_Hello_Info OlsrT_Rte_Deferred_Hello_Info;

struct OlsrT_Rte_Deferred_Hello_Info
	{
	OlsrT_Rte_Deferred_Hello_Info	* next_ptr;	/* next hello in deferred chain */
	Packet 							* pkptr;
	const OlsrT_Message 			* olsr_message_ptr;
	InetT_Address					* inet_local_intf_addr_ptr;
	InetT_Address 					* inet_ip_src_addr_ptr;
	};

/* Structure of a binary hash key for the duplicate set table */
typedef struct OlsrT_Dup_Set_Key
	{
	int		originator_addr;
	int		message_seq_num;
	
	} OlsrT_Dup_Set_Key;

/* Duplicate set entry */
typedef struct
	{
	double				dup_expiry_time;    	/* entry expiry time			*/
	Evhandle			entry_expiry_evhandle;	/* handle for expiry intrpt		*/
	OlsrT_Dup_Set_Key	key;					/* originator & seq_num 		*/
	Boolean				rexmitted;				/* flag to indicate if msg has 	*/
												/* been already retransmitted	*/
	} OlsrT_Duplicate_Set_Entry;

/* Structure of a binary hash key for the link set table */
typedef struct OlsrT_Link_Set_Key
	{
	int		nbr_intf_addr;
	int		local_intf_addr;
	
	} OlsrT_Link_Set_Key;

/* Link Set Entry */

typedef struct OlsrT_Link_Set_Entry OlsrT_Link_Set_Entry;
struct OlsrT_Link_Set_Entry
	{
	double				SYM_time; 			/* Expiry time for SYM Links 	*/
	double				ASYM_time;			/* Expiry for ASYM Links		*/
	double				expiry_time;		/* Expiry for this entry		*/
	Evhandle			sym_time_expiry_evhandle; /* handle for expiry intrpt */
	void*				nbr_entry_ptr;	    /* Pointer for nbr entry */
	OlsrT_Link_Set_Entry*next_link_set_entry_ptr;	/* next in chain */
	
	OlsrT_Link_Set_Key	key;
	};

/* Private shared variables (read-only after time MT safe initialization) */
OlsrT_Global_Stathandles			global_stat_handles;

static Pmohandle					olsr_djk_state_pmoh;
static Pmohandle					olsr_djk_edge_state_pmoh;
static Pmohandle					dup_entry_pmh;
static Pmohandle					rt_entry_pmh;
static Pmohandle					mpr_set_entry_pmh;
static Pmohandle					link_set_entry_pmh;
static Pmohandle					mpr_selector_set_entry_pmh;
static Pmohandle					nbr_set_entry_pmh;
static Pmohandle					nbr_addr_set_entry_pmh;
static Pmohandle					two_nbr_set_entry_pmh;
static Pmohandle					tc_set_pmh;
static Pmohandle					dest_entry_pmh;
static Pmohandle					mid_msg_pmh;
static Pmohandle					hello_msg_pmh;
static Pmohandle					hello_data_pmh;
static Pmohandle					tc_msg_pmh;
static Pmohandle					olsr_msg_pmh;
static Pmohandle					new_addr_pmh;
static Pmohandle					deferred_hello_info_pmh;

/* Prototypes */
static void							olsr_rte_local_stats_reg (void);
static void							olsr_rte_attributes_parse (Objid);
static void							olsr_rte_pkt_arrival_handle (void);
static void							olsr_rte_process_hello (const OlsrT_Message*, int, int);

static void
olsr_rte_process_hello_pk (Packet * pkptr, const OlsrT_Message * olsr_message_ptr,
	InetT_Address * inet_local_intf_addr_ptr, InetT_Address * inet_ip_src_addr_ptr);

static void
olsr_hello_processing_expiry_handle (void);

static void							olsr_rte_error (const char*, char*, char*);
static void							olsr_rte_hello_expiry_handle (void);
static void							olsr_rte_tc_expiry_handle (void);
static void							olsr_rte_send_hello (Boolean);
static void							olsr_rte_send_TC (Boolean);
static void							olsr_rte_table_calc_expiry_handle (void);

static int							olsr_rte_find_myself_in_hello_msg (PrgT_Vector*, int);
static void							olsr_rte_pkt_send (Packet*, InetT_Address, Boolean);
static Boolean						olsr_rte_forward_packet (OlsrT_Message*, int);
static Boolean						olsr_rte_process_TC (const OlsrT_Message*, int);
static int*							olsr_rte_address_create (int);

/* Table access function prototypes */
static void							olsr_rte_neighborhood_topology_check (void);
static void							olsr_rte_routing_table_entry_add (int, int, int, int, IpT_Dest_Prefix);

static OlsrT_Hello_Message_Data*	olsr_rte_hello_msg_data_linkcode_exists (PrgT_Vector*, int);

static OlsrT_Link_Set_Entry*		olsr_rte_link_set_entry_get (int, int);
static OlsrT_Link_Set_Entry*		olsr_rte_link_set_entry_create (int, int, double, double, double);
static void							olsr_rte_expired_link_set_entries_remove (void);
static int							olsr_rte_link_set_sym_time_check (int);

static OlsrT_MPR_Set_Entry* 		olsr_rte_mpr_set_entry_get (int);
static OlsrT_MPR_Set_Entry*			olsr_rte_mpr_set_entry_create (int);
static void							olsr_rte_calculate_mpr_set (void);
static int 		olsr_rte_strict_two_hop_set_create (void);
static void							olsr_rte_mpr_with_nbr_count_one_add (int*);
static void							olsr_rte_update_strict_two_hop_set (int*, OlsrT_Neighbor_Set_Entry*);
static void							olsr_rte_calculate_reachability (PrgT_List*);
static void 						olsr_rte_calculate_degree (void);

static void					olsr_rte_table_incremental_entry_add (PrgT_Bin_Hash_Table*, int, int, int, int);
static void					olsr_rte_routing_table_entry_update (OlsrT_Routing_Table_Entry*, int, int, int);

static void								olsr_rte_mpr_selector_set_entry_create (int, int);
static void								olsr_rte_expired_mpr_selector_set_entries_remove (void);

static OlsrT_Neighbor_Set_Entry*	olsr_rte_set_neighbor_status (int, OlsrT_Link_Set_Entry*);
static OlsrT_Neighbor_Set_Entry*	olsr_rte_neighbor_set_entry_get (int);
static OlsrT_Neighbor_Set_Entry*	olsr_rte_neighbor_set_entry_create (int, int, int);
static void							olsr_rte_update_neighbor_set_table (int, OlsrT_Neighbor_Set_Entry*);

static void									olsr_rte_two_hop_nbr_set_entry_match_n_delete( int, int);
static void									olsr_rte_two_hop_nbr_set_entry_create (int, int, int);
static void									olsr_rte_remove_nbr_from_two_hop_entry (int, OlsrT_Two_Hop_Neighbor_Set_Entry*);

static OlsrT_TC_Dest_Addr_Entry*	olsr_rte_tc_dest_addr_entry_create (PrgT_Vector*, int);
static void							olsr_rte_expired_tc_entries_remove (void);

static void							olsr_rte_duplicate_set_entry_add (int, int, Boolean);

static void							olsr_rte_breakup_linkcode (int, int*, int*);
static void							olsr_rte_build_new_mpr_set (void);

static Packet*					olsr_pkt_support_pkt_create (OlsrT_Message*, int);
static OlsrT_Message*			olsr_pkt_support_olsr_message_create (void*, int, int,
																	int, int, double, int, int, Boolean);
static OlsrT_TC_Message*		olsr_pkt_support_tc_message_create (void);

/*** SUPPORT FUNCTIONS ****/

static void						olsr_rte_link_set_print_to_string (void);
static void						olsr_rte_neighbor_set_print_to_string (void);
static void						olsr_rte_two_hop_neighbor_set_print_to_string (void);
static void						olsr_rte_mpr_selector_set_print_to_string (void);
static void						olsr_rte_tc_set_print_to_string (void);
static void						olsr_rte_routing_table_print_to_string (void);
static void						olsr_rte_interface_table_print_to_string (void);
static void						olsr_rte_clear_table (void*, int);
static void						olsr_rte_fail_rec_handle (int);
static void						olsr_rte_update_global_mpr_count (Boolean);

static void				olsr_rte_djk_rte_table_calculate (void);
static PrgT_Graph_Edge*	olsr_rte_djk_next_hop_get (PrgT_List*);

static void		olsr_rte_graph_nbr_add (OlsrT_Neighbor_Set_Entry*, OlsrT_Link_Set_Entry*);
static void		olsr_rte_graph_nbr_delete (OlsrT_Neighbor_Set_Entry*);
static void		olsr_rte_graph_two_hop_nbr_add (int, OlsrT_Two_Hop_Neighbor_Set_Entry*);
static void		olsr_rte_graph_two_hop_nbr_delete (int, int);
static void		olsr_rte_graph_tc_add (OlsrT_TC_Set_Entry*);
static void		olsr_rte_graph_tc_delete (OlsrT_TC_Set_Entry*);

static void						olsr_rte_symmetric_link_expiry_handle (void*, int);
static void							olsr_rte_two_hop_expiry_handle (void*, int);
static void						olsr_rte_duplicate_entry_delete (void*, int);
static void 					olsr_rte_tc_dest_addr_entry_destroy (void*);
static void						olsr_rte_olsr_message_print (void*, Prg_List*);
static void						olsr_rte_olsr_message_destroy (void*);
static void*					olsr_rte_olsr_message_copy (void*, size_t);


/* End of Header Block */

#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */



/* State variable definitions */
typedef struct
	{
	/* Internal state tracking for FSM */
	FSM_SYS_STATE
	/* State Variables */
	Prohandle	              		own_prohandle                                   ;	/* Process's own handle */
	int	                    		output_strm                                     ;	/* Outgoing stream */
	IpT_Rte_Module_Data*	   		module_data_ptr                                 ;	/* Module Data Pointer, to access ICI and to get the      */
	                        		                                                	/* list of my own interfaces for registering to MID table */
	Ici*	                   		command_ici_ptr                                 ;	/* Handle to Ici to get source address and interface address */
	PrgT_Bin_Hash_Table*	   		neighbor_set_table                              ;	/* Neighbor Table                                */
	                        		                                                	/* String Hash Table, with each entry as defined */
	                        		                                                	/* in OlsrT_Neighbor_Set_Entry                   */
	PrgT_Bin_Hash_Table*	   		link_set_table                                  ;	/* Link Table                                    */
	                        		                                                	/* String hash table, with each entry as defined */
	                        		                                                	/* in OlsrT_Link_Set_Entry                       */
	OlsrT_Link_Set_Entry*	  		link_set_chain_head_ptr                         ;	/* (sparse) link set table entries are also chained together for efficiency */
	int	                    		msg_seq_num                                     ;	/* Sequence number used in header of Olsr Message (OlsrT_Message) */
	                        		                                                	/* This is icreased for each new message created                  */
	int	                    		pkt_seq_num                                     ;	/* Seq Num used in the header of OLSR pkt                      */
	                        		                                                	/* This is increamented for each OLSR control packet generated */
	                        		                                                	/*                                                             */
	PrgT_Bin_Hash_Table*	   		mpr_set_table                                   ;	/* MPR Table                                     */
	                        		                                                	/* String Hash Table, with each entry as defined */
	                        		                                                	/* by OlsrT_MPR_Set_Entry                        */
	Boolean	                		neighborhood_changed                            ;	/* Becomes true whenever a change in neighborhood is detected.  */
	                        		                                                	/* Change in neighborhood is considered when a NBR or 2-HOP NBR */
	                        		                                                	/* is added or deleted. Used for computing MPR, Routing Tables. */
	PrgT_Bin_Hash_Table*	   		mpr_selector_set_table                          ;	/* MPR Selector Table */
	PrgT_Bin_Hash_Table*	   		two_hop_nbr_set_table                           ;	/* Two Hop Table */
	int	                    		own_main_address                                ;	/* Node's main address                                    */
	                        		                                                	/* If a node has multiple interfaces, the first loopback  */
	                        		                                                	/* address is taken as node's main address. This address  */
	                        		                                                	/* is used to represent the node in the network. (similar */
	                        		                                                	/* to router id concept).                                 */
	char	                   		pid_string[32]                                  ;	/* Print display for ltrace printouts */
	int	                    		ANSN                                            ;	/* TC seq number                                              */
	                        		                                                	/* Whenever a change in MPR selector set happens, this seq    */
	                        		                                                	/* num is increamented. It is used to determine the freshness */
	                        		                                                	/* of received TC message                                     */
	PrgT_Bin_Hash_Table*	   		tc_set_table                                    ;	/* Topology Table */
	PrgT_Bin_Hash_Table*	   		duplicate_set_table                             ;	/* Duplicate Table                                        */
	                        		                                                	/* This table holds the entries for the packets that have */
	                        		                                                	/* been processed or forwarded. The entries are indexed   */
	                        		                                                	/* by originatorAddr_mesgSeqNum. Used to avoid processing */
	                        		                                                	/* of duplicate packets                                   */
	double	                 		tc_entry_expiry_time                            ;	/* Minimum of the expiry time of all the entries in TC table */
	Boolean	                		selectorset_changed                             ;	/* Flag is set when MPR Selector Set is changed */
	                        		                                                	/* This is used while increamenting TC Seq Num  */
	OmsT_Topo_Table_Handle	 		topo_table_hndl                                 ;	/* Handle to the Common TC Table */
	PrgT_Bin_Hash_Table*	   		olsr_routing_table                              ;	/* Routing Table */
	Boolean	                		topology_changed                                ;	/* Set when either an entry created or deleted from TC set table */
	                        		                                                	/* If set, it triggers the routing table re-calculation          */
	IpT_Rte_Proc_Id	        		olsr_protocol_id                                ;	/* Olsr Protocol ID */
	InetT_Subnet_Mask	      		subnet_mask_32                                  ;	/* Length 32 subnet mask for routing entries */
	OlsrT_Local_Stathandles			local_stat_handles                              ;	/* Handle to Local Statistics */
	int	                    		node_willingness                                ;	/* Willingness of this node, taken from the OLSR attributes */
	                        		                                                	/* set for this node                                        */
	double	                 		neighbor_hold_time                              ;	/* Read from OLSR attributes set by user. Used to  */
	                        		                                                	/* determine the valid time for the information in */
	                        		                                                	/* a received hello message                        */
	double	                 		tc_hold_time                                    ;	/* Read from OLSR attributes set by user. Used to */
	                        		                                                	/* determine the valid time for the information   */
	                        		                                                	/* in a received TC message                       */
	double	                 		hello_interval                                  ;	/* Read from OLSR attributes set by user. This determines    */
	                        		                                                	/* the periodic time interval for hello message transmission */
	double	                 		tc_interval                                     ;	/* Read from OLSR attributes set by user. This determines */
	                        		                                                	/* the periodic time interval for TC message transmission */
	double	                 		dup_hold_time                                   ;	/* Read from OLSR attributes set by user. This determines */
	                        		                                                	/* the expiry time of entries in Duplicate Set Table      */
	Evhandle	               		hello_timer_evhandle                            ;	/* Stores the handle of the next scheduled interrupt for */
	                        		                                                	/* hello message transmission                            */
	double	                 		send_empty_tc_time                              ;	/* This stores the time till which a node should keep  */
	                        		                                                	/* on sending TC messages even if the mpr selector set */
	                        		                                                	/* becomes empty in order to nullify the previous TCs  */
	Boolean	                		is_ipv6_enabled                                 ;	/* If TRUE, this OLSR process carries only IPv6 addresses; otherwise, it carries IPv4 addresses */
	InetT_Subnet_Mask	      		subnet_mask_128                                 ;	/* Length 128 subnet mask for IPv6 routing entries */
	Evhandle	               		tc_timer_evhandle                               ;	/* Stores the handle of the next scheduled interrupt for TC message transmission */
	Boolean	                		rte_calc_already_scheduled                      ;	/* Flag to find if rte_table_calc is already scheduled or not */
	double	                 		rte_table_calc_timer                            ;
	double	                 		hello_processing_time_quantum                   ;	/* Support deferred HELLO processing on quantized sim domain time for better performance */
	OlsrT_Rte_Deferred_Hello_Info*			hello_processing_deferred_head_ptr              ;	/* Chain of deferred HELLO packets pending processing */
	OlsrT_Rte_Deferred_Hello_Info*			hello_processing_deferred_tail_ptr              ;
	PrgT_Bin_Hash_Table*	   		djk_hash_table_ptr                              ;
	PrgT_Graph*	            		prg_graph_ptr                                   ;
	PrgT_Graph_Vertex*	     		root_vertex_ptr                                 ;
	int	                    		my_process_id                                   ;
	double	                 		last_rte_calc_time                              ;	/* Time at which the latest routing table calculation was performed */
	double	                 		djk_processing_time_quantum                     ;	/* Similar to hello processing quantum: DJK calculations are deferred. */
	} olsr_rte_state;

#define own_prohandle           		op_sv_ptr->own_prohandle
#define output_strm             		op_sv_ptr->output_strm
#define module_data_ptr         		op_sv_ptr->module_data_ptr
#define command_ici_ptr         		op_sv_ptr->command_ici_ptr
#define neighbor_set_table      		op_sv_ptr->neighbor_set_table
#define link_set_table          		op_sv_ptr->link_set_table
#define link_set_chain_head_ptr 		op_sv_ptr->link_set_chain_head_ptr
#define msg_seq_num             		op_sv_ptr->msg_seq_num
#define pkt_seq_num             		op_sv_ptr->pkt_seq_num
#define mpr_set_table           		op_sv_ptr->mpr_set_table
#define neighborhood_changed    		op_sv_ptr->neighborhood_changed
#define mpr_selector_set_table  		op_sv_ptr->mpr_selector_set_table
#define two_hop_nbr_set_table   		op_sv_ptr->two_hop_nbr_set_table
#define own_main_address        		op_sv_ptr->own_main_address
#define pid_string              		op_sv_ptr->pid_string
#define ANSN                    		op_sv_ptr->ANSN
#define tc_set_table            		op_sv_ptr->tc_set_table
#define duplicate_set_table     		op_sv_ptr->duplicate_set_table
#define tc_entry_expiry_time    		op_sv_ptr->tc_entry_expiry_time
#define selectorset_changed     		op_sv_ptr->selectorset_changed
#define topo_table_hndl         		op_sv_ptr->topo_table_hndl
#define olsr_routing_table      		op_sv_ptr->olsr_routing_table
#define topology_changed        		op_sv_ptr->topology_changed
#define olsr_protocol_id        		op_sv_ptr->olsr_protocol_id
#define subnet_mask_32          		op_sv_ptr->subnet_mask_32
#define local_stat_handles      		op_sv_ptr->local_stat_handles
#define node_willingness        		op_sv_ptr->node_willingness
#define neighbor_hold_time      		op_sv_ptr->neighbor_hold_time
#define tc_hold_time            		op_sv_ptr->tc_hold_time
#define hello_interval          		op_sv_ptr->hello_interval
#define tc_interval             		op_sv_ptr->tc_interval
#define dup_hold_time           		op_sv_ptr->dup_hold_time
#define hello_timer_evhandle    		op_sv_ptr->hello_timer_evhandle
#define send_empty_tc_time      		op_sv_ptr->send_empty_tc_time
#define is_ipv6_enabled         		op_sv_ptr->is_ipv6_enabled
#define subnet_mask_128         		op_sv_ptr->subnet_mask_128
#define tc_timer_evhandle       		op_sv_ptr->tc_timer_evhandle
#define rte_calc_already_scheduled		op_sv_ptr->rte_calc_already_scheduled
#define rte_table_calc_timer    		op_sv_ptr->rte_table_calc_timer
#define hello_processing_time_quantum		op_sv_ptr->hello_processing_time_quantum
#define hello_processing_deferred_head_ptr		op_sv_ptr->hello_processing_deferred_head_ptr
#define hello_processing_deferred_tail_ptr		op_sv_ptr->hello_processing_deferred_tail_ptr
#define djk_hash_table_ptr      		op_sv_ptr->djk_hash_table_ptr
#define prg_graph_ptr           		op_sv_ptr->prg_graph_ptr
#define root_vertex_ptr         		op_sv_ptr->root_vertex_ptr
#define my_process_id           		op_sv_ptr->my_process_id
#define last_rte_calc_time      		op_sv_ptr->last_rte_calc_time
#define djk_processing_time_quantum		op_sv_ptr->djk_processing_time_quantum

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	olsr_rte_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((olsr_rte_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

/* Instrumenting functions for quantify profiling	*/
/* Does not affect models behavior.					*/
static void 
olsr_rte_neighborhood_topology_check_hello (void)
	{
	FIN (olsr_rte_neighborhood_topology_check_hello());
	
	olsr_rte_neighborhood_topology_check ();
	
	FOUT;
	}

static void 
olsr_rte_neighborhood_topology_check_TC (void)
	{
	FIN (olsr_rte_neighborhood_topology_check_TC());
	
	olsr_rte_neighborhood_topology_check ();
	
	FOUT;
	}
	
static void
olsr_rte_globals_init (void)
	{
	static int initialized = 0;

	/* Iniitalize once shared OLSR globals such as memory pools, global statistics, etc. */
	FIN (olsr_rte_globals_init);

	if (!initialized)
		{
		/* register global statistics */
		global_stat_handles.rte_traf_rcvd_bps_global_shandle = op_stat_reg ("OLSR.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.rte_traf_rcvd_pps_global_shandle = op_stat_reg ("OLSR.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.rte_traf_sent_bps_global_shandle = op_stat_reg ("OLSR.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.rte_traf_sent_pps_global_shandle = op_stat_reg ("OLSR.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.total_hello_sent_global_shandle = op_stat_reg ("OLSR.Total Hello Messages Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.hello_traffic_sent_bps_global_handle = op_stat_reg ("OLSR.Hello Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.total_tc_sent_global_shandle = op_stat_reg ("OLSR.Total TC Messages Sent", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.total_tc_forwarded_global_shandle = op_stat_reg ("OLSR.Total TC Messages Forwarded", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.tc_traffic_sent_bps_global_handle = op_stat_reg ("OLSR.TC Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.mpr_count_global_shandle = op_stat_reg ("OLSR.MPR Count", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);	
		
		/* OLSR Performance Statistics */
		global_stat_handles.rte_table_calcs_global_shandle = op_stat_reg ("OLSR Performance.Route Table Calcs", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);	
		global_stat_handles.mpr_calcs_global_shandle = op_stat_reg ("OLSR Performance.MPR Calcs", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.nbr_add_global_shandle = op_stat_reg ("OLSR Performance.Total Nbr Additions", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.nbr_delete_global_shandle = op_stat_reg ("OLSR Performance.Total Nbr Deletions", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.two_hop_add_global_shandle = op_stat_reg ("OLSR Performance.Two Hop Nbr Additions", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.nbrhood_change_global_shandle = op_stat_reg ("OLSR Performance.Neighborhood Changes", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.topology_change_global_shandle = op_stat_reg ("OLSR Performance.Topology Changes", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.two_hop_reachability_add_global_shandle = op_stat_reg ("OLSR Performance.Two Hop Reachability Add", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
		global_stat_handles.two_hop_reachability_del_global_shandle = op_stat_reg ("OLSR Performance.Two Hop Reachability Delete", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);

		/* initialize various memory pools */
		olsr_djk_state_pmoh = op_prg_pmo_define ("OLSR Dijkstra State", 
			sizeof (OlsrT_Djk_State), 100);
		
		olsr_djk_edge_state_pmoh = op_prg_pmo_define ("OLSR Dijkstra Edge State", 
		 	sizeof (OlsrT_Djk_Edge_State), 100);

		dup_entry_pmh = op_prg_pmo_define ("OLSR Duplicate Set Entry", 
			sizeof (OlsrT_Duplicate_Set_Entry), 32);

		rt_entry_pmh = op_prg_pmo_define ("OLSR Routing Table Entry",
			sizeof (OlsrT_Routing_Table_Entry), 32);

		mpr_set_entry_pmh = op_prg_pmo_define ("OLSR MPR Set Entry",
			sizeof (OlsrT_MPR_Set_Entry), 32);

		link_set_entry_pmh = op_prg_pmo_define ("OLSR Link Set Entry",
			sizeof (OlsrT_Link_Set_Entry), 32);

		mpr_selector_set_entry_pmh = op_prg_pmo_define ("OLSR MPR Selector Set Entry",
			sizeof (OlsrT_MPR_Selector_Set_Entry), 32);

		nbr_set_entry_pmh = op_prg_pmo_define ("OLSR Neighbor Set Entry",
			sizeof (OlsrT_Neighbor_Set_Entry), 32);

		nbr_addr_set_entry_pmh = op_prg_pmo_define ("OLSR Nbr Addr Two Hop Entry", 
			sizeof (OlsrT_Nbr_Addr_Two_Hop_Entry), 32);
		
		two_nbr_set_entry_pmh = op_prg_pmo_define ("OLSR Two Hop Nbr Set Entry", 
			sizeof (OlsrT_Two_Hop_Neighbor_Set_Entry), 32);

		tc_set_pmh = op_prg_pmo_define ("OLSR TC Set Entry",
			sizeof (OlsrT_TC_Set_Entry), 32);
		
		dest_entry_pmh = op_prg_pmo_define ("OLSR TC Dest Addr Entry",
			sizeof (OlsrT_TC_Dest_Addr_Entry), 32);
		
		mid_msg_pmh = op_prg_pmo_define ("OLSR MID Message",
			sizeof (OlsrT_MID_Message), 32);
		
		hello_msg_pmh = op_prg_pmo_define ("OLSR Hello Message",
			sizeof (OlsrT_Hello_Message), 32);
		
		hello_data_pmh = op_prg_pmo_define ("OLSR Hello Message Data",
			sizeof (OlsrT_Hello_Message_Data), 32);

		tc_msg_pmh = op_prg_pmo_define ("OLSR TC Message",
			sizeof (OlsrT_TC_Message), 32);

		olsr_msg_pmh = op_prg_pmo_define ("OLSR Message",
			sizeof (OlsrT_Message), 32);
		
        new_addr_pmh = op_prg_pmo_define ("OLSR Address",
			sizeof (int), 32);
		
        deferred_hello_info_pmh = prg_pmo_define_with_props ("Deferred OLSR HELLO info",
			sizeof (OlsrT_Rte_Deferred_Hello_Info), 32, PRGC_MEM_DONT_CLEAR);

		/* Register a namespace for OLSR Topology Graphs.	*/
		prg_graph_state_handler_register ("OLSR Topology Graph", 
			"OLSR Graph State", OPC_NIL, OPC_NIL);
		
		initialized = 1;
		}
	
	FOUT;
	}

static void
olsr_rte_sv_init (void)
	{
	Objid						own_module_objid;
	Objid						own_node_objid;
	Objid						udp_objid;
	List						proc_record_handle_list;
	int							record_handle_list_size;
	OmsT_Pr_Handle				process_record_handle;
	int							i,status, active_interfaces = 0;
	IpT_Interface_Info*			ith_intf_info_ptr;
	IpT_Phys_Interface_Info*	phys_intf_info_ptr;
	InetT_Address				inet_intf_addr;
	InetT_Address				inet_own_main_address;
	int							intf_addr, num_interfaces = 0;
	List*						intf_addr_lptr;
	int*						intf_addr_ptr;
	int							input_strm;
	IpT_Addr_Status				duplicate_status;
	
	OlsrT_Djk_State*			state_ptr;
	void*						old_entry = OPC_NIL;
	
	Boolean 					main_address_assigned = OPC_FALSE;

	/** Initialize the state variables	**/
	FIN (olsr_rte_sv_init (void));
	
	/* Obtain the process id of this module.*/
	own_module_objid = op_id_self ();

	/* Obtain the node ID of the node where this process resides.*/
	own_node_objid = op_topo_parent (own_module_objid);
	
	/* Obtain own process handle. */
	own_prohandle = op_pro_self ();
	
	my_process_id = op_pro_id (own_prohandle);
		
	/* Get IP Module data */
	module_data_ptr = ip_support_module_data_get (own_node_objid);
	
	/* Set up the display string. */
	sprintf (pid_string, "olsr_rte PID (%d)", op_pro_id (own_prohandle));

	/** Open UDP connection **/
	/* The following code is used to obtain the object id of the UDP	*/
	/* module used to transport OLSR packets over IP. Obtain the process*/
	/* record handle of neighboring udp process.						*/
	
	op_prg_list_init (&proc_record_handle_list);
	oms_pr_process_discover (own_module_objid, &proc_record_handle_list, 
							"protocol", 	OMSC_PR_STRING, 	"udp", 
							OPC_NIL);

	/* An error should be created if there are zero or more	*/
	/* than one	UDP process connected to the module.		*/
	record_handle_list_size = op_prg_list_size (&proc_record_handle_list);
	
	if (record_handle_list_size != 1)
		{
		/* Generate a simulation log message first and end simulation.	*/
		op_sim_end ("Error: Either zero or several udp processes connected to manet_dispatch.", "", "", "");
		}
	else
		{
		process_record_handle = (OmsT_Pr_Handle) op_prg_list_access (&proc_record_handle_list, OPC_LISTPOS_HEAD);
		
		/* Obtain the object id of the udp module.*/
		oms_pr_attr_get (process_record_handle, "module objid", OMSC_PR_OBJID, &udp_objid);
		
		/* Determine the input and output stream indices.				*/
		oms_tan_neighbor_streams_find (own_module_objid, udp_objid, &input_strm, &output_strm);
		}
	
	/* Deallocate the list pointer.*/
	while (op_prg_list_size (&proc_record_handle_list) > 0)
		op_prg_list_remove (&proc_record_handle_list, OPC_LISTPOS_HEAD);

	/* Read OLSR attributes. This must happen before  */
	/* the creation and initialization of the UDP ICI */
	olsr_rte_attributes_parse (own_module_objid);
	
	if(command_ici_ptr == OPC_NIL)
		{
		/* Create a receive port for this application.*/ 
		
		/* Issue the CREATE_PORT command. */
		command_ici_ptr = op_ici_create ("udp_command_inet");
		op_ici_attr_set_int32 (command_ici_ptr, "local_port", OLSRC_UDP_PORT);
		op_ici_attr_set (command_ici_ptr, "connection_class", CONNECTION_CLASS_1);
		op_ici_install (command_ici_ptr);
		op_intrpt_force_remote (UDPC_COMMAND_CREATE_PORT, udp_objid);
		}
	
	/* Get the status indication from the ici */
	op_ici_attr_get (command_ici_ptr, "status", &status); 
	
	/* Set the remote port and address in the UDP ici.		*/
	op_ici_attr_set_ptr (command_ici_ptr, "rem_addr", 
		(is_ipv6_enabled ? &InetI_Ipv6_All_Nodes_LL_Mcast_Addr: &InetI_Broadcast_v4_Addr));
	
	/* Setting the remote address in the UDP ici is sufficient  */
	/* to achieve broadcast, as needed in IPv4. However, in the */
	/* case of IPv6, the broadcast becomes multicast. For multi-*/
	/* cast to work, we need a setting into the "strm_index"    */
	/* field of the UDP ici - this setting will eventually be   */
	/* written into the "multicast_major_port" of the IP layer  */
	/* ICI. 													*/
	if (is_ipv6_enabled)
		op_ici_attr_set (command_ici_ptr, "strm_index", IPC_MCAST_ALL_MAJOR_PORTS); 
	
	op_ici_attr_set (command_ici_ptr, "rem_port", OLSRC_UDP_PORT);	

	/* Assign a unique name to this OLSR process.  It will include  */
    /* the name of the routing protocol, which in this case is OLSR */
    /* and the AS number of this process. Once the unique name is   */
    /* obtained, this process can be registered in IPs list of all  */
    /* routing processes.                                           */
    olsr_protocol_id = IP_CMN_RTE_TABLE_UNIQUE_ROUTE_PROTO_ID (IPC_DYN_RTE_OLSR, IPC_NO_MULTIPLE_PROC);
	
    Ip_Cmn_Rte_Table_Install_Routing_Proc (module_data_ptr->ip_route_table, olsr_protocol_id, own_prohandle);

    /* Create a /32 subnet mask for entries in the route table  */
    subnet_mask_32 = inet_smask_from_length_create (32);
	
	/* Create a /128 subnet mask for entries in the route table */
	subnet_mask_128 = inet_smask_from_length_create (128); 
	
	/* Create the tables */
	link_set_table = prg_bin_hash_table_create (4, sizeof (OlsrT_Link_Set_Key));
	neighbor_set_table = prg_bin_hash_table_create (4, 4);
	mpr_set_table = prg_bin_hash_table_create (4, 4);
	two_hop_nbr_set_table = prg_bin_hash_table_create (4, 4);
	mpr_selector_set_table = prg_bin_hash_table_create (4, 4);
	duplicate_set_table = prg_bin_hash_table_create (4, sizeof (OlsrT_Dup_Set_Key));

	tc_set_table = (PrgT_Bin_Hash_Table*) prg_bin_hash_table_create (4, 4);
	
	/* Bin hash table for OLSR rte table	 */
	olsr_routing_table = prg_bin_hash_table_create (4, 4);
		
	/* Create or get the handle for the OLSR TC Table */
	topo_table_hndl = oms_topo_table_create ("OLSR TC Table", olsr_rte_tc_dest_addr_entry_destroy);
	
	/* Initialize the state variables */
	pkt_seq_num = 0;
	msg_seq_num = 0;
	ANSN = 0;
	tc_entry_expiry_time = 5.0;
	neighborhood_changed = OPC_FALSE;
	selectorset_changed = OPC_FALSE;
	topology_changed = OPC_FALSE;
	send_empty_tc_time = 0.0;
	
	rte_calc_already_scheduled = OPC_FALSE;
	
	/* If this node has more than one interface, register it with the 	*/
	/* global interface set table as defined in olsr_support.ex.c 		*/
	/* Obtain the total number of active interfaces. 					*/
	/* Note: This is avoid MID messsage generation. MID messages help 	*/
	/* achieving interface to main addr mapping. Registering addresses	*/
	/* with global interface table is not in RFC						*/
	active_interfaces = inet_rte_num_interfaces_get (module_data_ptr);
 
	intf_addr_lptr = op_prg_list_create ();
	
	for (i=0; i< active_interfaces; i++)
		{
		/* Get the ith intf info ptr */
		ith_intf_info_ptr = inet_rte_intf_tbl_access (module_data_ptr, i);
			
		/* Check if this interface is not shutdown */
		phys_intf_info_ptr = ith_intf_info_ptr->phys_intf_info_ptr;
			
		if (phys_intf_info_ptr->intf_status != IpC_Intf_Status_Shutdown)
			{
			/* For IPv6, in addition to the physical interface not being */
			/* shutdown, we require the IPv6 interface to be active.     */
			/* If the interface is not active, just skip it. 			 */
			if (is_ipv6_enabled && (ip_rte_intf_ipv6_active (ith_intf_info_ptr) == OPC_FALSE))
				continue; 
			
			/* If this interface is not running OLSR, continue			*/
			if (ip_interface_routing_protocols_contains (ip_rte_intf_routing_prot_get (ith_intf_info_ptr), 
				IpC_Rte_Olsr) == OPC_FALSE)
				continue;
			
			/* If the main address has not yet been assigned, the first	*/
			/* OLSR interface we encounter is assigned the main address	*/
			if (!main_address_assigned)
				{
				inet_own_main_address = inet_rte_intf_addr_get (ith_intf_info_ptr, 
					(is_ipv6_enabled ? InetC_Addr_Family_v6:InetC_Addr_Family_v4));
				
				own_main_address = inet_rtab_unique_addr_convert (inet_own_main_address, &duplicate_status);
				if (op_prg_odb_ltrace_active ("mpr_trace"))
					printf ("-Own MA is %d", own_main_address);
				main_address_assigned = OPC_TRUE;
				
				/* Check for duplicate address */
				if (duplicate_status == IPC_ADDR_STATUS_DUPLICATE)
					{
					char   address_str [128]; 
					/* If the main address is a duplicate, we can't use it. */
					/* We could easily look for alternatives to the main    */
					/* address: for instance, in IPv6, there may be multiple*/
					/* IPv6 addresses on the iface, besides the duplicate   */
					/* address. We could therefore use one of the other     */
					/* addresses as the main address. 						*/
		
					/* Although we could find a non-duplicate main address  */
					/* in some cases, as a general policy we choose to just */
					/* ignore any duplicate addresses that arise in OLSR,   */
					/* without looking for alternatives in such cases when  */
					/* alternatives are available (e.g. when multiple IPv6  */
					/* addresses are defined on an interface, not all of    */
					/* which have duplicates in the network. 				*/

					/* If the main address could not be set to a non-dupli- */
					/* cate IP address, this OLSR process cannot continue   */
					/* to function. 									    */
					/* Tell the parent process to kill this OLSR child.     */
					/* The hash tables will be cleared in the termination   */
					/* block. The hello and TC timers will also be cleared  */
					/* in the termination block (if scheduled). 		    */
					op_intrpt_schedule_process (op_pro_parent (own_prohandle), op_sim_time(), IDLE_OLSR);
		
					if (LTRACE_ACTIVE)
						{
						op_prg_odb_print_major ("OLSR process will be killed, because its main address is a duplicate IP address", OPC_NIL); 
						}
		
					/* Notify the user via simulation log */
					inet_address_print (address_str, inet_own_main_address); 
					olsrnl_main_address_duplicate_log_write (address_str); 
		
					/* Destroy the inet_own_main_address to avoid a leak */
					inet_address_destroy (inet_own_main_address); 
		
					FOUT; 
					}
				}

			/* The interface address is gotten from the interface data  */
			/* structure. inet_rte_intf_addr_get() would accomplish the */
			/* task, but it will lead to a memory leak in case of IPv6  */
			/* addresses. When an InetT_Address holds an IPv6 address   */
			/* there is a pointer kept in the InetT_Address structure,  */
			/* that does not exist in the case of InetT_Address holding */
			/* an IPv4 address. The member "ipv6_addr_ptr" is meant to  */
			/* keep this IPv6 address pointer. Here in OLSR, we want to */
			/* avoid duplicating the memory pointed to by the IPv6 ptr  */
			/* in order to prevent a memory leak hazard. To that effect,*/
			/* we should use a version of inet_rte_intf_addr_get(), i.e.*/
			/* inet_rte_intf_addr_get_fast() that does not duplicate the*/
			/* memory pointed from the inner IPv6 address pointer. 		*/
			inet_intf_addr = inet_rte_intf_addr_get_fast (ith_intf_info_ptr, 
				(is_ipv6_enabled ? InetC_Addr_Family_v6:InetC_Addr_Family_v4));
			intf_addr = inet_rtab_unique_addr_convert (inet_intf_addr, &duplicate_status); 	
			
			/* Check for duplicate address */
			if (duplicate_status == IPC_ADDR_STATUS_DUPLICATE)
				{
				/* If this interface address has a duplicate somewhere */
				/* else in the network, we could seek to find an non-  */
				/* duplicate alternative. However, we choose not to    */
				/* follow this path, because this will lead to routing */
				/* tables being populated with non-duplicate addresses,*/
				/* that the applications running in the network will   */
				/* not use as destinations, because the applications   */
				/* do not have this intelligence to look beyond the    */
				/* first address configured on the interface. That is  */
				/* why there may be no benefit in our trying to do     */
				/* more work to recover from such IP misconfigurations */
				/* as duplicate addresses. In fact, IP itself should   */
				/* be responsible to solve the issue of duplicate      */
				/* addresses in a way that is transparent to the higher*/
				/* layers. Here in OLSR, we just take the simplest     */
				/* action to avoid the confusion due to a duplicate IP */
				/* address: simply ignore the interfaces on which such */
				/* address is configured. 						       */
				if (LTRACE_ACTIVE)
					{
					char  inet_address_str[64];
					char  tmp_str [256]; 
					
					inet_address_print (inet_address_str, inet_intf_addr); 
					sprintf (tmp_str, "Ignoring interface of duplicate address %s", inet_address_str); 
					op_prg_odb_print_major (tmp_str, OPC_NIL); 
					}
				
				continue; 
				}
			
			olsr_support_multiple_interface_register (intf_addr, (void*) &own_main_address, 1);
			 
			/* Create address and insert it in the intf_addr_lptr */
			intf_addr_ptr = olsr_rte_address_create (intf_addr);
			op_prg_list_insert (intf_addr_lptr, intf_addr_ptr, PRGC_LISTPOS_TAIL);
			
			/* We also need to register each active interface for */
			/* to join in the multicast group used for IPv6 OLSR  */
			/* to achieve the equivalent of IPv4 OLSR broadcast.  */
			/* Do this only if this is an IPv6 OLSR process.      */
			if (is_ipv6_enabled)
				{
				Inet_Address_Multicast_Register (InetI_Ipv6_All_Nodes_LL_Mcast_Addr, 
					i, OLSRC_UDP_PORT, module_data_ptr); 
				}
			
			num_interfaces++;
			}
		}
	
	if (num_interfaces > 1)
		{
		/* More than one interface is active on this node 						*/
		/* Add another entry in interface table indexed by main address 		*/
		/* This entry will contain the list of interfaces for this main addr 	*/
		olsr_support_multiple_interface_register (own_main_address, (void*) intf_addr_lptr, 2);
		}
	else if (num_interfaces == 1)
		{
		/* Only one address is active on this node 	*/
		/* Free the list of interface address		*/
		intf_addr_ptr = (int*) op_prg_list_remove (intf_addr_lptr, PRGC_LISTPOS_HEAD);
		op_prg_mem_free (intf_addr_ptr);
		op_prg_list_free (intf_addr_lptr);
		op_prg_mem_free (intf_addr_lptr);
		}
	else
		{
		/* There are no interfaces on this node */
		/* Free the list of interface addresses */
		op_prg_list_free (intf_addr_lptr); 
		op_prg_mem_free (intf_addr_lptr);

		/* Tell the parent process to kill this OLSR child.    */
		/* The hash tables will be cleared in the termination  */
		/* block. The hello and TC timers will also be cleared */
		/* in the termination block. 						   */
		op_intrpt_schedule_process (op_pro_parent (own_prohandle), op_sim_time(), IDLE_OLSR); 
		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major ("OLSR process will be killed, because there are no active interfaces on this node", OPC_NIL); 
			}
		
		olsrnl_no_active_interfaces_log_write(); 
		}

	/* Set print proc for the OLSR control packets */
	op_pk_format_print_proc_set ("olsr", "Message", olsr_rte_olsr_message_print);
	
	/* Start the timer for periodic hellos & TC */
	/* Schedule the first hello & TC interrupt	*/
	hello_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + op_dist_uniform (hello_interval), OLSRC_HELLO_TIMER_EXPIRY);
	tc_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + op_dist_uniform (tc_interval), OLSRC_TC_TIMER_EXPIRY);
	
	if (LTRACE_ACTIVE)
		{
		printf("CREATED OLSR CHILD PROCESS, CONNECTED UDP, NOW WAITING...\n");
		}
	
	/* Clear the IP Cmn Rte Table 															*/
	/* IP table is no longer blindly cleared to preserve direct and static routes			*/
	/* Inet_Cmn_Rte_Table_Clear (module_data_ptr->ip_route_table, (is_ipv6_enabled ? InetC_Addr_Family_v6:InetC_Addr_Family_v4)); */
		
	/* Before exiting this function, we need to destroy */
	/* the memory allocated for pointers inside the     */
	/* the InetT_Address used to get the NATO index of  */
	/* the main OLSR address of this OLSR process.      */
	inet_address_destroy (inet_own_main_address); 
	
	/* Graph related initialization */
	/* Initializations for DJK graph */
			
	 /* Create the graph */
	 prg_graph_ptr = prg_graph_create ("OLSR Topology Graph");
	
	 /* If there was a problem creating the graph, return NIL.	*/
	if (prg_graph_ptr == PRGC_NIL)
		{
		op_sim_end ("Error in olsr_rte_graph_create", "Could not create graph",
			OPC_NIL, OPC_NIL);
		}
		 
	 /* Create hash table to store graph vertices 		 */
	 /* Keys to store graph vertices are their NATO index */
	 djk_hash_table_ptr = prg_bin_hash_table_create (4, 4);
		
	 /* create the root vertex */
	 root_vertex_ptr = prg_graph_vertex_insert (prg_graph_ptr);
				
	 /* Allocate memory for an OSPF DJK node.	*/
	 state_ptr = (OlsrT_Djk_State *) op_prg_pmo_alloc (olsr_djk_state_pmoh);
				
	 state_ptr->addr = own_main_address;
	 state_ptr->willingness = OLSRC_INVALID;
				
	 /* Set the client state on the vertex */
	 prg_vertex_client_state_set (root_vertex_ptr, OLSRC_DJK_STATE, (void *) state_ptr);
			
	 /* Insert this entry in hash table */
	 prg_bin_hash_table_item_insert (djk_hash_table_ptr, &own_main_address, root_vertex_ptr, &old_entry);
	 
	 last_rte_calc_time = 0;

	FOUT;
	}

static void
olsr_rte_local_stats_reg (void)
	{	
	/** Initializes the statistic handles	**/
	FIN (olsr_rte_local_stats_reg (void));
			
	/* Register each of the local statistic	*/
	local_stat_handles.rte_traf_rcvd_bps_shandle = op_stat_reg ("OLSR.Routing Traffic Received (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.rte_traf_rcvd_pps_shandle = op_stat_reg ("OLSR.Routing Traffic Received (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.rte_traf_sent_bps_shandle = op_stat_reg ("OLSR.Routing Traffic Sent (bits/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.rte_traf_sent_pps_shandle = op_stat_reg ("OLSR.Routing Traffic Sent (pkts/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.total_hello_sent_shandle = op_stat_reg ("OLSR.Total Hello Messages Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.total_tc_sent_shandle = op_stat_reg ("OLSR.Total TC Messages Sent", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.total_tc_forwarded_shandle = op_stat_reg ("OLSR.Total TC Messages Forwarded", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	local_stat_handles.mpr_status_shandle = op_stat_reg ("OLSR.MPR Status", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	
	FOUT;
	}
	
static void
olsr_rte_attributes_parse (Objid own_module_objid)
	{
	/* This function reads in the attributes for OLSR */
	Objid			olsr_parms_id;
	Objid			olsr_parms_child_id;
	int				status; 
	
	FIN (olsr_rte_attributes_parse (void));
	
	/* Read the OLSR Parameters	attribute */
	op_ima_obj_attr_get (own_module_objid, "OLSR Parameters", &olsr_parms_id);
	olsr_parms_child_id = op_topo_child (olsr_parms_id, OPC_OBJTYPE_GENERIC, 0);
	
	/* Read in the attributes	*/
	op_ima_obj_attr_get (olsr_parms_child_id, "Willingness", &node_willingness);
	op_ima_obj_attr_get (olsr_parms_child_id, "Hello Interval", &hello_interval);
	op_ima_obj_attr_get (olsr_parms_child_id, "TC Interval", &tc_interval);
	op_ima_obj_attr_get (olsr_parms_child_id, "Neighbor Hold Time", &neighbor_hold_time);
	op_ima_obj_attr_get (olsr_parms_child_id, "Topology Hold Time", &tc_hold_time);
	op_ima_obj_attr_get (olsr_parms_child_id, "Duplicate Message Hold Time", &dup_hold_time);
	status = op_ima_obj_attr_get (olsr_parms_child_id, "Addressing Mode", &is_ipv6_enabled);

	if (op_ima_sim_attr_exists ("OLSR Hello Processing Time Quantum"))
		op_ima_sim_attr_get (OPC_IMA_DOUBLE, "OLSR Hello Processing Time Quantum", &hello_processing_time_quantum);
	else
		hello_processing_time_quantum = 0;
	
	if (op_ima_sim_attr_exists ("OLSR DJK Processing Time Quantum"))
		op_ima_sim_attr_get (OPC_IMA_DOUBLE, "OLSR DJK Processing Time Quantum", &djk_processing_time_quantum);
	else
		djk_processing_time_quantum = 0;
	
	if (status == OPC_COMPCODE_FAILURE)
		is_ipv6_enabled = OPC_FALSE; 
	
	
	FOUT;
	}


/*********************************************************************/
/******************** PACKET ARRIVAL FUNCTIONS ***********************/
/*********************************************************************/

static void
olsr_rte_pkt_arrival_handle (void)
	{
	Packet*					pkptr;
	Ici*					ici_ptr;
	const OlsrT_Message*	olsr_message_ptr;
	InetT_Address*			inet_local_intf_addr_ptr;
	InetT_Address*			inet_ip_src_addr_ptr;
	Boolean					do_not_process = OPC_FALSE;
	InetT_Address			inet_local_intf_addr;
	InetT_Address			inet_ip_src_addr;
	IpT_Addr_Status			is_local_ip_duplicate; 
	IpT_Addr_Status  		is_remote_ip_duplicate; 
	int						local_intf_addr, ip_src_addr;
	int 					addr_num;
	OlsrT_Duplicate_Set_Entry* 	dup_set_entry_ptr;
	OlsrT_Neighbor_Set_Entry*	nbr_set_entry_ptr;
		
	/** Whenever a control packet arrives at the**/
	/** port where OLSR process is listening, 	**/
	/** this process is invoked					**/
	/** A packet has arrived. Handle the packet	**/
	/** appropriately based on its type			**/
	FIN (olsr_rte_pkt_arrival_handle (void));
	
	/* The process was invoked by the parent		*/
	/* MANET_RTE_MGR process indicating the arrival	*/
	/* of a packet.	The packet is OLSR control		*/
	/* packet. Process depeding upon the MESSAGE	*/
	/* type.										*/
	
	pkptr = (Packet*) op_pro_argmem_access ();
		
	/* Obtain the interface address and ip source address from the installed ICI.*/
	ici_ptr = op_intrpt_ici ();
	
	op_ici_attr_get (ici_ptr, "interface received", &inet_local_intf_addr_ptr);
	op_ici_attr_get (ici_ptr, "rem_addr", &inet_ip_src_addr_ptr);
	
	/* Currently, we support only one message	*/ 
	/* type in each OLSR control packet. 		*/
	
	/* Get the OLSR Message */
	op_pk_nfd_access_read_only_ptr (pkptr, "Message", (const void **) &olsr_message_ptr);

	if ((LTRACE_ACTIVE)||
		(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
		(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
		{
		if (olsr_message_ptr->message_type == OLSRC_HELLO_MESSAGE)
			op_prg_odb_print_major (pid_string, "\nReceived HELLO message \n", OPC_NIL);
		else if (olsr_message_ptr->message_type == OLSRC_TC_MESSAGE)
			op_prg_odb_print_major (pid_string, "\nReceived TC message \n", OPC_NIL);
		}
	
	/* Update the statistics for the routing traffic received	*/
	olsr_support_routing_traffic_received_stats_update (&local_stat_handles, &global_stat_handles, pkptr);
	
	/* Discard the packet if it carries an IP family different than */
	/* the IP family of this OLSR process (e.g. if the packet is    */
	/* OLSR IPv6, when this process is OLSR IPv4.) 					*/
	if (is_ipv6_enabled != olsr_message_ptr->is_ipv6)
		{
		op_pk_destroy (pkptr); 
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			printf ("Received a packet of different IP address family. Destroy it \n");
			}
		
		FOUT; 
		}
	
	/* Discard the packet if its a loopback i.e. i received my own packet */
	/* Discard the packet if TTL has reached zero */
	if (olsr_message_ptr->originator_addr == own_main_address)
		{
		op_pk_destroy (pkptr);
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			printf ("Received my own packet. Destroy it \n");
			}

		FOUT;
		}
		
	if (olsr_message_ptr->ttl == 0)
		{
		op_pk_destroy (pkptr);
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			printf ("TTL reached zero. Destroy it \n");
			}

		FOUT;
		}

	if (olsr_message_ptr->message_type == OLSRC_HELLO_MESSAGE)
		{
		/* Defer hello packets processing if appropriate */
		if (hello_processing_time_quantum > 0)
			{
			/*  Insert information about this HELLO at the end of the chain */
			OlsrT_Rte_Deferred_Hello_Info * info_ptr = (OlsrT_Rte_Deferred_Hello_Info *)
				op_prg_pmo_alloc (deferred_hello_info_pmh);

			info_ptr->pkptr = pkptr;
			info_ptr->olsr_message_ptr = olsr_message_ptr;
			info_ptr->inet_local_intf_addr_ptr = inet_local_intf_addr_ptr;
			info_ptr->inet_ip_src_addr_ptr = inet_ip_src_addr_ptr;

			info_ptr->next_ptr = NULL;
			if (hello_processing_deferred_tail_ptr)
				{
				hello_processing_deferred_tail_ptr->next_ptr = info_ptr;
				hello_processing_deferred_tail_ptr = info_ptr;
				}
			else
				{
				hello_processing_deferred_tail_ptr = hello_processing_deferred_head_ptr = info_ptr;

				/* this also implies that interrupt to process hellos is not yet scheduled */
				/* (computation interrupts evenly spaced in simulation time domain to allow better chances of parallel processing) */
				op_intrpt_schedule_self (ceil (op_sim_time () / hello_processing_time_quantum) * hello_processing_time_quantum,
					OLSRC_HELLO_PROCESSING_TIMER_EXPIRY);
				}
			}
		else
			{
			olsr_rte_process_hello_pk (pkptr, olsr_message_ptr, inet_local_intf_addr_ptr, inet_ip_src_addr_ptr);
			
			/* Check if either neighborhood or topology has changed (after processing all hellos) */
			olsr_rte_neighborhood_topology_check_hello ();
			}
		FOUT;
		}
	
	/* Check if this packet has been processed earlier 	*/
	/* Get the duplicate set entry, it it exists 		*/
	{
	OlsrT_Dup_Set_Key	key;
	key.originator_addr = olsr_message_ptr->originator_addr;
	key.message_seq_num = olsr_message_ptr->message_seq_num;
	dup_set_entry_ptr = (OlsrT_Duplicate_Set_Entry*)
		prg_bin_hash_table_item_get (duplicate_set_table, &key);
	}
	
	if (dup_set_entry_ptr != OPC_NIL)
		do_not_process = OPC_TRUE;
	
	/* Get the InetT_Address from pointers */
	inet_local_intf_addr = *inet_local_intf_addr_ptr;
	inet_ip_src_addr = *inet_ip_src_addr_ptr;
				
	/* Convert this InetT_Address to NATO index */
    local_intf_addr = inet_rtab_unique_addr_convert (inet_local_intf_addr, &is_local_ip_duplicate);
    ip_src_addr = inet_rtab_unique_addr_convert (inet_ip_src_addr, &is_remote_ip_duplicate);
			
	/* If this Hello packet has the source or destination IP as duplicate */
	/* addresses, do not process the packet. It will be discarded.  By    */
	/* ignoring such hello packets, we are preventing link set entries    */
	/* with duplicate addresses from entering OLSR. This is not specified */
	/* by the RFC, it is part of this OLSR implementation protection      */
	/* against misconfiguration. 										  */
	if ((is_local_ip_duplicate == IPC_ADDR_STATUS_DUPLICATE) || 
		(is_remote_ip_duplicate == IPC_ADDR_STATUS_DUPLICATE))
		{
		op_pk_destroy (pkptr);
		
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			printf ("Check network for duplicate IP addresses. Destroying the packet\n");
			}
		
		/* Free the memory occupied by these addresses in the ICI */
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		FOUT;
		}
	
	/* This packet was not processed before */
	if (do_not_process == OPC_FALSE)
		{
		/* Process the message according to msg_type (which can only be TC here -- HELLO processed above) */
		switch (olsr_message_ptr->message_type)
			{
			case (OLSRC_TC_MESSAGE):
				{
				Boolean			discard_tc;
				
				if (LTRACE_ACTIVE ||(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
					{
					char			tmp_str [256];
					char			node_name [256];
					InetT_Address	inet_tmp_addr;

					inet_address_print (tmp_str, inet_ip_src_addr);
					inet_address_to_hname (inet_ip_src_addr, node_name);
					printf ("Processing TC received from: %s (%s)\n", tmp_str, node_name);
				
					inet_tmp_addr = inet_rtab_index_to_addr_convert (olsr_message_ptr->originator_addr);
					inet_address_print (tmp_str, inet_tmp_addr);
					inet_address_to_hname (inet_tmp_addr, node_name);
					printf ("TC originator address %s (%s)\n", tmp_str, node_name);
					}
				
				/* Process the  TC message	*/
				discard_tc = olsr_rte_process_TC (olsr_message_ptr, ip_src_addr);
				
				/* Check if either neighborhood or topology has changed */
				olsr_rte_neighborhood_topology_check_TC ();
	
				/* This TC is to be discarded 	*/
				/* It can be due to : 			*/
				/* 1. Heard from Non-Sym nbr	*/
				/* 2. Out of order TC mesg		*/
				if (discard_tc == OPC_TRUE)
					{
					if ((op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
						{
						op_prg_odb_print_minor ("Discarding this TC \n", OPC_NIL);
						}
					
					/* This TC message will not be considered for */
					/* forwarding. Hence, destroy this TC packet  */
					op_pk_destroy (pkptr);
					
					/* Free the memory occupied by these addresses in the ICI */
					inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
					inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
					
					/* Exit the function */
					FOUT;
					}
			
				break;
				}
			
			default:
				{
				/* Invalid message type */
				olsr_rte_error ("Invalid Message Type in OLSR packet", OPC_NIL, OPC_NIL);
				}
			}
		}/*end if do_not_process*/
	else
		{
		/* This else part just prints tracing information. 					*/
		/* Only TC messages add duplicate set entries, hence for HELLO msg, */
		/* do_not_process will never be True. We'll reach here when this 	*/
		/* TC message was already processed. Now consider it for forwarding.*/
		/* Do not destroy this packet yet as forwarding condition needs to	*/
		/* be checked.														*/
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			printf ("Message has already been processed (or it involves duplicate IP addresses). Do not process\n");
			}
		}
		
	/* Reached here either after processing the TC msg, 	*/
	/* or the TC msg was already processed					*/
	/* In either case, check if we need to forward this pkt	*/
	/* Note: We'll not reach here when Hello is received	*/
	
	/* Execute default forwarding algorithm for this TC */
	if (op_prg_odb_ltrace_active ("trace_tc"))
		{
		op_prg_odb_print_minor ("Checking forwarding conditions\n", OPC_NIL);
		}
		
	addr_num = olsr_support_main_addr_get (ip_src_addr);
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) prg_bin_hash_table_item_get (neighbor_set_table, &addr_num);
	
	/* If this was already processed TC, we need */
	/* to check if it is received from SYM NBR   */
	if ((do_not_process == OPC_TRUE) && 
		((nbr_set_entry_ptr == OPC_NIL) ||
		(nbr_set_entry_ptr->status != OLSRC_SYM_STATUS)))
		{
		/* Discard this packet as the sender is not 1-hop sym nbr */
		if (op_prg_odb_ltrace_active ("trace_tc"))
			op_prg_odb_print_minor ("TC not recvd by 1-hop SYM nbr, Discarding TC while forwarding\n", OPC_NIL);
		
		/* No further processing of this TC message	*/
		op_pk_destroy (pkptr);
		
		/* Free the memory occupied by these addresses in the ICI */
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
			
		/* Exit the function */
		FOUT;
		}
		
	/* Check if this TC was already forwarded */
	if ((dup_set_entry_ptr != OPC_NIL) && 
		(dup_set_entry_ptr->rexmitted == OPC_TRUE))
		{
		/* This TC has already been considered for forwarding */
		if (op_prg_odb_ltrace_active ("trace_tc"))
			op_prg_odb_print_minor ("This TC has already been considered for forwarding, Discarding TC \n", OPC_NIL);
					
		/* Discard the packet */
		op_pk_destroy (pkptr);
		
		/* Free the memory occupied by these addresses in the ICI */
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		/* Exit the function */
		FOUT;
		}
	else
		{
		/* Either no duplicate entry exists or this TC was not fwded*/
		/* Candidate for forwarding, check if the condition matches */
		/* Check if the source address is in my MPR selector list 	*/

		/* Packet forwarding will require a change to the structure (ttl--) */
		/* Thus we need to re-obtain an olsr message field which is modifiable  */
		OlsrT_Message*	mod_olsr_message_ptr;
		op_pk_nfd_access (pkptr, "Message", &mod_olsr_message_ptr);
		/* Update the const message ptr too */
		olsr_message_ptr = mod_olsr_message_ptr;
		
		if (olsr_rte_forward_packet (mod_olsr_message_ptr, ip_src_addr) == OPC_TRUE)
			{
			if (op_prg_odb_ltrace_active ("trace_tc"))
					op_prg_odb_print_minor ("Forwarding (Re-broadcasting) this TC message \n", OPC_NIL);
			
			/* Add entry in dup table set */
			olsr_rte_duplicate_set_entry_add 
				(olsr_message_ptr->originator_addr,olsr_message_ptr->message_seq_num, OPC_TRUE);
	
			/* Update the number of TC messages forwarded statistic	*/
			op_stat_write (local_stat_handles.total_tc_forwarded_shandle, 1.0);
			op_stat_write (global_stat_handles.total_tc_forwarded_global_shandle, 1.0);
				
			/* Update the tc traffic sent in bits per second	*/
			op_stat_write (global_stat_handles.tc_traffic_sent_bps_global_handle, op_pk_total_size_get (pkptr));
			op_stat_write (global_stat_handles.tc_traffic_sent_bps_global_handle, 0.0);
				
			/* Forward this TC Message */
			olsr_rte_pkt_send (pkptr, (is_ipv6_enabled? InetI_Ipv6_All_Nodes_LL_Mcast_Addr: InetI_Broadcast_v4_Addr), OPC_TRUE);
			
			/* Free the memory occupied by these addresses in the ICI */
			inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
			inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
			}
		else
			{
			/* Forwarding conditions did not match */
			if (op_prg_odb_ltrace_active ("trace_tc"))
				op_prg_odb_print_minor ("The node is not an MPR to forward this TC, Discarding TC \n", OPC_NIL);
				
			/* Update the duplicate set entry for this TC */
			olsr_rte_duplicate_set_entry_add (olsr_message_ptr->originator_addr,
													olsr_message_ptr->message_seq_num, OPC_FALSE);
			op_pk_destroy (pkptr);
			
			/* Free the memory occupied by these addresses in the ICI */
			inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
			inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
			}
		}
	
	FOUT;
	}


/****************************************/
/***** Packet Process Functions *********/
/****************************************/
static void
olsr_hello_processing_expiry_handle (void)
	{
	OlsrT_Rte_Deferred_Hello_Info * info_ptr;
	
	/* process all pending hello packets */
	FIN (olsr_hello_processing_expiry_handle);

	/* decouple current list of pending hellos from the chain */
	info_ptr = hello_processing_deferred_head_ptr;
	hello_processing_deferred_head_ptr = hello_processing_deferred_tail_ptr = NULL;
	while (info_ptr)
		{
		OlsrT_Rte_Deferred_Hello_Info * ref_ptr = info_ptr;
		olsr_rte_process_hello_pk (info_ptr->pkptr, info_ptr->olsr_message_ptr,
			info_ptr->inet_local_intf_addr_ptr, info_ptr->inet_ip_src_addr_ptr);
		info_ptr = info_ptr->next_ptr;
		op_prg_mem_free (ref_ptr);
		}
	
	/* Check if either neighborhood or topology has changed (after processing all hellos) */
	olsr_rte_neighborhood_topology_check_hello ();
	
	FOUT;
	}

static void
olsr_rte_process_hello_pk (Packet * pkptr, const OlsrT_Message * olsr_message_ptr,
	InetT_Address * inet_local_intf_addr_ptr, InetT_Address * inet_ip_src_addr_ptr)
	{
	int				local_intf_addr, ip_src_addr;
	InetT_Address	inet_local_intf_addr;
	InetT_Address	inet_ip_src_addr;
	IpT_Addr_Status	is_local_ip_duplicate; 
	IpT_Addr_Status	is_remote_ip_duplicate;
	
	/* Validate & process single hello packet */
	/* Also dealocate all memory associated with that request */
	FIN (olsr_rte_process_hello_pk);

	inet_local_intf_addr = *inet_local_intf_addr_ptr;
	inet_ip_src_addr = *inet_ip_src_addr_ptr;
	
	/* Convert this InetT_Address to NATO index */
    local_intf_addr = inet_rtab_unique_addr_convert (inet_local_intf_addr, &is_local_ip_duplicate);
    ip_src_addr = inet_rtab_unique_addr_convert (inet_ip_src_addr, &is_remote_ip_duplicate);
	
	/* If this Hello packet has the source or destination IP as duplicate */
	/* addresses, do not process the packet. It will be discarded.  By    */
	/* ignoring such hello packets, we are preventing link set entries    */
	/* with duplicate addresses from entering OLSR. This is not specified */
	/* by the RFC, it is part of this OLSR implementation protection      */
	/* against misconfiguration. 										  */
	if ((is_local_ip_duplicate == IPC_ADDR_STATUS_DUPLICATE) || 
		(is_remote_ip_duplicate == IPC_ADDR_STATUS_DUPLICATE))
		{
		op_pk_destroy (pkptr);
		
		/* Free the memory occupied by these addresses in the ICI */
		inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
		inet_address_destroy_dynamic (inet_ip_src_addr_ptr);
		
		if ((LTRACE_ACTIVE)||
			(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE) ||
			(op_prg_odb_ltrace_active ("trace_tc")== OPC_TRUE))
			{
			op_prg_odb_print_minor ("Check network for duplicate IP addresses. Destroying the packet\n", OPC_NIL);
			}
		
		FOUT;
		}
	
	if (LTRACE_ACTIVE ||(op_prg_odb_ltrace_active ("trace_hello")== OPC_TRUE))
		{
		char	tmp_str [256];
		
		inet_address_print (tmp_str, inet_ip_src_addr);
		printf ("Processing HELLO received from: %s ", tmp_str);
		inet_address_to_hname (inet_ip_src_addr, tmp_str);
		printf ("(%s)\n", tmp_str);
		}
				
	/* Process the Hello Message	*/
	olsr_rte_process_hello (olsr_message_ptr, ip_src_addr, local_intf_addr);
	
	/* Free the memory occupied by these addresses in the ICI */
	inet_address_destroy_dynamic (inet_local_intf_addr_ptr);
	inet_address_destroy_dynamic (inet_ip_src_addr_ptr);

	/* Hello message is not considered for forwarding */
	/* Hence, destroy the message after processing 	  */
	op_pk_destroy (pkptr);
	
	FOUT;
	}

		
static void
olsr_rte_process_hello (const OlsrT_Message* olsr_message_ptr, int ip_src_addr, int local_intf_addr)
	{
	/* Processes hello message received */
	/* Updates link set, neigbor set, 	*/
	/* 2hop neighbor set, mpr set etc.	*/
	int									originator_addr;
	int									message_size, ttl, hop_count, message_seq_num;
	int									vtime, htime, willingness;
	OlsrT_Hello_Message*				hello_msg_ptr;
	PrgT_Vector*						hello_msg_data_vptr;
	OlsrT_Link_Set_Entry*				link_set_entry_ptr;
	
	FIN (olsr_rte_process_hello (OlsrT_Message*, int, int));

	/* Get information from the olsr message header */
	originator_addr = olsr_message_ptr->originator_addr;
	message_size = olsr_message_ptr->message_size;
	message_seq_num = olsr_message_ptr->message_seq_num;
	vtime = olsr_message_ptr->vtime;
	ttl = olsr_message_ptr->ttl;
	hop_count = olsr_message_ptr->hop_count;
	
	/* Get the hello message from olsr_msg */
	hello_msg_ptr = (OlsrT_Hello_Message*) olsr_message_ptr->message;
	
	/* Get information from hello message */
	htime = hello_msg_ptr->htime;
	willingness = hello_msg_ptr->willingness;

	/* Get the list of hello msg data with different linkcodes */
	hello_msg_data_vptr = (PrgT_Vector*) hello_msg_ptr->hello_msg_vptr;
	
	/* First update link set and neigbor set from the	*/
	/* ip_src_addr and originator addr info	in this		*/
	/* hello message									*/
	/* Note: ip_src_addr can be different from 			*/
	/* originitor address, as hello is sent per 		*/
	/* interface										*/
	
	if ((link_set_entry_ptr = olsr_rte_link_set_entry_get (ip_src_addr, local_intf_addr)) == OPC_NIL)
		{
		OlsrT_Neighbor_Set_Entry*			neighbor_set_entry_ptr;
		
		/* New link, no entry exist in routing table	*/
		/* Create a new entry							*/
		link_set_entry_ptr = olsr_rte_link_set_entry_create (local_intf_addr, ip_src_addr,
			op_sim_time() -1, op_sim_time() + vtime, op_sim_time() + vtime);
		
		if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
			{
			InetT_Address	inet_tmp_addr;
			char			tmp_str [256], node_name [256];
			
			inet_tmp_addr = inet_rtab_index_to_addr_convert (ip_src_addr);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("Adding %s (%s) as new link entry\n", tmp_str, node_name);
			}
		
		/* Add neighbor if the originator_addr is not 	*/
		/* listed as one of the neigbor in nbr table	*/
		if ((neighbor_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) 
			olsr_rte_neighbor_set_entry_get (originator_addr)) == OPC_NIL)
			{
			if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
				{
				InetT_Address	inet_tmp_addr;
				char			tmp_str [256], node_name [256];
				
				inet_tmp_addr = inet_rtab_index_to_addr_convert (originator_addr);
				inet_address_print (tmp_str, inet_tmp_addr);
				inet_address_to_hname (inet_tmp_addr, node_name);
				printf ("Adding %s (%s) as new neighbor\n", tmp_str, node_name);
				}
			
			/* New neighbor, create an entry */
			neighbor_set_entry_ptr = olsr_rte_neighbor_set_entry_create 
				(originator_addr, OLSRC_NOT_SYM_STATUS, willingness);
			
			}
		
		/* In either case, added new neigbhor or neighbor already existed */
		/* increament the link_entries for this neighbor as its a new link */
		neighbor_set_entry_ptr->link_entry_count++;
		
		/* Add the neighbor_entry_ptr to the link set entry */
		link_set_entry_ptr->nbr_entry_ptr = neighbor_set_entry_ptr;
		}
	else 
		{
		/* This link entry already exists. Update the ASYM_time */
		link_set_entry_ptr->ASYM_time = op_sim_time () + vtime;
		
		/* expiry_time = max (expiry_time, ASYM_time) */
		if (link_set_entry_ptr->expiry_time < link_set_entry_ptr->ASYM_time)
			link_set_entry_ptr->expiry_time = link_set_entry_ptr->ASYM_time;
		}
	
	/* Now check if there are any hello message data in this hello mesg */
	/* hello_msg_data_vptr contains the list of hello msg data listed 	*/
	/* by linkcode. Each hello msg data has list of interfaces with 	*/
	/* corresponding linkcode. It is possible that hello message is 	*/
	/* empty and there is no interface address information present		*/
	if (hello_msg_data_vptr != OPC_NIL)
		{
		int							found_myself_linkcode;
		int							hello_msg_data_num;
		OlsrT_Neighbor_Set_Entry*	neighbor_set_entry_ptr;
		
		/* There are hello message data in this hello message 	*/
		/* Get the number of these hello msg data 				*/
		hello_msg_data_num = prg_vector_size (hello_msg_data_vptr);
		
		/* If i find my own address in the list of intf addresses in hello mesg */
		/* function will return the linkcode, else it will return -99			*/
		found_myself_linkcode = olsr_rte_find_myself_in_hello_msg (hello_msg_data_vptr, local_intf_addr);

		/* Update the SYM_time and expiry_time for link set  	*/
		/* and Neighbor Status in Nbr Set depending upon the	*/
		/* condition if we find ourself in the list of addresses*/
		/* in hello msg											*/
		if (found_myself_linkcode != OLSRC_ADDR_NOT_FOUND)
			{
			int				link_type, nbr_type;
			
			/* We found our address in the hello msg, Get the 	*/
			/* link type and nbr_type from this linkcode 		*/
			olsr_rte_breakup_linkcode (found_myself_linkcode, &link_type, &nbr_type);
			
			if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
				{
				printf ("Found my interface address in this hello message\n");
				}
			
			switch (link_type)
				{
				case (OLSRC_LOST_LINK):
					{
					/* If this link was symmetric and we have just lost it */
					if (link_set_entry_ptr->SYM_time > op_sim_time ())
						{
						/* Cancel the previosuly scheduled interrupt for this entry */
						op_ev_cancel_if_pending (link_set_entry_ptr->sym_time_expiry_evhandle);
					
						/* Call olsr_rte_symmetric_link_expiry_handle */
						olsr_rte_symmetric_link_expiry_handle (link_set_entry_ptr, OLSRC_CONN_LOSS_TIMER);
						}
					
					/* This link is advertised as broken, make it ASYM */
					link_set_entry_ptr->SYM_time = op_sim_time () - 1;
					
					break;
					}
				case (OLSRC_SYMMETRIC_LINK):
				case (OLSRC_ASYMMETRIC_LINK):
					{
					/* First check if this link was already symmetric or not */
					if (link_set_entry_ptr->SYM_time < op_sim_time ())
						{
						/* This link was asymmetric, now it is going to become symmetric*/
						/* We would have handled the interrupt when this expired		*/
						/* We should schedule a fresh interrupt for this entry 			*/
						
						/* Increament the sym_link_entry_count */
						neighbor_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) link_set_entry_ptr->nbr_entry_ptr;
						neighbor_set_entry_ptr->sym_link_entry_count++;
						}
					else
						{
						/* This link was already symmetric, we are just refreshing it 	*/
						/* Cancel the previously scheduled event if its pending	and   	*/
						/* schedule a new interrupt for the new time.					*/
						op_ev_cancel_if_pending (link_set_entry_ptr->sym_time_expiry_evhandle);
						}
								
					/* Update the SYM time */
					link_set_entry_ptr->SYM_time = op_sim_time () + vtime;
					link_set_entry_ptr->expiry_time = link_set_entry_ptr->SYM_time + neighbor_hold_time;
					
					/* Schedule the interrupt for sym link expiry */
					link_set_entry_ptr->sym_time_expiry_evhandle = op_intrpt_schedule_call (link_set_entry_ptr->SYM_time, 
						OLSRC_SYM_LINK_EXPIRY,	olsr_rte_symmetric_link_expiry_handle, link_set_entry_ptr);
										
					break;
					}
				default:
					break;
				} /*end switch*/
			
			/* Check if i have been selected as MPR, If yes, 	*/
			/* add the source of Hello msg in my selector set 	*/
			if (nbr_type == OLSRC_MPR_NEIGHBOR)
				{
				if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
					{
					InetT_Address	inet_tmp_addr;
					char			tmp_str [256], node_name [256];
					
					inet_tmp_addr = inet_rtab_index_to_addr_convert (originator_addr);
					inet_address_print (tmp_str, inet_tmp_addr);
					inet_address_to_hname (inet_tmp_addr, node_name);
					printf ("MPR Selector entry added for %s (%s)-", tmp_str, node_name);
					}
				olsr_rte_mpr_selector_set_entry_create (originator_addr, vtime);
				}
			}
		
		/* expiry_time = max (expiry_time, ASYM_time) */
		if (link_set_entry_ptr->expiry_time < link_set_entry_ptr->ASYM_time)
			link_set_entry_ptr->expiry_time = link_set_entry_ptr->ASYM_time;
		
		/* Check all the interfaces of this neighbor, if any of them */
		/* has SYM_time > current_time, set neighbor status as SYM */
		neighbor_set_entry_ptr = olsr_rte_set_neighbor_status (originator_addr, link_set_entry_ptr);
		
		/* Now we will parse through each interface address */
		/* listed in hello msg and update the 2hop_nbr set 	*/
		
		/* Process only, if this hello was received from a symmetric neighbor */
		if ((neighbor_set_entry_ptr->status == OLSRC_SYM_STATUS))
			{
			int		i;
			
			for (i=0; i < hello_msg_data_num; i++)
				{
				int				j;
				int				linkcode;
				int				link_type, nbr_type;
				int				intf_addr_num;
				PrgT_Vector*	intf_addr_vptr;
				
				/* For each hello msg data, Get hello msg data */
				OlsrT_Hello_Message_Data	*hello_msg_data_ptr =
					(OlsrT_Hello_Message_Data*) prg_vector_access (hello_msg_data_vptr, i);
		
				/* Get the linkcode for this hello msg data */
				linkcode = hello_msg_data_ptr->linkcode;
				
				/* Get link_type and nbr_type */
				olsr_rte_breakup_linkcode (linkcode, &link_type, &nbr_type);
						
				/* Get list of interface address in the data */
				intf_addr_vptr = (PrgT_Vector*) hello_msg_data_ptr->nbr_intf_addr_vptr;
		
				/* Get the number of interface address */
				intf_addr_num = prg_vector_size (intf_addr_vptr);
		
				for (j=0; j < intf_addr_num; j++)
					{
					/* For each of the interface address listed in this hello msg data */
					int * intf_addr_ptr = (int*) prg_vector_access (intf_addr_vptr, j);
					
					/* Get the main addr of this interface */
					int	two_hop_nbr_addr = olsr_support_main_addr_get (*intf_addr_ptr);
					
					/* Update 2-hop neighbor set */
					if ((nbr_type == OLSRC_SYM_NEIGHBOR) || (nbr_type == OLSRC_MPR_NEIGHBOR))
						{
						/* Do not add to two hop nbr if the interface addr belongs to	*/
						/* me i.e. my main addr = neighbor main addr. 					*/
						if (own_main_address != two_hop_nbr_addr)
							{
							/* Not my interface, Create a 2hop_set entry */
							olsr_rte_two_hop_nbr_set_entry_create (two_hop_nbr_addr, originator_addr, vtime);
							}
						}

					if (nbr_type == OLSRC_NOT_NEIGHBOR)
						{
						/* This two hop neighbor cannot be reached through  	*/
						/* given orig addr anymore, update two hop neighbor set */
						olsr_rte_two_hop_nbr_set_entry_match_n_delete (two_hop_nbr_addr, originator_addr);
						
						}
					}
				}
			} /* end this hello msg was recvd from sym nbr */
		} /* close if-else: (hello_msg_ptr != OPC_NIL)*/
		
	FOUT;
		
	}

static Boolean
olsr_rte_process_TC (const OlsrT_Message* olsr_message_ptr, int ip_src_addr)
	{
	/* Process the received TC message 		*/
	/* Returns TRUE if this tc message is 	*/
	/* out of order and should be discarded	*/
	/* Else, returns FALSE					*/
			
	OlsrT_TC_Message*			tc_msg_ptr = OPC_NIL;
	int							rcvd_seq_num, message_size, message_seq_num;
	int							vtime, ttl, hop_count;
	void*						old_contents_ptr;
	OlsrT_TC_Set_Entry*			tc_set_entry_ptr = OPC_NIL;
	OlsrT_TC_Dest_Addr_Entry*	dest_addr_entry_ptr = OPC_NIL;
	void*						state_ptr = OPC_NIL;
	int							num_mpr_selectors;
	Boolean						entry_found = OPC_FALSE;
	char						tmp_str [256], node_name [256];
	InetT_Address				inet_tmp_addr;
	Boolean						tmp_var;
	int							originator_addr;
	OlsrT_Neighbor_Set_Entry*	nbr_set_entry_ptr = OPC_NIL;
	int addr_num;	

	FIN ( olsr_rte_process_TC (OlsrT_Message*, int));

	/* Get information from the olsr message header */
	originator_addr = olsr_message_ptr->originator_addr;
	message_size = olsr_message_ptr->message_size;
	message_seq_num = olsr_message_ptr->message_seq_num;
	vtime = olsr_message_ptr->vtime;
	ttl = olsr_message_ptr->ttl;
	hop_count = olsr_message_ptr->hop_count;
	
	/* Lazy method: Delete expired entries from TC table */
	olsr_rte_expired_tc_entries_remove ();

	/* Check if this TC packet was received from a symmetric 1 hop nbr */
	addr_num = olsr_support_main_addr_get (ip_src_addr);
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) 
		prg_bin_hash_table_item_get (neighbor_set_table, &addr_num);
	
	if ((nbr_set_entry_ptr == OPC_NIL) ||
		(nbr_set_entry_ptr->status != OLSRC_SYM_STATUS))
		{
		/* Discard this packet as the sender is not 1-hop sym nbr 	*/
		/* Return OPC_TRUE, so that it is destroyed. 				*/
		/* No further processing of this TC message					*/
		if (op_prg_odb_ltrace_active ("trace_tc"))
			{
			printf ("Discarding TC not recvd by SYM 1 hop nbr\n");
			}
		
		/* Add this in duplicate set, so that this msg is not 		*/
		/* considered for processing or forwarding again.			*/
		olsr_rte_duplicate_set_entry_add
				(olsr_message_ptr->originator_addr, olsr_message_ptr->message_seq_num, OPC_TRUE);
					
		FRET (OPC_TRUE);
		}
	
	/* Get the tc message from olsr_msg */
	tc_msg_ptr = (OlsrT_TC_Message*) olsr_message_ptr->message;
	rcvd_seq_num = tc_msg_ptr->tc_seq_num;
	
	/* Originator address is the last_addr field in TC table 	*/
	/* TC entries are indexed with last_addr field				*/
	if ((tc_set_entry_ptr = (OlsrT_TC_Set_Entry*) 
		prg_bin_hash_table_item_get (tc_set_table, &originator_addr)) != OPC_NIL)
		{
		/* An entry for this originator address exists in TC msg 		*/
		/* We are going to compare current seq num with rcvd seq num	*/
		
		if (op_prg_odb_ltrace_active ("trace_tc"))
			{
			inet_tmp_addr = inet_rtab_index_to_addr_convert (originator_addr);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("Entry for last_addr: %s (%s) found in TC table\n", tmp_str, node_name);
			}
	
		/* Check for out-of-order TC */
		if (tc_set_entry_ptr->tc_seq_num > rcvd_seq_num)
			{
			if (op_prg_odb_ltrace_active ("trace_tc"))
				{
				printf ("Discarding OUT OF ORDER TC, Curr Seq No:%d, TC seq no:%d\n",
								tc_set_entry_ptr->tc_seq_num , rcvd_seq_num);
				}
			
			/* This is an out of order TC message, discard this TC  */
			/* Return OPC_TRUE, so that it is destroyed. 			*/
			/* No further processing of this TC message				*/
			
			/* Add the entry in duplicate set table. We don't want	*/
			/* to process it again nor consider for it for fwding 	*/
			olsr_rte_duplicate_set_entry_add
				(olsr_message_ptr->originator_addr, olsr_message_ptr->message_seq_num, OPC_TRUE);
			
			FRET (OPC_TRUE);
			}
		
		if (tc_set_entry_ptr->tc_seq_num < rcvd_seq_num)
			{
			/* Delete old entries that have Seq number smaller than the recevied TC */
			
			if (op_prg_odb_ltrace_active ("trace_tc"))
				{
				printf ("Deleting elements with smaller Seq Num: Old Seq No:%d, TC Seq No:%d\n",
					tc_set_entry_ptr->tc_seq_num ,rcvd_seq_num);
				}
	

			olsr_rte_graph_tc_delete (tc_set_entry_ptr);

			/* Remove this entry from common TC Table */
			tmp_var = oms_topo_table_item_remove (topo_table_hndl, 
				originator_addr, tc_set_entry_ptr->tc_seq_num);
			
			/* Reset the state_ptr */
			tc_set_entry_ptr->state_ptr = OPC_NIL;
			
			/* Set topology set changed */
			topology_changed = OPC_TRUE;
			}
	
		/* Get the list of MPR addresses listed in the TC message */
		num_mpr_selectors = prg_vector_size (tc_msg_ptr->mpr_nodes_vptr);
		
		/* There are no addresses in this TC message 		*/
		/* Since we found the entry for this originator		*/
		/* address, the entry for old seq number must have	*/
		/* been removed from  common TC Table. Remove the 	*/
		/* hash table entry from local tc set table also 	*/
		if (num_mpr_selectors == 0)
			{
			if (op_prg_odb_ltrace_active ("trace_tc"))
				{
				printf ("No MPR selector addresses in this TC mesg, deleting the hash table entry\n");
				}
			/* Delete the hash table entry */
			prg_bin_hash_table_item_remove (tc_set_table, &originator_addr);
			op_prg_mem_free (tc_set_entry_ptr);
			}
		else
			{
			/* There are addresses in TC message */
			/* Check if seq numbers are same for this existing entry */
			if (tc_set_entry_ptr->tc_seq_num == rcvd_seq_num)
				{
				if (op_prg_odb_ltrace_active ("trace_tc"))
					{
					printf ("Found last_addr/seq_num entry. Updating expiry time\n");
					}
				/* Entry for last addr and dest addr exists */
				/* Update the expiry time */
				tc_set_entry_ptr->tc_time = op_sim_time () + vtime;
				entry_found = OPC_TRUE;
				}
			
			if (entry_found == OPC_FALSE)
				{
				/* No entry for this seq num though entry for orig addr exists 	*/
				/* Add it to this entry 										*/
				tc_set_entry_ptr->last_addr = originator_addr;
				tc_set_entry_ptr->tc_seq_num = rcvd_seq_num;
				tc_set_entry_ptr->tc_time = op_sim_time () + vtime;
				
				/* Check if such entry exists in common TC table */
				if (oms_topo_table_item_exists (topo_table_hndl, originator_addr, rcvd_seq_num, &state_ptr) == OPC_TRUE)
					{
					/* An entry to TC exists in common TC table 		*/
					/* State_ptr will have the refrence to that entry 	*/
					tc_set_entry_ptr->state_ptr = (OlsrT_TC_Dest_Addr_Entry*) state_ptr;
					oms_topo_table_item_insert (topo_table_hndl, originator_addr, rcvd_seq_num, state_ptr);
					}
				else
					{
					if (op_prg_odb_ltrace_active ("trace_tc"))
						{
						printf ("Creating new entry in OLSR TC Table \n");
						}
					
					/* Create a  new entry  */
					dest_addr_entry_ptr = olsr_rte_tc_dest_addr_entry_create (tc_msg_ptr->mpr_nodes_vptr, rcvd_seq_num);
					
					/* Add it to the common TC table */
					oms_topo_table_item_insert (topo_table_hndl, originator_addr, rcvd_seq_num, (void*) dest_addr_entry_ptr);
					
					tc_set_entry_ptr->state_ptr = dest_addr_entry_ptr;
					}
		
				olsr_rte_graph_tc_add (tc_set_entry_ptr);
		
				/* Set topology set changed */
				topology_changed = OPC_TRUE;
				
				} /* end if entry_found == OPC_FALSE*/
			
			}/*end else-if num_mpr_selectors == 0*/
				
		}/*end if*/
	else
		{
		/* Entry for this orginiator addr = last_addr 	*/
		/* is not found in TC table 					*/
		/* Before creating an entry first check if 		*/
		/* there are any addresses in the TC mesg 		*/
		/* If not, do not create a new TC entry 		*/
		num_mpr_selectors = prg_vector_size (tc_msg_ptr->mpr_nodes_vptr);
		
		if (num_mpr_selectors != 0)
			{
			/* MPR addresses are there in this TC msg */
			
			/* Create a new entry in TC table */
			tc_set_entry_ptr = (OlsrT_TC_Set_Entry*) op_prg_pmo_alloc (tc_set_pmh);
			
			/* New information */
			/* Add it to this entry */
			tc_set_entry_ptr->last_addr = originator_addr;
			tc_set_entry_ptr->tc_seq_num = rcvd_seq_num;
			tc_set_entry_ptr->tc_time = op_sim_time () + vtime;
		
			if (op_prg_odb_ltrace_active ("trace_tc"))
				{
				inet_tmp_addr = inet_rtab_index_to_addr_convert (originator_addr);
				inet_address_print (tmp_str, inet_tmp_addr);
				inet_address_to_hname (inet_tmp_addr, node_name);
				printf ("Creating New TC Entry: last_addr: %s (%s) and seq no:%d \n", tmp_str, node_name, rcvd_seq_num);
				}
		
			/* Check if such entry exists in common TC table */
			if (oms_topo_table_item_exists (topo_table_hndl, originator_addr, rcvd_seq_num, &state_ptr) == OPC_TRUE)
				{
				if (op_prg_odb_ltrace_active ("trace_tc"))
					{
					printf ("Entry already exists in OLSR TC Table\n");
					}
					
				/* State_ptr will have the refrence to that entry */
				tc_set_entry_ptr->state_ptr = (OlsrT_TC_Dest_Addr_Entry*) state_ptr;
				oms_topo_table_item_insert (topo_table_hndl, originator_addr, rcvd_seq_num, state_ptr);
				
				}
			else
				{
				if (op_prg_odb_ltrace_active ("trace_tc"))
					{
					printf ("Creating a new entry in OLSR TC Table\n");
					}
				
				/* Create a  new one */
				dest_addr_entry_ptr = olsr_rte_tc_dest_addr_entry_create (tc_msg_ptr->mpr_nodes_vptr, rcvd_seq_num);
					
				/* Add it to the common TC table */
				oms_topo_table_item_insert (topo_table_hndl, originator_addr, rcvd_seq_num, (void*) dest_addr_entry_ptr);
					
				tc_set_entry_ptr->state_ptr = dest_addr_entry_ptr;
				}
			
			prg_bin_hash_table_item_insert (tc_set_table, &originator_addr, tc_set_entry_ptr, &old_contents_ptr);
			
			/* Set topology set changed */
			topology_changed = OPC_TRUE;

			olsr_rte_graph_tc_add (tc_set_entry_ptr);
			}
		}
	
	/* Return false, so that this TC msg */
	/* can be considered for forwarding  */
	FRET (OPC_FALSE);
	
	}
		
		

/*************************************/
/***** Packet Send Functions *********/
/*************************************/

static void
olsr_rte_send_hello (Boolean send_jittered)
	{
	OlsrT_Hello_Message*		hello_msg_ptr;
	int							olsr_hello_message_size;
	int							originator_addr;
	OlsrT_Message*				olsr_msg_ptr;
	Packet*						olsr_pkptr;
		
	FIN (olsr_rte_send_hello (Boolean));

	/* Lazy Method: Remove expired link set entries */
	olsr_rte_expired_link_set_entries_remove ();
	
	/* Allocate memory for OlsrT_Hello_Message */
	hello_msg_ptr = (OlsrT_Hello_Message*) op_prg_pmo_alloc (hello_msg_pmh);
	
	/* Model the size of the fields in OlsrT_Hello_Message 	 */
	/* Reserved (16) + Htime (8) + Willingness (8) = 32	bits */
	olsr_hello_message_size = 32;
	
	originator_addr = own_main_address;

	/* see if link set is not empty */
	if (link_set_chain_head_ptr)
		{
		PrgT_Vector*				nbr_intf_addr_vptr;
		PrgT_Vector*				hello_msg_vptr;
		OlsrT_Link_Set_Entry*		link_set_entry_ptr;
		int							prev_linkcode = 15; /* not possible */
		
		/* Since the set is not zero, we will send a hello message 	*/
		/* with atleast one set linkcode with corresponding list 	*/
		/* of intf address list										*/
		
		/* Create vector (list) to hold all hello msg data (i.e. list of linkcodes) */
		hello_msg_vptr = prg_vector_create (1, OPC_NIL, OPC_NIL);
		
		/* Model the size of the fields in OlsrT_Hello_Message_Data	  */
		/* Linkcode (8) + Reserved (8) + Link Msg Size (16) = 32 bits */
		olsr_hello_message_size += 32;

		/* Go through each entry of the link set	*/
		/* and add the interface info in hello msg 	*/
		for (link_set_entry_ptr = link_set_chain_head_ptr;
			 link_set_entry_ptr;
			 link_set_entry_ptr = link_set_entry_ptr->next_link_set_entry_ptr)
			{
			int 						link_type, neighbor_type;
			int							nbr_intf_addr;
			int							neighbor_main_addr;
			int							linkcode;
			OlsrT_Hello_Message_Data*	prev_hello_msg_data_ptr;
			
			if (link_set_entry_ptr->SYM_time > op_sim_time())
				{
				/* This link is still symmetric */
				link_type = OLSRC_SYMMETRIC_LINK;
				}
			else if (link_set_entry_ptr->ASYM_time > op_sim_time())
				{
				/* Link type is assymetric */
				link_type = OLSRC_ASYMMETRIC_LINK;
				}
			else if ((link_set_entry_ptr->ASYM_time < op_sim_time()) &&
					(link_set_entry_ptr->SYM_time < op_sim_time()))
				{
				/* This link is lost */
				link_type = OLSRC_LOST_LINK;
				}
		
			/* Check this same neighbor's status in neighbor table 	*/
			/* Note: intf address are used only for link sensing	*/
			/* For all other tables, main address of node is used	*/
			
			/* Get the nbr_intf_address */
			nbr_intf_addr = link_set_entry_ptr->key.nbr_intf_addr;
			
			/* Get the neighbor main addr for this interface addr, use MID table	*/
			neighbor_main_addr = olsr_support_main_addr_get (nbr_intf_addr);
			
			/* If this neighbor is found in MPR set */
			if (olsr_rte_mpr_set_entry_get (neighbor_main_addr) != OPC_NIL)
				{
				neighbor_type = OLSRC_MPR_NEIGHBOR;
				}
			else
				{
				/* Find the entry in neigbor table */
				OlsrT_Neighbor_Set_Entry* neighbor_set_entry_ptr =
					olsr_rte_neighbor_set_entry_get (neighbor_main_addr);
		
				if (neighbor_set_entry_ptr->status == OLSRC_SYM_STATUS)
					{
					neighbor_type = OLSRC_SYM_NEIGHBOR;
					}
				else if (neighbor_set_entry_ptr->status == OLSRC_NOT_SYM_STATUS)
					{
					neighbor_type = OLSRC_NOT_NEIGHBOR;
					}
				}
		
			/* Left shift link_type 2 bit places */
			link_type <<=2;
		
			/* OR it with neighbor_type to obtain linkcode */
			/* Set linkcode in hello_message_data */
			linkcode = link_type | neighbor_type;
			
			prev_hello_msg_data_ptr = olsr_rte_hello_msg_data_linkcode_exists 
														(hello_msg_vptr, linkcode);
			
			if ((linkcode != prev_linkcode) && (prev_hello_msg_data_ptr == OPC_NIL))
				{
				int*						nbr_intf_addr_ptr;
				
				/* Different linkcode than previous one & no such linkcode created previously */
	
				/* Create a new hello msg data for this */
				/* new linkcode type list of interfaces */
				OlsrT_Hello_Message_Data	*hello_msg_data_ptr =
					(OlsrT_Hello_Message_Data*) op_prg_pmo_alloc (hello_data_pmh);
				
				/* Model the size of the fields in OlsrT_Hello_Message_Data	  */
				/* Linkcode (8) + Reserved (8) + Link Msg Size (16) = 32 bits */
				olsr_hello_message_size += 32; 
							
				nbr_intf_addr_vptr = prg_vector_create (1, OPC_NIL, OPC_NIL);
				
				hello_msg_data_ptr->nbr_intf_addr_vptr = nbr_intf_addr_vptr;
			
				/* Create inet address pointer to insert in the vector */
				nbr_intf_addr_ptr = olsr_rte_address_create (nbr_intf_addr);
				
				/* Add to the exisiting intf vector (list) */
				prg_vector_insert (hello_msg_data_ptr->nbr_intf_addr_vptr, 
					nbr_intf_addr_ptr, PRGC_VECTOR_INSERT_POS_TAIL);
				
				/* Model the size of the Interface Address inserted in Hello Msg */
				/* Neighbor Interface Address (32) = 32	bits for IPv4 OLSR.	     */
				/* Neighbor Interface Address (128) = 128 bits for IPv6 OLSR	 */
				olsr_hello_message_size += (is_ipv6_enabled? 128:32); 
				
				hello_msg_data_ptr->linkcode = linkcode;
				hello_msg_data_ptr->reserved = 8;
				hello_msg_data_ptr->link_msg_size = prg_vector_size (nbr_intf_addr_vptr);
				
				/* Add this msg data to the vector (list) of 	*/
				/* hello msgs that hello_msg_vptr will hold 	*/
				prg_vector_insert (hello_msg_vptr, hello_msg_data_ptr, PRGC_VECTOR_INSERT_POS_TAIL);
				}
			else
				{
				int*						nbr_intf_addr_ptr;
				
				/* Either of conditions is true:				*/
				/* (1) same linkcode as previous one OR			*/
				/* (2) similar linkcode was created earlier 	*/
				/* Either of the case, we will have 		 	*/
				/* hello_msg_data_ptr != OPC_NIL				*/
									
				/* Add this nbr_intf_addr in earlier created hello msg data */
				/* hello_msg_data was created earlier for this linkcode 	*/
				OlsrT_Hello_Message_Data	*hello_msg_data_ptr = prev_hello_msg_data_ptr;
			
				/* Create inet address pointer to insert in the vector */
				nbr_intf_addr_ptr = olsr_rte_address_create (nbr_intf_addr);
		
				/* Add to the exisiting intf vector (list) */
				prg_vector_insert (hello_msg_data_ptr->nbr_intf_addr_vptr, 
					nbr_intf_addr_ptr, PRGC_VECTOR_INSERT_POS_TAIL);
			
				hello_msg_data_ptr->link_msg_size = prg_vector_size (nbr_intf_addr_vptr);
			
				/* Model the size of the Interface Address inserted in Hello Msg */
				/* Neighbor Interface Address (32) = 32	bits for IPv4 OLSR.	     */
				/* Neighbor Interface Address (128) = 128 bits for IPv6 OLSR	 */
				olsr_hello_message_size += (is_ipv6_enabled? 128:32); 
				}
	
			prev_linkcode = linkcode;
			} /* end for */

		/* Add the hello message header information and	*/
		/* complete the hello message creation 			*/
		hello_msg_ptr->reserved = 16;
		hello_msg_ptr->htime = OLSRC_HELLO_EMISSION_INTERVAL;
		hello_msg_ptr->willingness = node_willingness;
		hello_msg_ptr->hello_msg_vptr = hello_msg_vptr;

		/* Add this hello message in OLSR Message */
		olsr_msg_ptr = olsr_pkt_support_olsr_message_create (hello_msg_ptr, originator_addr, 
			olsr_hello_message_size , msg_seq_num++, OLSRC_HELLO_MESSAGE, neighbor_hold_time, 1, 0, 
			is_ipv6_enabled);
	
		} /* end if (link table not empty) */

	else
		{
		/* Create an OLSR Message wtih msg_type equal to HELLO_MESSAGE 	*/
		/* and message_size = 0. Currently there is no link or NBR		*/
		/* there will be no actual hello message with interface info 	*/
		/* in this OLSR control packet (hello message)					*/
		
		/* Add the hello message header information and	*/
		/* complete the hello message creation 			*/
		hello_msg_ptr->reserved = 16;
		hello_msg_ptr->htime = OLSRC_HELLO_EMISSION_INTERVAL;
		hello_msg_ptr->willingness = node_willingness;
		hello_msg_ptr->hello_msg_vptr = OPC_NIL; /* No hello msg data */
		
		/* Create OLSR message */
		olsr_msg_ptr = olsr_pkt_support_olsr_message_create (hello_msg_ptr, originator_addr, 
			olsr_hello_message_size, msg_seq_num++, OLSRC_HELLO_MESSAGE, neighbor_hold_time, 1, 0, 
			is_ipv6_enabled);
		}
		
	if (LTRACE_ACTIVE)
		{
		op_prg_odb_print_major (pid_string, "Broadcasting HELLO message \n", OPC_NIL);
		/* To print the Hello Message which is going out, uncomment the following line 	*/
		/* olsr_rte_olsr_message_print ((void*) olsr_msg_ptr, OPC_NIL);					*/
		}

		
	/* Create olsr pkt by adding pkt_len and pkt_seq_num to this olsr_msg */
	olsr_pkptr = olsr_pkt_support_pkt_create (olsr_msg_ptr, pkt_seq_num++);
		
	/* Update the hello traffic sent in bits per second	*/
	op_stat_write (global_stat_handles.hello_traffic_sent_bps_global_handle, op_pk_total_size_get (olsr_pkptr));
	op_stat_write (global_stat_handles.hello_traffic_sent_bps_global_handle, 0.0);
	
	/* Update the number of hello messages sent statistic	*/
	op_stat_write (local_stat_handles.total_hello_sent_shandle, 1.0);
	op_stat_write (global_stat_handles.total_hello_sent_global_shandle, 1.0);
	
	/* Send olsr pkt to udp */
	olsr_rte_pkt_send (olsr_pkptr, (is_ipv6_enabled? InetI_Ipv6_All_Nodes_LL_Mcast_Addr: InetI_Broadcast_v4_Addr), send_jittered);
	
	FOUT;
	}

static OlsrT_Hello_Message_Data*
olsr_rte_hello_msg_data_linkcode_exists (PrgT_Vector* hello_msg_vptr, int linkcode)
	{
	/* This function checks if there already exists a similar linkcode 			*/
	/* If yes, returns the handle to that hello_msg_data else returns OPC_NIL	*/
	
	OlsrT_Hello_Message_Data* 	hello_msg_data_ptr = OPC_NIL;
	int 						hello_msg_data_num, i;
	OlsrT_Hello_Message_Data*	prev_hello_msg_data_ptr = OPC_NIL;
	
	FIN (olsr_rte_hello_msg_data_linkcode_exists (<args>));
	
	hello_msg_data_num = prg_vector_size (hello_msg_vptr);
	
	/* Go through each msg_data from the HEAD */
	for (i= 0; i <= hello_msg_data_num -1; i++)
		{
		hello_msg_data_ptr = (OlsrT_Hello_Message_Data*) prg_vector_access (hello_msg_vptr, i);
		
		if (hello_msg_data_ptr->linkcode == linkcode)
			{
			prev_hello_msg_data_ptr = hello_msg_data_ptr;
			}
		}
	
	FRET (prev_hello_msg_data_ptr);
	}

static void
olsr_rte_send_TC (Boolean send_jittered)
	{
	/* Sends TC message periodically 		*/
	/* TC message is sent by MPR nodes only */
	/* Note: We are not fragmenting TC msg	*/
	/* according to MTU size as suggested	*/
	/* in RFC. Fragmentation will be taken 	*/
	/* care of in IP layer					*/
	
	Packet*							olsr_pkptr = OPC_NIL;
	OlsrT_MPR_Selector_Set_Entry*	mpr_selector_entry_ptr = OPC_NIL;
	OlsrT_Message*					olsr_msg_ptr = OPC_NIL;
	OlsrT_TC_Message*				tc_msg_ptr = OPC_NIL;
	int								i, num_mpr_selectors;
	List*							keys_lptr;
	int*							mprs_addr_ptr;
	int								olsr_tc_message_size = 0;
	
	FIN (olsr_rte_send_TC (Boolean));
	
	/* Remove the expired mpr_selector entries, lazy method */
	olsr_rte_expired_mpr_selector_set_entries_remove ();
	
	/* If MPR selector list is non-empty. I have been selected MPR 	*/
	/* Get all the addresses in the mpr selector set 				*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_selector_set_table);
	num_mpr_selectors = prg_list_size (keys_lptr);
	
	/* Set the stats for mpr_status of this node 	*/
	/* This statistics will periodically check with	*/
	/* each TC transmission, if this node is an 	*/
	/* MPR. If yes, then write 1 else write 0		*/
	if (num_mpr_selectors > 0)
		op_stat_write (local_stat_handles.mpr_status_shandle, 1.0);	
	else
		op_stat_write (local_stat_handles.mpr_status_shandle, 0.0);	
	
	if ((num_mpr_selectors >0) || (send_empty_tc_time > op_sim_time ()))
		{
		/*Create a new TC message */
		tc_msg_ptr = olsr_pkt_support_tc_message_create ();
				
		/* Model the size of OlsrT_TC_Message 	*/
		/* ANSN (16) + Reserved (16) = 32 bits 	*/
		olsr_tc_message_size = 32;
		
		/* Increament ANSN seq num if MPR Selector Set Has been changed */
		if (selectorset_changed == OPC_TRUE)
			{
			ANSN++;
			selectorset_changed = OPC_FALSE;
			}
		
		/* Fill the fields */
		tc_msg_ptr->tc_seq_num = ANSN;
		tc_msg_ptr->reserved = 8;
		
		for (i=0; i<num_mpr_selectors; i++)
			{
			/* Add each mpr selector address in this new TC message */
			mpr_selector_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) prg_list_access (keys_lptr, i);
			mprs_addr_ptr = olsr_rte_address_create (mpr_selector_entry_ptr->mpr_selector_ptr->nbr_addr);
			prg_vector_insert (tc_msg_ptr->mpr_nodes_vptr, mprs_addr_ptr, PRGC_VECTOR_INSERT_POS_TAIL);
			
			/* Model the size of the Interface Address inserted in Hello Msg */
			/* Advertised Neighbor Main Address (32) = 32 bits for Ipv4		 */
			/* Advertised Neighbor Main Address (128) = 128 bits for Ipv6	 */
			olsr_tc_message_size += (is_ipv6_enabled? 128:32);
			}
		
		/* Add this tc message in OLSR Message */
		olsr_msg_ptr = olsr_pkt_support_olsr_message_create (tc_msg_ptr, own_main_address, 
			olsr_tc_message_size , msg_seq_num++, OLSRC_TC_MESSAGE, tc_hold_time, 255, 0, 
			is_ipv6_enabled);
				
		/* Create olsr pkt by adding pkt_len and pkt_seq_num to this olsr_msg */
		olsr_pkptr = olsr_pkt_support_pkt_create (olsr_msg_ptr, pkt_seq_num++);
		
		if (LTRACE_ACTIVE)
			{
			op_prg_odb_print_major (pid_string, "Broadcasting new TC message \n", OPC_NIL);
			/* To print the TC Message which is going out, uncomment the following line 	*/
			//olsr_rte_olsr_message_print ((void*) olsr_msg_ptr, OPC_NIL);	
			}
	
		/* Update the send_empty_tc_time for emptry TC mesg transmission check 	*/
		/* Update send_empty_tc_time only while sending non-empty TCs			*/
		if (num_mpr_selectors >0) 
			{
			send_empty_tc_time = op_sim_time () + tc_hold_time;
			}
		
		/* Update the tc traffic sent in bits per second	*/
		op_stat_write (global_stat_handles.tc_traffic_sent_bps_global_handle, op_pk_total_size_get (olsr_pkptr));
		op_stat_write (global_stat_handles.tc_traffic_sent_bps_global_handle, 0.0);
		
		/* Update the number of TC messages sent statistic	*/
		op_stat_write (local_stat_handles.total_tc_sent_shandle, 1.0);
		op_stat_write (global_stat_handles.total_tc_sent_global_shandle, 1.0);

		/* Send olsr pkt to udp */
		olsr_rte_pkt_send (olsr_pkptr, (is_ipv6_enabled? InetI_Ipv6_All_Nodes_LL_Mcast_Addr: InetI_Broadcast_v4_Addr), send_jittered);
		}
	
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;	
	}


static void
olsr_rte_pkt_send (Packet* pkptr, InetT_Address PRG_ARG_UNUSED (dest_addr), Boolean forward)
	{
	/* Sends the OLSR packet to UDP */
	double 			delay;
	
	FIN (olsr_rte_pkt_send (<args>));

	/* Update the statistics for the routing traffic sent	*/
	olsr_support_routing_traffic_sent_stats_update (&local_stat_handles, &global_stat_handles, pkptr);
	
	/* Set the connection class to class 1.	*/
	op_ici_install (command_ici_ptr);
	
	if (forward)
		{
		/* While forwarding the packet, delay is higher */
		/* to avoid, collisions							*/
		delay = op_dist_uniform (0.5);
		op_pk_send_delayed (pkptr, output_strm, delay);
		}
	else
		{
		/* While transmitting the packet for the first 		*/
		/* time, we have alreayd jittered the periodicity	*/
		/* of packet transmission, so no delay			*/
		op_pk_send_forced (pkptr, output_strm);
		}
	
	/* Uninstall the ICI */
	op_ici_install (OPC_NIL);

	FOUT;
	}

static Boolean
olsr_rte_forward_packet (OlsrT_Message* olsr_message_ptr, int ip_src_addr)
	{
	/* Returns OPC_TRUE if this packet will be forwarded 			*/
	/* Else returns OPC_FALSE and packet should be destroyed 		*/
	/* Checks if the sender of the this message is in the node's	*/
	/* MPR selector list 											*/
	
	int 							nbr_main_addr;
	OlsrT_MPR_Selector_Set_Entry*	mprs_entry_ptr = OPC_NIL;
		
	FIN (olsr_rte_forward_packet (OlsrT_Message*, int));
	
	/* Get the main address of this nbr from which we received this TC */
	nbr_main_addr = olsr_support_main_addr_get (ip_src_addr);
	
	/* Check if this node is in my MPR selector list */
	if ((mprs_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) prg_bin_hash_table_item_get
		(mpr_selector_set_table, &nbr_main_addr)) != OPC_NIL)
		{
		/* Packet is eligible for forwarding */
		
		/* Decreament the ttl */
		olsr_message_ptr->ttl--;
		
		if (olsr_message_ptr->ttl == 0)
			{
			/* TTL has reached zero, do not forward */
			FRET (OPC_FALSE);
			}
		
		/* Increament the hop count */
		olsr_message_ptr->hop_count++;
		
		FRET (OPC_TRUE);
		}
	
	FRET (OPC_FALSE);
	}

	
static void
olsr_rte_neighborhood_topology_check (void)
	{
	/* Checks the status of the static varaibles 		*/
	/* "neighborhood_changed" and "topology_changed" 	*/
	/* and updates the required tables					*/
	InetT_Address		inet_tmp_addr;
	char				tmp_str [256], node_name [256];

	FIN (olsr_rte_neighborhood_topology_check (void));

	if (neighborhood_changed == OPC_TRUE) 
		{
		/* Update the OLSR Performance. Neighborhood Change statistics */
		op_stat_write (global_stat_handles.nbrhood_change_global_shandle, 1.0);
			
		if (LTRACE_ACTIVE)
			{
			inet_tmp_addr = inet_rtab_index_to_addr_convert (own_main_address);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("Neighborhood Changed at node %s (%s) \n", tmp_str, node_name);
			printf ("Re-calculating MPR set and Routing Table \n");
			}
		
		/* Calculate or update MPR set */
		olsr_rte_calculate_mpr_set();
		}
	
	if (topology_changed == OPC_TRUE)
		{
		/* Update the OLSR Performance. Topology Change statistics */
		op_stat_write (global_stat_handles.topology_change_global_shandle, 1.0);
			
		if (LTRACE_ACTIVE)
			{
			inet_tmp_addr = inet_rtab_index_to_addr_convert (own_main_address);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("Topology Changed at node %s (%s) \n", tmp_str, node_name);
			printf ("Re-calculating Routing Table \n");
			}
		} 
	
	if ((neighborhood_changed) || (topology_changed))
		{
		/* Either neighborhood or topology has changed, */
		/* perform rte table calculation.				*/
		
		/* If DJK quantum is greater than 0, align your computation to the nearest instant	*/
		if (djk_processing_time_quantum > 0)
			{
			if (!rte_calc_already_scheduled)
				{
				op_intrpt_schedule_self (ceil (op_sim_time ()/djk_processing_time_quantum) * djk_processing_time_quantum, 
					OLSRC_RTE_CALC_TIMER_EXPIRY);
				rte_calc_already_scheduled = OPC_TRUE;
				}
			}
		/* Otherwise proceed to calculation immediately				*/
		else
			{
			/* Perform route table calc */
			olsr_rte_djk_rte_table_calculate ();
			last_rte_calc_time = op_sim_time ();
			}
				
		/* Reset the flags */
		neighborhood_changed = topology_changed = OPC_FALSE;
		}
		
	FOUT;
	}

/* DUPLICATE SET*/

static void
olsr_rte_duplicate_set_entry_add (int orig_addr, int seq_num, Boolean rexmitted)
	{
	/* Adds an entry in duplicate set table */
	
	OlsrT_Duplicate_Set_Entry*	dup_entry_ptr;
	OlsrT_Dup_Set_Key			key;
	
	FIN (olsr_rte_duplicate_set_entry_add (int, int));
	
	key.originator_addr = orig_addr;
	key.message_seq_num = seq_num;
	
	if ((dup_entry_ptr = (OlsrT_Duplicate_Set_Entry*) 
		prg_bin_hash_table_item_get (duplicate_set_table, &key)) != OPC_NIL)
		{
		/* Entry already exists */		
		/* Cancel the previously scheduled event	*/
		op_ev_cancel_if_pending (dup_entry_ptr->entry_expiry_evhandle);
		
		/* Scehdule a fresh interrupt for expiry */
		dup_entry_ptr->dup_expiry_time = op_sim_time () + dup_hold_time;
		if (rexmitted)
			dup_entry_ptr->rexmitted = rexmitted;

		dup_entry_ptr->entry_expiry_evhandle = op_intrpt_schedule_call (dup_entry_ptr->dup_expiry_time,
			OLSRC_DUP_ENTRY_EXPIRY,	olsr_rte_duplicate_entry_delete, dup_entry_ptr);	
		}
	else
		{
		/* Create a new entry */
		dup_entry_ptr = (OlsrT_Duplicate_Set_Entry*) op_prg_pmo_alloc (dup_entry_pmh);
		dup_entry_ptr->dup_expiry_time = op_sim_time () + dup_hold_time;
		dup_entry_ptr->rexmitted = rexmitted;
		dup_entry_ptr->key = key;
		
		/* Schedule a procedure interrupt to remove the entry when expired */
		dup_entry_ptr->entry_expiry_evhandle = op_intrpt_schedule_call (dup_entry_ptr->dup_expiry_time,
			OLSRC_DUP_ENTRY_EXPIRY,	olsr_rte_duplicate_entry_delete, dup_entry_ptr);	
	
		/* Inset entry in duplicate set table */
		prg_bin_hash_table_item_insert 
			(duplicate_set_table, &key, dup_entry_ptr, OPC_NIL);
		}
	
	FOUT;
	}
	
/*****************************************************/
/********* Timer Expiry Handle Functions *************/
/*****************************************************/

static void
olsr_rte_hello_expiry_handle (void)
	{
	/* Handles hello timer expiry and */
	/* Sends periodic hello messeages */
	
	FIN (olsr_rte_hello_expiry_handle (void));
	
	/* Calls function to create and send hello message */
	olsr_rte_send_hello (OPC_FALSE);
	
	/* Schedule the next check to send hello messages	*/
	/* Jitter Hello msg with op_dist_uniform (0.5)		*/
	if (op_dist_uniform (2.0) > 1.0)
		{
		hello_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + hello_interval -  op_dist_uniform (0.5), OLSRC_HELLO_TIMER_EXPIRY);
		}
	else
		{
		hello_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + hello_interval +  op_dist_uniform (0.5), OLSRC_HELLO_TIMER_EXPIRY);
		}
	
	
	FOUT;
	}

static void
olsr_rte_tc_expiry_handle (void)
	{
	/* Handles TC timer expiry and */
	/* Sends periodic TC messeages */
	
	FIN (olsr_rte_tc_expiry_handle (void));
	
	/* Calls function to create and send hello message */
	olsr_rte_send_TC (OPC_FALSE);
	
	if ((tc_entry_expiry_time + 20.0) < op_sim_time ())
		{
		/* TC table has not been updated in last 20 seconds 	*/
		/* This node may have been isolated, remove expired 	*/
		/* entries and schedule the next check after 50 seconds */
		olsr_rte_expired_tc_entries_remove ();
		tc_entry_expiry_time = op_sim_time () + 10* tc_interval;
		}
	
	/* Schedule the next check to send tc message		*/
	/* Jitter the TC message with op_dist_uniform (0.5)	*/
	if (op_dist_uniform (2.0) > 1.0)
		{
		tc_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + tc_interval -  op_dist_uniform (0.5), OLSRC_TC_TIMER_EXPIRY);
		}
	else
		{
		tc_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + tc_interval +  op_dist_uniform (0.5), OLSRC_TC_TIMER_EXPIRY);
		}
	
		
	FOUT;
	}



	
/********************************************************/
/********* Table Entry Access/Create Functions **********/
/********************************************************/

/******************************/
/*** ROUTE TABLE: FUNCTIONS ***/
/******************************/

static void
olsr_rte_table_calc_expiry_handle (void)
	{
	/* Function to perform route table calculation 	*/
	/* after timer expiry. Timer is started with 	*/
	/* first nbrhood or topology change detection.	*/
	
	FIN (olsr_rte_table_calc_expiry_handle (void));
	
	/* Call calculate route table function */

	olsr_rte_djk_rte_table_calculate ();
	
	rte_calc_already_scheduled = OPC_FALSE;
	last_rte_calc_time = op_sim_time ();
	
	FOUT;
	}


static void
olsr_rte_routing_table_entry_add (int dest_addr,int next_addr, int hops, int local_iface_addr, IpT_Dest_Prefix dest_prefix)
	{
	OlsrT_Routing_Table_Entry*		rt_entry_ptr = OPC_NIL;
	void*							old_contents_ptr;
	
	/* Add an entry to the OLSR routing table. Besides specifying the destination address, */
	/* the next hop to reach the destination address, the number of hops to reach the      */
	/* destination address, the local interface to reach the next hop, the new route table */
	/* entry will also contain the destination prefix that was used to insert the address  */
	/* in the IP Fwd table. The destination prefix needs to be retained by OLSR because    */
	/* OLSR is responsible for destroying this destination prefix, when it withdraws this  */
	/* route from IP Fwd and from OLSR's routing table. The OLSR retains the destination   */
	/* prefix by setting it into the OLSR routing table entry that is added via this func- */
	/* tion. As a corrolary, it follows that for every route OLSR inserts into IP, there   */
	/* should be a corresponding entry in the OLSR routing table, in which entry OLSR keeps*/
	/* the destination prefix OLSR gives to IP. In particular, the OLSR table should hold  */
	/* entries to the directly connected routes that OLSR inserts into IP (after flushing  */
	/* all the IP entries during a calc) - that OLSR keeps directly connected routes in its*/
	/* table is purely an implementation detail, it does not reflect OLSR operational logic*/
	
	
	FIN (olsr_rte_routing_table_entry_add(<args>));
	
	/* Allocate the routing table entry from the pooled memory	*/
	rt_entry_ptr = (OlsrT_Routing_Table_Entry*) op_prg_pmo_alloc (rt_entry_pmh);
	
	rt_entry_ptr->dest_addr = dest_addr;
	rt_entry_ptr->next_addr = next_addr;
	rt_entry_ptr->hops = hops;
	rt_entry_ptr->local_iface_addr = local_iface_addr;
	rt_entry_ptr->dest_prefix = dest_prefix; 
	
	prg_bin_hash_table_item_insert (olsr_routing_table, &dest_addr, rt_entry_ptr, &old_contents_ptr);
	
	FOUT;
	}


/****************************/
/*** MPR SET : FUNCTIONS ****/
/****************************/

static void
olsr_rte_calculate_mpr_set (void)
	{
	/* Calculates MPRs for this node  */
	/* and fills up the MPR set table */
	int 	num_strict_two_hops = 0;
	int				num_keys, i, willingness, degree, reachability;
	OlsrT_Neighbor_Set_Entry*	mpr_candidate_ptr = OPC_NIL;
	OlsrT_Neighbor_Set_Entry*	nbr_set_entry_ptr = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
		
	FIN (olsr_rte_calculate_mpr_set (void));
	
	/* Update the OLSR Performance. MPR Calcs statistics */
	op_stat_write (global_stat_handles.mpr_calcs_global_shandle, 1.0);
				
	/* For each entry in neighbor table, neighbor with */
	/* willingness WILL_ALWAYS should be marked as MPR */
	
	//olsr_rte_mpr_will_always_nbr_add ();
	
	/* Make a list of strict 2 hop neighbors as the two hop	*/
	/* nbr table can have neighbor and 2 hop neighbors		*/
	num_strict_two_hops = olsr_rte_strict_two_hop_set_create ();

	/* Calculate degree D(y) for each entry in nbr set */
	olsr_rte_calculate_degree ();

	/* Find all two hop nbrs that are reachable through */
	/* only one neighbor. Mark that neighbor as MPR 	*/
	olsr_rte_mpr_with_nbr_count_one_add (&num_strict_two_hops);
	
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (neighbor_set_table);
	num_keys = op_prg_list_size (keys_lptr);

	/* For each 2 hop neighbor that can be reached by */
	/* only one neighbor, add that neighbor as mpr	  */
	while (num_strict_two_hops > 0)
		{		
	
		olsr_rte_calculate_reachability (keys_lptr);
		/* Initialize the values for the key attributes */
		willingness = -1;
		reachability = -20;
		degree = -1;
		mpr_candidate_ptr = OPC_NIL;
			
		for (i=0; i< num_keys; i++)
			{
			/* For each entry value in neighbor table	*/
			nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, i);
			
			if ((nbr_set_entry_ptr->status == OLSRC_NOT_SYM_STATUS) ||
				(nbr_set_entry_ptr->willingness == OLSRC_WILL_NEVER) ||
				(nbr_set_entry_ptr->reachability == OLSRC_VALUE_MPR))
				{
				/* Either its not a SYM NBR or the 		*/
				/* willingness is WILL_NEVER 			*/
				/* or its already selected as MPR		*/
				/* Do nothing,go to next neighbor entry	*/
				continue;
				}
			
			if (nbr_set_entry_ptr->willingness > willingness)
				{
				mpr_candidate_ptr = nbr_set_entry_ptr;
				willingness = nbr_set_entry_ptr->willingness;
				reachability = nbr_set_entry_ptr->reachability;
				degree = nbr_set_entry_ptr->degree;
				}
		
			else if ( (nbr_set_entry_ptr->willingness == willingness) &&
						(nbr_set_entry_ptr->reachability > reachability))
				{
				mpr_candidate_ptr = nbr_set_entry_ptr;
				reachability = nbr_set_entry_ptr->reachability;
				degree = nbr_set_entry_ptr->degree;
				}
			else if ((nbr_set_entry_ptr->willingness == willingness) &&
						(nbr_set_entry_ptr->reachability == reachability) &&
						 ((nbr_set_entry_ptr->degree) > degree))
				{
				mpr_candidate_ptr = nbr_set_entry_ptr;
				degree = nbr_set_entry_ptr->degree;
				}
			}
		
		/* We are not performing NIL ptr check 	*/
		/* for mpr_candidate since, we always	*/
		/* expect to find MPR to cover all the	*/
		/* nodes in strict two hop nbr list		*/
		
		/* Mark this best candiate for MPR */
		mpr_candidate_ptr->reachability = OLSRC_VALUE_MPR;
		
		/* Delete all those two hop nodes which are covered by */
		/* this MPR from the strict two hop list */
		olsr_rte_update_strict_two_hop_set (&num_strict_two_hops, mpr_candidate_ptr);
		} /*end while*/
	
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	/* Optimization of MPR set should happen here 	*/
	/* Not required if all the nodes in the network	*/
	/* have same Willingness. MPR calculation 		*/
	/* itself gives optimized set, more optimization*/
	/* will only be helpful when nodes in network	*/
	/* have different level of WILLINGNESS			*/
	/* Extra optimization currently not implemented.*/
	
	olsr_rte_build_new_mpr_set ();
	
	/* Note: Here we are not doing an explicit check */
	/* if the new MPR set is different from the 	*/
	/* previous one. RFC says, if different send 	*/
	/* out an Hello message to notify the changes. 	*/
	/* We will wait for next scheduled hello rather	*/
	/* than explicit hello message. Lazy method. 	*/
			
	FOUT;
	}

static void olsr_rte_calculate_degree (void)
	{
	List*			keys_lptr;
	PrgT_List_Cell	* key_cell_ptr;

	/* This function calculates the degree for all the neighbor nodes */
	/* and populates the degree field of each nbr 	*/
	/* Degree means: Number of strict two hop nbrs 	*/
	/* that this node can reach 					*/
	FIN (olsr_rte_calculate_degree (void));
		
	/* Get all entry values from the neighbor table */
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (neighbor_set_table);
	for (key_cell_ptr = prg_list_head_cell_get (keys_lptr);
		 key_cell_ptr;
		 key_cell_ptr = prg_list_cell_next_get (key_cell_ptr))
		{
		PrgT_List_Cell * list_cell_ptr;
		
		/* For each entry value in neighbor table	*/
		OlsrT_Neighbor_Set_Entry * nbr_set_entry_ptr =
			(OlsrT_Neighbor_Set_Entry*) prg_list_cell_data_get (key_cell_ptr);
		
		/* Clear the degree field */
		nbr_set_entry_ptr->degree  = OLSRC_VALUE_CLEAR;
		nbr_set_entry_ptr->reachability = OLSRC_VALUE_CLEAR;
			
		/* Get the list of 2 hop neighbors */
		for (list_cell_ptr = prg_list_head_cell_get (&nbr_set_entry_ptr->two_hop_nbr_list); list_cell_ptr; 
			list_cell_ptr = prg_list_cell_next_get (list_cell_ptr))
			{
			/* Get neighbor address */
			OlsrT_Two_Hop_Neighbor_Set_Entry * two_hop_addr_ptr =
				(OlsrT_Two_Hop_Neighbor_Set_Entry*) prg_list_cell_data_get (list_cell_ptr);
			
			if (two_hop_addr_ptr->strict_two_hop == OPC_TRUE || (nbr_set_entry_ptr->status != OLSRC_SYM_STATUS))
			    {
			    nbr_set_entry_ptr->degree++;
			    }
			}
		}
	
	/* Destory the list, but do not free the values */
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	
	}

static void
olsr_rte_build_new_mpr_set (void)
	{
	/* First deletes all entries from the MPR set */
	/* then inserts new ones */
	List*								keys_lptr = OPC_NIL;
	List*								m_keys_lptr = OPC_NIL;
	int 								i, num_keys;
/*	Boolean								MPR_SET_CHANGED = OPC_FALSE;*/
	OlsrT_Neighbor_Set_Entry*			nbr_set_entry_ptr = OPC_NIL;
	OlsrT_MPR_Set_Entry*				mpr_entry_ptr = OPC_NIL;
	
	FIN (olsr_rte_build_new_mpr_set (void));
	
	/* Delete all entries from the mpr set */
	m_keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_set_table);
	num_keys = op_prg_list_size (m_keys_lptr);
	
	for (i=0; i< num_keys; i++)
		{
		/* For each entry in mpr set table	*/
		mpr_entry_ptr = (OlsrT_MPR_Set_Entry*) op_prg_list_access (m_keys_lptr, i);
		prg_bin_hash_table_item_remove (mpr_set_table, &(mpr_entry_ptr->mpr_addr));
		op_prg_mem_free (mpr_entry_ptr);
		}
	
	prg_list_destroy (m_keys_lptr, OPC_FALSE);
	
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (neighbor_set_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	for (i=0; i< num_keys; i++)
		{
		/* For each entry in two hop neighbor table	*/
		nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, i);
					
		if (nbr_set_entry_ptr->reachability == OLSRC_VALUE_MPR)
			{
			/* This nbr is selected as MPR, 		*/
			/* Check if it already exists in the	*/
			/* previous list of MPRs. If not add	*/
			/* this nbr in MPR list and mark 		*/
			/* MPR_SET_CHANGED = OPC_TRUE 			*/
			
			/* Add this neighbor to MPR set */
			olsr_rte_mpr_set_entry_create (nbr_set_entry_ptr->nbr_addr);
			
			}
		}
	
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	}


static OlsrT_MPR_Set_Entry*
olsr_rte_mpr_set_entry_create (int mpr_addr)
	{
	OlsrT_MPR_Set_Entry* 		mpr_set_entry_ptr = OPC_NIL;
	void*						old_contents_ptr;
	
	FIN (olsr_rte_mpr_set_entry_create (int));
	
	/* Create a new netry for MPR set	*/
	mpr_set_entry_ptr = (OlsrT_MPR_Set_Entry*) op_prg_pmo_alloc (mpr_set_entry_pmh);
	mpr_set_entry_ptr->mpr_addr = mpr_addr;

	/* Set the mpr set entry for this interface 	*/
	/* address in the mpr set table					*/
	prg_bin_hash_table_item_insert 
		(mpr_set_table, &mpr_addr, mpr_set_entry_ptr, &old_contents_ptr);

	FRET (mpr_set_entry_ptr);
	
	}

static int
olsr_rte_strict_two_hop_set_create (void)
	{
	/* Returns number of strict two hop addresses by 	*/
	/* iterating through the 2 hop nbr table 			*/
	OlsrT_Two_Hop_Neighbor_Set_Entry*	two_hop_nbr_entry_ptr;
	OlsrT_Neighbor_Set_Entry*			nbr_entry_ptr;
	List*								keys_lptr;
	int									i, num_keys;
	int									j, num_one_hop_nbrs;
	OlsrT_Nbr_Addr_Two_Hop_Entry*		two_hop_nbr_addr_ptr = OPC_NIL;
	int num_strict_two_hops = 0;
	
	FIN (olsr_rte_strict_two_hop_set_create (void));
		
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (two_hop_nbr_set_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	/* For each entry in two hop neighbor table	*/
	for (i=0; i< num_keys; i++)
		{
		/* Get the two-hop-nbr entry ptr */
		two_hop_nbr_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, i);
		
		/* Strict two hop nbr set as per section 1.1 */
		/* 2 hop nbr that is    					 */
		/*  - Not node itself 			 			 */
		/*  - Not a SYM nbr 						 */
		/*  - Can be reached through a nbr with      */
		/*    willingness other than WILL_NEVER      */

		/* Checking if 2 hop nbr not the node itself */
		if (two_hop_nbr_entry_ptr->two_hop_addr == own_main_address)
			{
			continue;
			}
		
		/* Initializing nbr ptr */
		nbr_entry_ptr = OPC_NIL;
		
		/* Checking if this 2-hop-nbr is a SYM NBR */
		nbr_entry_ptr = (OlsrT_Neighbor_Set_Entry*) 
			prg_bin_hash_table_item_get (neighbor_set_table, &(two_hop_nbr_entry_ptr->two_hop_addr));
		
		/* Check the SYM-nbr condition */
		if ((nbr_entry_ptr != OPC_NIL) &&
			(nbr_entry_ptr->status == OLSRC_SYM_STATUS))
			{
			/* This 2-hop nbr is my SYM nbr */
			/* Skip this 2-hop nbr			*/
			continue;
			}
		
		/* Checking if this 2 hop nbr is reachable through 	*/
		/* a willing NBR (i.e. a nbr with willingness other	*/
		/* than WILL_NEVER)					*/

		/* Get the number of one-hop-nbr to reach this 2-hop-nbr */
		num_one_hop_nbrs = op_prg_list_size (&two_hop_nbr_entry_ptr->neighbor_list);
		
		for (j=0; j< num_one_hop_nbrs; j++)
			{
			/* For each one hop nbr addr */
			two_hop_nbr_addr_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*)
				op_prg_list_access (&two_hop_nbr_entry_ptr->neighbor_list, j);

			/* Get the neighbor entry ptr */
			nbr_entry_ptr = two_hop_nbr_addr_ptr->nbr_ptr;

			/* Check the willingness of this nbr */
			if (nbr_entry_ptr->willingness != OLSRC_WILL_NEVER)
				{
				/* Marking two hop neighbor as strict two hop */
				two_hop_nbr_entry_ptr->strict_two_hop = OPC_TRUE;
				++num_strict_two_hops;
				break;
				
				}
			}
		
		}/*end for*/
		
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FRET (num_strict_two_hops);
	}

static void
olsr_rte_mpr_with_nbr_count_one_add (int *num_strict_two_hops)
	{
	/* This function goes through each node in strict hop nbr list 	*/
	/* and checks if any of these two hop nodes can be reached only */
	/* by one neighbor. Then that neighbor is marked as an MPR 		*/
	
	OlsrT_Two_Hop_Neighbor_Set_Entry* 	two_hop_nbr_entry_ptr = OPC_NIL;
	OlsrT_Nbr_Addr_Two_Hop_Entry*		two_hop_nbr_addr_ptr = OPC_NIL;
	OlsrT_Neighbor_Set_Entry*			nbr_set_entry_ptr = OPC_NIL;
	List*								keys_lptr = OPC_NIL;
	int									num_keys, i;
	
	FIN (olsr_rte_mpr_with_nbr_count_one_add (<args>));
	
	/* Get all the keys from the strict two hop neighbor list */
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (two_hop_nbr_set_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	for (i=0; i< num_keys; i++)
		{
		/* For each address in strict two hop list 				*/
		/* Find the corresponding entry in 2 hop neighbor table */
		two_hop_nbr_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) prg_list_access (keys_lptr, i);
		if (two_hop_nbr_entry_ptr->strict_two_hop == OPC_FALSE)
		         continue;
		
		if (op_prg_list_size (&two_hop_nbr_entry_ptr->neighbor_list) == 1)
			{
			/* This two hop nbr can only be reached through		*/
			/* one SYM nbr, mark that neighbor as MPR 			*/
			
			/* Get the neighbor address */
			two_hop_nbr_addr_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*)
				op_prg_list_access (&two_hop_nbr_entry_ptr->neighbor_list, 0);
			
			/* Get the neighbor entry */
			nbr_set_entry_ptr = two_hop_nbr_addr_ptr->nbr_ptr;
						
			if ((nbr_set_entry_ptr->status == OLSRC_SYM_STATUS) &&
				(nbr_set_entry_ptr->reachability != OLSRC_VALUE_MPR) &&
				(nbr_set_entry_ptr->willingness != OLSRC_WILL_NEVER) &&
				((*num_strict_two_hops) > 0))
				{
				/* This neighbor is not yet chosen as MPR */
				/* and its willingness in not will_never  */
				/* and there are more than one strict 	  */
				/* 2-hop nbrs to be covered 			  */
				
				/* Mark this neighbor as MPR */
				nbr_set_entry_ptr->reachability = OLSRC_VALUE_MPR;
				
				/* Since this neighbor has become MPR,  */
				/* delete all the two hop addresses 	*/
				/* reachable through it from the strict */
				/* two hop addr list.				  	*/
				olsr_rte_update_strict_two_hop_set (num_strict_two_hops, nbr_set_entry_ptr);
				
				}
			
			if ((*num_strict_two_hops) == 0)
				break;
			
			}/*end if (op_prg_list_size (&two_hop_nbr_entry_ptr->neighbor_list) == 1)*/
		}/*end for*/
	
	prg_list_destroy (keys_lptr, OPC_FALSE);

	FOUT;
	}

static void
olsr_rte_update_strict_two_hop_set (int *num_strict_two_hops, OlsrT_Neighbor_Set_Entry* mpr_candidate_ptr)
	{
	/* This function removes all two hop neighbor addresses  */
	/* which are covered by a given neighbor from the strict */
	/* two hop set											 */
	int							i, two_hop_size;
	OlsrT_Two_Hop_Neighbor_Set_Entry*			two_hop_addr_ptr;
	
	FIN (olsr_rte_update_strict_two_hop_set (<args>));
	
	/* Get the list of two hop neighbors that this mpr can reach */
	two_hop_size = op_prg_list_size (&mpr_candidate_ptr->two_hop_nbr_list);
	
	for (i=0; i < two_hop_size; i++)
		{
		/* For each two hop reachable node, if this address  */
		/* exists in strict two hop set, remove it 			 */
		two_hop_addr_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_access (&mpr_candidate_ptr->two_hop_nbr_list, i);
		
		if (two_hop_addr_ptr->strict_two_hop == OPC_TRUE)
			{
			/* This address is a strict two hop, unmark it */
			two_hop_addr_ptr->strict_two_hop = OPC_FALSE;
			(*num_strict_two_hops)--;
			}
		}
	
	FOUT;
	
	}

static void
olsr_rte_calculate_reachability (PrgT_List * keys_lptr)
	{
	/* Calculates and populates the reachability field in NBR table */
	PrgT_List_Cell * key_cell_ptr;
	
	FIN (olsr_rte_calculate_reachability (List*));
	
	/* Get all entry values from the neighbor table */
	for (key_cell_ptr = prg_list_head_cell_get (keys_lptr);
		 key_cell_ptr;
		 key_cell_ptr = prg_list_cell_next_get (key_cell_ptr))
		{
		/* For each entry value in neighbor table	*/
		OlsrT_Neighbor_Set_Entry * nbr_set_entry_ptr =
			(OlsrT_Neighbor_Set_Entry*) prg_list_cell_data_get (key_cell_ptr);
		
		/* If this neighbor has not been chosen as MPR yet,  */
		/* then only consider it for reahability computation */
		if (nbr_set_entry_ptr->reachability != OLSRC_VALUE_MPR)
			{
			PrgT_List_Cell* list_cell_ptr;
			
			/* Clear the reachability field */
			nbr_set_entry_ptr->reachability  = OLSRC_VALUE_CLEAR;
							
			/* for each 2 hop neighbor address */
			for (list_cell_ptr = prg_list_head_cell_get (&nbr_set_entry_ptr->two_hop_nbr_list); list_cell_ptr; 
				list_cell_ptr = prg_list_cell_next_get (list_cell_ptr))
				{
				/* Get neighbor address */
				OlsrT_Two_Hop_Neighbor_Set_Entry * two_hop_addr_ptr =
					(OlsrT_Two_Hop_Neighbor_Set_Entry*) prg_list_cell_data_get (list_cell_ptr);
						
				/* check if it is there in two_hop_lptr list */
				if (two_hop_addr_ptr->strict_two_hop == OPC_TRUE)
					nbr_set_entry_ptr->reachability++;
			
				}
			}
		}
		
	FOUT;
	}

static OlsrT_MPR_Set_Entry* 
olsr_rte_mpr_set_entry_get (int nbr_addr)
	{
	/* Returns the MPR set entry */
	OlsrT_MPR_Set_Entry* 	mpr_set_entry_ptr = OPC_NIL;
	
	FIN (olsr_rte_mpr_set_entry_get (int));
	
	mpr_set_entry_ptr = (OlsrT_MPR_Set_Entry*) 
		prg_bin_hash_table_item_get (mpr_set_table, &nbr_addr);
	
	FRET (mpr_set_entry_ptr);
	}


/****************************/
/*** LINK SET : FUNCTIONS ***/
/****************************/

static OlsrT_Link_Set_Entry*
olsr_rte_link_set_entry_get (int nbr_addr, int local_intf_addr)
	{
	OlsrT_Link_Set_Key				key;
	OlsrT_Link_Set_Entry*			link_set_entry_ptr;
		
	/** Determines whether an entry exists	**/
	/** in the link set for this intf_addr		**/
	FIN (olsr_rte_link_set_entry_get (<args>));

	key.nbr_intf_addr = nbr_addr;
	key.local_intf_addr = local_intf_addr;
	
	/* Get the entry for this intf_addr	*/
	link_set_entry_ptr = (OlsrT_Link_Set_Entry*) 
		prg_bin_hash_table_item_get (link_set_table, &key);
	
	FRET (link_set_entry_ptr);
	}


static OlsrT_Link_Set_Entry*
olsr_rte_link_set_entry_create (int local_intf_addr, int  nbr_intf_addr, 
								double sym_time, double asym_time, double time)
	{
	/* Adds an entry to the link set */
	OlsrT_Link_Set_Entry* 		link_set_entry_ptr;
	
	FIN (olsr_rte_link_set_entry_create (<args>));

	/* Allocate memory for the link set entry	*/
	link_set_entry_ptr = (OlsrT_Link_Set_Entry*) op_prg_pmo_alloc (link_set_entry_pmh);
	
	link_set_entry_ptr->key.local_intf_addr = local_intf_addr;
	link_set_entry_ptr->key.nbr_intf_addr = nbr_intf_addr;
	link_set_entry_ptr->SYM_time = sym_time;
	link_set_entry_ptr->ASYM_time = asym_time;
	link_set_entry_ptr->expiry_time = time;
	link_set_entry_ptr->nbr_entry_ptr = OPC_NIL;

	/* chain link set entries together */
	link_set_entry_ptr->next_link_set_entry_ptr = link_set_chain_head_ptr;
	link_set_chain_head_ptr = link_set_entry_ptr;
	
	/* Set the link set entry for this local interface 	*/
	/* address in the link set table					*/
	prg_bin_hash_table_item_insert 
		(link_set_table, &link_set_entry_ptr->key, link_set_entry_ptr, OPC_NIL);

	FRET (link_set_entry_ptr);
	}

static void
olsr_rte_symmetric_link_expiry_handle (void* v_link_set_entry_ptr, int code)	
	{
	/* Handles the expiry of a symmetric link */
	OlsrT_Link_Set_Entry* 			link_set_entry_ptr = (OlsrT_Link_Set_Entry*) v_link_set_entry_ptr;
	OlsrT_Neighbor_Set_Entry*		nbr_set_entry_ptr;

	FIN (olsr_rte_symmetric_link_expiry_handle (<args>));
	
	/* This link has become ASYMMETRIC 				*/
	/* Check the number of links to this neighbor	*/
	/* This is considered as neighbor loss if this	*/
	/* was the only link to this neighbor. 	If 		*/
	/* there is another symmetric link to this 		*/
	/* neighbor, do nothing.						*/
		
	/* Make sym_time less than current time */
	link_set_entry_ptr->SYM_time = op_sim_time () -1;
		
	/* Get the corresponding neighbor entry 	*/
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) link_set_entry_ptr->nbr_entry_ptr;
		
	olsr_rte_graph_nbr_delete (nbr_set_entry_ptr);
	
	/* Sanity check, if no sym links, return */
	if (nbr_set_entry_ptr->sym_link_entry_count == 0)
		{
		FOUT;
		}
		
	/* Decreament the link_entry_count 		*/
	/* link_entry_count maintains the no.	*/
	/* of symmetric links for this neighbor */
	nbr_set_entry_ptr->sym_link_entry_count--;
		
	if (nbr_set_entry_ptr->sym_link_entry_count == 0)
		{
		Boolean					send_tc_flag = OPC_FALSE;
		int						j;
		int						nbr_addr;
		OlsrT_MPR_Set_Entry*	mpr_entry_ptr;
		OlsrT_MPR_Selector_Set_Entry*	mpr_selector_entry_ptr;
		
		/* This was the last symmetric link to this nbr	*/
		/* This will be considered as neighbor loss		*/
		/* Remove all two hop tuples through this nbr	*/
		/* Update MPR selector set						*/
		/* Re-calculate MPR set							*/
		/* Send additional Hello message 				*/
			
		/* For this link,get the main address			*/
		nbr_addr = olsr_support_main_addr_get (link_set_entry_ptr->key.nbr_intf_addr);
				
		/* Check how many 2 hop nodes were reachable through this nbr */				
		for (j = op_prg_list_size (&nbr_set_entry_ptr->two_hop_nbr_list)-1;j>=0;j--)
			{
			/* For each two hop address reached through this nbr */
			OlsrT_Two_Hop_Neighbor_Set_Entry *two_hop_addr_ptr =
				(OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_remove (&nbr_set_entry_ptr->two_hop_nbr_list, j);
					
			/* Remove yourself from the nbr list maintained in this two hop addr entry */
			olsr_rte_remove_nbr_from_two_hop_entry (nbr_addr, two_hop_addr_ptr);
			}
				
		/* Do not free the two_hop_nbr_list list of nbr table 	*/
		/* As this nbr can become symmetric again before 		*/
		/* being removed from the nbr table and add two hop nbrs*/
				
		/* If this neighbor was in my MPR set, remove it */
		mpr_entry_ptr = (OlsrT_MPR_Set_Entry*) 
			prg_bin_hash_table_item_remove (mpr_set_table, &nbr_addr);
				
		if (mpr_entry_ptr)
			{
			op_prg_mem_free (mpr_entry_ptr);
			}
				
		/* If this neighbor was in my MPR Selector set, remove it */
		mpr_selector_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) 
			prg_bin_hash_table_item_remove (mpr_selector_set_table, &nbr_addr);
				
		if (mpr_selector_entry_ptr != OPC_NIL)
			{
			if ((op_prg_odb_ltrace_active ("mpr_trace") == OPC_TRUE))
				{
				char			tmp_str [256], node_name [256];
				InetT_Address	inet_tmp_addr = inet_rtab_index_to_addr_convert (nbr_addr);
				inet_address_print (tmp_str, inet_tmp_addr);
				inet_address_to_hname (inet_tmp_addr, node_name);
				printf ("Expired link set entry: Deleting MPR Selector %s (%s) \n", tmp_str, node_name);
				}
									
			mpr_selector_entry_ptr->mpr_selector_ptr->mpr_selector = OPC_FALSE;
			op_prg_mem_free (mpr_selector_entry_ptr);
				
			/* Check if number of mpr selectors have become zero */
			/* If yes, update the global mpr count statistics 	 */
			if (prg_bin_hash_table_num_items_get (mpr_selector_set_table) == 0)
				olsr_rte_update_global_mpr_count (OPC_FALSE);
				
			/* Mark the state variable to reflect that selector set has changed */
			selectorset_changed = OPC_TRUE;
			send_tc_flag = OPC_TRUE;
			}
					
		/* Mark the status of this neighbor as ASYMMETRIC */
		nbr_set_entry_ptr->status = OLSRC_NOT_SYM_STATUS;	
		neighborhood_changed = OPC_TRUE;
			
		/* Update the OLSR Performance. 1-hop Nbr delete statistics */
		op_stat_write (global_stat_handles.nbr_delete_global_shandle, 1.0);
			
		/* If this function was called while processing LOST_LINK 	*/
		/* information in hello, do not perform nbrhood_topology	*/
		/* check. It will be done after processing hello anyway.	*/
		if (code != OLSRC_CONN_LOSS_TIMER)
			olsr_rte_neighborhood_topology_check ();
			
		/* send_jittered = (code == OLSRC_CONN_LOSS_TIMER) ? OPC_TRUE : OPC_FALSE; */
		/* Send an immediate Hello message here to notify this 	*/
		/* neighbor loss. OPTIONAL in RFC, not implemented here	*/
			
		/* olsr_rte_send_hello (send_jittered);					*/
			
		/* Send TC if MPR selector set has changed. Its OPTIONAL */ 
		/* in RFC. Uncomment the code to send TC immediately 	 */
		/* if (send_tc_flag)									 */
		/*	olsr_rte_send_TC (send_jittered);					 */
			
		} /*end if (nbr_set_entry_ptr->link_entry_count == 0)*/
		
	
	FOUT;
	}

static void
olsr_rte_expired_link_set_entries_remove (void)
	{
	/* Lazy method of removing all the expired link set entries */
	/* Note: The link entires which are already asymmetric are	*/
	/* kept for neighbor_hold_time. If they are not refreshed	*/
	/* by then, they are deleted.								*/	
	OlsrT_Link_Set_Entry*			link_set_entry_ptr;
	OlsrT_Link_Set_Entry*			prev_link_set_entry_ptr = OPC_NIL;
	
	FIN (olsr_rte_expired_link_set_entries_remove(void));

	/* scan thru all link set entries */
	link_set_entry_ptr = link_set_chain_head_ptr;
	while (link_set_entry_ptr)
		{
		OlsrT_Link_Set_Entry*			next_entry_ptr = link_set_entry_ptr->next_link_set_entry_ptr;
		
		/* For each entry in link set */
		if (link_set_entry_ptr->expiry_time < op_sim_time ())
			{
			/* This entry is expired 						*/
			/* First, get the corresponding neighbor entry 	*/
			OlsrT_Neighbor_Set_Entry *nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*)
				link_set_entry_ptr->nbr_entry_ptr;
			
			/* Decreament the link_entry_count */
			nbr_set_entry_ptr->link_entry_count--;
			
			if (nbr_set_entry_ptr->link_entry_count == 0)
				{
				/* This was the last link for this neighbor */
				/* Delete the neighbor from neighbor table  */
				/* For this get the main address, find the 	*/
				/* entry in nbr table and remove it 		*/
				int	nbr_addr = olsr_support_main_addr_get (link_set_entry_ptr->key.nbr_intf_addr);

				/* SA: Should we remove the two hop nbr entries from the two hop nbr bin hash? */
				/* Free the two_hop_nbr_list list of nbr table */
				op_prg_list_free (&nbr_set_entry_ptr->two_hop_nbr_list);
			
				/* Remove the nbr set entry from nbr set table */
				prg_bin_hash_table_item_remove (neighbor_set_table, &nbr_addr);
				op_prg_mem_free (nbr_set_entry_ptr);
				
				if (LTRACE_ACTIVE)
					{
					InetT_Address	inet_tmp_addr;
					char			tmp_str [256], node_name [256];
					
					inet_tmp_addr = inet_rtab_index_to_addr_convert (nbr_addr);
					inet_address_print (tmp_str, inet_tmp_addr);
					inet_address_to_hname (inet_tmp_addr, node_name);
					printf ("Expired link set entry: Deleting Neighbor %s (%s) \n", tmp_str, node_name);
					}
								
				} /*end if (nbr_set_entry_ptr->link_entry_count == 0)*/
			
			/* Delete the link set entry */
			prg_bin_hash_table_item_remove (link_set_table, &link_set_entry_ptr->key);
			if (prev_link_set_entry_ptr)
				{
				prev_link_set_entry_ptr->next_link_set_entry_ptr = next_entry_ptr;
				}
			else
				{
				link_set_chain_head_ptr = next_entry_ptr;
				}
			op_prg_mem_free (link_set_entry_ptr);
			}
		else
			{
			prev_link_set_entry_ptr = link_set_entry_ptr;
			}

		/* move on */
		link_set_entry_ptr = next_entry_ptr;
		}
	
	FOUT;
	}

static int
olsr_rte_link_set_sym_time_check (int intf_addr)
	{
	OlsrT_Link_Set_Entry*	link_set_entry_ptr;
	
	/* Checks the SYM_time for this interface in link set		*/
	FIN (olsr_rte_link_set_sym_time_check (int));
		
	/* Scan through the values in link set table and match the nbr_intf_addr */
	for (link_set_entry_ptr = link_set_chain_head_ptr;
		 link_set_entry_ptr;
		 link_set_entry_ptr = link_set_entry_ptr->next_link_set_entry_ptr)
		{
		/* Check if the entry is for this interface */
		if ((link_set_entry_ptr->key.nbr_intf_addr == intf_addr) &&
			(link_set_entry_ptr->SYM_time > op_sim_time ()))
			{
			FRET (OLSRC_SYM_STATUS);
			}
		}
		
	FRET (OLSRC_NOT_SYM_STATUS);
	}


/**********************************/
/** MPR SELECTOR SET : FUNCTIONS **/
/**********************************/

static void
olsr_rte_mpr_selector_set_entry_create (int nbr_addr, int vtime)
	{
	/* If entry already does not exists 	*/
	/* Adds an entry in mpr selector set 	*/
	/* else updates the expiry time			*/
	
	OlsrT_MPR_Selector_Set_Entry*	mpr_selector_set_entry_ptr = OPC_NIL;
	void*							old_contents_ptr;
	int								num_mpr_selectors = 0;
		
	FIN (olsr_rte_mpr_selector_set_entry_create (int, int));

	mpr_selector_set_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) 
		prg_bin_hash_table_item_get (mpr_selector_set_table, &nbr_addr);
	
	if (mpr_selector_set_entry_ptr != OPC_NIL)
		{
		/* Update expiry time for this entry */
		mpr_selector_set_entry_ptr->expiry_time = op_sim_time () + vtime;
		}
	else
		{
		
		/* Create new entry */
		/* Allocate memory for the mpr selector set entry	*/
		mpr_selector_set_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) op_prg_pmo_alloc (mpr_selector_set_entry_pmh);
		mpr_selector_set_entry_ptr->mpr_selector_ptr = (OlsrT_Neighbor_Set_Entry*) prg_bin_hash_table_item_get (neighbor_set_table, &nbr_addr);
		mpr_selector_set_entry_ptr->mpr_selector_ptr->mpr_selector = OPC_TRUE;
		mpr_selector_set_entry_ptr->expiry_time = op_sim_time () + vtime;
		
		num_mpr_selectors = prg_bin_hash_table_num_items_get (mpr_selector_set_table);
	
		if (num_mpr_selectors == 0)
			{
			/* This will be the first mpr selector set for this node */
			/* i.e. this node has just chosen as MPR , update global */
			/* counter for mprs										 */
			olsr_rte_update_global_mpr_count (OPC_TRUE);
			}
		
		/* Set the mpr set entry for this nbr			*/
		/* address in the mpr selector set table		*/
		prg_bin_hash_table_item_insert (mpr_selector_set_table, &nbr_addr,
			mpr_selector_set_entry_ptr, &old_contents_ptr);

		/* Mark the state variable to reflect that selector set has changed */
		selectorset_changed = OPC_TRUE;
		
		}

	FOUT;
	}

static void
olsr_rte_update_global_mpr_count (Boolean increament_mpr_count)
	{
	/* This function updates the global statistics MPR Count, 	*/
	/* which tells the number of MPRs in the network at a 		*/
	/* given time.												*/
	
	static int 			global_mpr_count = 0;
	
	FIN (olsr_rte_update_global_mpr_count (Boolean));
	
	if (increament_mpr_count)
		global_mpr_count++;
	else
		global_mpr_count--;
	
	/* Write to the global statistics */
	op_stat_write (global_stat_handles.mpr_count_global_shandle, global_mpr_count);
	
	FOUT;
	}

static void
olsr_rte_expired_mpr_selector_set_entries_remove (void)
	{
	/* Deletes the expired entries from mpr_selector_set */
	
	OlsrT_MPR_Selector_Set_Entry*		mpr_selector_entry_ptr = OPC_NIL;
	List*								keys_lptr = OPC_NIL;
	int									i, num_keys, remaining_entries;
	char								tmp_str [256], node_name [256];
	InetT_Address						inet_tmp_addr;
	
	FIN (olsr_rte_expired_mpr_selector_set_entries_remove (void));
	
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_selector_set_table);
	num_keys = op_prg_list_size (keys_lptr);
	remaining_entries = num_keys;

	for (i=0; i<num_keys; i++)
		{
		/* For each entry */
		mpr_selector_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) prg_list_access (keys_lptr, i);
		
		if (mpr_selector_entry_ptr->expiry_time < op_sim_time ())
			{
			
			if ((op_prg_odb_ltrace_active ("mpr_trace") == OPC_TRUE))
				{
				inet_tmp_addr = inet_rtab_index_to_addr_convert 
					(mpr_selector_entry_ptr->mpr_selector_ptr->nbr_addr);
				
				inet_address_print (tmp_str, inet_tmp_addr);
				inet_address_to_hname (inet_tmp_addr, node_name);
				printf ("Expired MPR Selector entry %s (%s) \n", tmp_str, node_name);
				}
			
			/* remove this entry */
			prg_bin_hash_table_item_remove (mpr_selector_set_table, &(mpr_selector_entry_ptr->mpr_selector_ptr->nbr_addr));
			mpr_selector_entry_ptr->mpr_selector_ptr->mpr_selector = OPC_FALSE;
			op_prg_mem_free (mpr_selector_entry_ptr);
			
			/* Mark the state variable to reflect that selector set has changed */
			selectorset_changed = OPC_TRUE;
			
			remaining_entries--;
			
			if (remaining_entries == 0)
				{
				/* Node's MPR status has just changed 	*/
				/* This node is not MPR anymore 		*/
				/* Update the global mpr count 			*/
				olsr_rte_update_global_mpr_count (OPC_FALSE);
				}
			}
		}
	
	/* Free keys */
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	
	}
	
/******************************/
/** NEIGHBOR SET : FUNCTIONS **/
/******************************/
/** MS: DJK **/
/** For DJK support, link_set_entry_ptr is added in the function argument **/
static OlsrT_Neighbor_Set_Entry*
olsr_rte_set_neighbor_status (int nbr_addr, OlsrT_Link_Set_Entry* link_set_entry_ptr)
	{
	
	OlsrT_Neighbor_Set_Entry*	nbr_set_entry_ptr = OPC_NIL;
	int 						i;
	PrgT_Bin_Hash_Table*		intf_set_table_ptr = OPC_NIL;
	OlsrT_Interface_Set_Entry*	intf_set_entry_ptr = OPC_NIL;
	int							orig_status, two_hop_addr_num, ret_status;
	OlsrT_Two_Hop_Neighbor_Set_Entry*			two_hop_addr_ptr;
	
	/* Checks all the link entries for this nbr   	*/
	/* and set neighbor status as SYM if any of the */
	/* links is SYM, else sets this nbr as ASYM		*/
	
	FIN (olsr_rte_set_neighbor_status (int));
	
	/* Get the neighbor set entry */
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*)
		prg_bin_hash_table_item_get (neighbor_set_table, &nbr_addr);
	
	/* Get the original status */
	orig_status = nbr_set_entry_ptr->status;
		
	/* Initialize the status */
	nbr_set_entry_ptr->status = OLSRC_NOT_SYM_STATUS;
	
	/* Go through each entry in Interface table */
	/* If the main address matches the neigbor, */
	/* Check the status of that link 			*/
	/* Get the handle for interface set table	*/
	
	intf_set_table_ptr = olsr_support_interface_table_ptr_get();
	
	/* Get the interface table entry for this nbr node */
	intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) 
		prg_bin_hash_table_item_get (intf_set_table_ptr, &nbr_addr);
	
	if (intf_set_entry_ptr->type == 2)
		{
		PrgT_List_Cell * cell_ptr;
		
		/* This nbr has more than one interface */
		for (cell_ptr = prg_list_head_cell_get (intf_set_entry_ptr->intf_addr_lptr);
			 cell_ptr;
			 cell_ptr = prg_list_cell_next_get (cell_ptr))
			{
			/* For each interace of this nbr node	*/
			int * nbr_intf_addr_ptr = (int*) prg_list_cell_data_get (cell_ptr);
			
			ret_status = olsr_rte_link_set_sym_time_check (*nbr_intf_addr_ptr);
			
			if (ret_status == OLSRC_SYM_STATUS)
				{
				nbr_set_entry_ptr->status = OLSRC_SYM_STATUS;
				}
			}
		}
	else
		{
		/* type = 1, there is only one active interface for this node */
		ret_status = olsr_rte_link_set_sym_time_check (nbr_addr);
		nbr_set_entry_ptr->status = ret_status;
		}
			
	/* Now check for Neighbor loss or appearance */
	if ((orig_status == OLSRC_NOT_SYM_STATUS) &&
		(nbr_set_entry_ptr->status == OLSRC_SYM_STATUS))			
		{
		/* It is considered as neighbor appearance */
		neighborhood_changed = OPC_TRUE;

		olsr_rte_graph_nbr_add (nbr_set_entry_ptr, link_set_entry_ptr);
		
		/* Update the OLSR Performance. 1-hop Nbr add statistics */
		op_stat_write (global_stat_handles.nbr_add_global_shandle, 1.0);
		}
		
	/* If the status has changed from SYM to NOT_SYM 	*/
	/* remove nbr address from all two hop nbr entries 	*/
	if ((orig_status == OLSRC_SYM_STATUS) && 
		(nbr_set_entry_ptr->status == OLSRC_NOT_SYM_STATUS ))
		{
		/* It is considered as neighbor loss */
		neighborhood_changed = OPC_TRUE;
				
		olsr_rte_graph_nbr_delete (nbr_set_entry_ptr);

		/* Update the OLSR Performance. 1-hop Nbr delete statistics */
		op_stat_write (global_stat_handles.nbr_delete_global_shandle, 1.0);
				
		two_hop_addr_num = op_prg_list_size (&nbr_set_entry_ptr->two_hop_nbr_list);
			
		for (i= two_hop_addr_num -1; i >=0; i--)
			{
			/* For each two hop address */
			two_hop_addr_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_remove 
				(&nbr_set_entry_ptr->two_hop_nbr_list, i);
				
			/* Remove this nbr address from the two hop entry */
			olsr_rte_remove_nbr_from_two_hop_entry (nbr_addr, two_hop_addr_ptr);
				
			/* We are not going to remove the two_hop_nbr_vptr 	*/
			/* as this nbr may again become SYM and add 		*/
			/* two hop addrs to its two_hop_nbr_vptr			*/
			}
		}
	
	FRET (nbr_set_entry_ptr);
	
	}


static OlsrT_Neighbor_Set_Entry*
olsr_rte_neighbor_set_entry_get (int nbr_addr)
	{
	
	OlsrT_Neighbor_Set_Entry* 	nbr_set_entry_ptr = OPC_NIL;
	
	FIN (olsr_rte_neighbor_set_entry_get (int));
	
	/** Determines whether an entry exists			**/
	/** in the neighbor set for this nbr_addr		**/
	
	/* Create the neighbor address string	*/
			
	/* Get the entry for this intf_addr	*/
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) 
		prg_bin_hash_table_item_get (neighbor_set_table, &nbr_addr);
	
	FRET (nbr_set_entry_ptr);
	}

static OlsrT_Neighbor_Set_Entry*
olsr_rte_neighbor_set_entry_create (int nbr_addr, int status, int will)
	{
	/* Adds an entry to the link set */
	OlsrT_Neighbor_Set_Entry* 		nbr_set_entry_ptr = OPC_NIL;
	void*							old_contents_ptr;
		
	FIN (olsr_rte_neighbor_set_entry_create (<args>));

	/* Create the local interface address string	*/
	
	/* Allocate memory for the link set entry	*/
	nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) op_prg_pmo_alloc (nbr_set_entry_pmh);
	
	nbr_set_entry_ptr->nbr_addr = nbr_addr;
	nbr_set_entry_ptr->status = status;
	nbr_set_entry_ptr->willingness = will;
	nbr_set_entry_ptr->link_entry_count = 0;
	nbr_set_entry_ptr->sym_link_entry_count = 0;
	nbr_set_entry_ptr->reachability = OLSRC_VALUE_CLEAR;
	nbr_set_entry_ptr->mpr_selector = OPC_FALSE;	

	/* Set the link set entry for this local interface 	*/
	/* address in the link set table					*/
	prg_bin_hash_table_item_insert
		(neighbor_set_table, &nbr_addr, nbr_set_entry_ptr, &old_contents_ptr);
	
	FRET (nbr_set_entry_ptr);
	}

static void
olsr_rte_update_neighbor_set_table (int two_hop_addr, OlsrT_Neighbor_Set_Entry* nbr_set_entry_ptr)
	{
	/* This function is called to remove two_hop_addr from the 	*/
	/* nbr table entry of neighbor_addr. It updates the neighbor*/
	/* table by deleting the entry for two hop addr from its 	*/
	/* list of two hop reachable addresses 						*/
	int 						j, two_hop_num;
	OlsrT_Two_Hop_Neighbor_Set_Entry*		two_hop_ptr;
	
	FIN (olsr_rte_update_neighbor_set_table (<args>));
				
	if (nbr_set_entry_ptr != OPC_NIL)
		{
		/* Find my two hop address there */
		two_hop_num = op_prg_list_size (&nbr_set_entry_ptr->two_hop_nbr_list);
		
		for (j=two_hop_num -1; j>=0;j--)
			{
			two_hop_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_access 
				(&nbr_set_entry_ptr->two_hop_nbr_list, j);
			
			if (two_hop_addr == two_hop_ptr->two_hop_addr)
				{
				/* found my two hop address in the list */
				op_prg_list_remove (&nbr_set_entry_ptr->two_hop_nbr_list, j);
				}
			}
		}
	
	FOUT;
	
	}

/**************************************/
/** TWO HOP NEIGHBOR SET : FUNCTIONS **/
/*************************************/

static void
olsr_rte_two_hop_nbr_set_entry_create (int two_hop_addr, int nbr_addr, int vtime)
	{
	/* This function first checks if an entry already exisits,	*/
	/* if yes, then changes the expiry time, else creates a new	*/
	/* entry to the two hop neighbor set 						*/
	
	OlsrT_Two_Hop_Neighbor_Set_Entry* 	two_hop_entry_ptr;
	Boolean								olsrc_created_new = OPC_FALSE;
	OlsrT_Neighbor_Set_Entry*			nbr_set_entry_ptr;
	OlsrT_Nbr_Addr_Two_Hop_Entry*		nbr_addr_entry_ptr;
	
	FIN (olsr_rte_two_hop_nbr_set_entry_create (int, int, int));
		
	/* Get the entry for this nbr_addr	*/
	two_hop_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) 
		prg_bin_hash_table_item_get (two_hop_nbr_set_table, &(two_hop_addr));
		
	/* Neighbor addr and 2 hop address both should match,	*/
	/* else new entry is created 							*/
	if (two_hop_entry_ptr != OPC_NIL)
		{
		/* Find neighbor */
		nbr_addr_entry_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*)
			prg_bin_hash_table_item_get (two_hop_entry_ptr->neighbor_hash_ptr, &nbr_addr);

		if (nbr_addr_entry_ptr)
			{				
			/* This neighbor is already there in the list 	*/
			/* Update the expiry time and RETURN			*/
			op_ev_cancel_if_pending (nbr_addr_entry_ptr->two_hop_expiry_evhandle);
						
			/* Schedule the interrupt for sym link expiry */
			nbr_addr_entry_ptr->two_hop_expiry_evhandle = op_intrpt_schedule_call (op_sim_time () + vtime, 
				OLSRC_TWO_HOP_EXPIRY, olsr_rte_two_hop_expiry_handle, nbr_addr_entry_ptr);
		
			/* return, no two hop neighborhood change */
			FOUT;
			}
		/* If no neighbor entry is found here, one will be created later in this function.					*/
		/* So add a link between the neighbor and the corresponding 2-hop neighbor at this point.			*/	
		else
			olsr_rte_graph_two_hop_nbr_add (nbr_addr, two_hop_entry_ptr);
		}
	else
		{
		/* Create new entry. Allocate memory for the two hop nbr set entry	*/
		two_hop_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_pmo_alloc (two_nbr_set_entry_pmh);
		two_hop_entry_ptr->two_hop_addr = two_hop_addr;
		two_hop_entry_ptr->neighbor_hash_ptr = prg_bin_hash_table_create (4, 4);
		
		olsrc_created_new = OPC_TRUE;
						
		if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
			{
			char				tmp_str [256], node_name [256];
			InetT_Address		inet_tmp_addr;
			
			inet_tmp_addr = inet_rtab_index_to_addr_convert (two_hop_addr);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("2 hop neighbor entry added for %s (%s) \n", tmp_str, node_name);
			}
		
		/* Set the neighbor set entry for this nbr_addr 	*/
		/* address in the neighbor set table				*/
		prg_bin_hash_table_item_insert 
			(two_hop_nbr_set_table, &two_hop_addr, two_hop_entry_ptr, OPC_NIL);

		olsr_rte_graph_two_hop_nbr_add (nbr_addr, two_hop_entry_ptr);
		}
	
	/* Reached here means:					*/
	/* (1) Either two hop nbr entry existed */
	/* and nbr_addr not present in 			*/
	/* neighbor_lptr of two hop entry OR	*/
	/* (2) New entry created				*/
	
	/* Add the neighbor in neighbor_lptr 	*/
	/* with correct expiry time. Also add 	*/
	/* this two hop nbr	to the nbr's two 	*/
	/* hop nbr list in the nbr table entry 	*/

	nbr_set_entry_ptr = olsr_rte_neighbor_set_entry_get (nbr_addr);
	nbr_addr_entry_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*) op_prg_pmo_alloc (nbr_addr_set_entry_pmh);
	
	nbr_addr_entry_ptr->nbr_ptr = nbr_set_entry_ptr;
	nbr_addr_entry_ptr->two_hop_addr = two_hop_addr;

	/* Schedule the expiry interrupt for this two hop entry */
	nbr_addr_entry_ptr->two_hop_expiry_evhandle = op_intrpt_schedule_call (op_sim_time () + vtime,
		OLSRC_TWO_HOP_EXPIRY, olsr_rte_two_hop_expiry_handle, nbr_addr_entry_ptr);
	
	/* Add this Nbr info into neighbor_list of two hop entry */
	op_prg_list_insert (&two_hop_entry_ptr->neighbor_list, nbr_addr_entry_ptr, PRGC_LISTPOS_TAIL);
	/* put a reference to hashed neighbor set too */
	prg_bin_hash_table_item_insert (two_hop_entry_ptr->neighbor_hash_ptr,
		&nbr_addr, nbr_addr_entry_ptr, OPC_NIL);
			
	op_prg_list_insert (&nbr_set_entry_ptr->two_hop_nbr_list, two_hop_entry_ptr, PRGC_LISTPOS_TAIL);
	
	/* Mark change in neighborhood only when this was a strict two hop nbr 	*/
	/* Check if this two hop nbr addr is present in nbr table 				*/
	if ((nbr_set_entry_ptr = olsr_rte_neighbor_set_entry_get (two_hop_addr)) == OPC_NIL)
		{
		neighborhood_changed = OPC_TRUE;
		
		if (olsrc_created_new)
			{
			/* Update the OLSR Performance. Two Hop Nbr Add statistics */
			op_stat_write (global_stat_handles.two_hop_add_global_shandle, 1.0);
			}
		else
			{
			/* Update the OLSR Performance. Two Hop Nbr reachability changed statistics */
			op_stat_write (global_stat_handles.two_hop_reachability_add_global_shandle, 1.0);
			}					
			
		}

	FOUT;
	}

static void
olsr_rte_remove_nbr_from_two_hop_entry (int nbr_addr, OlsrT_Two_Hop_Neighbor_Set_Entry* two_hop_set_entry_ptr)
	{
	/* Removes the nbr_addr from two hop addr entry	*/
	/* Given nbr_addr is either is no more neighbor,*/
	/* or its status has become NOT_SYM.  		 	*/	
	FIN (olsr_rte_remove_nbr_from_two_hop_entry (int, OlsrT_Two_Hop_Neighbor_Set_Entry*));
	
	if (op_prg_odb_ltrace_active ("olsr_crash"))
		{
		InetT_Address		inet_two_hop_addr;
		InetT_Address 		remove_nbr_address;
		char				two_hop_str [256];
		char				remove_nbr_str [256];
		
		inet_two_hop_addr = inet_rtab_index_to_addr_convert (two_hop_set_entry_ptr->two_hop_addr);
		inet_address_print (two_hop_str, inet_two_hop_addr);
		
		remove_nbr_address = inet_rtab_index_to_addr_convert (nbr_addr);
		inet_address_print (remove_nbr_str, remove_nbr_address);
		
		printf ("\nProblematic two hop address: %s, removing neighbor %s", two_hop_str, remove_nbr_str);
		}
	
	/* Get the two hop nbr set entry */
	if (two_hop_set_entry_ptr != OPC_NIL)
		{
		/* Two hop entry exists 	*/
		/* Remove yourself from the	*/
		/* nbr list maintained in 	*/
		/* this two hop entry 		*/
		OlsrT_Nbr_Addr_Two_Hop_Entry* nbr_addr_entry_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry *)
			prg_bin_hash_table_item_remove (two_hop_set_entry_ptr->neighbor_hash_ptr, &nbr_addr);

		if (nbr_addr_entry_ptr)
			{
			PrgT_List_Cell * cell_ptr;
			
			/* remove myself from the list*/

			/* First cancel the pending intrpt */
			op_ev_cancel_if_pending (nbr_addr_entry_ptr->two_hop_expiry_evhandle);

			/* Remove and free this entry from the list */
			/* The nbr ptr inside nbr_addr_entry_ptr will still be valid */
			/* It can still be referenced through the bin_hash neighbor_set_table */
			/* Delete this neighbor entry from two hop addr list */
			for (cell_ptr = prg_list_head_cell_get (&two_hop_set_entry_ptr->neighbor_list);
				 cell_ptr;
				 cell_ptr = prg_list_cell_next_get (cell_ptr))
				{
				if (prg_list_cell_data_get (cell_ptr) == nbr_addr_entry_ptr)
					{
					prg_list_cell_remove (&two_hop_set_entry_ptr->neighbor_list, cell_ptr);
					break;
					}
				}
			op_prg_mem_free (nbr_addr_entry_ptr);

			/* see if neighbor list became empty */
			if (op_prg_list_size (&two_hop_set_entry_ptr->neighbor_list) == 0)
				{
				/* No element in this vector i.e. this 	*/
				/* 2 hop neighbor cannot be reached 	*/
				/* anymore, delete two hop entry 		*/
			
				/* Not adding towards 2-hop nbr delete stats, since  */
				/* 1-hop nbr delete is the reason for nbrhood change */
				prg_bin_hash_table_destroy (two_hop_set_entry_ptr->neighbor_hash_ptr, OPC_NIL);
				prg_bin_hash_table_item_remove (two_hop_nbr_set_table, &(two_hop_set_entry_ptr->two_hop_addr));
				op_prg_mem_free (two_hop_set_entry_ptr);
				}
			}
		}/*end if two_hop_entry != OPC_NIL*/
	
	FOUT;
	}
		
static void
olsr_rte_two_hop_expiry_handle (void* ptr1, int PRG_ARG_UNUSED (code))
	{
	OlsrT_Nbr_Addr_Two_Hop_Entry*		nbr_addr_entry_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry *) ptr1;
	OlsrT_Two_Hop_Neighbor_Set_Entry*	two_hop_entry_ptr;
	PrgT_List_Cell *					cell_ptr;
	
	FIN (olsr_rte_two_hop_expiry_handle (<args>));
		
	/* Get two hop entry */
	two_hop_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) 
		prg_bin_hash_table_item_get (two_hop_nbr_set_table, &(nbr_addr_entry_ptr->two_hop_addr));
		
	/* Expired 2 hop-neighbor relationship 	*/
	/* This 2 hop is not reachable through 	*/
	/* this neigbhor anymore			 	*/	
	olsr_rte_graph_two_hop_nbr_delete (nbr_addr_entry_ptr->nbr_ptr->nbr_addr, two_hop_entry_ptr->two_hop_addr);
				
	/* Remove this two hop from the neighbor table */
	olsr_rte_update_neighbor_set_table 
		(two_hop_entry_ptr->two_hop_addr, nbr_addr_entry_ptr->nbr_ptr);
				
	/* Delete this neighbor entry from two hop addr list & two hop neighbors hash */
	prg_bin_hash_table_item_remove (two_hop_entry_ptr->neighbor_hash_ptr, &nbr_addr_entry_ptr->nbr_ptr->nbr_addr);
	
	for (cell_ptr = prg_list_head_cell_get (&two_hop_entry_ptr->neighbor_list);
		 cell_ptr;
		 cell_ptr = prg_list_cell_next_get (cell_ptr))
		{
		if (prg_list_cell_data_get (cell_ptr) == nbr_addr_entry_ptr)
			{
			prg_list_cell_remove (&two_hop_entry_ptr->neighbor_list, cell_ptr);
			break;
			}
		}
				
	op_prg_mem_free (nbr_addr_entry_ptr);
				
	if (LTRACE_ACTIVE)
		{
		InetT_Address	inet_tmp_addr;
		char			tmp_str [256], node_name [256];
		
		inet_tmp_addr = inet_rtab_index_to_addr_convert (two_hop_entry_ptr->two_hop_addr);
		inet_address_print (tmp_str, inet_tmp_addr);
		inet_address_to_hname (inet_tmp_addr, node_name);
		printf ("One of the NBR expired to reach this TWO-HOP-NBR %s (%s) \n", tmp_str, node_name);
		}
				
	/* If this change was in a strict 2-hop nbr, mark neighborhood change */
	if (prg_bin_hash_table_item_get (neighbor_set_table, &(two_hop_entry_ptr->two_hop_addr)) == OPC_NIL)
		{
		neighborhood_changed = OPC_TRUE;
		
		/* Update the OLSR Performance. Two Hop Nbr reachability changed statistics */
		op_stat_write (global_stat_handles.two_hop_reachability_del_global_shandle, 1.0);
		}
							
	/* Before moving on to next entry, check if this entry is still valid */
	if (op_prg_list_size (&two_hop_entry_ptr->neighbor_list) == 0)
		{
		if (LTRACE_ACTIVE)
			{
			InetT_Address	inet_tmp_addr;
			char			tmp_str [256], node_name [256];
			
			inet_tmp_addr = inet_rtab_index_to_addr_convert (two_hop_entry_ptr->two_hop_addr);
			inet_address_print (tmp_str, inet_tmp_addr);
			inet_address_to_hname (inet_tmp_addr, node_name);
			printf ("Deleting 2 hop neighbor entry for %s (%s) \n", tmp_str, node_name);
			}
			
		/* Not updating 2-hop nbr delete since 1-hop nbr */
		/* loss is the main reason for nbrhood change.	 */
		prg_bin_hash_table_item_remove (two_hop_nbr_set_table, &(two_hop_entry_ptr->two_hop_addr));
		prg_bin_hash_table_destroy (two_hop_entry_ptr->neighbor_hash_ptr, OPC_NIL);
		op_prg_mem_free (two_hop_entry_ptr);
		}
		
	FOUT;	
	}

static void
olsr_rte_two_hop_nbr_set_entry_match_n_delete (int two_hop_addr, int nbr_addr)
	{
	/* Deletes all the entires that have matching nbr_addr */
	OlsrT_Two_Hop_Neighbor_Set_Entry* 	two_hop_entry_ptr;
	
	FIN (olsr_rte_two_hop_nbr_set_entry_match_n_delete( int, int));
		
	/* Get the two hop addr entry */
	two_hop_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) 
		prg_bin_hash_table_item_get (two_hop_nbr_set_table, &two_hop_addr);
	
	/* Neighbor addr and 2 hop address both should match,	*/
	if(two_hop_entry_ptr != OPC_NIL)
		{
		OlsrT_Nbr_Addr_Two_Hop_Entry* addr_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*)
			prg_bin_hash_table_item_remove (two_hop_entry_ptr->neighbor_hash_ptr, &nbr_addr);
		
		if (addr_ptr)
			{
			PrgT_List_Cell *					cell_ptr;
			
			/* This neighbor is there in the list 		*/
			/* delete this entry from the neighbor_lptr */
			olsr_rte_graph_two_hop_nbr_delete (nbr_addr, two_hop_entry_ptr->two_hop_addr);

			/* First cancel the pending intrp */
			op_ev_cancel_if_pending (addr_ptr->two_hop_expiry_evhandle);
		
			/* Remove this entry from the list */
			for (cell_ptr = prg_list_head_cell_get (&two_hop_entry_ptr->neighbor_list);
				 cell_ptr;
				 cell_ptr = prg_list_cell_next_get (cell_ptr))
				{
				if (prg_list_cell_data_get (cell_ptr) == addr_ptr)
					{
					prg_list_cell_remove (&two_hop_entry_ptr->neighbor_list, cell_ptr);
					break;
					}
				}
				
			olsr_rte_update_neighbor_set_table (two_hop_addr, addr_ptr->nbr_ptr);
				
			/* Destroy this neighbor from two hop table entry */
			op_prg_mem_free (addr_ptr);
				
			/* If this change was in a strict 2-hop nbr, mark neighborhood change */
			if (prg_bin_hash_table_item_get (neighbor_set_table, &two_hop_addr) == OPC_NIL)
				{
				neighborhood_changed = OPC_TRUE;
					
				/* Update the OLSR Performance. Two Hop Nbr reachability changed statistics */
				op_stat_write (global_stat_handles.two_hop_reachability_del_global_shandle, 1.0);
				}
			
			if (op_prg_list_size (&two_hop_entry_ptr->neighbor_list) == 0)
				{
				if ((op_prg_odb_ltrace_active ("trace_hello") == OPC_TRUE))
					{
					char			tmp_str [256], node_name [256];
					InetT_Address	inet_tmp_addr;
				
					inet_tmp_addr = inet_rtab_index_to_addr_convert (two_hop_addr);
					inet_address_print (tmp_str, inet_tmp_addr);
					inet_address_to_hname (inet_tmp_addr, node_name);
					printf ("2 hop neighbor entry deleted for %s (%s) \n", tmp_str, node_name);
					}
			
				/* Not updating 2-hop nbr delete since 1-hop nbr */
				/* loss is the main reason for nbrhood change.	 */
			
				prg_bin_hash_table_item_remove (two_hop_nbr_set_table, &two_hop_addr);
				prg_bin_hash_table_destroy (two_hop_entry_ptr->neighbor_hash_ptr, OPC_NIL);
				op_prg_mem_free (two_hop_entry_ptr);			
				}
			}
		}
		
	FOUT;
	}

	
/*********************************/
/******* TC SET : FUNCTIONS ******/
/*********************************/
	
static OlsrT_TC_Dest_Addr_Entry*
olsr_rte_tc_dest_addr_entry_create (PrgT_Vector* mprs_addr_vptr, int rcvd_seq_num)
	{
	/* Creates TC Addr Entry */
	OlsrT_TC_Dest_Addr_Entry*		dest_addr_entry_ptr;
	int								i, num_size;
	int*							dest_addr_ptr;
	int*							mprs_addr_ptr;
		
	FIN (olsr_rte_tc_dest_addr_entry_create (<args>));
		
	/* Allocate the tc entry from the pooled memory	*/
	dest_addr_entry_ptr = (OlsrT_TC_Dest_Addr_Entry*) op_prg_pmo_alloc (dest_entry_pmh);
		
	/* Insert all the addresses from mprs_addr_vptr*/
	num_size = prg_vector_size (mprs_addr_vptr);
	
	for (i=0; i< num_size; i++)
		{
		mprs_addr_ptr = (int*) prg_vector_access (mprs_addr_vptr, i);
		dest_addr_ptr = olsr_rte_address_create (*mprs_addr_ptr);
		op_prg_list_insert (&dest_addr_entry_ptr->dest_addr_list,
			dest_addr_ptr, PRGC_LISTPOS_TAIL);
		}
	
	dest_addr_entry_ptr->tc_seq_num = rcvd_seq_num;
	
	FRET (dest_addr_entry_ptr);
	}


static void 
olsr_rte_tc_dest_addr_entry_destroy (void* entry)
	{
	/* Function to free the memory allocated for TC_Dest_Addr_Entry */
	
	OlsrT_TC_Dest_Addr_Entry*		dest_addr_entry_ptr;
	
	FIN (olsr_rte_tc_dest_addr_entry_destroy (void));
	
	/* Go through each element of the list and destroy the dest_addr */
	dest_addr_entry_ptr = (OlsrT_TC_Dest_Addr_Entry*) entry;
	op_prg_list_free (&dest_addr_entry_ptr->dest_addr_list);
	op_prg_mem_free (dest_addr_entry_ptr);
	
	FOUT;
	}

static void
olsr_rte_expired_tc_entries_remove (void)
	{
	/* Deletes expired entries from TC table */
	/* Each vector in each entry is a sorted List */
	
	OlsrT_TC_Set_Entry*			tc_entry_ptr = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
	int							num_keys, i;
	double						expiry_time;
	Boolean						tmp_var;
	InetT_Address				inet_tmp_addr;
	char						tmp_str [256], node_name [256];
		
	FIN (olsr_rte_expired_tc_entries_remove (void));
	
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (tc_set_table);
	num_keys = op_prg_list_size (keys_lptr);
	
	expiry_time = OLSRC_INFINITE_VALUE;
		
	for (i=0; i<num_keys; i++)
		{
		/* For each entry (last addr) in tc set table */
		tc_entry_ptr = (OlsrT_TC_Set_Entry*) op_prg_list_access (keys_lptr, i);
		
		if (tc_entry_ptr->tc_time < op_sim_time())
			{
			if (LTRACE_ACTIVE)
				{
				inet_tmp_addr = inet_rtab_index_to_addr_convert (tc_entry_ptr->last_addr);
				inet_address_print (tmp_str, inet_tmp_addr);
				inet_address_to_hname (inet_tmp_addr, node_name);
				printf ("Expired TC entry %s (%s) \n", tmp_str, node_name);
				}

			olsr_rte_graph_tc_delete (tc_entry_ptr);

			tmp_var = oms_topo_table_item_remove (topo_table_hndl, 
				tc_entry_ptr->last_addr, tc_entry_ptr->tc_seq_num);

			prg_bin_hash_table_item_remove (tc_set_table, &(tc_entry_ptr->last_addr));
			op_prg_mem_free (tc_entry_ptr);
			
			/* Topology set has changed, set the flag */
			topology_changed = OPC_TRUE;
			}
		else
			{
			/* Update for the minimum expiry time in tc set */
			if (expiry_time > tc_entry_ptr->tc_time)
				{
				expiry_time = tc_entry_ptr->tc_time;
				tc_entry_expiry_time = tc_entry_ptr->tc_time;
				}
			}
			
		}/*end for*/

	/* Free the list memory without destroying values */
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	}


/****************************/
/********** OTHERS **********/
/****************************/

static void
olsr_rte_breakup_linkcode (int linkcode, int* link, int* nbr)
	{
	/* Returns the Link Type and Neighbor Type 	*/
	/* for the given Linkcode value				*/
	
	int 	mask_link=12;
	int 	mask_nbr=3;
		
	FIN (olsr_rte_breakup_linkcode (int, int*, int* ));
	
	*link = linkcode & mask_link;
	*link >>=2;
	*nbr = linkcode & mask_nbr;
	
	FOUT;
	
	}

static int
olsr_rte_find_myself_in_hello_msg (PrgT_Vector* hello_msg_data_vptr, int addr)
	{
	/* Parses through the list of interface addresses	*/
	/* in the hello msg and check if "addr" is listed	*/
	/* in it. If yes, returns the corresponding 		*/
	/* linkcode, else returns OLSRC_ADDR_NOT_FOUND = -99*/
	
	OlsrT_Hello_Message_Data*	hello_msg_data = OPC_NIL;
	PrgT_Vector*				intf_addr_vptr = OPC_NIL;
	int 						i,j,linkcode = OLSRC_ADDR_NOT_FOUND;
	int							intf_addr_num = 0;
	int							hello_msg_data_num = 0;
	int*						intf_addr_ptr;
	
	FIN (olsr_rte_find_myself_in_hello_msg (PrgT_Vector* , int ));
	
	/* Get the number of hello_msg_data */
	hello_msg_data_num = prg_vector_size (hello_msg_data_vptr);
	
	/* For each hello_msg_data */
	for (i=0; i < hello_msg_data_num; i++)
		{
		/* Get hello msg data */
		hello_msg_data = (OlsrT_Hello_Message_Data*) 
			prg_vector_access (hello_msg_data_vptr, i);
		
		/* Get list of interface address in the data */
		intf_addr_vptr = (PrgT_Vector*) hello_msg_data->nbr_intf_addr_vptr;
		
		/* Get the number of interface address */
		intf_addr_num = prg_vector_size (intf_addr_vptr);
		
		/* For each of the interface addresses */
		for (j=0; j < intf_addr_num; j++)
			{
			intf_addr_ptr = (int*) prg_vector_access (intf_addr_vptr, j);
			
			/* If "addr" is found */
			if (addr == *intf_addr_ptr)
				{
				/* Get the linkcode for this hello msg data */
				linkcode = hello_msg_data->linkcode;
				
				FRET (linkcode);
				}
			}
		}
		
	FRET (linkcode);
	}


/************************************************/
/********* Packet Support functions *************/
/************************************************/

static OlsrT_TC_Message*
olsr_pkt_support_tc_message_create (void)
	{
	OlsrT_TC_Message*		tc_msg_ptr;
	
	/** Allocates pooled memory for the tc_msg **/
	FIN (olsr_pkt_support_tc_message_create (void));
		
	/* Allocate the tc message from the pooled memory	*/
	tc_msg_ptr = (OlsrT_TC_Message*) op_prg_pmo_alloc (tc_msg_pmh);
	
	/* Create an empty vector for the mpr selector addresses */
	/* to be carried by this TC message */
	tc_msg_ptr->mpr_nodes_vptr = prg_vector_create (1, OPC_NIL, OPC_NIL);
		
	FRET (tc_msg_ptr);
	}


static Packet*
olsr_pkt_support_pkt_create (OlsrT_Message* olsr_message_ptr, int seq_num)
	{
	Packet*			olsr_pkptr;
	int				packet_length =0;
	
	/** Creates OLSR packet by taking list of 	 **/
	/** OLSR messages and adding olsr pkt header **/
		
	FIN (olsr_pkt_support_pkt_create (<args>));
	
	/* Get the size of the message */
	packet_length = olsr_message_ptr->message_size;
	
	/* Add the size of packet header : OlsrT_Message = 96 bits for  */
	/* IPv4 or 192 bits for IPv6. 									*/
	/* OlsrT_Message: msg_type (8) + Vtime (8) + Msg_size (16) + 	*/
	/* Orig_addr (32 or 128, if IPv4 or IPv6 respectively) +TTL (8) */
	/* + Hop_count (8) + Msg_Seq_Num (16)							*/
	packet_length += (is_ipv6_enabled? 192:96);
	
	/* Create the OLSR packet	*/
	olsr_pkptr = op_pk_create_fmt ("olsr");
	
	/* Set the options in the packet	*/
	op_pk_nfd_set_ptr (olsr_pkptr, "Message", olsr_message_ptr, 
		olsr_rte_olsr_message_copy, olsr_rte_olsr_message_destroy, sizeof (OlsrT_Message));
	
	/* Set the packet length */
	op_pk_nfd_set_int32 (olsr_pkptr, "Packet Length", packet_length);
	op_pk_nfd_set_int32 (olsr_pkptr, "Packet Sequence Number", seq_num);
	
	/* Model the size of this packet */
	op_pk_bulk_size_set (olsr_pkptr, packet_length);
	
	FRET (olsr_pkptr);
	}

static OlsrT_Message*
olsr_pkt_support_olsr_message_create (void* olsr_msg_data_ptr, int originator_addr, 
	int msg_size, int seq_num, int msg_type, double vtime, int ttl, int hop_count, Boolean is_ipv6)
	{
	OlsrT_Message*	olsr_message_ptr;
	/** Creates an OLSR message by taking the OLSR message			 	**/
	/** data (can be Hello, TC, MID) and adding OLSR message header		**/	
	FIN (olsr_pkt_support_olsr_message_create (<args>));
	
	/* Allocate memory for the hello message header	**/
	olsr_message_ptr = (OlsrT_Message*) op_prg_pmo_alloc (olsr_msg_pmh);
		
	olsr_message_ptr->message_type 			= msg_type;
	olsr_message_ptr->message_size 			= msg_size;
	olsr_message_ptr->ttl 					= ttl;
	olsr_message_ptr->hop_count 			= hop_count;
	olsr_message_ptr->originator_addr 		= originator_addr;
	olsr_message_ptr->message_seq_num 		= seq_num;
	olsr_message_ptr->vtime 				= vtime;
	olsr_message_ptr->message 				= olsr_msg_data_ptr;
	olsr_message_ptr->is_ipv6 				= is_ipv6; 
	
	FRET (olsr_message_ptr);	
	}

static void*
olsr_rte_olsr_message_copy (void* message_ptr, size_t PRG_ARG_UNUSED (size))
    {
	OlsrT_Message* 				olsr_message_ptr = (OlsrT_Message*) message_ptr;
	OlsrT_Hello_Message*		hello_message_ptr;
	OlsrT_Message* 				copy_olsr_message_ptr;
	
	/* Makes a copy of the OlsrT Message */
	FIN (olsr_rte_olsr_message_copy (<args>));
 
	hello_message_ptr = (OlsrT_Hello_Message*) olsr_message_ptr->message;
	
	/* Each Message (Hello, MID, TC) has ref_count fields first	*/
	/* which indicated how many other process is referencing	*/
	/* this same packet 										*/
	prg_mt_spinlock_acquire (&hello_message_ptr->ref_count_lock);
	hello_message_ptr->ref_count++;
	prg_mt_spinlock_release (&hello_message_ptr->ref_count_lock);
	
	/* Just make the copy of the OlsrT_Message */
	copy_olsr_message_ptr = olsr_pkt_support_olsr_message_create (olsr_message_ptr->message, 
							olsr_message_ptr->originator_addr, olsr_message_ptr->message_size,
							olsr_message_ptr->message_seq_num, olsr_message_ptr->message_type,
							olsr_message_ptr->vtime, olsr_message_ptr->ttl, olsr_message_ptr->hop_count, 
							olsr_message_ptr->is_ipv6);
	
	FRET ((void*) copy_olsr_message_ptr);
	
	}


static void
olsr_rte_olsr_message_destroy (void* message_ptr)
    {
	
	/* Destroys the Olsr Message depeding upon the message type */
	OlsrT_Message* 				olsr_message_ptr = (OlsrT_Message*) message_ptr;
	OlsrT_Hello_Message*		hello_msg_ptr;
	OlsrT_MID_Message*			mid_msg_ptr;
	PrgT_Vector*				hello_msg_data_vptr;	
	OlsrT_Hello_Message_Data*	hello_msg_data_ptr;
	OlsrT_TC_Message*			tc_msg_ptr;
	int 						hello_msg_data_num;
	PrgT_Vector*				intf_addr_vptr;
	int							intf_addr_num;
	int*						intf_addr_ptr;
	int							i,j;
			
    /*  Deallocate the packet structure memory.     */
    FIN (olsr_rte_olsr_message_destroy (olsr_message_ptr));
		
	/* Each Message (Hello, MID, TC) has ref_count fields first	*/
	hello_msg_ptr = (OlsrT_Hello_Message*) olsr_message_ptr->message;
	prg_mt_spinlock_acquire (&hello_msg_ptr->ref_count_lock);
	if (hello_msg_ptr->ref_count-- > 0)
		{
		op_prg_mem_free (olsr_message_ptr);
		prg_mt_spinlock_release (&hello_msg_ptr->ref_count_lock);
		}
	else
		{
		/* No other process is referencing this memory, so free it */
		
		/* Check what kind of message is there */
		switch (olsr_message_ptr->message_type)
			{			
			case (OLSRC_HELLO_MESSAGE):
				{
				hello_msg_ptr = (OlsrT_Hello_Message*) olsr_message_ptr->message;
				if (hello_msg_ptr != OPC_NIL)
					{
					hello_msg_data_vptr = (PrgT_Vector*) hello_msg_ptr->hello_msg_vptr;
					hello_msg_data_num = prg_vector_size (hello_msg_data_vptr);
					/* Go through each msg_data from the TAIL */
					for (i= hello_msg_data_num -1; i >= 0; i--)
						{
						hello_msg_data_ptr = (OlsrT_Hello_Message_Data*)
							prg_vector_remove (hello_msg_data_vptr, i);
						
						intf_addr_vptr = (PrgT_Vector*) hello_msg_data_ptr->nbr_intf_addr_vptr;
						intf_addr_num = prg_vector_size (intf_addr_vptr);
				
						for (j=intf_addr_num -1; j>=0; j--)
							{
							/* Destroy each address in the intf_addr vector */
							intf_addr_ptr = (int*) prg_vector_remove (intf_addr_vptr, j);
							op_prg_mem_free (intf_addr_ptr);
							}
						
						/* Free this vector and hello_msg_data */
						prg_vector_destroy (intf_addr_vptr, OPC_FALSE); 
						op_prg_mem_free (hello_msg_data_ptr);
						}
				
					/* Free this vector and hello_msg */
					prg_vector_destroy (hello_msg_data_vptr, OPC_FALSE); 
					op_prg_mem_free (hello_msg_ptr);
					break;
					}
				}
			
			case (OLSRC_TC_MESSAGE):
				{
				tc_msg_ptr = (OlsrT_TC_Message*) olsr_message_ptr->message;
				if (tc_msg_ptr != OPC_NIL)
					{
					intf_addr_vptr = (PrgT_Vector*) tc_msg_ptr->mpr_nodes_vptr;
					intf_addr_num = prg_vector_size (intf_addr_vptr);
					/* Go through each intf_addr from the TAIL */
					for (i= intf_addr_num -1; i >= 0; i--)
						{
						/* Destroy each address in the intf_addr vector */
						intf_addr_ptr = (int*) prg_vector_remove (intf_addr_vptr, i);
						op_prg_mem_free (intf_addr_ptr);
						}
					/* Free this vector and TC message */
					prg_vector_destroy (intf_addr_vptr, OPC_FALSE); 
					op_prg_mem_free (tc_msg_ptr);
					break;
					}
				}
				
			case (OLSRC_MID_MESSAGE):
				{
				mid_msg_ptr = (OlsrT_MID_Message*) olsr_message_ptr->message;
				if (mid_msg_ptr != OPC_NIL)
					{
					intf_addr_vptr = (PrgT_Vector*) mid_msg_ptr->intf_addr_vptr;
					intf_addr_num = prg_vector_size (intf_addr_vptr);
					/* Go through each intf_addr from the TAIL */
					for (i= intf_addr_num -1; i >= 0; i--)
						{
						/* Destroy each address in the intf_addr vector */
						intf_addr_ptr = (int*) prg_vector_remove (intf_addr_vptr, i);
						op_prg_mem_free (intf_addr_ptr);
						}
					/* Free this vector and TC message */
					prg_vector_destroy (intf_addr_vptr, OPC_FALSE); 
					op_prg_mem_free (mid_msg_ptr);
					break;
					}
				}
				
			}/*end switch */
			
			/* Free the olsr message */
			op_prg_mem_free (olsr_message_ptr);
		
		} /*end if-else*/
	  
    FOUT;
    }
		

static int*
olsr_rte_address_create (int addr)
    {
    int*                new_addr_ptr;
   
    FIN (olsr_rte_address_create (int));
     
    /* Allocate the duplicate set entry from the pooled memory  */
    new_addr_ptr = (int*) op_prg_pmo_alloc (new_addr_pmh);
	*new_addr_ptr = addr;
   
    FRET (new_addr_ptr);
    }

/************************************************/
/************ SUPPORT FUNCTIONS *****************/
/************************************************/ 

static void
olsr_rte_olsr_message_print (void* message_ptr, Prg_List* PRG_ARG_UNUSED (output_list))
	{
	OlsrT_Message* 				olsr_message_ptr;
	OlsrT_Hello_Message*		hello_msg_ptr = OPC_NIL;
	OlsrT_Hello_Message_Data*	hello_msg_data = OPC_NIL;
	OlsrT_TC_Message*			tc_msg_ptr = OPC_NIL;
	PrgT_Vector*				hello_msg_data_vptr = OPC_NIL;
	PrgT_Vector*				intf_addr_vptr = OPC_NIL;
	int 						i,j,linkcode = -99;
	int							intf_addr_num = 0;
	int							hello_msg_data_num = 0;
	char						addr_str[256];
	int							message_size, message_seq_num, vtime;
	int							ttl, hop_count, htime, willingness;
	int							originator_addr;
	int*						intf_addr_ptr;
	InetT_Address				inet_tmp_addr;
	
	FIN (lsr_rte_olsr_message_print (OlsrT_Message*));
	
	olsr_message_ptr = (OlsrT_Message*) message_ptr;
	
	/* Get information from the olsr message header */
	originator_addr = olsr_message_ptr->originator_addr;
	message_size = olsr_message_ptr->message_size;
	message_seq_num = olsr_message_ptr->message_seq_num;
	vtime = olsr_message_ptr->vtime;
	ttl = olsr_message_ptr->ttl;
	hop_count = olsr_message_ptr->hop_count;
	
	printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	
	inet_tmp_addr = inet_rtab_index_to_addr_convert (originator_addr);
	inet_address_print (addr_str, inet_tmp_addr);
	
	printf("OLSR Message Header Info \n");
	printf("Orig Addr:%s	MsgSize:%d	SeqNum:%d	Vtime:%d	TTL:%d	HopCnt:%d\n\n", 
		addr_str, message_size, message_seq_num, vtime, ttl, hop_count);
				
	switch (olsr_message_ptr->message_type)
		{
		
		case (OLSRC_HELLO_MESSAGE):
			{
		
			/* Get the hello message from olsr_msg */
			hello_msg_ptr = (OlsrT_Hello_Message*) olsr_message_ptr->message;
	
			/* Get information from hello message */
			htime = hello_msg_ptr->htime;
			willingness = hello_msg_ptr->willingness;
	
			printf("Hello Message Header Info \n");
			printf("Htime: %d	Willingness:%d		", htime, willingness);
		
			/* Get the list of hello msg data with different linkcodes */
			hello_msg_data_vptr = (PrgT_Vector*) hello_msg_ptr->hello_msg_vptr;
	

			if (hello_msg_data_vptr == OPC_NIL)
				{
				printf ("Total Msg Data: 0\n");
				}
			else
				{
			
				/* Get the number of hello_msg_data */
				hello_msg_data_num = prg_vector_size (hello_msg_data_vptr);
				printf ("Total Msg Data: %d\n\n", hello_msg_data_num);
	
				/* For each hello_msg_data */
				for (i=0; i < hello_msg_data_num; i++)
					{
					/* Get hello msg data */
					hello_msg_data = (OlsrT_Hello_Message_Data*) prg_vector_access (hello_msg_data_vptr, i);
		
					/* Get the linkcode for this hello msg data */
					linkcode = hello_msg_data->linkcode;
		
					/* Get list of interface address in the data */
					intf_addr_vptr = (PrgT_Vector*) hello_msg_data->nbr_intf_addr_vptr;
		
					/* Get the number of interface address */
					intf_addr_num = prg_vector_size (intf_addr_vptr);
		
					printf ("Hello Msg Data [%d]	LinkCode:%d\n", i, linkcode);
					printf("Intf Addresses: ");
		
					/* For each of the interface addresses */
					for (j=0; j < intf_addr_num; j++)
						{
						intf_addr_ptr = (int*) prg_vector_access (intf_addr_vptr, j);
						inet_tmp_addr = inet_rtab_index_to_addr_convert (*intf_addr_ptr);
						inet_address_print (addr_str, inet_tmp_addr);
					
						printf ("Intf addresses:%s	", addr_str);
						}
					printf("\n\n");
					}
				}
			break;
			}
			
		case (OLSRC_TC_MESSAGE):
			{
			tc_msg_ptr = (OlsrT_TC_Message*) olsr_message_ptr->message;
			
			printf ("TC Message\n\n");
			
			intf_addr_vptr = (PrgT_Vector*) tc_msg_ptr->mpr_nodes_vptr;
			intf_addr_num = prg_vector_size (intf_addr_vptr);
			
			if (intf_addr_num != 0)
				{
				printf ("MPR Addresses: ");
				/* Go through each intf_addr from the TAIL */
				for (i= 0; i < intf_addr_num; i++)
					{
					
					intf_addr_ptr = (int*) prg_vector_access (intf_addr_vptr, i);
					inet_tmp_addr = inet_rtab_index_to_addr_convert (*intf_addr_ptr);
					inet_address_print (addr_str, inet_tmp_addr);
					printf (" %s  ", addr_str);
					}
				printf ("\n\n");
				}
			else
				{
				printf ("\nEmpty TC Message. No MPR addresses \n");
				}
			
			break;
			}
		
		default:
			{
			printf ("\n ERROR: UNKNOWN Message Type\n\n");
			break;
			}
			
		}/*end switch*/
		
	printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
	FOUT;
	
	}

static void
olsr_rte_link_set_print_to_string (void)
	{
	/** Prints the Link Set Table maintained at this node	**/
	FIN (olsr_rte_link_set_print_to_string (void));
	
	if(!link_set_chain_head_ptr)
		{
		printf("=====================================================================\n");
		printf("		Empty Link Set \n");
		printf("=====================================================================\n");
		}
	else
		{
		OlsrT_Link_Set_Entry*		link_set_entry_ptr;
		
		printf("=====================================================================\n");
		printf("-                              LINK SET                              \n");
		printf("=====================================================================\n");
		printf("Nbr Intf Addr	My Intf Addr	SYM_time	ASYM_time	expiry_time	 	\n");
		printf("------------	-------------	----------	-----------	-----------  \n");

		for (link_set_entry_ptr = link_set_chain_head_ptr; link_set_entry_ptr;
			 link_set_entry_ptr = link_set_entry_ptr->next_link_set_entry_ptr)
			{		
			char						local_addr_str [INETC_ADDR_STR_LEN];
			char						nbr_str [INETC_ADDR_STR_LEN];
			
			InetT_Address inet_nbr_intf_addr = inet_rtab_index_to_addr_convert (link_set_entry_ptr->key.nbr_intf_addr);
            InetT_Address inet_local_intf_addr = inet_rtab_index_to_addr_convert (link_set_entry_ptr->key.local_intf_addr);

            inet_address_print (nbr_str, inet_nbr_intf_addr);
            inet_address_print (local_addr_str, inet_local_intf_addr);

			printf ("%s\t%s\t%10.3f\t%10.3f\t%10.3f\n", nbr_str, local_addr_str,
				link_set_entry_ptr->SYM_time, link_set_entry_ptr->ASYM_time, 
				link_set_entry_ptr->expiry_time);
			}
		printf("\n");
		}
		
	FOUT;
	
	}


static void
olsr_rte_neighbor_set_print_to_string (void)
	{
	OlsrT_Neighbor_Set_Entry*	nbr_set_entry_ptr = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
	int							num_routes, count;
	char						status_str[128];
	char						will_str[128];
	char						nbr_str [256];
	InetT_Address				inet_nbr_addr;
		
	/** Prints the Neighbor Set Table maintained at this node	**/
	FIN (olsr_rte_neighbor_set_print_to_string (void));

	/* Get the number of entries in the link set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (neighbor_set_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty Neighbor Set \n");
		printf("=====================================================================\n");
		}
	else
		{
		
		printf("=====================================================================\n");
		printf("-                  NEIGHBOR SET                                      \n");
		printf("=====================================================================\n");
		printf("Neighbor Addr.	  Status	Willingness	\n");
		printf("--------------	------------	------------ \n");
		
		for (count = 0; count < num_routes; count++)
		{
	
		/* For each entry	*/
		nbr_set_entry_ptr = (OlsrT_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, count);
		
		if(nbr_set_entry_ptr == OPC_NIL)
			{
			printf("One Entry but Empty NBR Set \n");
			continue;
			}
		else
			{
			
			if (nbr_set_entry_ptr->status == OLSRC_SYM_STATUS)
				sprintf(status_str, "SYMMETRIC");
			else
				sprintf(status_str, "NOT SYMMETRIC");
			
			if (nbr_set_entry_ptr->willingness == OLSRC_WILL_NEVER)
				sprintf(will_str, "WILL NEVER");
			else if (nbr_set_entry_ptr->willingness == OLSRC_WILL_LOW)
				sprintf(will_str, "WILL LOW");
			else if (nbr_set_entry_ptr->willingness == OLSRC_WILL_DEFAULT)
				sprintf(will_str, "WILL DEFAULT");
			else if (nbr_set_entry_ptr->willingness == OLSRC_WILL_HIGH)
				sprintf(will_str, "WILL HIGH");
			else if (nbr_set_entry_ptr->willingness == OLSRC_WILL_ALWAYS)
				sprintf(will_str, "WILL ALWAYS");
			
			inet_nbr_addr = inet_rtab_index_to_addr_convert (nbr_set_entry_ptr->nbr_addr);
			inet_address_print (nbr_str, inet_nbr_addr);
			printf ("%s\t%s\t%s\t%d\n", nbr_str, status_str, will_str, op_prg_list_size (&nbr_set_entry_ptr->two_hop_nbr_list));
		
			}
		}
		printf("\n");
		
		}/* close else */
	
	/* Free the keys	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
		
	FOUT;
	
	}

static void
olsr_rte_two_hop_neighbor_set_print_to_string (void)
	{
	OlsrT_Two_Hop_Neighbor_Set_Entry*	two_hop_nbr_set_entry_ptr = OPC_NIL;
	List*								keys_lptr = OPC_NIL;
	int									num_routes, count;
	char								nbr_addr_str [INETC_ADDR_STR_LEN];
	OlsrT_Nbr_Addr_Two_Hop_Entry*		nbr_addr_ptr;
	int									nbr_num, i;
	InetT_Address						inet_two_hop_addr, inet_nbr_addr;
	char								two_hop_str [256];
	
	/** Prints the Neighbor Set Table maintained at this node	**/
	FIN (olsr_rte_two_hop_neighbor_set_print_to_string (void));

	/* Get the number of entries in the link set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (two_hop_nbr_set_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty Two Hop Neighbor Set \n");
		printf("=====================================================================\n");
		
		}
	else
		{
		
		printf("=====================================================================\n");
		printf("-              TWO HOP NEIGHBOR SET                                  \n");
		printf("=====================================================================\n");
		printf("Two Hop Addr.	# Pointers	Nbr Addr List	\n");
		printf("--------------	----------	-------------- \n");
			   								
		for (count = 0; count < num_routes; count++)
		{
	
		/* For each entry	*/
		two_hop_nbr_set_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, count);
		
		if(two_hop_nbr_set_entry_ptr == OPC_NIL)
			{
			printf("One Entry but Empty NBR Set \n");
			continue;
			}
		else
			{
			inet_two_hop_addr = inet_rtab_index_to_addr_convert (two_hop_nbr_set_entry_ptr->two_hop_addr);
			inet_address_print (two_hop_str, inet_two_hop_addr);
		
			printf ("%s\t\t%d\t", two_hop_str, op_prg_list_size (&two_hop_nbr_set_entry_ptr->neighbor_list));
			nbr_num = op_prg_list_size (&two_hop_nbr_set_entry_ptr->neighbor_list);
			for (i=0; i< nbr_num; i++)
				{
				nbr_addr_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*) op_prg_list_access 
										(&two_hop_nbr_set_entry_ptr->neighbor_list, i);
				inet_nbr_addr = inet_rtab_index_to_addr_convert (nbr_addr_ptr->nbr_ptr->nbr_addr);
				inet_address_print (nbr_addr_str, inet_nbr_addr);
		
				if (!strcmp (nbr_addr_str, "192.0.1.31"))
					printf ("%s\n\t\t\t\t", nbr_addr_str);
				
				}
			printf ("\n");
			
			}
		}
		printf("\n");
		
		}/* close else */
		
	/* Free the keys	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
		
	FOUT;
	
	}


static void
olsr_rte_tc_set_print_to_string (void)
	{
	OlsrT_TC_Set_Entry*				tc_set_entry_ptr = OPC_NIL;
	OlsrT_TC_Dest_Addr_Entry*		cmn_tc_entry_ptr = OPC_NIL;
	List*							keys_lptr = OPC_NIL;
	int								num_routes, count;
	char							dest_addr_str [INETC_ADDR_STR_LEN];
	int								dest_num, i;
	int*							dest_addr_ptr;
	InetT_Address					inet_addr, inet_dest_addr;
	char							addr_str [256];
	
	/** Prints the Neighbor Set Table maintained at this node	**/
	FIN (olsr_rte_tc_set_print_to_string (void));

	/* Get the number of entries in the link set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (tc_set_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty TOPOLOGY Set \n");
		printf("=====================================================================\n");
		}
	else
		{
		
		printf("=====================================================================\n");
		printf("-              TOPOLOGY SET                                          \n");
		printf("=====================================================================\n");
		printf("Last Addr.	Seq Num		Exp Time	Dest Addr	\n");
		printf("----------	-------		--------	---------\n");
			   								
		for (count = 0; count < num_routes; count++)
		{
	
		/* For each TC entry	*/
		tc_set_entry_ptr = (OlsrT_TC_Set_Entry*) op_prg_list_access (keys_lptr, count);
		
		if(tc_set_entry_ptr == OPC_NIL)
			{
			printf("One Entry but Empty TC Set \n");
			continue;
			}
		else
			{
			inet_addr = inet_rtab_index_to_addr_convert (tc_set_entry_ptr->last_addr);
			inet_address_print (addr_str, inet_addr);
			printf ("%s\t %d\t%12.3f\t\t", addr_str, tc_set_entry_ptr->tc_seq_num, tc_set_entry_ptr->tc_time);
			
			cmn_tc_entry_ptr = (OlsrT_TC_Dest_Addr_Entry*) tc_set_entry_ptr->state_ptr;
			if (cmn_tc_entry_ptr == OPC_NIL)
				{
				printf ("No OLSR TC Table entry, state_ptr is NIL\n");
				FOUT;
				}
				
			dest_num = op_prg_list_size (&cmn_tc_entry_ptr->dest_addr_list);
			
			for (i=0; i< dest_num; i++)
				{
				dest_addr_ptr = (int*) op_prg_list_access (&cmn_tc_entry_ptr->dest_addr_list, i);
				inet_dest_addr = inet_rtab_index_to_addr_convert (*dest_addr_ptr);
				inet_address_print (dest_addr_str, inet_dest_addr);
				
				printf ("%s\n\t\t\t\t\t\t", dest_addr_str);
				
				}
			printf ("\n");
			
			}
		}
		printf("\n");
		
		}/* close else */
	
	/* Free the keys	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	
	}


static void
olsr_rte_mpr_set_print_to_string (void)
	{
	List*					keys_lptr = OPC_NIL;
	OlsrT_MPR_Set_Entry*	mpr_entry_ptr = OPC_NIL;
	int						num_routes, count;
	char					addr_str [INETC_ADDR_STR_LEN];
	InetT_Address			inet_addr;

	/** Prints the MPR Set Table maintained at this node	**/
	FIN (olsr_rte_mpr_set_print_to_string (void));

	/* Get the number of entries in the mpr set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_set_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty MPR Set \n");
		printf("=====================================================================\n");
		}
	else
		{
		printf("=====================================================================\n");
		printf("-             		 MPR SET                                         \n");
		printf("=====================================================================\n");
		
		for (count = 0; count < num_routes; count++)
			{
	
			/* For each entry	*/
			mpr_entry_ptr = (OlsrT_MPR_Set_Entry*) op_prg_list_access (keys_lptr, count);
			inet_addr = inet_rtab_index_to_addr_convert (mpr_entry_ptr->mpr_addr);
			inet_address_print (addr_str, inet_addr);
			printf ("%s\t", addr_str);
		
			}
		
		}/* close else */
	
	printf ("\n\n");
	
	/* Free the keys	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	
	}

static void
olsr_rte_mpr_selector_set_print_to_string (void)
	{
	OlsrT_MPR_Selector_Set_Entry*	mprs_entry_ptr = OPC_NIL;
	List*							keys_lptr = OPC_NIL;
	int								num_routes, count;
	char							addr_str [INETC_ADDR_STR_LEN];
	InetT_Address					inet_addr;

	/** Prints the MPR Set Table maintained at this node	**/
	FIN (olsr_rte_mpr_set_print_to_string (void));

	/* Get the number of entries in the mpr set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_selector_set_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty MPR Selector Set \n");
		printf("=====================================================================\n");
		}
	else
		{
		
		printf("=====================================================================\n");
		printf("-              MPR SELECTOR SET                                      \n");
		printf("=====================================================================\n");
		
		for (count = 0; count < num_routes; count++)
			{
	
			/* For each entry	*/
			mprs_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) op_prg_list_access (keys_lptr, count);
			inet_addr = inet_rtab_index_to_addr_convert (mprs_entry_ptr->mpr_selector_ptr->nbr_addr);
			inet_address_print (addr_str, inet_addr);
			printf ("%s\t%12.3f\n", addr_str, mprs_entry_ptr->expiry_time );
			}
		
		printf("\n");
		
		}/* close else */
	
	/* Free the keys	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
		
	FOUT;
	
	}


static void
olsr_rte_routing_table_print_to_string (void)
	{
	OlsrT_Routing_Table_Entry*	rt_entry_ptr = OPC_NIL;
	List*						keys_lptr = OPC_NIL;
	int							num_routes, count;
	char						local_addr_str [INETC_ADDR_STR_LEN];
	char						dest_addr_str [256];
	char						next_addr_str [256];
	InetT_Address				inet_next_addr;
	InetT_Address				inet_dest_addr;
	InetT_Address				inet_local_intf_addr;

	/** Prints the Routing Table maintained at this node	**/
	FIN (olsr_rte_routing_table_print_to_string (void));

	/* Get the number of entries in the link set	*/
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (olsr_routing_table);
	num_routes = op_prg_list_size (keys_lptr);
	
	if(num_routes == 0)
		{
		printf("=====================================================================\n");
		printf("		Empty Routing Table\n");
		printf("=====================================================================\n");
		}
	else
		{
		
		printf("=======================================================================\n");
		printf("-                     ROUTING TABLE                                    \n");
		printf("=======================================================================\n");
		printf("Dest Addr	Next Addr	Hops	Local Intf	\n");
		printf("----------	-----------	-------	----------- \n");
		
		for (count = 0; count < num_routes; count++)
		{
		
		/* For each entry	*/
		rt_entry_ptr = (OlsrT_Routing_Table_Entry*) op_prg_list_access (keys_lptr, count);
		
		if(rt_entry_ptr == OPC_NIL)
			{
			printf("Route Entry pointer = OPC_NIL \n");
			continue;
			}
		else if (rt_entry_ptr->next_addr == DIRECTLY_CONNECTED)
			{
			/* We should not be printing the directly connected  */
			/* routes. These are artificially inserted in the    */
			/* OLSR table and serve as placeholders for the      */
			/* dest_prefix that OLSR passes to IP. OLSR needs to */
			/* hang onto this dest_prefix, since OLSR is respon- */
			/* sible for destroying it. However, we are hiding   */
			/* this implementation detail from any trace. As far */
			/* as this function that prints the OLSR route table */
			/* is concerned, the OLSR route table contains only  */
			/* OLSR routes, not directly connected routes.       */
			continue; 
			}
		else
			{
			inet_dest_addr = inet_rtab_index_to_addr_convert (rt_entry_ptr->dest_addr);
			inet_next_addr = inet_rtab_index_to_addr_convert (rt_entry_ptr->next_addr);
			inet_local_intf_addr = inet_rtab_index_to_addr_convert (rt_entry_ptr->local_iface_addr);
			inet_address_print (dest_addr_str, inet_dest_addr);
			inet_address_print (next_addr_str, inet_next_addr);
            inet_address_print (local_addr_str, inet_local_intf_addr);

			printf ("%s\t%s\t%d\t%s\n", dest_addr_str, next_addr_str, rt_entry_ptr->hops, local_addr_str);
			}
		}
				
		}/* close else */
	
	printf("\n---------------------------------------------------------------------------------------------\n");
		
	/* Free the keys	*/
	op_prg_mem_free (keys_lptr);
		
	FOUT;
	
	}

static void
olsr_rte_interface_table_print_to_string (void)
	{
	PrgT_Bin_Hash_Table*	intf_set_table_ptr = OPC_NIL;	
	List*					keys_lptr = OPC_NIL;
	List*					intf_lptr;
	int						i, num_entries, num_intf, j;
	int*					intf_addr_ptr;
	InetT_Address			inet_tmp_addr;
	char					intf_addr_str [256], main_addr_str [256];
	OlsrT_Interface_Set_Entry* 	intf_set_entry_ptr;
	
	FIN (olsr_rte_interface_table_print_to_string(void));
	intf_set_table_ptr = olsr_support_interface_table_ptr_get();
	
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (intf_set_table_ptr);
	num_entries = op_prg_list_size (keys_lptr);
	
	printf("===================================================================\n");
	printf("-                       Interface TABLE                            \n");
	printf("===================================================================\n");
	printf("Intf Addr		Main Addr	Type	List of Interfaces\n");
	printf("----------		----------	----	------------------\n");
	
	for (i=0; i< num_entries; i++)
		{
		/* for each entry in MID table */
		intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) op_prg_list_access (keys_lptr, i);
		
		if (intf_set_entry_ptr->type == 2)
			{
			inet_tmp_addr = inet_rtab_index_to_addr_convert (intf_set_entry_ptr->intf_addr);
			inet_address_print (intf_addr_str, inet_tmp_addr);
			inet_tmp_addr = inet_rtab_index_to_addr_convert (intf_set_entry_ptr->main_addr);
			inet_address_print (main_addr_str, inet_tmp_addr);
			printf ("%s\t\t%s\t%d\t", intf_addr_str, main_addr_str, intf_set_entry_ptr->type);
		
			/* This will have list of intf */
			intf_lptr = (List*) intf_set_entry_ptr->intf_addr_lptr;
			num_intf = op_prg_list_size (intf_lptr);
			
			for (j=0; j< num_intf; j++)
				{
				intf_addr_ptr = (int*) op_prg_list_access (intf_lptr, j);
				inet_tmp_addr = inet_rtab_index_to_addr_convert (*intf_addr_ptr);
				inet_address_print (intf_addr_str, inet_tmp_addr);
				printf ("%s   ", intf_addr_str);
				}		
			}
		else
			{
			inet_tmp_addr = inet_rtab_index_to_addr_convert (intf_set_entry_ptr->intf_addr);
			inet_address_print (intf_addr_str, inet_tmp_addr);
			inet_tmp_addr = inet_rtab_index_to_addr_convert (intf_set_entry_ptr->main_addr);
			inet_address_print (main_addr_str, inet_tmp_addr);
		
			printf ("%s\t\t%s\t%d\tNIL", intf_addr_str, main_addr_str, intf_set_entry_ptr->type);
			}
		printf ("\n");
		
		}
	
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	FOUT;
	
	}

	
static void
olsr_rte_clear_table (void* table_ptr, int which_table)
	{
	/* Destroys the contents of the table */
	OlsrT_TC_Set_Entry* 				tc_entry_ptr;
	OlsrT_Neighbor_Set_Entry* 			nbr_entry_ptr;
	OlsrT_Two_Hop_Neighbor_Set_Entry*	two_hop_entry_ptr;
	int									i,j, num_keys, list_size;
	char*								key_ptr;
	List*								keys_lptr;
	PrgT_String_Hash_Table*				str_tbl_ptr;
	PrgT_Bin_Hash_Table*				bin_tbl_ptr;
	OlsrT_Routing_Table_Entry*			rt_entry_ptr;
	OlsrT_MPR_Selector_Set_Entry* 		mpr_selector_entry_ptr;
	OlsrT_MPR_Set_Entry* 				mpr_entry_ptr;
	OlsrT_Nbr_Addr_Two_Hop_Entry* 		two_hop_addr_ptr;
	InetT_Address						inet_next_addr;

	FIN (olsr_rte_clear_table (<args>));
	
	switch (which_table)
		{
		case (1):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing Nbr Table */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				nbr_entry_ptr = (OlsrT_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, i);
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &(nbr_entry_ptr->nbr_addr));
				
				list_size = op_prg_list_size (&nbr_entry_ptr->two_hop_nbr_list);
					
				for (j=0; j< list_size; j++)
					{
					op_prg_list_remove (&nbr_entry_ptr->two_hop_nbr_list, PRGC_LISTPOS_HEAD);
					}
				op_prg_list_free (&nbr_entry_ptr->two_hop_nbr_list);
				op_prg_mem_free (nbr_entry_ptr);
				}
			
			/* Free the keys and the list */
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (2):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing Two Hop Nbr Table */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				two_hop_entry_ptr = (OlsrT_Two_Hop_Neighbor_Set_Entry*) op_prg_list_access (keys_lptr, i);
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &(two_hop_entry_ptr->two_hop_addr));
				
				list_size = op_prg_list_size (&two_hop_entry_ptr->neighbor_list);
				
				for (j=0; j< list_size; j++)
					{
					two_hop_addr_ptr = (OlsrT_Nbr_Addr_Two_Hop_Entry*) 
						op_prg_list_remove (&two_hop_entry_ptr->neighbor_list, PRGC_LISTPOS_HEAD);

					op_ev_cancel_if_pending (two_hop_addr_ptr->two_hop_expiry_evhandle);
					
					/* destroy memory												*/
					op_prg_mem_free (two_hop_addr_ptr);
					}
				prg_bin_hash_table_destroy (two_hop_entry_ptr->neighbor_hash_ptr, OPC_NIL);
				op_prg_mem_free (two_hop_entry_ptr);
				}
			
			/* Free the keys and the list */
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (3):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing TC Table */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				tc_entry_ptr = (OlsrT_TC_Set_Entry*) op_prg_list_access (keys_lptr, i);
				
				oms_topo_table_item_remove (topo_table_hndl, 
					tc_entry_ptr->last_addr, tc_entry_ptr->tc_seq_num);
				
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &(tc_entry_ptr->last_addr));
				op_prg_mem_free (tc_entry_ptr);
				}
			
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (4):
			{
			OlsrT_Link_Set_Entry*			link_set_entry_ptr;
			OlsrT_Link_Set_Entry*			next_link_set_entry_ptr;
			char*							src_dest_addr_str;
			
			/* Clear entries in the link set table			*/
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			//str_tbl_ptr = (PrgT_String_Hash_Table *) table_ptr;
			//keys_lptr = (List*) prg_string_hash_table_keys_get (str_tbl_ptr);
			//num_keys = op_prg_list_size (keys_lptr);

			for (i = 0; i < num_keys; i++)
				{
				/* For each entry in the table	*/
				//key_ptr = (char*) op_prg_list_access (keys_lptr, i);
				//link_set_entry_ptr = (OlsrT_Link_Set_Entry*) prg_string_hash_table_item_remove (str_tbl_ptr, key_ptr);

				link_set_entry_ptr = (OlsrT_Link_Set_Entry*) op_prg_list_access (keys_lptr, i);
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &link_set_entry_ptr->key);
					
				if (op_ev_valid (link_set_entry_ptr->sym_time_expiry_evhandle))
					{
					src_dest_addr_str = (char*) op_ev_state (link_set_entry_ptr->sym_time_expiry_evhandle);
					op_prg_mem_free (src_dest_addr_str);
					op_ev_cancel (link_set_entry_ptr->sym_time_expiry_evhandle);
					}
				
				//op_prg_mem_free (link_set_entry_ptr);
				}
			
			/* Free the keys and the list */
			//op_prg_list_free (keys_lptr);
			//op_prg_mem_free (keys_lptr);
			
			/* Link set entries are also chained together...make sure to remove them 		*/
			link_set_entry_ptr = link_set_chain_head_ptr;
			while (link_set_entry_ptr)
				{
				next_link_set_entry_ptr = link_set_entry_ptr->next_link_set_entry_ptr;
				op_prg_mem_free (link_set_entry_ptr);
				link_set_entry_ptr = next_link_set_entry_ptr;
				link_set_chain_head_ptr = link_set_entry_ptr;
				}
			
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
						
		case (5):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing MPR table		 */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				mpr_entry_ptr = (OlsrT_MPR_Set_Entry*) op_prg_list_access (keys_lptr, i);
				op_prg_mem_free (prg_bin_hash_table_item_remove (bin_tbl_ptr, &(mpr_entry_ptr->mpr_addr)));
				}
			
			/* Free the keys and the list */
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (6):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing MPR Selector table */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				mpr_selector_entry_ptr = (OlsrT_MPR_Selector_Set_Entry*) op_prg_list_access (keys_lptr, i);
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &(mpr_selector_entry_ptr->mpr_selector_ptr->nbr_addr));
				op_prg_mem_free (mpr_selector_entry_ptr);
				}
		
			/* Free the keys and the list */
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (7):
			{
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Clearing OLSR Rte Table */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				rt_entry_ptr = (OlsrT_Routing_Table_Entry*) op_prg_list_access (keys_lptr, i);
				
				/* Remove the corresponding entry from IP forwarding table				*/
		
				/* First get the next hop address 											*/
				inet_next_addr = inet_rtab_index_to_addr_convert (rt_entry_ptr->next_addr);
				Inet_Cmn_Rte_Table_Entry_Delete (module_data_ptr->ip_route_table, 
					rt_entry_ptr->dest_prefix, inet_next_addr, olsr_protocol_id);
				
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &(rt_entry_ptr->dest_addr));
				op_prg_mem_free (rt_entry_ptr);
				}
			
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
			
		case (8):
			{
			OlsrT_Duplicate_Set_Entry*		dup_entry_ptr;
			char*							temp_str;
			
			/* Clear entries in the Duplicate set table			*/
			bin_tbl_ptr = (PrgT_Bin_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (bin_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			//str_tbl_ptr = (PrgT_String_Hash_Table *) table_ptr;
			//keys_lptr = (List*) prg_string_hash_table_keys_get (str_tbl_ptr);
			//num_keys = op_prg_list_size (keys_lptr);
			
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				//key_ptr = (char*) op_prg_list_access (keys_lptr, i);
				//dup_entry_ptr = (OlsrT_Duplicate_Set_Entry*) prg_string_hash_table_item_remove (str_tbl_ptr, key_ptr);
				
				dup_entry_ptr = (OlsrT_Duplicate_Set_Entry*) op_prg_list_access (keys_lptr, i);
				prg_bin_hash_table_item_remove (bin_tbl_ptr, &dup_entry_ptr->key);
				
				if (op_ev_valid (dup_entry_ptr->entry_expiry_evhandle))
					{
					temp_str = (char*) op_ev_state (dup_entry_ptr->entry_expiry_evhandle);
					op_prg_mem_free (temp_str);
				
					op_ev_cancel (dup_entry_ptr->entry_expiry_evhandle);
					}
				
				op_prg_mem_free (dup_entry_ptr);
				}
			
			/* Free the keys and the list */
			//op_prg_list_free (keys_lptr);
			//op_prg_mem_free (keys_lptr);
			prg_list_destroy (keys_lptr, OPC_FALSE);
			break;
			}
		default:
			{
			str_tbl_ptr = (PrgT_String_Hash_Table *) table_ptr;
			keys_lptr = (List*) prg_string_hash_table_keys_get (str_tbl_ptr);
			num_keys = op_prg_list_size (keys_lptr);
			
			/* Default clearing algorithm: Since we have accounted for all cases, this is no longer used */
			for (i=0; i< num_keys; i++)
				{
				/* For each entry in table */
				key_ptr = (char*) op_prg_list_access (keys_lptr, i);
		
				op_prg_mem_free (prg_string_hash_table_item_remove (str_tbl_ptr, key_ptr));
				}
			
			/* Free the keys and the list */
			op_prg_list_free (keys_lptr);
			op_prg_mem_free (keys_lptr);
			break;
			}
		}/*end switch */
	
	FOUT;
	}

static void
olsr_rte_fail_rec_handle (int intrpt_type)
	{
	/* Function to handle FAILURE / RECOVERY interrupts */
	Objid		fail_rec_obj_id;
	Objid		own_node_objid;
	
	FIN (olsr_rte_fail_rec_handle (int));
	
	/* First check if this FAILURE/RECOVERY interrupt 	*/
	/* was for this node. If not, do not process this	*/
	/* interrupt 										*/
	
	/* Obtain the ID of the failed/recovered object.*/
	fail_rec_obj_id = op_intrpt_source ();
	
	own_node_objid = op_topo_parent (op_id_self ());
	
	if (own_node_objid == fail_rec_obj_id)
		{
		
		if (intrpt_type == OPC_INTRPT_FAIL)
			{
			/* FAILURE interrupt 			  */
			
			List*	keys_lptr;
			int		num_mpr_selectors;
			
			/* Clear all scheduled interrupts */
			/* Clear HELLO Timer */
			op_ev_cancel_if_pending (hello_timer_evhandle);
			
			/* Clear TC Timer */
			op_ev_cancel_if_pending (tc_timer_evhandle);
			
			/* If this node was an MPR, update global MPR count		*/
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (mpr_selector_set_table);
			num_mpr_selectors = prg_list_size (keys_lptr);
			if (num_mpr_selectors > 0)
				olsr_rte_update_global_mpr_count (OPC_FALSE);
			
			/* Clear all tables				  */
			olsr_rte_clear_table (mpr_selector_set_table, 6);
			olsr_rte_clear_table (neighbor_set_table, 1);
			olsr_rte_clear_table (two_hop_nbr_set_table, 2);
			olsr_rte_clear_table (tc_set_table, 3);
			olsr_rte_clear_table (link_set_table, 4);
			olsr_rte_clear_table (mpr_set_table, 5);
			olsr_rte_clear_table (olsr_routing_table, 7);
			olsr_rte_clear_table (duplicate_set_table, 8);
			
			/* Node loses its MPR status				*/
			if (op_stat_valid (local_stat_handles.mpr_status_shandle))
				op_stat_write (local_stat_handles.mpr_status_shandle, 0.0);
			}
		else if (intrpt_type == OPC_INTRPT_RECOVER)
			{
			/* RECOVERY interrupt				*/
			/* Schedule interrupts for periodic	*/
			/* hello and TC messages 			*/
			hello_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + op_dist_uniform (hello_interval), OLSRC_HELLO_TIMER_EXPIRY);
			tc_timer_evhandle = op_intrpt_schedule_self (op_sim_time () + op_dist_uniform (tc_interval), OLSRC_TC_TIMER_EXPIRY);
			
			/* Node is not an MPR yet				*/
			if (op_stat_valid (local_stat_handles.mpr_status_shandle))
				op_stat_write (local_stat_handles.mpr_status_shandle, 0.0);
			}
		else
			{
			/* Unknown interrupt 	*/
			/* Do not do anything 	*/
			}
		}
	
	FOUT;
	}
		
static void
olsr_rte_error (const char* str1, char* str2, char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (olsr_rte_error <args>);
	
	op_sim_end ("OLSR Routing Process : ", str1, str2, str3);
	
	FOUT;
	}

static void
olsr_rte_duplicate_entry_delete (void* v_dup_entry_ptr, int PRG_ARG_UNUSED (code))	
	{
	/* Deletes the expired duplicate set entry */
	OlsrT_Duplicate_Set_Entry* 	dup_entry_ptr = (OlsrT_Duplicate_Set_Entry*) v_dup_entry_ptr;
	
	FIN (olsr_rte_duplicate_entry_delete (<args>));
	
	prg_bin_hash_table_item_remove (duplicate_set_table, &dup_entry_ptr->key);
	prg_mem_free (dup_entry_ptr);
	
	FOUT;
	}
	
static void
olsr_rte_routing_table_entry_update (OlsrT_Routing_Table_Entry* rt_entry_ptr, int new_next_addr, 
	int new_hops, int new_local_iface_addr)
	{
	/* Function to update nextaddr, hops and local intf  in OLSR rte tbl */
	FIN (olsr_rte_routing_table_entry_update (<args>));
	
	rt_entry_ptr->next_addr = new_next_addr;
	rt_entry_ptr->hops = new_hops;
	rt_entry_ptr->local_iface_addr = new_local_iface_addr;
	
	FOUT;
	}

static void
olsr_rte_table_incremental_entry_add (PrgT_Bin_Hash_Table* old_olsr_rte_table_ptr, int dest_addr,
	int new_nexthop, int new_cost, int new_local_intf)
	{
	InetT_Address				inet_next_addr;
	InetT_Address				old_inet_next_addr;
	InetT_Address				inet_local_intf_addr;
	InetT_Address				inet_dest_addr;
	IpT_Dest_Prefix				dest_prefix;
	int							intf_index;
	IpT_Port_Info				port_info;
	OlsrT_Routing_Table_Entry*	olsr_rte_entry_ptr = OPC_NIL;
	void*						old_contents_ptr;
	
	/* Function to implement incremental routing addition	*/
	/* Function checks if a rte entry for this destination 	*/
	/* is present in Old OLSR table. If no, then create new	*/
	/* entry in IP Cmn Tbl and new OLSR tbl. If old entry 	*/
	/* exists and there's change in cost/nexthop, update IP	*/
	/* Cmn Tbl. Then add this entry in new OLSR table.		*/
	
	FIN (olsr_rte_table_incremental_entry_add (<args>));
	
	/* Get the outgoing port info */
	inet_local_intf_addr = inet_rtab_index_to_addr_convert (new_local_intf);
	inet_rte_is_local_address (inet_local_intf_addr, module_data_ptr, &intf_index);
	port_info = ip_rte_port_info_from_tbl_index (module_data_ptr, intf_index);
	
	/* First check if an entry for this dest exists in Old olsr rte tbl */
	if ((olsr_rte_entry_ptr = (OlsrT_Routing_Table_Entry*) 
		prg_bin_hash_table_item_remove (old_olsr_rte_table_ptr, &dest_addr)) != OPC_NIL)
		{
		/* Old Olsr entry exists */
		
		/* Check if there's change in nexthop or cost */
		if ((new_nexthop != olsr_rte_entry_ptr->next_addr) ||
			(new_cost != olsr_rte_entry_ptr->hops))
			{
			/* Either nexthop or path costs are different 	*/
			/* If nexthops are not different, do not send	*/
			/* new_next_hop info to IP.						*/
			if (new_nexthop == olsr_rte_entry_ptr->next_addr)
				inet_next_addr = INETC_ADDRESS_INVALID;
			else
				inet_next_addr = inet_rtab_index_to_addr_convert (new_nexthop);
			
			/* Get old next hop address */
			old_inet_next_addr = inet_rtab_index_to_addr_convert (olsr_rte_entry_ptr->next_addr);
			
			/* Update the IP Cmn Rte Table: using the old dest_prefix */
			Inet_Cmn_Rte_Table_Entry_Update (module_data_ptr->ip_route_table, olsr_rte_entry_ptr->dest_prefix,
				old_inet_next_addr, olsr_protocol_id, inet_next_addr, port_info, new_cost, OPC_NIL);
			
			/* Update the next_hop and cost of old entry */
			olsr_rte_routing_table_entry_update (olsr_rte_entry_ptr, new_nexthop, new_cost, new_local_intf);
			}
		
		/* Transfer the old rte entry to new olsr_routing_table */
		prg_bin_hash_table_item_insert (olsr_routing_table, &dest_addr, olsr_rte_entry_ptr, &old_contents_ptr);
		}
	else
		{
		/* This is a new entry. Add it in OLSR and IP Cmn Tbl */
		
		inet_dest_addr = inet_rtab_index_to_addr_convert (dest_addr);
		inet_next_addr = inet_rtab_index_to_addr_convert (new_nexthop);
		
		/* Create new dest_prefix */
		dest_prefix = ip_cmn_rte_table_dest_prefix_create (inet_dest_addr,
			(is_ipv6_enabled ? subnet_mask_128 : subnet_mask_32));
		
		/* Add in Olsr routing table */
		olsr_rte_routing_table_entry_add (dest_addr, new_nexthop, 
			new_cost, new_local_intf, dest_prefix);
		
		/* Add in IP Cmn Tbl */
		Inet_Cmn_Rte_Table_Entry_Add_Options (module_data_ptr->ip_route_table, OPC_NIL, dest_prefix,
			inet_next_addr, port_info, new_cost, olsr_protocol_id, 1, IPC_CMN_RTE_TABLE_ENTRY_ADD_INDIRECT_NEXTHOP_OPTION);
		
		}
	
	FOUT;
	}

/****** DJK related functions ******/

static void
olsr_rte_djk_rte_table_calculate (void)
	{
	PrgT_Vector*				edge_vector_ptr;
	PrgT_Graph_Vertex*			vertex_ptr;
	PrgT_Graph_Vertex*			next_hop_vertex_ptr;
	PrgT_Graph_Edge *			next_hop_edge_ptr;
	List*						keys_lptr;
	InetT_Address				inet_dest_addr, inet_next_addr, inet_local_intf_addr;
	
	OlsrT_Djk_Edge_State*		edge_state_ptr;
	OlsrT_Djk_State*			next_hop_state_ptr;
	OlsrT_Djk_State*			dest_state_ptr;
	int							intf_index;
	IpT_Port_Info				port_info;
	int							num_edges = 0;
	OlsrT_Routing_Table_Entry*			olsr_rte_entry_ptr = OPC_NIL;
	PrgT_Bin_Hash_Table*		old_olsr_rte_table;
	int							num_old_entries, k;
	InetT_Address				old_inet_next_addr;
	void* next_hops_ptr = PRGC_NIL;
	double path_cost;
	int num_next_hops = 0;
	PrgT_List* next_hops_lptr = PRGC_NIL;
	
	PrgT_Bin_Hash_Table*		intf_set_table_ptr;
	OlsrT_Interface_Set_Entry*	intf_set_entry_ptr;
	int							i;

	/* Use DJK to build route table */
	FIN (olsr_rte_djk_rte_table_calculate (void));

	if (op_prg_odb_ltrace_active ("trace_rte_table")== OPC_TRUE)
		{
		op_prg_odb_print_major (pid_string, "\n Deleting all the entries of routing table \n", OPC_NIL);
		}

	/****************************************************/
	/******** Starting Cals for New Rte Table ***********/
	/****************************************************/
	
	op_stat_write (global_stat_handles.rte_table_calcs_global_shandle, 1.0);
	
	/* First initialize the graph for PRG DJK.	*/
	prg_djk_graph_init (prg_graph_ptr);
	
	/* Enable the caching */
	prg_djk_graph_cache_enable (prg_graph_ptr);

	/* Set the weights to all edges */
	edge_vector_ptr = prg_graph_edge_all_get (prg_graph_ptr);
	
	num_edges = prg_vector_size (edge_vector_ptr);

	/* If there are less than 1 ege, FOUT */
	if (num_edges < 1)
		{
		prg_vector_destroy (edge_vector_ptr, OPC_FALSE);
		FOUT;
		}
	
	/* Destroy the vector of edges.		*/
	prg_vector_destroy (edge_vector_ptr, PRGC_FALSE);

	/* Put the handle of olsr_routing_table into an old_olsr_rte_table_ptr */
	old_olsr_rte_table = olsr_routing_table;
	
	/* Create a new olsr_routing_table */
	olsr_routing_table = prg_bin_hash_table_create (4, 4);

	/* Invoke DJK for the root vertex.	*/
	if (prg_djk_single_source_compute_with_options (root_vertex_ptr, PRGC_DJK_SINGLE_SOURCE_COMPUTE_OPT_FIRST_HOP) == PrgC_Compcode_Failure)
		{
		/* Something went wrong while running DJK. 	*/
		/* Warn the user, and continue on.			*/
		op_sim_error(OPC_SIM_ERROR_WARNING, "Error while running PRG DJK on the OLSR Topology Graph", OPC_NIL);
		
		/* Disable the caching */
		prg_djk_graph_cache_disable (prg_graph_ptr);
		prg_djk_graph_init (prg_graph_ptr);
		
		FOUT;
		}
	
	/* Initialize the iterator for the routing table constructed at the source vertex by single_source_compute */
	if (!prg_djk_graph_routing_table_map_init (root_vertex_ptr))
		FOUT;

	/* Loop through the routing table entries */
	/* Calling entry_next_get without initializing the map iterator leads to undefined results */
	/* Return values: vertex_ptr = destn vertex, path_cost = cost to destn, next_hops_lptr = list of next hop nodes to destn */
	/* vertex_ptr should never be freed */
	/* Depending upon the value of num_next_hops_get, next_hops_ptr is dereferenced - such an API has been provided for perf reasons */
	while (prg_djk_graph_routing_table_map_entry_next_get (root_vertex_ptr, &vertex_ptr, &path_cost, &next_hops_ptr, &num_next_hops))
		{
		/* Ignore if this is root vertex */
		if (vertex_ptr == root_vertex_ptr)
			continue;
		
		if (num_next_hops == 0)
			continue;

		if (num_next_hops == 1)
			{
			next_hop_edge_ptr = (PrgT_Graph_Edge*) next_hops_ptr;
			if (!next_hop_edge_ptr)
				continue;
			}
		else
			{
			next_hops_lptr = (PrgT_List*) next_hops_ptr;
			if ((next_hops_lptr == PRGC_NIL) || (prg_list_size (next_hops_lptr) == 0))
				continue;
			}
		
		if (path_cost == -1)
			continue;

		/* We are going to add this in route table */
		
		/************************************************/
		/*********** Get dest address, prefix ***********/
		/************************************************/
		
		/* Get the state_ptr of the vertex */
		dest_state_ptr = (OlsrT_Djk_State *) prg_vertex_client_state_get (vertex_ptr, OLSRC_DJK_STATE);
		
		/* Get the destination address & Create dest prefix */
		inet_dest_addr = inet_rtab_index_to_addr_convert (dest_state_ptr->addr);
		
		/************************************************/
		/*********** Get nexthop address ****************/
		/************************************************/
		
		// Find the best nexthop (tiebreaker for nexthops)
		if (num_next_hops != 1)
			next_hop_edge_ptr = olsr_rte_djk_next_hop_get (next_hops_lptr);
		
		/* Get the next hop vertex and nexthop address */
		next_hop_vertex_ptr = prg_edge_vertex_b_get (next_hop_edge_ptr);
		next_hop_state_ptr = (OlsrT_Djk_State *) prg_vertex_client_state_get (next_hop_vertex_ptr, OLSRC_DJK_STATE);
		inet_next_addr = inet_rtab_index_to_addr_convert (next_hop_state_ptr->addr);
		
		/************************************************/
		/*********** Get outgoing port ******************/
		/************************************************/
		
		/* Get the edge state ptr  and local interface address */
		edge_state_ptr = (OlsrT_Djk_Edge_State *) prg_edge_client_state_get (next_hop_edge_ptr, OLSRC_DJK_STATE);
		inet_local_intf_addr = inet_rtab_index_to_addr_convert (edge_state_ptr->local_intf);
			
		/*Get outgoing port info */
		inet_rte_is_local_address (inet_local_intf_addr, module_data_ptr, &intf_index);
		port_info = ip_rte_port_info_from_tbl_index (module_data_ptr, intf_index);
		
		/*****************************************************/
		/************ Check OLSR Table ***********************/
		/************** Update IP Table **********************/
		
		/* The next hop is NOT the node's main address, rather its interface address	*/
		//olsr_rte_table_incremental_entry_add (old_olsr_rte_table, dest_state_ptr->addr, next_hop_state_ptr->addr,
		//	path_cost, edge_state_ptr->local_intf);
		olsr_rte_table_incremental_entry_add (old_olsr_rte_table, dest_state_ptr->addr, edge_state_ptr->neighbor_intf,
			path_cost, edge_state_ptr->local_intf);

		/* When an entry corresponding to *any* prefix for the given node is added			*/
		/* we will add entries for *all* prefixes of this node through the same next hop.	*/
		/* This is valid due to the simple fact that if the SPT yields a next hop to reach	*/
		/* any prefix of the given node, the same next hop will be on at least one of the 	*/
		/* shortest hop paths to reach all prefixes of that node.							*/
		
		/* Prefixes of the node can be obtained through the global MID table						*/
		intf_set_table_ptr = olsr_support_interface_table_ptr_get ();
		
		/* If this table is NOT NIL, obtain the list of interfaces corresponding to the main address	*/
		/* of the current vertex.																		*/
		if (intf_set_table_ptr != OPC_NIL)
			{
			keys_lptr = (List*) prg_bin_hash_table_item_list_get (intf_set_table_ptr);
			for (i = 0; i < op_prg_list_size (keys_lptr); i++)
				{
				intf_set_entry_ptr = (OlsrT_Interface_Set_Entry*) op_prg_list_access (keys_lptr, i);
				
				/* If the entry's main address matches the current vertex's main address, add this interface	*/
				/* prefix to the routing table if applicable													*/
				if ((intf_set_entry_ptr->main_addr == dest_state_ptr->addr) && 
					(intf_set_entry_ptr->intf_addr != intf_set_entry_ptr->main_addr))
					olsr_rte_table_incremental_entry_add (old_olsr_rte_table, intf_set_entry_ptr->intf_addr, 
						edge_state_ptr->neighbor_intf, path_cost, edge_state_ptr->local_intf);
				}
			
			prg_list_destroy (keys_lptr, OPC_FALSE);
			}
		
		} /* End looping through all vertices */
		
	
	/* go through all the remaining entries in old OLSR rte table */
	keys_lptr = (List*) prg_bin_hash_table_item_list_get (old_olsr_rte_table);
	num_old_entries = op_prg_list_size (keys_lptr);
	
	for (k=0; k < num_old_entries; k++)
		{
		/* for each entry in routing table */
		olsr_rte_entry_ptr = (OlsrT_Routing_Table_Entry*) op_prg_list_access (keys_lptr, k);
		prg_bin_hash_table_item_remove (old_olsr_rte_table, &(olsr_rte_entry_ptr->dest_addr));
		
		/* Get the old next hop address */
		old_inet_next_addr = inet_rtab_index_to_addr_convert (olsr_rte_entry_ptr->next_addr);
				
		/* Delete these entries from IP Rte table */
		Inet_Cmn_Rte_Table_Entry_Delete (module_data_ptr->ip_route_table, 
			olsr_rte_entry_ptr->dest_prefix, old_inet_next_addr, olsr_protocol_id);
		
		/* Destroy the dest_prefix */
		ip_cmn_rte_table_dest_prefix_destroy (olsr_rte_entry_ptr->dest_prefix); 	
		
		/* Destroy this entry from old olsr table */
		op_prg_mem_free (olsr_rte_entry_ptr);
		}
	
	/* Free the key list		*/
	/* but not the keys themselves as these are still in the hash table	*/
	prg_list_destroy (keys_lptr, OPC_FALSE);
	
	/* Destroy the old OLSR String HASH TaBLE */
	/* Does not destroy table values - but we do not expect any value to be there */
	prg_bin_hash_table_destroy (old_olsr_rte_table, OPC_NIL);
	
	/* Disable the caching */
	prg_djk_graph_cache_disable (prg_graph_ptr);
	
	/* Note: Make sure we do not need graph_init() here */
	//prg_djk_graph_init (prg_graph_ptr);

	FOUT;
	
	}
		
static PrgT_Graph_Edge*
olsr_rte_djk_next_hop_get (PrgT_List* nexthop_list_ptr)
	{
	int 				list_size, index;
	int					mprs_willingness = -1;
	int					non_mprs_willingness = -1;
	PrgT_Graph_Edge*	next_hop_edge_ptr;
	PrgT_Graph_Edge*	mprs_edge_ptr = OPC_NIL;
	PrgT_Graph_Edge*	non_mprs_edge_ptr = OPC_NIL;
	OlsrT_Djk_State*	next_hop_st_ptr;
	
	/* Obtain the MPR and non-MPR selector nodes with highest willingness 		*/
	/* then return element with highest willingness, MPR selector property		*/
	/* being used to break the tie.												*/
	FIN (olsr_rte_djk_next_hop_get (PrgT_Vector* nexthop_vector_ptr));
	
	list_size = prg_list_size (nexthop_list_ptr);
	for (index = 0; index < list_size; index++)
		{
		/* Access the first next-hop path in the path list.	*/
		next_hop_edge_ptr = (PrgT_Graph_Edge *) prg_list_access (nexthop_list_ptr, index);
		
		next_hop_st_ptr = (OlsrT_Djk_State *) prg_vertex_client_state_get 
			(prg_edge_vertex_b_get (next_hop_edge_ptr), OLSRC_DJK_STATE);
		
		if (next_hop_st_ptr->mpr_selector)
			{
			/* It is a mpr selector */
			if ((mprs_edge_ptr == OPC_NIL) || 
				(next_hop_st_ptr->willingness > mprs_willingness))
				{
				mprs_edge_ptr = next_hop_edge_ptr;
				mprs_willingness = next_hop_st_ptr->willingness;
				}
			}
		else
			{
			if ((non_mprs_edge_ptr == OPC_NIL) ||
				(next_hop_st_ptr->willingness > non_mprs_willingness))
				{
				non_mprs_edge_ptr = next_hop_edge_ptr;
				non_mprs_willingness = next_hop_st_ptr->willingness;
				}
			}
		}
	
	if (mprs_willingness < non_mprs_willingness)
		FRET (non_mprs_edge_ptr);
	FRET (mprs_edge_ptr);
	}
	

/***********************************************************/
/******* Incremental Graph Build Functions *****************/
/***********************************************************/

static void
olsr_rte_graph_nbr_add (OlsrT_Neighbor_Set_Entry* nbr_set_entry_ptr, OlsrT_Link_Set_Entry* link_set_entry_ptr)
	{
	PrgT_Graph_Vertex*			vertex_ptr = PRGC_NIL;
	PrgT_Graph_Edge*			edge_ptr = PRGC_NIL;
	OlsrT_Djk_State*			state_ptr;
	OlsrT_Djk_Edge_State*		edge_state_ptr;
	void*						old_entry = OPC_NIL;
	
	FIN (olsr_rte_graph_nbr_add (<args>));
	
	/* This is a symmetric and willing neighbor 	 */
	/* Check if vertex corresponding to main address */
	/* is not already present in the graph		   	 */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &(nbr_set_entry_ptr->nbr_addr))) == PRGC_NIL)
		{
		/* No such vertex already exists, Create new */
		vertex_ptr = prg_graph_vertex_insert (prg_graph_ptr);
				
		/* Allocate memory for an OLSR Vertex State.	*/
		state_ptr = (OlsrT_Djk_State *) op_prg_pmo_alloc (olsr_djk_state_pmoh);
				
		state_ptr->addr = nbr_set_entry_ptr->nbr_addr;
		state_ptr->willingness = nbr_set_entry_ptr->willingness;
				
		if (prg_bin_hash_table_item_get (mpr_selector_set_table, &(nbr_set_entry_ptr->nbr_addr)))
			state_ptr->mpr_selector = OPC_TRUE;
		else
			state_ptr->mpr_selector = OPC_FALSE;
				
		/* Set the client state on the vertex */
		prg_vertex_client_state_set (vertex_ptr, OLSRC_DJK_STATE, (void *) state_ptr);
				
		/* Insert this entry in hash table */
		prg_bin_hash_table_item_insert (djk_hash_table_ptr, &(nbr_set_entry_ptr->nbr_addr), vertex_ptr, &old_entry);
				
		}
	else
		{
		/* There's already an vertex created for this NBR */
		/* Make sure willingness and mpr_selector fields  */
		/* were correctly populated.					  */
								
		/* Get the state */
		state_ptr = (OlsrT_Djk_State *) prg_vertex_client_state_get (vertex_ptr, OLSRC_DJK_STATE);
				
		/* Set willingness and mpr_selector fields */
		state_ptr->willingness = nbr_set_entry_ptr->willingness;
				
		if (prg_bin_hash_table_item_get (mpr_selector_set_table, &(nbr_set_entry_ptr->nbr_addr)))
			state_ptr->mpr_selector = OPC_TRUE;
		else
			state_ptr->mpr_selector = OPC_FALSE;
		}
					
			
	/* Create an edge between root and this 1-hop nbr if its not already present */
	if ((edge_ptr = prg_graph_edge_exists (prg_graph_ptr, root_vertex_ptr, 
		vertex_ptr, PrgC_Graph_Edge_Simplex)) == PRGC_NIL)
		{
		/* Create an edge between this vertex and root vertex */
		edge_ptr = prg_graph_edge_insert (prg_graph_ptr, root_vertex_ptr, vertex_ptr, PrgC_Graph_Edge_Simplex);
				
		/* Populate edge_client state */
		edge_state_ptr = (OlsrT_Djk_Edge_State *) op_prg_pmo_alloc (olsr_djk_edge_state_pmoh);
		edge_state_ptr->local_intf = link_set_entry_ptr->key.local_intf_addr;
		edge_state_ptr->neighbor_intf = link_set_entry_ptr->key.nbr_intf_addr;
				
		prg_edge_client_state_set (edge_ptr, OLSRC_DJK_STATE, (void *) edge_state_ptr);
		}
	else
		{
		/* Edge already exists. Make sure the local_intf value is set correctly */
		/* Get the state */
		edge_state_ptr = (OlsrT_Djk_Edge_State *) prg_edge_client_state_get (edge_ptr, OLSRC_DJK_STATE);
		edge_state_ptr->local_intf = link_set_entry_ptr->key.local_intf_addr;
		edge_state_ptr->neighbor_intf = link_set_entry_ptr->key.nbr_intf_addr;
		}
		
	
	FOUT;
	}

static void
olsr_rte_graph_nbr_delete (OlsrT_Neighbor_Set_Entry* nbr_set_entry_ptr)
	{
	PrgT_Graph_Vertex*			vertex_ptr = PRGC_NIL;
	PrgT_Graph_Edge*			edge_ptr = PRGC_NIL;
	PrgT_Graph_Edge*			two_hop_edge_ptr;
	
	int							j;
	PrgT_Vector*				edge_vector_ptr;
	int							num_edges;	
	
	FIN (olsr_rte_graph_nbr_delete (<args>));
	
	/* Check if this nbr vertex exists in the graph */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &(nbr_set_entry_ptr->nbr_addr))) != PRGC_NIL)
		{
		/* This Nbr vertex exists in the graph */
		/* Check if there exist and edge between root vertex and this nbr */
		if ((edge_ptr = prg_graph_edge_exists (prg_graph_ptr, root_vertex_ptr, 
			vertex_ptr, PrgC_Graph_Edge_Simplex)) != PRGC_NIL)
			{
			/* An edge exists between root_vertex and this nbr */
			/* Delete this edge */
			prg_graph_edge_remove (prg_graph_ptr, edge_ptr, 1);
			}
		
		/* Get all the edges going out of this vertex and delete them */
		edge_vector_ptr = (PrgT_Vector *) prg_vertex_edges_get (vertex_ptr, PrgC_Graph_Element_Set_Outgoing);
		
		num_edges = prg_vector_size (edge_vector_ptr);
		
		for (j=num_edges -1; j >= 0; j--)
			{
			two_hop_edge_ptr = (PrgT_Graph_Edge *) prg_vector_remove (edge_vector_ptr, j);
			prg_graph_edge_remove (prg_graph_ptr, two_hop_edge_ptr, 1);
			}
		
		prg_vector_destroy (edge_vector_ptr, OPC_FALSE);

		} /* End - if we find nbr vertex */			
	
	FOUT;
	}

static void
olsr_rte_graph_two_hop_nbr_add (int nbr_addr, OlsrT_Two_Hop_Neighbor_Set_Entry* two_hop_entry_ptr)
	{
	PrgT_Graph_Vertex*			vertex_ptr = PRGC_NIL;
	PrgT_Graph_Vertex*			two_hop_vertex_ptr = PRGC_NIL;
	
	PrgT_Graph_Edge*			edge_ptr = PRGC_NIL;
	void*						old_entry = OPC_NIL;
	OlsrT_Djk_State*			state_ptr;
	OlsrT_Djk_State* 			one_hop_neighbor_state_ptr;
	
	FIN (olsr_rte_graph_two_hop_nbr_add (<args>));
		
	/* Check if this nbr vertex exists in the graph */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &nbr_addr)) != PRGC_NIL)
		{
		/* This Nbr exists, add this two hop vertex in graph */
		
		/* Check if two-hop vertex is already present in the graph */
		if ((two_hop_vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get
			(djk_hash_table_ptr, &(two_hop_entry_ptr->two_hop_addr))) == PRGC_NIL)
			{
			/* No such vertex already exists, Create new */
			two_hop_vertex_ptr = prg_graph_vertex_insert (prg_graph_ptr);
				
			/* Allocate memory for an OLSR Vertex State.	*/
			state_ptr = (OlsrT_Djk_State *) op_prg_pmo_alloc (olsr_djk_state_pmoh);
				
			state_ptr->addr = two_hop_entry_ptr->two_hop_addr;
			state_ptr->willingness = OLSRC_INVALID;
	
			/* Set the client state on the vertex */
			prg_vertex_client_state_set (two_hop_vertex_ptr, OLSRC_DJK_STATE, (void *) state_ptr);
					
			/* Insert this entry in hash table */
			prg_bin_hash_table_item_insert (djk_hash_table_ptr, 
				&two_hop_entry_ptr->two_hop_addr, two_hop_vertex_ptr, &old_entry);
			}
				
		/* Either we have added new vertex for this 2-hop or it was already present */
		/* In either case, create an edge between 1-hop-nbr and 2-hop-nbr			*/
		
		/* If the neighbor has a willingness of NEVER, a link must not be added.	*/
		one_hop_neighbor_state_ptr = (OlsrT_Djk_State*) prg_vertex_client_state_get (vertex_ptr, OLSRC_DJK_STATE);
		if (one_hop_neighbor_state_ptr->willingness == OLSRC_WILL_NEVER)
			FOUT;
			
		/* Create edge only when edge does not exists */
		if (prg_graph_edge_exists (prg_graph_ptr, vertex_ptr, two_hop_vertex_ptr, PrgC_Graph_Edge_Simplex) == PRGC_NIL)
			{
			edge_ptr = prg_graph_edge_insert (prg_graph_ptr, vertex_ptr /*1-hop-nbr*/, 
						two_hop_vertex_ptr /*2-hop-nbr*/, PrgC_Graph_Edge_Simplex);
				
			/* Set OPC_NIL for client state edge */
			prg_edge_client_state_set (edge_ptr, OLSRC_DJK_STATE, (void *) OPC_NIL);
			}

		}
	
	
	FOUT;
	}

static void
olsr_rte_graph_two_hop_nbr_delete (int nbr_addr, int two_hop_addr)
	{
	PrgT_Graph_Vertex*			vertex_ptr = PRGC_NIL;
	PrgT_Graph_Vertex*			two_hop_vertex_ptr = PRGC_NIL;
	PrgT_Graph_Edge*			two_hop_edge_ptr = PRGC_NIL;
	
	FIN (olsr_rte_graph_two_hop_nbr_delete (<args>));
	
	/* Check if this nbr vertex exists in the graph */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &nbr_addr)) != PRGC_NIL)
		{
		/* This Nbr vertex exists in the graph */
		/* Check if two-hop vertex is already present in the graph */
		if ((two_hop_vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
			(djk_hash_table_ptr, &two_hop_addr)) != PRGC_NIL)
			{
			/* Check if there exist an edge between nbr and 2-hop-nbr */
			if ((two_hop_edge_ptr = prg_graph_edge_exists (prg_graph_ptr, vertex_ptr, 
				two_hop_vertex_ptr, PrgC_Graph_Edge_Simplex)) != PRGC_NIL)
				{
				/* An edge exists between nbr and this 2-hop-nbr */
				/* Delete this edge */
				prg_graph_edge_remove (prg_graph_ptr, two_hop_edge_ptr, 1);
				}
			}
		
		} /* End - if we find nbr vertex */			
	
	FOUT;
	}

static void
olsr_rte_graph_tc_add (OlsrT_TC_Set_Entry* tc_set_entry_ptr)
	{
	OlsrT_Djk_State* 	state_ptr;
	PrgT_Graph_Vertex *	vertex_ptr;
	PrgT_List_Cell *	dest_addr_list_cell_ptr;
	
	FIN (olsr_rte_graph_tc_add (<args>));
	
	/* Check if this vertex is already present in the graph */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &(tc_set_entry_ptr->last_addr))) == PRGC_NIL)
		{
		/* No such vertex already exists, Create new */
		vertex_ptr = prg_graph_vertex_insert (prg_graph_ptr);
				
		/* Allocate memory for an OLSR Vertex State	*/
		state_ptr = (OlsrT_Djk_State *) op_prg_pmo_alloc (olsr_djk_state_pmoh);
				
		state_ptr->addr = tc_set_entry_ptr->last_addr;
		state_ptr->willingness = OLSRC_INVALID;
	
		/* Set the client state on the vertex */
		prg_vertex_client_state_set (vertex_ptr, OLSRC_DJK_STATE, (void *) state_ptr);
					
		/* Insert this entry in hash table */
		prg_bin_hash_table_item_insert (djk_hash_table_ptr, &(tc_set_entry_ptr->last_addr), vertex_ptr, OPC_NIL);
		}
		
	/* Either we've created new vertex for this MPR 	*/
	/* node or it was already present. In either case, 	*/
	/* vertex_ptr variable has its handle. 				*/
		
	/* Go through the MPR selectors this MPR node can reach */
	for (dest_addr_list_cell_ptr = prg_list_head_cell_get (&tc_set_entry_ptr->state_ptr->dest_addr_list);
		 dest_addr_list_cell_ptr;
		 dest_addr_list_cell_ptr = prg_list_cell_next_get (dest_addr_list_cell_ptr))
		{
		PrgT_Graph_Vertex *	mprs_vertex_ptr;
		int * 				dest_addr_ptr = (int*) prg_list_cell_data_get (dest_addr_list_cell_ptr);			
			
		if ((mprs_vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
			(djk_hash_table_ptr, dest_addr_ptr)) == PRGC_NIL)
			{
			/* No such vertex already exists, Create new */
			mprs_vertex_ptr = prg_graph_vertex_insert (prg_graph_ptr);
				
			/* Allocate memory for an OSPF DJK node.	*/
			state_ptr = (OlsrT_Djk_State *) op_prg_pmo_alloc (olsr_djk_state_pmoh);
				
			state_ptr->addr = *dest_addr_ptr;
			state_ptr->willingness = OLSRC_INVALID;
	
			/* Set the client state on the vertex */
			prg_vertex_client_state_set (mprs_vertex_ptr, OLSRC_DJK_STATE, (void *) state_ptr);
					
			/* Insert this entry in hash table */
			prg_bin_hash_table_item_insert (djk_hash_table_ptr, dest_addr_ptr, mprs_vertex_ptr, OPC_NIL);
			}
			
		/* Either we created new vertex or it was already present 	*/
		/* In either case, we will create edge between MPR node		*/
		/* and MPR selector node. 									*/
		if (prg_graph_edge_exists (prg_graph_ptr, vertex_ptr, mprs_vertex_ptr, PrgC_Graph_Edge_Simplex) == PRGC_NIL)
			{
			PrgT_Graph_Edge * edge_ptr = prg_graph_edge_insert (prg_graph_ptr, vertex_ptr /*mpr-node*/, 
				mprs_vertex_ptr /*mpr-selector-node*/, PrgC_Graph_Edge_Simplex);
			
			/* Set OPC_NIL for client state edge */
			prg_edge_client_state_set (edge_ptr, OLSRC_DJK_STATE, (void *) OPC_NIL);
			}
		
		} /* End loop through mpr-selectors */
	
	FOUT;
	}

static void
olsr_rte_graph_tc_delete (OlsrT_TC_Set_Entry* tc_set_entry_ptr)
	{	
	PrgT_Graph_Vertex* 			vertex_ptr;
	
	FIN (olsr_rte_graph_tc_delete (<args>));
	
	/* Check if this vertex is already present in the graph */
	if ((vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
		(djk_hash_table_ptr, &(tc_set_entry_ptr->last_addr))) != PRGC_NIL)
		{
		/* Mpr vertex already exists */
		PrgT_List_Cell* list_cell_ptr;
		
		for (list_cell_ptr = prg_list_head_cell_get (&tc_set_entry_ptr->state_ptr->dest_addr_list);
			 list_cell_ptr;
			 list_cell_ptr = prg_list_cell_next_get (list_cell_ptr))
			{
			PrgT_Graph_Vertex* 		mprs_vertex_ptr;
			int * dest_addr_ptr 	= (int*) prg_list_cell_data_get (list_cell_ptr);
			
			/* Check if mpr-selector vertex exists */
			if ((mprs_vertex_ptr = (PrgT_Graph_Vertex*) prg_bin_hash_table_item_get 
				(djk_hash_table_ptr, dest_addr_ptr)) != PRGC_NIL)
				{				
				PrgT_Graph_Edge*			mpr_edge_ptr;
				
				/* Check if edge between MPR and Mpr-Selector exists */
				if ((mpr_edge_ptr = prg_graph_edge_exists (prg_graph_ptr, vertex_ptr, 
					mprs_vertex_ptr, PrgC_Graph_Edge_Simplex)) != PRGC_NIL)
					{
					/* Delete the edge between MPR and Mpr-Selector */
					prg_graph_edge_remove (prg_graph_ptr, mpr_edge_ptr, 1);
					}
				}
			} /* End for looping of mpr-selectors */
		} /*end if MPR vertex exists */	
		
	FOUT;
	}




/* End of Function Block */

/* Undefine optional tracing in FIN/FOUT/FRET */
/* The FSM has its own tracing code and the other */
/* functions should not have any tracing.		  */
#undef FIN_TRACING
#define FIN_TRACING

#undef FOUTRET_TRACING
#define FOUTRET_TRACING

#if defined (__cplusplus)
extern "C" {
#endif
	void olsr_rte (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_olsr_rte_init (int * init_block_ptr);
	void _op_olsr_rte_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_olsr_rte_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_olsr_rte_alloc (VosT_Obtype, int);
	void _op_olsr_rte_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
olsr_rte (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (olsr_rte ());

		{
		/* Temporary Variables */
		int					invoke_mode;
		int					intrpt_type;
		int					intrpt_code;
		/* End of Temporary Variables */


		FSM_ENTER ("olsr_rte")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "olsr_rte [init enter execs]")
				FSM_PROFILE_SECTION_IN ("olsr_rte [init enter execs]", state0_enter_exec)
				{
				/* Initialize shared OLSR globals */
				olsr_rte_globals_init ();
				
				/* Initializa the state variables	*/
				olsr_rte_sv_init ();
				
				/* Register the statistics	*/
				olsr_rte_local_stats_reg ();
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "olsr_rte [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "wait", "tr_0", "olsr_rte [init -> wait : default / ]")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "wait", state1_enter_exec, "olsr_rte [wait enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"olsr_rte")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "wait", "olsr_rte [wait exit execs]")
				FSM_PROFILE_SECTION_IN ("olsr_rte [wait exit execs]", state1_exit_exec)
				{
				op_pro_invoker (own_prohandle, &invoke_mode);
				
				/* Get the interrupt type and code	*/
				intrpt_type = op_intrpt_type ();
				intrpt_code = op_intrpt_code ();
				
				if (intrpt_type == OPC_INTRPT_FAIL || intrpt_type == OPC_INTRPT_RECOVER)
					{
					olsr_rte_fail_rec_handle (intrpt_type);	
					}
				
				if (invoke_mode == OPC_PROINV_INDIRECT)
					{	
					olsr_rte_pkt_arrival_handle();
					}
					
					
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (wait) transition processing **/
			FSM_PROFILE_SECTION_IN ("olsr_rte [wait trans conditions]", state1_trans_conds)
			FSM_INIT_COND (HELLO_TIMER_EXPIRY)
			FSM_TEST_COND (TC_TIMER_EXPIRY)
			FSM_TEST_COND (RTE_CALC_TIMER_EXPIRY)
			FSM_TEST_COND (HELLO_PROCESSING_TIMER_EXPIRY)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("wait")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, olsr_rte_hello_expiry_handle();, "HELLO_TIMER_EXPIRY", "olsr_rte_hello_expiry_handle()", "wait", "wait", "tr_2", "olsr_rte [wait -> wait : HELLO_TIMER_EXPIRY / olsr_rte_hello_expiry_handle()]")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, olsr_rte_tc_expiry_handle();, "TC_TIMER_EXPIRY", "olsr_rte_tc_expiry_handle()", "wait", "wait", "tr_4", "olsr_rte [wait -> wait : TC_TIMER_EXPIRY / olsr_rte_tc_expiry_handle()]")
				FSM_CASE_TRANSIT (2, 1, state1_enter_exec, olsr_rte_table_calc_expiry_handle();, "RTE_CALC_TIMER_EXPIRY", "olsr_rte_table_calc_expiry_handle()", "wait", "wait", "tr_4_1", "olsr_rte [wait -> wait : RTE_CALC_TIMER_EXPIRY / olsr_rte_table_calc_expiry_handle()]")
				FSM_CASE_TRANSIT (3, 1, state1_enter_exec, olsr_hello_processing_expiry_handle();, "HELLO_PROCESSING_TIMER_EXPIRY", "olsr_hello_processing_expiry_handle()", "wait", "wait", "tr_4_2", "olsr_rte [wait -> wait : HELLO_PROCESSING_TIMER_EXPIRY / olsr_hello_processing_expiry_handle()]")
				FSM_CASE_TRANSIT (4, 1, state1_enter_exec, ;, "default", "", "wait", "wait", "tr_4_0", "olsr_rte [wait -> wait : default / ]")
				}
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"olsr_rte")
		}
	}




void
_op_olsr_rte_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
#if defined (OPD_ALLOW_ODB)
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__+1;
#endif

	FIN_MT (_op_olsr_rte_diag ())

	if (1)
		{

		/* Diagnostic Block */

		BINIT
		{
		/* Diagnostic block, calling functions to print */
		/* different tables on the node					*/
		/* These functions will be exectued when "prold	*/
		/* process_id" command is executed through ODB	*/
		
		/* Print all the tables */
		if (op_prg_odb_ltrace_active ("olsr_all"))
			{
			olsr_rte_link_set_print_to_string ();
			olsr_rte_neighbor_set_print_to_string();
			olsr_rte_two_hop_neighbor_set_print_to_string ();
			olsr_rte_tc_set_print_to_string ();
			olsr_rte_mpr_selector_set_print_to_string ();
			olsr_rte_mpr_set_print_to_string ();
			olsr_rte_routing_table_print_to_string ();
		    olsr_rte_interface_table_print_to_string (); 
			}
		
		/* Print individual tables */
		if (op_prg_odb_ltrace_active ("olsr_link"))
			{
			/* Link table */
			olsr_rte_link_set_print_to_string ();
			}
		if (op_prg_odb_ltrace_active ("olsr_nbr"))
			{
			/* NBR and 2 HOPS NBR tables */
			olsr_rte_neighbor_set_print_to_string ();
			olsr_rte_two_hop_neighbor_set_print_to_string ();
			}
		if (op_prg_odb_ltrace_active ("olsr_tc"))
			{
			/* TC table */
			olsr_rte_tc_set_print_to_string ();
			}
		if (op_prg_odb_ltrace_active ("olsr_mpr"))
			{
			/* MPR and MPR Selector tables */
			olsr_rte_mpr_selector_set_print_to_string ();
			olsr_rte_mpr_set_print_to_string ();
			}
		if (op_prg_odb_ltrace_active ("olsr_routing"))
			{
			/* Routing table */
			olsr_rte_routing_table_print_to_string ();
			}
		}

		/* End of Diagnostic Block */

		}

	FOUT
#endif /* OPD_ALLOW_ODB */
	}




void
_op_olsr_rte_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (_op_olsr_rte_terminate ())

	if (1)
		{

		/* Termination Block */

		BINIT
		{
		/* Clear HELLO Timer */
		op_ev_cancel_if_pending (hello_timer_evhandle);
				
		/* Clear TC Timer */
		op_ev_cancel_if_pending (tc_timer_evhandle); 
		
		/* Clear all tables				  */
		olsr_rte_clear_table (neighbor_set_table, 1);
		olsr_rte_clear_table (two_hop_nbr_set_table, 2);
		olsr_rte_clear_table (tc_set_table, 3);
		olsr_rte_clear_table (link_set_table, 4);
		olsr_rte_clear_table (mpr_set_table, 5);
		olsr_rte_clear_table (mpr_selector_set_table, 6);
		olsr_rte_clear_table (olsr_routing_table, 7);
		olsr_rte_clear_table (duplicate_set_table, 8);
		
		}

		/* End of Termination Block */

		}
	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_olsr_rte_svar function. */
#undef own_prohandle
#undef output_strm
#undef module_data_ptr
#undef command_ici_ptr
#undef neighbor_set_table
#undef link_set_table
#undef link_set_chain_head_ptr
#undef msg_seq_num
#undef pkt_seq_num
#undef mpr_set_table
#undef neighborhood_changed
#undef mpr_selector_set_table
#undef two_hop_nbr_set_table
#undef own_main_address
#undef pid_string
#undef ANSN
#undef tc_set_table
#undef duplicate_set_table
#undef tc_entry_expiry_time
#undef selectorset_changed
#undef topo_table_hndl
#undef olsr_routing_table
#undef topology_changed
#undef olsr_protocol_id
#undef subnet_mask_32
#undef local_stat_handles
#undef node_willingness
#undef neighbor_hold_time
#undef tc_hold_time
#undef hello_interval
#undef tc_interval
#undef dup_hold_time
#undef hello_timer_evhandle
#undef send_empty_tc_time
#undef is_ipv6_enabled
#undef subnet_mask_128
#undef tc_timer_evhandle
#undef rte_calc_already_scheduled
#undef rte_table_calc_timer
#undef hello_processing_time_quantum
#undef hello_processing_deferred_head_ptr
#undef hello_processing_deferred_tail_ptr
#undef djk_hash_table_ptr
#undef prg_graph_ptr
#undef root_vertex_ptr
#undef my_process_id
#undef last_rte_calc_time
#undef djk_processing_time_quantum

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_olsr_rte_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_olsr_rte_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (olsr_rte)",
		sizeof (olsr_rte_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_olsr_rte_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	olsr_rte_state * ptr;
	FIN_MT (_op_olsr_rte_alloc (obtype))

	ptr = (olsr_rte_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "olsr_rte [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_olsr_rte_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	olsr_rte_state		*prs_ptr;

	FIN_MT (_op_olsr_rte_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (olsr_rte_state *)gen_ptr;

	if (strcmp ("own_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_prohandle);
		FOUT
		}
	if (strcmp ("output_strm" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->output_strm);
		FOUT
		}
	if (strcmp ("module_data_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->module_data_ptr);
		FOUT
		}
	if (strcmp ("command_ici_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->command_ici_ptr);
		FOUT
		}
	if (strcmp ("neighbor_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighbor_set_table);
		FOUT
		}
	if (strcmp ("link_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->link_set_table);
		FOUT
		}
	if (strcmp ("link_set_chain_head_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->link_set_chain_head_ptr);
		FOUT
		}
	if (strcmp ("msg_seq_num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->msg_seq_num);
		FOUT
		}
	if (strcmp ("pkt_seq_num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pkt_seq_num);
		FOUT
		}
	if (strcmp ("mpr_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mpr_set_table);
		FOUT
		}
	if (strcmp ("neighborhood_changed" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighborhood_changed);
		FOUT
		}
	if (strcmp ("mpr_selector_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mpr_selector_set_table);
		FOUT
		}
	if (strcmp ("two_hop_nbr_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->two_hop_nbr_set_table);
		FOUT
		}
	if (strcmp ("own_main_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_main_address);
		FOUT
		}
	if (strcmp ("pid_string" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pid_string);
		FOUT
		}
	if (strcmp ("ANSN" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ANSN);
		FOUT
		}
	if (strcmp ("tc_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tc_set_table);
		FOUT
		}
	if (strcmp ("duplicate_set_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->duplicate_set_table);
		FOUT
		}
	if (strcmp ("tc_entry_expiry_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tc_entry_expiry_time);
		FOUT
		}
	if (strcmp ("selectorset_changed" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->selectorset_changed);
		FOUT
		}
	if (strcmp ("topo_table_hndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->topo_table_hndl);
		FOUT
		}
	if (strcmp ("olsr_routing_table" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->olsr_routing_table);
		FOUT
		}
	if (strcmp ("topology_changed" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->topology_changed);
		FOUT
		}
	if (strcmp ("olsr_protocol_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->olsr_protocol_id);
		FOUT
		}
	if (strcmp ("subnet_mask_32" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->subnet_mask_32);
		FOUT
		}
	if (strcmp ("local_stat_handles" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->local_stat_handles);
		FOUT
		}
	if (strcmp ("node_willingness" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_willingness);
		FOUT
		}
	if (strcmp ("neighbor_hold_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neighbor_hold_time);
		FOUT
		}
	if (strcmp ("tc_hold_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tc_hold_time);
		FOUT
		}
	if (strcmp ("hello_interval" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_interval);
		FOUT
		}
	if (strcmp ("tc_interval" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tc_interval);
		FOUT
		}
	if (strcmp ("dup_hold_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dup_hold_time);
		FOUT
		}
	if (strcmp ("hello_timer_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_timer_evhandle);
		FOUT
		}
	if (strcmp ("send_empty_tc_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->send_empty_tc_time);
		FOUT
		}
	if (strcmp ("is_ipv6_enabled" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->is_ipv6_enabled);
		FOUT
		}
	if (strcmp ("subnet_mask_128" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->subnet_mask_128);
		FOUT
		}
	if (strcmp ("tc_timer_evhandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tc_timer_evhandle);
		FOUT
		}
	if (strcmp ("rte_calc_already_scheduled" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rte_calc_already_scheduled);
		FOUT
		}
	if (strcmp ("rte_table_calc_timer" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rte_table_calc_timer);
		FOUT
		}
	if (strcmp ("hello_processing_time_quantum" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_processing_time_quantum);
		FOUT
		}
	if (strcmp ("hello_processing_deferred_head_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_processing_deferred_head_ptr);
		FOUT
		}
	if (strcmp ("hello_processing_deferred_tail_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hello_processing_deferred_tail_ptr);
		FOUT
		}
	if (strcmp ("djk_hash_table_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->djk_hash_table_ptr);
		FOUT
		}
	if (strcmp ("prg_graph_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->prg_graph_ptr);
		FOUT
		}
	if (strcmp ("root_vertex_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->root_vertex_ptr);
		FOUT
		}
	if (strcmp ("my_process_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_process_id);
		FOUT
		}
	if (strcmp ("last_rte_calc_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_rte_calc_time);
		FOUT
		}
	if (strcmp ("djk_processing_time_quantum" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->djk_processing_time_quantum);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

