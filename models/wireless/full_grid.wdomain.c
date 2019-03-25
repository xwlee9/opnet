/* Wireless cell system model C form file: full_grid.wdomain.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
static const char full_grid_wdomain_c [] = "MIL_3_Tfile_Hdr_ 145A 85A op_mkso 7F 47C5041C 47C5041C 1 WTN09149 opnet 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcb 1                                                                                                                                                                                                                                                                                                                                                                                                          ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */


#include <string.h>
#include <math.h>

/* At the moment, only store the information used by the kernel.	*/
/* We do not pay attention to time.									*/
typedef struct
	{
	SimT_Wcluster_Info	info;
	} WdomainT_Full_Grid_Cell;

typedef struct
	{
	SimT_Objid	objid;
	double		lat_min, lat_max;
	double		long_min, long_max;
	double		lat_cell_size;
	double		long_cell_size;
	OpT_uInt32	lat_cell_number;	/* cells per row */
	OpT_uInt32	long_cell_number; 	/* number of columns */
	OpT_uInt32	dim1_multiplier;
	OpT_uInt32	dim2_multiplier;
	WdomainT_Full_Grid_Cell *	grid;
	} WdomainT_Full_Grid_State;

/* *** Global variables *** */
/* categorized memory */
Cmohandle	WdomainI_Full_Grid_Cmohandle = OPC_NIL;

/* *** Prototypes of local functions *** */
static WdomainT_Full_Grid_Cell *
grid_access (WdomainT_Full_Grid_State * wdomain_sptr, SimT_Wcluster_Id from_id, 
	SimT_Wcluster_Id to_id);

static void
wdomain_id_assign_subnet (WdomainT_Full_Grid_State * wdomain_sptr, 
	SimT_Objid subnet_id, int test_coverage);

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


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif

static void
wdomain_id_assign_node (WdomainT_Full_Grid_State * wdomain_sptr, SimT_Objid node_id)
	{
	unsigned int	i, j, num_mod, num_ch;
	SimT_Objid		mod_id;
	SimT_Objid		cmp_id;
	SimT_Objid		ch_id;

	/** Look for radio modules and assign the cell id to their channels **/
	FIN (wdomain_id_assign_node)

	num_mod = op_topo_child_count (node_id, OPC_OBJTYPE_RATX);
	for (i = 0; i < num_mod; i++)
		{
		mod_id = op_topo_child (node_id, OPC_OBJTYPE_RATX, i);
		cmp_id = op_topo_child (mod_id, OPC_OBJTYPE_COMP, 0);
		num_ch = op_topo_child_count (cmp_id, OPC_OBJTYPE_RATXCH);
		for (j = 0; j < num_ch; j++)
			{
			ch_id = op_topo_child (cmp_id, OPC_OBJTYPE_RATXCH, j);
			op_ima_obj_attr_set (ch_id, "wireless domain", wdomain_sptr->objid);
			}
		}
	num_mod = op_topo_child_count (node_id, OPC_OBJTYPE_RARX);
	for (i = 0; i < num_mod; i++)
		{
		mod_id = op_topo_child (node_id, OPC_OBJTYPE_RARX, i);
		cmp_id = op_topo_child (mod_id, OPC_OBJTYPE_COMP, 0);
		num_ch = op_topo_child_count (cmp_id, OPC_OBJTYPE_RARXCH);
		for (j = 0; j < num_ch; j++)
			{
			ch_id = op_topo_child (cmp_id, OPC_OBJTYPE_RARXCH, j);
			op_ima_obj_attr_set (ch_id, "wireless domain", wdomain_sptr->objid);
			}
		}

	FOUT
	}

static int
check_overlap (double mina, double maxa, double minb, double maxb)
	{
	/** Figures out if there is overlap between [mina,maxa] and 	**/
	/** [minb, maxb]												**/
	FIN (check_overlap)

	/* [a,A] [b,B] */
	if (maxa <= minb) 
		FRET (OPC_FALSE)
	/* [b,B] [a,A] */
	if (mina >= maxb) 
		FRET (OPC_FALSE)

	/* Anything else implies that there is some form of overlap */
	FRET (OPC_TRUE)
	}

static void
wdomain_id_assign_subnet (WdomainT_Full_Grid_State * wdomain_sptr, 
	SimT_Objid subnet_id, int test_coverage)
	{
	unsigned int	i, size;
	SimT_Objid		objid;
	double			x, y, xspan, yspan;

	/** Assigning cell system id to radio channels of nodes in subnet 	**/
	/** (if within range) 												**/
	/** if 'test_coverage' is OPC_FALSE, always assign to channel.		**/
	FIN (wdomain_id_assign_subnet);

	/* Go through child subnets, recurse if covered by grid or mobile 	*/
	/* (the latter kind might move into the grid)						*/
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_FIX);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_FIX, i);
		if (test_coverage)
			{
			op_ima_obj_attr_get (objid, "x position", &x);
			op_ima_obj_attr_get (objid, "y position", &y);
			op_ima_obj_attr_get (objid, "x span", &xspan);
			op_ima_obj_attr_get (objid, "y span", &yspan);
			if (check_overlap (wdomain_sptr->long_min, wdomain_sptr->long_max,
					x-xspan/2, x+xspan/2) &&
				check_overlap (wdomain_sptr->lat_min, wdomain_sptr->lat_max,
					y-yspan/2, y+yspan/2))
				{
				/* Recurse */
				wdomain_id_assign_subnet (wdomain_sptr, objid, test_coverage);
				}
			}
		else
			/* some parent was a moving subnet */
			wdomain_id_assign_subnet (wdomain_sptr, objid, test_coverage);
		}

	/* mobile and sat subnets will set the info to all their radio channels	*/
	/* since these might move into the grid coverage.						*/
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_MOB);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_MOB, i);
		wdomain_id_assign_subnet (wdomain_sptr, objid, OPC_FALSE);
		}
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_SAT);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_SAT, i);
		wdomain_id_assign_subnet (wdomain_sptr, objid, OPC_FALSE);
		}

	/* Ditto for nodes */
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDFIX);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDFIX, i);
		if (test_coverage)
			{
			op_ima_obj_attr_get (objid, "x position", &x);
			op_ima_obj_attr_get (objid, "y position", &y);
			if ((wdomain_sptr->long_min <= x) &&
				(wdomain_sptr->long_max >= x) &&
				(wdomain_sptr->lat_min <= y) &&
				(wdomain_sptr->lat_max >= y))
				{
				/* Check for radio modules */
				wdomain_id_assign_node (wdomain_sptr, objid);
				}
			}
		else
			/* some parent was a moving subnet */
			wdomain_id_assign_node (wdomain_sptr, objid);
		}

	/* mobile and sat nodes will set the info to all their radio channels	*/
	/* since these might move into the grid coverage.						*/
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDMOB);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDMOB, i);
		wdomain_id_assign_node (wdomain_sptr, objid);
		}
	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDSAT);
	for (i = 0; i < size; i++)
		{
		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDSAT, i);
		wdomain_id_assign_node (wdomain_sptr, objid);
		}

	FOUT
	}

static WdomainT_Full_Grid_Cell *
grid_access (WdomainT_Full_Grid_State * wdomain_sptr, SimT_Wcluster_Id from_id, 
	SimT_Wcluster_Id to_id)
	{
	OpT_uInt32	from_lat, from_long, to_lat, to_long;
	OpT_uInt32	index;

	/** Return the cache element corresponding to the exchange from	**/
	/** cell 'from_id' to cell 'to_id'.								**/
	FIN (grid_access);

	from_lat = from_id >> 32;
	from_long = from_id & (0xFFFFFFFF);
	to_lat = to_id >> 32;
	to_long = to_id & (0xFFFFFFFF);

	/* grid [from_lat][from_log][to_lat][to_long] */
	index = from_lat * wdomain_sptr->dim1_multiplier +
		from_long * wdomain_sptr->dim2_multiplier +
		to_lat * wdomain_sptr->lat_cell_number +
		to_long;

	FRET (&(wdomain_sptr->grid[index]))
	}

/* End of Function Block */

/* Init Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
void *
full_grid_init(OP_SIM_CONTEXT_ARG_OPT_COMMA Objid wdomain_id)
	{
	WdomainT_Full_Grid_State *	wdomain_sptr;
	double				lat_center, lat_span;
	double				long_center, long_span;

	/** Create state information based on attributes.					**/
	FIN_MT (full_grid_init (wdomain_id));

	/* Use categorized memory for clear reporting in mem dump	*/
	/* We will use that for both the state and the grid, so		*/
	/* can't use a pool.										*/
	if (WdomainI_Full_Grid_Cmohandle == OPC_NIL)
		{
		WdomainI_Full_Grid_Cmohandle = 
			prg_cmo_define ("Full wireless grid state");
		}
	wdomain_sptr = (WdomainT_Full_Grid_State *)
		prg_cmo_alloc (WdomainI_Full_Grid_Cmohandle, 
			sizeof (WdomainT_Full_Grid_State));
	if (wdomain_sptr == OPC_NIL)
		op_sim_end ("Unable to allocate wireless domain structure", "", "", "");

	wdomain_sptr->objid = wdomain_id;

	/* The attribute validity will be taken care of by the editor	*/
	/* This means max > min, cluster_size <= cell_number */
	op_ima_obj_attr_get (wdomain_id, "y position", &lat_center);
	op_ima_obj_attr_get (wdomain_id, "y span", &lat_span);
	wdomain_sptr->lat_min = lat_center - (lat_span / 2);
	wdomain_sptr->lat_max = lat_center + (lat_span / 2);
	op_ima_obj_attr_get (wdomain_id, "y cell number", &wdomain_sptr->lat_cell_number);
	wdomain_sptr->lat_cell_size = lat_span / wdomain_sptr->lat_cell_number;

	op_ima_obj_attr_get (wdomain_id, "x position", &long_center);
	op_ima_obj_attr_get (wdomain_id, "x span", &long_span);
	wdomain_sptr->long_min = long_center - (long_span / 2);
	wdomain_sptr->long_max = long_center + (long_span / 2);
	op_ima_obj_attr_get (wdomain_id, "x cell number", &wdomain_sptr->long_cell_number);
	wdomain_sptr->long_cell_size = long_span / wdomain_sptr->long_cell_number;

	wdomain_sptr->dim2_multiplier = wdomain_sptr->long_cell_number * wdomain_sptr->lat_cell_number;
	wdomain_sptr->dim1_multiplier = wdomain_sptr->dim2_multiplier * wdomain_sptr->long_cell_number;

	/* Allocate an array of <lat#>*<long#>*<lat#>*<long#> 	*/
	/* for all possible pairs.								*/
	wdomain_sptr->grid = (WdomainT_Full_Grid_Cell *)
		prg_cmo_alloc (WdomainI_Full_Grid_Cmohandle,
			wdomain_sptr->lat_cell_number * 
			wdomain_sptr->long_cell_number *
			wdomain_sptr->lat_cell_number * 
			wdomain_sptr->long_cell_number *
			sizeof (WdomainT_Full_Grid_Cell));
	if (wdomain_sptr->grid == OPC_NIL)
		{
		char	msg [200];
		sprintf (msg, 
			"Wireless domain %d: Unable to allocate grid, caching will not take place", 
			wdomain_id);
		op_sim_message (msg, "");
		}
	else
		{
		/* Iterate through nodes in the covered area and set the radio 		*/
		/* channel's 'wireless cell system' attributes to this system's id	*/
		wdomain_id_assign_subnet (wdomain_sptr, 0, OPC_TRUE);
		}

	FRET (wdomain_sptr)
	}

/* Member Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
int
full_grid_member(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr, Objid channel_id, Objid site_id,
	SimT_Wcluster_Id * id_ptr)	
	{
	WdomainT_Full_Grid_State *	wdomain_sptr;
	double						site_lat, site_long, dummy;

	/** Check if the site falls inside the grid.			**/
	/** If so, return the appropriate cluster/cell info		**/
	/** Otherwise return the non-membership value (-1)		**/
	FIN_MT (full_grid_member (wdomain_state_ptr, channel_id, site_id, id_ptr));

	wdomain_sptr = (WdomainT_Full_Grid_State *)wdomain_state_ptr;
	op_ima_obj_pos_get (site_id, &site_lat, &site_long, &dummy,
		&dummy, &dummy, &dummy);
	if ((wdomain_sptr->lat_min <= site_lat) &&
		(wdomain_sptr->lat_max >= site_lat) &&
		(wdomain_sptr->long_min <= site_long) &&
		(wdomain_sptr->long_max >= site_long))
		{
		/* Inside the grid, compute id */
		OpT_uInt64	lat_id, long_id;
		lat_id = floor ((site_lat - wdomain_sptr->lat_min) / wdomain_sptr->lat_cell_size);
		long_id = floor ((site_long - wdomain_sptr->long_min) / wdomain_sptr->long_cell_size);
		*id_ptr = lat_id << 32 | long_id;
		FRET (OPC_TRUE);
		}

	/* Outside the grid */
	FRET (OPC_FALSE);
	}

/* Info_Get Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
full_grid_info_get(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr, unsigned int request_flags,	
	SimT_Wcluster_Id from_cell_id, SimT_Wcluster_Id to_cell_id,
	SimT_Wcluster_Info * info_ptr, void ** continuation_ptr)
	{
	WdomainT_Full_Grid_State * wdomain_sptr;
	WdomainT_Full_Grid_Cell  * grid_info;

	/** Given the two cells, fill in the associated info.			**/
	FIN_MT (full_grid_info_get (wdomain_state_ptr, request_flags, from_cell_id,
		to_cell_id, info_ptr, continuation_ptr));

	wdomain_sptr = (WdomainT_Full_Grid_State *)wdomain_state_ptr;
	grid_info = grid_access (wdomain_sptr, from_cell_id, to_cell_id);
	memcpy (info_ptr, &grid_info->info, sizeof (SimT_Wcluster_Info));

	/* If there was no cached info, then return the address of the	*/
	/* cache in the continuation pointer for direct access in the	*/
	/* info set callbacks.											*/
	*continuation_ptr = (void *)grid_info;

	FRET (OPC_COMPCODE_SUCCESS)
	}

/* Info_Set Callback */
/* You can rename the arguments but leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
full_grid_info_set(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr, 
	SimT_Wcluster_Id from_cell_id, SimT_Wcluster_Id to_cell_id,
	SimT_Wcluster_Info * info_ptr, void ** continuation_ptr)
	{
	WdomainT_Full_Grid_Cell * grid_info;

	FIN_MT (full_grid_info_set(wdomain_state_ptr, from_cell_id, to_cell_id,
		info_ptr, continuation_ptr));

	if (*continuation_ptr != OPC_NIL)
		grid_info = (WdomainT_Full_Grid_Cell *)(*continuation_ptr);
	else
		{
		/* Search for info using the from/to ids */
		WdomainT_Full_Grid_State * wdomain_sptr = 
			(WdomainT_Full_Grid_State *)wdomain_state_ptr;
		grid_info = grid_access (wdomain_sptr, from_cell_id, to_cell_id);
		*continuation_ptr = (void *)grid_info;
		}
	
	/* Update values based on specified info */
	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_CLOSURE)
		{
		grid_info->info.closure = info_ptr->closure;
		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_CLOSURE;
		}
	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_CHANMATCH)
		{
		grid_info->info.channel_match = info_ptr->channel_match;
		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_CHANMATCH;
		}
	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_PROPDEL)
		{
		grid_info->info.start_propagation_delay = info_ptr->start_propagation_delay;
		grid_info->info.end_propagation_delay = info_ptr->end_propagation_delay;
		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_PROPDEL;
		}
	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_PATHLOSS)
		{
		grid_info->info.path_loss = info_ptr->path_loss;
		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_PATHLOSS;
		}

	FRET (OPC_COMPCODE_SUCCESS)
	}

/* Auxilliary Procedure Callback */
/* Please leave the special function name as is */
#if defined (__cplusplus)
extern "C"
#endif
Compcode
full_grid_auxproc(OP_SIM_CONTEXT_ARG_OPT_COMMA void * wdomain_state_ptr,
	OpT_Int64 code,  /* arbitrary code value */
	va_list args)    /* variable argument list, use va_arg to access elements */
	{
	FIN_MT (full_grid_auxproc(wdomain_state_ptr, code, args));
	FRET (OPC_COMPCODE_SUCCESS);
	}
