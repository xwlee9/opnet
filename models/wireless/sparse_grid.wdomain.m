MIL_3_Tfile_Hdr_ 90A 85A opnet 16 3CFA651D 3D79178A 18 jbemba opnet 0 0 none none 0 0 none 51550F8B 5D28 0 0 0 0                                                                                                                                                                                                                                                                                                                                                                                                                      HThe lat and long cell numbers will define the overall array size. But it   Dwould be very wasteful to allocate all of the cells if the sites are   Aknown to be grouped in a few locations (e.g., we are dealing with   EFlorida where land-based sites will be in the "northern" or "eastern"   Bparts of thearray.  And even if the entire array is possibly used,   /is a good chance that grouping will take place.   CTo deal with this, the model allows the definition of a group size.   EWhen a cell is needed, only the group it belongs to will be allocated   instead of the entire array.   AThis means that accessing a cell will require twice as many array   Coperations but that's still O(1) and can mean a large space saving.       The grid cell id will be:   /<lat_portion [32 bits]><long_portion [32 bits]>   Jwith each portion made up of a group index and a cell index in that group.   DE.g.: a 6x6 space with grouping of lat 2 and long 3 could look like:   9	<lat group | index in group,long group | index in group>       7	<0|0,0|0><0|0,0|1><0|0,0|2><0|0,1|0><0|0,1|1><0|0,1|2>   7	<0|1,0|0><0|1,0|1><0|1,0|2><0|1,1|0><0|1,1|1><0|1,1|2>   7	<1|0,0|0><1|0,0|1><1|0,0|2><1|0,1|0><1|0,1|1><1|0,1|2>   7	<1|1,0|0><1|1,0|1><1|1,0|2><1|1,1|0><1|1,1|1><1|1,1|2>   7	<2|0,0|0><2|0,0|1><2|0,0|2><2|0,1|0><2|0,1|1><2|0,1|2>   8	N<2|1,0|0><2|1,0|1><2|1,0|2><2|1,1|0><2|1,1|1><2|1,1|2>       BOf course, there are really two levels of arrays:  The first array   Frepresents the potential sending cells.  Each sending cell has its own   Csubarray to store the information from that cell to all the others.              x cell number    �������    ����         
����         ����          ����             $Numer of cells in the x (longitude)    
direction.   y cell number    �������    ����         
����         ����          ����             $Number of cells in the Y (latitude)    
direction.   x group size    �������    ����      ����   Entire span         ����          ����         Entire span   ��������         &Number of cells in a clusters.  Cells    $are allocated one cluster at a time.   "The smaller the cluster, the more    sparse the array can be.   'The special value 'Entire span' can be    'used to indicate that all cells are in    a single cluster.   y group size    �������    ����      ����   Entire span         ����          ����         Entire span   ��������         &Number of cells in a clusters.  Cells    $are allocated one cluster at a time.   "The smaller the cluster, the more    sparse the array can be.   'The special value 'Entire span' can be    'used to indicate that all cells are in    a single cluster.                   	   chanmatch model            NONE      closure model            NONE      doc file         
   wdomain   
   	icon name            wcell      outline color   
         ����      propdel model            NONE      tooltip         
   Wireless Domain   
   x span         
        ����   
   y span         
        ����   
   1       #include <string.h>   #include <math.h>       =/* See model comments for details about the grid structure */       C/* At the moment, only store the information used by the kernel.	*/   ./* We do not pay attention to time.									*/   typedef struct   	{   	SimT_Wcluster_Info	info;   $	} WdomainT_Group_Grid_Long_Cell_To;       Ntypedef WdomainT_Group_Grid_Long_Cell_To *		WdomainT_Group_Grid_Long_Group_To;   Ltypedef WdomainT_Group_Grid_Long_Group_To *	WdomainT_Group_Grid_Lat_Cell_To;   Ltypedef WdomainT_Group_Grid_Lat_Cell_To *		WdomainT_Group_Grid_Lat_Group_To;   Otypedef WdomainT_Group_Grid_Lat_Group_To * 	WdomainT_Group_Grid_Long_Cell_From;   Qtypedef WdomainT_Group_Grid_Long_Cell_From *	WdomainT_Group_Grid_Long_Group_From;   Ptypedef WdomainT_Group_Grid_Long_Group_From *	WdomainT_Group_Grid_Lat_Cell_From;   Ptypedef WdomainT_Group_Grid_Lat_Cell_From *		WdomainT_Group_Grid_Lat_Group_From;       typedef struct   	{   	SimT_Objid	objid;   	double		lat_min, lat_max;   	double		lat_cell_width;   	double		long_min, long_max;   	double		long_cell_width;   	OpT_uInt32	lat_group_number;   0	OpT_uInt32	lat_cell_number;	/* cells per row */   2	OpT_uInt32	lat_group_size;		/* cells per group */   	OpT_uInt32	long_group_number;   6	OpT_uInt32	long_cell_number; 	/* number of columns */   2	OpT_uInt32	long_group_size;	/* cells per group */   +	WdomainT_Group_Grid_Lat_Group_From *	grid;   	} WdomainT_Group_Grid_State;       /* *** Global variables *** */   /* categorized memory */   2Cmohandle	WdomainI_Group_Grid_Cmohandle = OPC_NIL;       +/* *** Prototypes of local functions *** */   )static WdomainT_Group_Grid_Long_Cell_To *   7grid_access (WdomainT_Group_Grid_State * wdomain_sptr,    3	SimT_Wcluster_Id from_id, SimT_Wcluster_Id to_id);       static void   Dwdomain_id_assign_subnet (WdomainT_Group_Grid_State * wdomain_sptr,    *	SimT_Objid subnet_id, int test_coverage);     static void   Uwdomain_id_assign_node (WdomainT_Group_Grid_State * wdomain_sptr, SimT_Objid node_id)   	{   $	unsigned int	i, j, num_mod, num_ch;   	SimT_Objid		mod_id;   	SimT_Objid		cmp_id;   	SimT_Objid		ch_id;       H	/** Look for radio modules and assign the cell id to their channels **/   	FIN (wdomain_id_assign_node)       ;	num_mod = op_topo_child_count (node_id, OPC_OBJTYPE_RATX);   	for (i = 0; i < num_mod; i++)   		{   8		mod_id = op_topo_child (node_id, OPC_OBJTYPE_RATX, i);   7		cmp_id = op_topo_child (mod_id, OPC_OBJTYPE_COMP, 0);   <		num_ch = op_topo_child_count (cmp_id, OPC_OBJTYPE_RATXCH);   		for (j = 0; j < num_ch; j++)   			{   9			ch_id = op_topo_child (cmp_id, OPC_OBJTYPE_RATXCH, j);   G			op_ima_obj_attr_set (ch_id, "wireless domain", wdomain_sptr->objid);   			}   		}   ;	num_mod = op_topo_child_count (node_id, OPC_OBJTYPE_RARX);   	for (i = 0; i < num_mod; i++)   		{   8		mod_id = op_topo_child (node_id, OPC_OBJTYPE_RARX, i);   7		cmp_id = op_topo_child (mod_id, OPC_OBJTYPE_COMP, 0);   <		num_ch = op_topo_child_count (cmp_id, OPC_OBJTYPE_RARXCH);   		for (j = 0; j < num_ch; j++)   			{   9			ch_id = op_topo_child (cmp_id, OPC_OBJTYPE_RARXCH, j);   G			op_ima_obj_attr_set (ch_id, "wireless domain", wdomain_sptr->objid);   			}   		}       	FOUT   	}       
static int   Bcheck_overlap (double mina, double maxa, double minb, double maxb)   	{   A	/** Figures out if there is overlap between [mina,maxa] and 	**/    	/** [minb, maxb]												**/   	FIN (check_overlap)       	/* [a,A] [b,B] */   	if (maxa <= minb)    		FRET (OPC_FALSE)   	/* [b,B] [a,A] */   	if (mina >= maxb)    		FRET (OPC_FALSE)       ?	/* Anything else implies that there is some form of overlap */   	FRET (OPC_TRUE)   	}       static void   Ywdomain_id_assign_subnet (WdomainT_Group_Grid_State * wdomain_sptr, SimT_Objid subnet_id,   	int test_coverage)   	{   	unsigned int	i, size;   	SimT_Objid		objid;   	double			x, y, xspan, yspan;       G	/** Assigning cell system id to radio channels of nodes in subnet 	**/   &	/** (if within range) 												**/   D	/** if 'test_coverage' is OPC_FALSE, always assign to channel.		**/    	FIN (wdomain_id_assign_subnet);       F	/* Go through child subnets, recurse if covered by grid or mobile 	*/   6	/* (the latter kind might move into the grid)						*/   @	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_FIX);   	for (i = 0; i < size; i++)   		{   ?		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_FIX, i);   		if (test_coverage)   			{   1			op_ima_obj_attr_get (objid, "x position", &x);   1			op_ima_obj_attr_get (objid, "y position", &y);   1			op_ima_obj_attr_get (objid, "x span", &xspan);   1			op_ima_obj_attr_get (objid, "y span", &yspan);   E			if (check_overlap (wdomain_sptr->long_min, wdomain_sptr->long_max,   					x-xspan/2, x+xspan/2) &&   @				check_overlap (wdomain_sptr->lat_min, wdomain_sptr->lat_max,   					y-yspan/2, y+yspan/2))   				{   				/* Recurse */   B				wdomain_id_assign_subnet (wdomain_sptr, objid, test_coverage);   				}   			}   		else   (			/* some parent was a moving subnet */   A			wdomain_id_assign_subnet (wdomain_sptr, objid, test_coverage);   		}       K	/* mobile and sat subnets will set the info to all their radio channels	*/   :	/* since these might move into the grid coverage.						*/   @	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_MOB);   	for (i = 0; i < size; i++)   		{   ?		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_MOB, i);   <		wdomain_id_assign_subnet (wdomain_sptr, objid, OPC_FALSE);   		}   @	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_SUBNET_SAT);   	for (i = 0; i < size; i++)   		{   ?		objid = op_topo_child (subnet_id, OPC_OBJTYPE_SUBNET_SAT, i);   <		wdomain_id_assign_subnet (wdomain_sptr, objid, OPC_FALSE);   		}       	/* Ditto for nodes */   ;	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDFIX);   	for (i = 0; i < size; i++)   		{   :		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDFIX, i);   		if (test_coverage)   			{   1			op_ima_obj_attr_get (objid, "x position", &x);   1			op_ima_obj_attr_get (objid, "y position", &y);   '			if ((wdomain_sptr->long_min <= x) &&   $				(wdomain_sptr->long_max >= x) &&   #				(wdomain_sptr->lat_min <= y) &&   !				(wdomain_sptr->lat_max >= y))   				{   !				/* Check for radio modules */   1				wdomain_id_assign_node (wdomain_sptr, objid);   				}   			}   		else   (			/* some parent was a moving subnet */   0			wdomain_id_assign_node (wdomain_sptr, objid);   		}       I	/* mobile and sat nodes will set the info to all their radio channels	*/   :	/* since these might move into the grid coverage.						*/   ;	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDMOB);   	for (i = 0; i < size; i++)   		{   :		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDMOB, i);   /		wdomain_id_assign_node (wdomain_sptr, objid);   		}   ;	size = op_topo_child_count (subnet_id, OPC_OBJTYPE_NDSAT);   	for (i = 0; i < size; i++)   		{   :		objid = op_topo_child (subnet_id, OPC_OBJTYPE_NDSAT, i);   /		wdomain_id_assign_node (wdomain_sptr, objid);   		}       	FOUT   	}       )static WdomainT_Group_Grid_Long_Cell_To *   7grid_access (WdomainT_Group_Grid_State * wdomain_sptr,    2	SimT_Wcluster_Id from_id, SimT_Wcluster_Id to_id)   	{   	OpT_uInt32						id_part;   &	OpT_uInt32						lat_group_index_from;   %	OpT_uInt32						lat_cell_index_from;   '	OpT_uInt32						long_group_index_from;   &	OpT_uInt32						long_cell_index_from;   $	OpT_uInt32						lat_group_index_to;   #	OpT_uInt32						lat_cell_index_to;   %	OpT_uInt32						long_group_index_to;   $	OpT_uInt32						long_cell_index_to;   2	WdomainT_Group_Grid_Long_Group_To 	long_group_to;   /	WdomainT_Group_Grid_Lat_Cell_To 		lat_cell_to;   1	WdomainT_Group_Grid_Lat_Group_To  	lat_group_to;   5	WdomainT_Group_Grid_Long_Cell_From 		long_cell_from;   6	WdomainT_Group_Grid_Long_Group_From 	long_group_from;   3	WdomainT_Group_Grid_Lat_Cell_From 		lat_cell_from;   5	WdomainT_Group_Grid_Lat_Group_From  	lat_group_from;       D	/** Return the cache element corresponding to the exchange from	**/   /	/** cell 'from_id' to cell 'to_id'.								**/   B	/** If this information doesn't exist yet, the cache entry is	**/   	/** created. 													**/   :	/** If the information is obsolete, it is updated.				**/   7	FIN (grid_access (wdomain_state_ptr, from_id, to_id));       $	/* Step 1) latitude of from cell */   	id_part = from_id >> 32;   ?	lat_group_index_from = id_part / wdomain_sptr->lat_group_size;   <	lat_group_from = wdomain_sptr->grid [lat_group_index_from];   	if (lat_group_from == OPC_NIL)   		{   C		/* Allocate new lat group, i.e., an array of lat cell pointers */   7		lat_group_from = (WdomainT_Group_Grid_Lat_Group_From)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   "				wdomain_sptr->lat_group_size *   0				sizeof (WdomainT_Group_Grid_Lat_Cell_From));   =		wdomain_sptr->grid [lat_group_index_from] = lat_group_from;   		}       >	lat_cell_index_from = id_part % wdomain_sptr->lat_group_size;   6	lat_cell_from = lat_group_from [lat_cell_index_from];   	if (lat_cell_from == OPC_NIL)   		{   C		/* Allocate new lat cell, i.e., an array of long group pointer */   5		lat_cell_from = (WdomainT_Group_Grid_Lat_Cell_From)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   %				wdomain_sptr->long_group_number *   2				sizeof (WdomainT_Group_Grid_Long_Group_From));   7		lat_group_from [lat_cell_index_from] = lat_cell_from;   		}       %	/* Step 2) longitude of from cell */   "	id_part = from_id & (0xFFFFFFFF);   A	long_group_index_from = id_part / wdomain_sptr->long_group_size;   9	long_group_from = lat_cell_from [long_group_index_from];    	if (long_group_from == OPC_NIL)   		{   E		/* Allocate new long group, i.e., an array of long cell pointers */   9		long_group_from = (WdomainT_Group_Grid_Long_Group_From)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   #				wdomain_sptr->long_group_size *   1				sizeof (WdomainT_Group_Grid_Long_Cell_From));   :		lat_cell_from [long_group_index_from] = long_group_from;   		}       @	long_cell_index_from = id_part % wdomain_sptr->long_group_size;   9	long_cell_from = long_group_from [long_cell_index_from];   	if (long_cell_from == OPC_NIL)   		{   G		/* Allocate new from cell, i.e., an array of lat groups for to grid*/   7		long_cell_from = (WdomainT_Group_Grid_Long_Cell_From)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   $				wdomain_sptr->lat_group_number *   /				sizeof (WdomainT_Group_Grid_Lat_Group_To));   :		long_group_from [long_cell_index_from] = long_cell_from;   		}       "	/* Step 3) latitude of to cell */   	id_part = to_id >> 32;   =	lat_group_index_to = id_part / wdomain_sptr->lat_group_size;   4	lat_group_to = long_cell_from [lat_group_index_to];   	if (lat_group_to == OPC_NIL)   		{   C		/* Allocate new lat group, i.e., an array of lat cell pointers */   3		lat_group_to = (WdomainT_Group_Grid_Lat_Group_To)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   "				wdomain_sptr->lat_group_size *   0				sizeof (WdomainT_Group_Grid_Lat_Cell_To *));   5		long_cell_from [lat_group_index_to] = lat_group_to;   		}       <	lat_cell_index_to = id_part % wdomain_sptr->lat_group_size;   0	lat_cell_to = lat_group_to [lat_cell_index_to];   	if (lat_cell_to == OPC_NIL)   		{   F		/* Allocate new lat cell, i.e., an array of long group structures */   1		lat_cell_to = (WdomainT_Group_Grid_Lat_Cell_To)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   %				wdomain_sptr->long_group_number *   2				sizeof (WdomainT_Group_Grid_Long_Group_To *));   1		lat_group_to [lat_cell_index_to] = lat_cell_to;   		}       #	/* Step 4) longitude of to cell */    	id_part = to_id & (0xFFFFFFFF);   ?	long_group_index_to = id_part / wdomain_sptr->long_group_size;   3	long_group_to = lat_cell_to [long_group_index_to];   	if (long_group_to == OPC_NIL)   		{   E		/* Allocate new long group, i.e., an array of long cell pointers */   5		long_group_to = (WdomainT_Group_Grid_Long_Group_To)   0			prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   #				wdomain_sptr->long_group_size *   /				sizeof (WdomainT_Group_Grid_Long_Cell_To));   4		lat_cell_to [long_group_index_to] = long_group_to;   		}       >	long_cell_index_to = id_part % wdomain_sptr->long_group_size;   -	FRET (&(long_group_to [long_cell_index_to]))   	}   T   L/* You can rename the arguments but leave the special function name as is */   #if defined (__cplusplus)   
extern "C"   #endif   void *   !<init_callback>(Objid wdomain_id)   	{   *	WdomainT_Group_Grid_State *	wdomain_sptr;    	double				lat_center, lat_span;   "	double				long_center, long_span;   	OpT_Int32			size;       :	/** Create state information based on attributes.					**/   $	FIN (<init_callback> (wdomain_id));       =	/* Use categorized memory for clear reporting in mem dump	*/   <	/* We will use that for both the state and the grid, so		*/   !	/* can't use a pool.										*/   .	if (WdomainI_Group_Grid_Cmohandle == OPC_NIL)   		{   "		WdomainI_Group_Grid_Cmohandle =    2			prg_cmo_define ("Grouped wireless grid state");   		}       -	wdomain_sptr = (WdomainT_Group_Grid_State *)   /		prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   '			sizeof (WdomainT_Group_Grid_State));   	if (wdomain_sptr == OPC_NIL)   J		op_sim_end ("Unable to allocate wireless domain structure", "", "", "");       "	wdomain_sptr->objid = wdomain_id;       A	/* The attribute validity will be taken care of by the editor	*/   6	/* This means max > min, group_size <= cell_number */   =	op_ima_obj_attr_get (wdomain_id, "y position", &lat_center);   7	op_ima_obj_attr_get (wdomain_id, "y span", &lat_span);   5	wdomain_sptr->lat_min = lat_center - (lat_span / 2);   5	wdomain_sptr->lat_max = lat_center + (lat_span / 2);   S	op_ima_obj_attr_get (wdomain_id, "y cell number", &wdomain_sptr->lat_cell_number);   I	wdomain_sptr->lat_cell_width = lat_span / wdomain_sptr->lat_cell_number;   9	op_ima_obj_attr_get (wdomain_id, "y group size", &size);   	if (size == -1)   		{   ?		wdomain_sptr->lat_group_size = wdomain_sptr->lat_cell_number;   %		wdomain_sptr->lat_group_number = 1;   		}   	else   		{   2		wdomain_sptr->lat_group_size = (OpT_uInt32)size;   #		wdomain_sptr->lat_group_number =    _			ceil (((double)(wdomain_sptr->lat_cell_number)) / ((double)(wdomain_sptr->lat_group_size)));   		}       >	op_ima_obj_attr_get (wdomain_id, "x position", &long_center);   8	op_ima_obj_attr_get (wdomain_id, "x span", &long_span);   8	wdomain_sptr->long_min = long_center - (long_span / 2);   8	wdomain_sptr->long_max = long_center + (long_span / 2);   T	op_ima_obj_attr_get (wdomain_id, "x cell number", &wdomain_sptr->long_cell_number);   L	wdomain_sptr->long_cell_width = long_span / wdomain_sptr->long_cell_number;   9	op_ima_obj_attr_get (wdomain_id, "x group size", &size);   	if (size == -1)   		{   A		wdomain_sptr->long_group_size = wdomain_sptr->long_cell_number;   &		wdomain_sptr->long_group_number = 1;   		}   	else   		{   3		wdomain_sptr->long_group_size = (OpT_uInt32)size;   $		wdomain_sptr->long_group_number =    a			ceil (((double)(wdomain_sptr->long_cell_number)) / ((double)(wdomain_sptr->long_group_size)));   		}       :	/* Well start with at least an array of nil lat groups */   <	wdomain_sptr->grid = (WdomainT_Group_Grid_Lat_Group_From *)   /		prg_cmo_alloc (WdomainI_Group_Grid_Cmohandle,   $			wdomain_sptr->lat_group_number *    0			sizeof (WdomainT_Group_Grid_Lat_Group_From));       D	/* Iterate through nodes in the covered area and set the radio 		*/   F	/* channel's 'wireless cell system' attributes to this system's id	*/   6	wdomain_id_assign_subnet (wdomain_sptr, 0, OPC_TRUE);       	FRET (wdomain_sptr)   	}   #   L/* You can rename the arguments but leave the special function name as is */   #if defined (__cplusplus)   
extern "C"   #endif   int   L<member_callback>(void * wdomain_state_ptr, Objid channel_id, Objid site_id,   	SimT_Wcluster_Id * id_ptr)	   	{   *	WdomainT_Group_Grid_State *	wdomain_sptr;   &	double				site_lat, site_long, dummy;       3	/** Check if the site falls inside the grid.			**/   7	/** If so, return the appropriate group/cell info		**/   8	/** Otherwise return the non-membership value (-1)		**/   B	FIN (<member_callback> (wdomain_state_ptr, channel_id, site_id));       ?	wdomain_sptr = (WdomainT_Group_Grid_State *)wdomain_state_ptr;   <	op_ima_obj_pos_get (site_id, &site_lat, &site_long, &dummy,   		&dummy, &dummy, &dummy);   +	if ((wdomain_sptr->lat_min <= site_lat) &&   (		(wdomain_sptr->lat_max >= site_lat) &&   *		(wdomain_sptr->long_min <= site_long) &&   (		(wdomain_sptr->long_max >= site_long))   		{   #		/* Inside the grid, compute id */   		OpT_uInt64	lat_id, long_id;   U		lat_id = floor ((site_lat - wdomain_sptr->lat_min) / wdomain_sptr->lat_cell_width);   Y		long_id = floor ((site_long - wdomain_sptr->long_min) / wdomain_sptr->long_cell_width);   #		*id_ptr = lat_id << 32 | long_id;   		FRET (OPC_TRUE);   		}       	/* Outside the grid */   	FRET (OPC_FALSE)   	}      L/* You can rename the arguments but leave the special function name as is */   #if defined (__cplusplus)   
extern "C"   #endif   Compcode   J<info_get_callback>(void * wdomain_state_ptr, unsigned int request_flags,	   <	SimT_Wcluster_Id from_cell_id, SimT_Wcluster_Id to_cell_id,   9	SimT_Wcluster_Info * info_ptr, void ** continuation_ptr)   	{   *	WdomainT_Group_Grid_State *	wdomain_sptr;   .	WdomainT_Group_Grid_Long_Cell_To * grid_info;       <	/** Given the two cells, fill in the associated info.			**/   J	FIN (<info_get_callback> (wdomain_state_ptr, request_flags, from_cell_id,   +		to_cell_id, info_ptr, continuation_ptr));       ?	wdomain_sptr = (WdomainT_Group_Grid_State *)wdomain_state_ptr;   B	grid_info = grid_access (wdomain_sptr, from_cell_id, to_cell_id);   B	memcpy (info_ptr, &grid_info->info, sizeof (SimT_Wcluster_Info));       B	/* If there was no cached info, then return the address of the	*/   A	/* cache in the continuation pointer for direct access in the	*/   $	/* info set callbacks.											*/   '	*continuation_ptr = (void *)grid_info;       	FRET (OPC_COMPCODE_SUCCESS)   	}   2   L/* You can rename the arguments but leave the special function name as is */   #if defined (__cplusplus)   
extern "C"   #endif   Compcode   .<info_set_callback>(void * wdomain_state_ptr,    <	SimT_Wcluster_Id from_cell_id, SimT_Wcluster_Id to_cell_id,   9	SimT_Wcluster_Info * info_ptr, void ** continuation_ptr)   	{   .	WdomainT_Group_Grid_Long_Cell_To * grid_info;       F	FIN (<info_set_callback>(wdomain_state_ptr, from_cell_id, to_cell_id,   		info_ptr, continuation_ptr));       "	if (*continuation_ptr != OPC_NIL)   F		grid_info = (WdomainT_Group_Grid_Long_Cell_To *)(*continuation_ptr);   	else   		{   -		/* Search for info using the from/to ids */   +		WdomainT_Group_Grid_State * wdomain_sptr;   @		wdomain_sptr = (WdomainT_Group_Grid_State *)wdomain_state_ptr;   C		grid_info = grid_access (wdomain_sptr, from_cell_id, to_cell_id);   (		*continuation_ptr = (void *)grid_info;   		}   	   ,	/* Update values based on specified info */   8	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_CLOSURE)   		{   .		grid_info->info.closure = info_ptr->closure;   <		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_CLOSURE;   		}   :	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_CHANMATCH)   		{   :		grid_info->info.channel_match = info_ptr->channel_match;   >		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_CHANMATCH;   		}   8	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_PROPDEL)   		{   N		grid_info->info.start_propagation_delay = info_ptr->start_propagation_delay;   J		grid_info->info.end_propagation_delay = info_ptr->end_propagation_delay;   <		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_PROPDEL;   		}   9	if (info_ptr->valid_fields & OPC_WCLUSTER_INFO_PATHLOSS)   		{   2		grid_info->info.path_loss = info_ptr->path_loss;   =		grid_info->info.valid_fields |= OPC_WCLUSTER_INFO_PATHLOSS;   		}       	FRET (OPC_COMPCODE_SUCCESS)   	}      2/* Please leave the special function name as is */   #if defined (__cplusplus)   
extern "C"   #endif   void   <diagnostic_callback>(void)   	{   	}        