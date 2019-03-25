/* Process model C form file: random_waypoint.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char random_waypoint_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5C9240A2 5C9240A2 1 shen-Lab li 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <mobility_support.h>
#include <math.h>
#include <oms_ext_file_support.h>
#include <oms_sim_attr_cache.h>
#include <oms_tan.h>

/* CONSTANTS DEFINITION */
#define PAUSE  			0
#define	MOVE   			1
#define UPDATE 			2

#define	MIN_DISTANCE	1.0e-9

static void		mobility_random_waypoint_sv_init (void);
static void     mobility_random_waypoint_initialize_node_movement (void);
static void     mobility_random_waypoint_process_pause (MobilityT_Mobile_Node_Rec* mobile_node_ptr);
static void     mobility_random_waypoint_process_move (MobilityT_Mobile_Node_Rec* mobile_node_ptr);
static void     mobility_random_waypoint_process_update (MobilityT_Mobile_Node_Rec* mobile_node_ptr);
static void     mobility_random_waypoint_error (const char* str1, const char* str2, const char* str3);

EXTERN_C_BEGIN
static void     mobility_random_waypoint_process_intrpt (void* mobile_node_ptr, int code);
static void 	mobility_random_waypoint_end_processing (void * PRG_ARG_UNUSED (v_state_ptr), int PRG_ARG_UNUSED(code));
EXTERN_C_END


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
	Mobility_Profile_Desc*	 		profile_ptr                                     ;	/* The Mobility Mgr ivkoes and sends this Profile description pointer  */
	Objid	                  		my_objid                                        ;	/* Module Objid  */
	Prohandle	              		my_prohandle                                    ;	/* Module Process handle  */
	OmsT_Dist_Handle	       		start_time_dist                                 ;	/* The start time of the mobility model  */
	double	                 		stop_time                                       ;	/* The stop time of the mobility model  */
	double	                 		xmin                                            ;	/* The min value of x dimension of the simulation area  */
	double	                 		xmax                                            ;	/* The max value of x dimension of the simulation area  */
	double	                 		ymin                                            ;	/* The min value of y dimension of the simulation area  */
	double	                 		ymax                                            ;	/* The max value of y dimension of the simulation area  */
	OmsT_Dist_Handle	       		speed_dist                                      ;	/* The distribution handle for speed  */
	OmsT_Dist_Handle	       		pause_time_dist                                 ;	/* The distribution handle for pause time  */
	Boolean	                		trace_active                                    ;	/* Debugging flag for all activities in this process.  */
	double	                 		update_freq                                     ;	/* How often we should move the node  */
	Boolean	                		use_mobility_domain                             ;	/* If it is true, then consider the dimensions      */
	                        		                                                	/* of the mobility domain, otherwise, consider the  */
	                        		                                                	/* coordinates user entered.                        */
	Boolean	                		record_trajectory                               ;	/* Flag indicating whether the locations of the  */
	                        		                                                	/* nodes will be recorded into trajectory files  */
	                        		                                                	/* during the random movement.                   */
	} random_waypoint_state;

#define profile_ptr             		op_sv_ptr->profile_ptr
#define my_objid                		op_sv_ptr->my_objid
#define my_prohandle            		op_sv_ptr->my_prohandle
#define start_time_dist         		op_sv_ptr->start_time_dist
#define stop_time               		op_sv_ptr->stop_time
#define xmin                    		op_sv_ptr->xmin
#define xmax                    		op_sv_ptr->xmax
#define ymin                    		op_sv_ptr->ymin
#define ymax                    		op_sv_ptr->ymax
#define speed_dist              		op_sv_ptr->speed_dist
#define pause_time_dist         		op_sv_ptr->pause_time_dist
#define trace_active            		op_sv_ptr->trace_active
#define update_freq             		op_sv_ptr->update_freq
#define use_mobility_domain     		op_sv_ptr->use_mobility_domain
#define record_trajectory       		op_sv_ptr->record_trajectory

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	random_waypoint_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((random_waypoint_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

static void
mobility_random_waypoint_sv_init (void)
	{
	Mobility_Random_Waypoint_Desc*  RW_ptr = OPC_NIL;

	/** 1. Initializes the state variables.	             		**/
	/** 2. Access the memory from Parent process.         		**/
	/** 3. Assign the Random Waypoint state variables.    		**/
	FIN (mobility_random_waypoint_sv_init ());

	/* Obtain the module Objid.									*/
	my_objid = op_id_self ();

	/* Obtain own process handle. 								*/
	my_prohandle = op_pro_self ();

	/* Get the Random Waypoint profile from the Parent Process.	*/
	profile_ptr = (Mobility_Profile_Desc *) op_pro_argmem_access ();

	/* Assign the state variables for the Random Waypoint		*/
	/* mobility.												*/
	RW_ptr      	= (Mobility_Random_Waypoint_Desc *) profile_ptr->Mobility_Desc_ptr;	
	start_time_dist = RW_ptr->Start_Time;
	stop_time   	= RW_ptr->Stop_Time;
	speed_dist      = RW_ptr->Speed;
	pause_time_dist = RW_ptr->Pause_Time;
	update_freq 	= RW_ptr->Update_Freq;
	
	xmin        	= RW_ptr->X_min;
	xmax       	 	= RW_ptr->X_max;
	ymin        	= RW_ptr->Y_min;
	ymax        	= RW_ptr->Y_max;

	/* Set the operational flags based on related configuration.*/	
	if ((RW_ptr->Mobility_Domain != OPC_NIL) && strcmp (RW_ptr->Mobility_Domain, "Not Used") != 0) 
		use_mobility_domain = OPC_TRUE;
	else
		use_mobility_domain = OPC_FALSE;


	if (RW_ptr->Record_Trajectory)
		{
		/* Schedule an interrupt for the end of simulation.		*/
		op_intrpt_schedule_call (OPC_INTRPT_SCHED_CALL_ENDSIM, 0, mobility_random_waypoint_end_processing, NULL);
		
		/* Set the flag for trajectory recording.				*/
		record_trajectory = OPC_TRUE;
		}
	else
		record_trajectory = OPC_FALSE;
	
	FOUT;
	}


static void
mobility_random_waypoint_initialize_node_movement (void)
	{
	int								i, node_count;
	MobilityT_Mobile_Node_Rec*		mobile_node_ptr = OPC_NIL;
	List*							node_list_ptr;
	double							start_time;	
	Objid         					subnet_id;
	double        					long_offset_max = 0.0, lat_offset_max = 0.0;
	double        					long_offset_min = 0.0, lat_offset_min = 0.0;
	double        					sub_longitude_min, sub_latitude_max;
	double        					sub_latitude, sub_longitude, sub_altitude, sub_x_pos, sub_y_pos, sub_z_pos;
	double        					sub_x_span, sub_y_span;
	double							long_distance, lat_distance;
	OpT_Ima_Obj_Position_Unit_Type	sub_span_unit;
	PrgT_Distance_Units_Type		sub_span_unit_prgc;
	char							node_hname [OMSC_HNAME_MAX_LEN];
	
	/** This function initializes the random waypoint movement  **/
	/** for each node. Then it calls the function starting the  **/
	/** random waypoint mobility.                               **/
	FIN (mobility_random_waypoint_initialize_node_movement (void));
	
	/* Check if debugging is enabled.				       		*/
	trace_active = op_prg_odb_ltrace_active ("random_waypoint");
   
	/* Get the list of node ids for this profile.               */
	node_list_ptr = profile_ptr->Mobile_entity_id_list;
	node_count    = op_prg_list_size (node_list_ptr);
	
	/* Loop through each node and schedule a call for the       */
	/* coresponding node.		                                */
	for (i = 0; i < node_count; i++)
		{
		/* Access the mobile node's record.						*/
		mobile_node_ptr = (MobilityT_Mobile_Node_Rec *) op_prg_list_access (node_list_ptr, i);
		mobile_node_ptr->movement_area_ptr = (MobilityT_Movement_Area *) op_prg_mem_alloc (sizeof (MobilityT_Movement_Area));

		/* Find out the coordinates of the upper left corner of	*/
		/* the node's subnet, which will be used as a reference	*/
		/* point if the	trajectory recording feature is enabled	*/
		/* or if mobility domains are not used.					*/
		if (record_trajectory == OPC_TRUE || use_mobility_domain == OPC_FALSE)
			{
			/* Get the node's subnet id.						*/
			subnet_id = op_topo_parent (mobile_node_ptr->mobile_entity_objid);
		
			/* Check whether the node is in the top subnet or	*/
			/* not.												*/
			if (op_topo_parent (subnet_id) != OPC_OBJID_NULL)
				{
				/* Obtain the global position of the subnet.	*/
				op_ima_obj_pos_get (subnet_id, &sub_latitude, &sub_longitude, &sub_altitude, &sub_x_pos, &sub_y_pos, &sub_z_pos);	
				op_ima_obj_attr_get (subnet_id, "x span", &sub_x_span);
				op_ima_obj_attr_get (subnet_id, "y span", &sub_y_span);
				
				/* Find out the unit of x and y-span values.	*/
				op_ima_obj_attr_get (op_topo_parent (subnet_id), "unit", &sub_span_unit);
				
				/* Compute the position of the upper left		*/
				/* corner of the mobile node's subnet.			*/
				if (sub_span_unit == OpC_Ima_Obj_Position_Unit_Degrees)
					{
					sub_longitude_min = sub_longitude - (sub_x_span / 2);
					sub_latitude_max  = sub_latitude  + (sub_y_span / 2);	
					}
				else
					{
					/* Convert the unit type into a PrgC value.	*/
					switch (sub_span_unit)
						{
						case OpC_Ima_Obj_Position_Unit_Meters:
							sub_span_unit_prgc = PrgC_Unit_Meters_Type;
							break;
						case OpC_Ima_Obj_Position_Unit_Kilometers:
							sub_span_unit_prgc = PrgC_Unit_Kilometers_Type;
							break;
						case OpC_Ima_Obj_Position_Unit_Feet:
							sub_span_unit_prgc = PrgC_Unit_Feet_Type;
							break;
						case OpC_Ima_Obj_Position_Unit_Miles:
							sub_span_unit_prgc = PrgC_Unit_Miles_Type;
							break;
						default:
							break;
						};
					
					/* Convert the distance between the center	*/
					/* and the corner of the subnet into		*/
					/* degrees.									*/
					prg_geo_distances_to_degrees_convert (sub_longitude, sub_latitude, sub_x_span / 2, sub_y_span / 2, sub_span_unit_prgc,
														  &long_distance, &lat_distance);
					
					/* Compute the coordinates of the upper 	*/
					/* left corner.								*/
					sub_longitude_min = sub_longitude - long_distance;
					sub_latitude_max  = sub_latitude  + lat_distance;						
					}

				/* Update the node's record with subnet info.	*/
				mobile_node_ptr->movement_area_ptr->sub_x_min = sub_longitude_min;
				mobile_node_ptr->movement_area_ptr->sub_y_min = sub_latitude_max;
				}
			
			/* Initialize the fields that are used when			*/
			/* trajectory recording is enabled.					*/
			if (record_trajectory == OPC_TRUE)
				{
				char* net_name;

				oms_tan_hname_get (mobile_node_ptr->mobile_entity_objid, node_hname);
				net_name = Oms_Sim_Attr_Net_Name_Get ();

				/* Trajectory files will be collected without RUN# information */
				mobile_node_ptr->trj_fhndl  = Oms_Ext_File_Handle_Get (net_name, node_hname, "trj");
				
				op_prg_mem_free (net_name);
				}

			mobile_node_ptr->num_transit_points = 0;
			}

		/* If the movement area of the nodes using this profile	*/
		/* is not specified using a mobility domain object,		*/
		/* then compute the movement area of each node which	*/
		/* depends on the user defined area size and position,	*/
		/* and the location of node's subnet.					*/
		if (use_mobility_domain == OPC_FALSE)
			{
			if (op_topo_parent (subnet_id) != OPC_OBJID_NULL)
				{	
				/* Determine the movement area of the node		*/
				/* within the subnet, which is specified as		*/
				/* offset values by taking the upper left		*/
				/* corner of the subnet as the reference point.	*/				

				/* First find out the offset values in degrees	*/
				/* from the parameters user entered.			*/
				prg_geo_distances_to_degrees_convert (sub_longitude_min, sub_latitude_max, xmin, ymin, PrgC_Unit_Meters_Type, &long_offset_min, &lat_offset_min);				
				prg_geo_distances_to_degrees_convert (sub_longitude_min, sub_latitude_max, xmax, ymax, PrgC_Unit_Meters_Type, &long_offset_max, &lat_offset_max);				
				
				/* Then, determine the mobility area in degrees	*/
				/* from the offset values.						*/
				mobile_node_ptr->movement_area_ptr->x_min = sub_longitude_min + long_offset_min;
				mobile_node_ptr->movement_area_ptr->x_max = sub_longitude_min + long_offset_max;
				mobile_node_ptr->movement_area_ptr->y_min = sub_latitude_max  - lat_offset_min;
				mobile_node_ptr->movement_area_ptr->y_max = sub_latitude_max  - lat_offset_max;
				}
			else
				{
				/* The node resides in the top subnet. Hence,	*/
				/* we can't use the upper left corner of its	*/
				/* subnet as the reference point for our offset	*/
				/* values. Instead, use the center of top subet	*/
				/* (Earth) as the reference point.				*/
				prg_geo_distances_to_degrees_convert (0.0, 0.0, xmin, ymin, PrgC_Unit_Meters_Type, 
													  &(mobile_node_ptr->movement_area_ptr->x_min), &(mobile_node_ptr->movement_area_ptr->y_min));				
				prg_geo_distances_to_degrees_convert (0.0, 0.0, xmax, ymax, PrgC_Unit_Meters_Type, 
													  &(mobile_node_ptr->movement_area_ptr->x_max), &(mobile_node_ptr->movement_area_ptr->y_max));				
				}
			}
		else if (record_trajectory == OPC_FALSE)
			{
			/* Free the movement area record since it is not	*/
			/* needed.											*/
			op_prg_mem_free (mobile_node_ptr->movement_area_ptr);
			mobile_node_ptr->movement_area_ptr = OPC_NIL;
			}
		
		/* Start the random waypoint mobility if it is less		*/
		/* than the stop time                					*/
		start_time = (double) oms_dist_nonnegative_outcome (start_time_dist);
		if ((stop_time == -1.0) || (start_time < stop_time))
			{
			op_intrpt_schedule_call (start_time, MOVE, mobility_random_waypoint_process_intrpt, mobile_node_ptr);
	
			/* If animation is not currently enabled, avoid		*/
			/* unnecessary animation updates. Use the last node	*/
			/* id processed from the node list.					*/
			if (op_sim_anim () == OPC_TRUE)
				{
				op_intrpt_schedule_call (start_time, UPDATE, mobility_random_waypoint_process_intrpt, mobile_node_ptr);			
				}
			}
		
		}
	FOUT;	
	}

static void
mobility_random_waypoint_process_intrpt (void* mobile_node_ptr, int code)
	{
 	
	/** This function processes the scheduled interrupts.	**/
	FIN (mobility_random_process_intrpt (mobile_node_ptr, code));
			
	if (code == PAUSE)
		mobility_random_waypoint_process_pause ((MobilityT_Mobile_Node_Rec *) mobile_node_ptr);
	else if	(code == MOVE)
		mobility_random_waypoint_process_move ((MobilityT_Mobile_Node_Rec *) mobile_node_ptr);
	else if	(code == UPDATE)
		mobility_random_waypoint_process_update ((MobilityT_Mobile_Node_Rec *) mobile_node_ptr);	
	else
		mobility_random_waypoint_error("Wrong code!!!It should be MOVE or PAUSE or UPDATE",(char *) "", (char *)"");

	FOUT;
	}


static void
mobility_random_waypoint_process_pause (MobilityT_Mobile_Node_Rec* mobile_node_ptr)
	{
	double 		Speed;
	double      end_of_pause_time;
	char        speed_setting [256];
	
	/** This Function stops the node moving for a pause time **/
	FIN (mobility_random_waypoint_process_pause (mobile_node_ptr));
	
	/* Stop the node moving */
	Speed = (double) 0;
	sprintf (speed_setting, "%f meter/sec", Speed);
	op_ima_obj_attr_set_str (mobile_node_ptr->mobile_entity_objid, "ground speed", speed_setting);
  
	/* Determine the pause time*/
	end_of_pause_time = (double) oms_dist_nonnegative_outcome (pause_time_dist) + op_sim_time ();
	mobile_node_ptr->movement_delay = end_of_pause_time - op_sim_time();
	
	/* Continue the random waypoint mobility if it is less than the	*/
	/* stop time                									*/
	if ((stop_time == -1.0) || (end_of_pause_time < stop_time))
		{
		/* The node can start to move after a pause time */
		op_intrpt_schedule_call (end_of_pause_time, MOVE, mobility_random_waypoint_process_intrpt, mobile_node_ptr);	   
		}
	
	FOUT;
		}

static void
mobility_random_waypoint_process_move (MobilityT_Mobile_Node_Rec* mobile_node_ptr)
	{
	double        					speed, dist_to_cover;
	double        					bearing, move_time;
	double        					longitude_target, latitude_target;
	double							latitude, longitude, altitude, x_pos, y_pos, z_pos, x_pos_meters, y_pos_meters;
	char          					odb_msg1 [128], odb_msg2 [128];
	char          					speed_setting [256], trj_file_output_string [512];
	Objid         					subnet_id;
	double							x_min, x_max, y_min, y_max, sub_x_min, sub_y_min;	
	OpT_Ima_Obj_Position_Unit_Type	cur_subnet_unit;
	PrgT_Distance_Units_Type		subnet_unit_prg;
	
	/** This function does the following;           			**/
	/** 1. Determine the target position                   		**/
	/** 2. Compute the distance to cover and the bearing angle 	**/
	/** 3. Compute how long it will take to arrive at the		**/
	/**    target and move it with the ground speed and the		**/
	/**    bearing angle for that much time              		**/
  	FIN (mobility_random_waypoint_process_move (mobile_node_ptr));

	/* When mobility domain is selected, use the domain			*/
	/* coordinates, otherwise, use the coordinates the user		*/
	/* specified.												*/
	if (use_mobility_domain == OPC_TRUE)
		{
		/* Get the coordinates that correspond to this domain.	*/
		x_min = xmin;
		x_max = xmax;
		y_min = ymin;
		y_max = ymax;

		}
	else
		{			
		/* Get the coordinates specified by the user.			*/		
		x_min = mobile_node_ptr->movement_area_ptr->x_min;
		x_max = mobile_node_ptr->movement_area_ptr->x_max;		
		y_min = mobile_node_ptr->movement_area_ptr->y_min;
		y_max = mobile_node_ptr->movement_area_ptr->y_max;		
		}
				
	/* Compute the target position in degrees.					*/
	longitude_target = x_min + op_dist_uniform (x_max - x_min);
	latitude_target  = y_min + op_dist_uniform (y_max - y_min);		

	/* Obtain the global positions of the node.					*/
	op_ima_obj_pos_get (mobile_node_ptr->mobile_entity_objid, &latitude, &longitude, &altitude, &x_pos, &y_pos, &z_pos);

	/* Find out the distance from current position to the		*/
	/* target position in meters.								*/
	dist_to_cover = prg_geo_great_circle_distance_get (latitude, longitude, latitude_target, longitude_target);
	
	/* Make sure that the distance is not zero if we are also	*/
	/* recording our movement into a trajectory to prevent		*/
	/* having zero travel times, which invalidates the			*/
	/* trajectory data.											*/
	if (dist_to_cover < MIN_DISTANCE && record_trajectory)
		dist_to_cover = MIN_DISTANCE;
	
	/* Get the bearing angle that will take the node to the		*/
	/* target position.											*/
	bearing = prg_geo_bearing_compute (latitude, longitude, latitude_target, longitude_target); 
		
	/* Get the random speed and set it as a ground speed.		*/
	speed = (double) oms_dist_positive_outcome (speed_dist);
	sprintf (speed_setting, "%f meter/sec", speed);
	op_ima_obj_attr_set_str (mobile_node_ptr->mobile_entity_objid, "ground speed", speed_setting);
	
	/* Set the bearing angle.									*/
	op_ima_obj_attr_set_dbl (mobile_node_ptr->mobile_entity_objid, "bearing", bearing);
	
	/* Determine the move time and keep moving the node for		*/
	/* that much time.											*/
	move_time = dist_to_cover / speed;

	/* Trajectory recording is enabled then update the node's	*/
	/* trajectory file using the new destination information.	*/
	if (record_trajectory)
		{
		/* Get the coordinates of subnet's upper left corner.	*/
		sub_x_min = mobile_node_ptr->movement_area_ptr->sub_x_min;
		sub_y_min = mobile_node_ptr->movement_area_ptr->sub_y_min;
		
		/* Record initial position of node if needed.			*/
		if (mobile_node_ptr->num_transit_points == 0)
			{			
			/* Obtain the units of the node's subnet and store	*/
			/* it in the node's record for future use.			*/
			subnet_id = op_topo_parent (mobile_node_ptr->mobile_entity_objid);
			op_ima_obj_attr_get (subnet_id, "unit", &cur_subnet_unit);

			/* Convert the unit type into a PrgC value.			*/
			switch (cur_subnet_unit)
				{
				case OpC_Ima_Obj_Position_Unit_Meters:
					mobile_node_ptr->movement_area_ptr->subnet_unit = PrgC_Unit_Meters_Type;
					break;
				case OpC_Ima_Obj_Position_Unit_Kilometers:
					mobile_node_ptr->movement_area_ptr->subnet_unit = PrgC_Unit_Kilometers_Type;
					break;
				case OpC_Ima_Obj_Position_Unit_Feet:
					mobile_node_ptr->movement_area_ptr->subnet_unit = PrgC_Unit_Feet_Type;
					break;
				case OpC_Ima_Obj_Position_Unit_Miles:
					mobile_node_ptr->movement_area_ptr->subnet_unit = PrgC_Unit_Miles_Type;
					break;
				case OpC_Ima_Obj_Position_Unit_Degrees:
					mobile_node_ptr->movement_area_ptr->subnet_unit = PrgC_Unit_Degrees_Type;
				break;
				default:
				break;
				}	
			subnet_unit_prg = mobile_node_ptr->movement_area_ptr->subnet_unit;
			
			/* Find out the initial position in the units of	*/
			/* the subnet.										*/
			if (subnet_unit_prg == PrgC_Unit_Degrees_Type)
				{
				x_pos_meters = longitude;
				y_pos_meters = latitude;
				}
			else
				{
				/* Convert distances between subnet's upper		*/
				/* left-corner and node's initial position into	*/
				/* the units of the subnet.						*/
				prg_geo_degrees_to_distances_convert (sub_x_min, sub_y_min, longitude - sub_x_min, 
					sub_y_min - latitude, subnet_unit_prg, &x_pos_meters, &y_pos_meters);					
				}
			
			/* Write to the trajectory file.					*/
			sprintf (trj_file_output_string,"%.15f\t,%.15f\t,0\t,0h0m0.00s\t,0h0m%.15fs\n",
				x_pos_meters, y_pos_meters, op_sim_time());
			Oms_Ext_File_Info_Append (mobile_node_ptr->trj_fhndl, trj_file_output_string);

			/* Keep track of the number of moves.				*/
			mobile_node_ptr->num_transit_points++;
			}
		else
			subnet_unit_prg = mobile_node_ptr->movement_area_ptr->subnet_unit;
			
		/* Record the next position of the node.				*/
		
		/* Find the next position in the units of the subnet.	*/
		if (subnet_unit_prg == PrgC_Unit_Degrees_Type)
			{
			x_pos_meters = longitude_target;
			y_pos_meters = latitude_target;
			}
		else
			{
			/* Convert distance between subnet's upper			*/
			/* left-corner and node's new position into units	*/
			/* of the subnet.									*/
			prg_geo_degrees_to_distances_convert (sub_x_min, sub_y_min, longitude_target - sub_x_min, sub_y_min - latitude_target, 
				subnet_unit_prg, &x_pos_meters, &y_pos_meters);					
			}
	
		/* Write next position, pause time and travel time		*/
		/* information into the file.							*/
		sprintf(trj_file_output_string, "%.15f\t,%.15f\t,0\t,0h0m%.15fs\t,0h0m%.15fs\n", 
			x_pos_meters, y_pos_meters, move_time, mobile_node_ptr->movement_delay);
		Oms_Ext_File_Info_Append (mobile_node_ptr->trj_fhndl, trj_file_output_string);

		/* Keep track of the number of moves */
		mobile_node_ptr->num_transit_points++;	
		}
	
	/* Move the node until the stop time.						*/
	if ((stop_time != -1.0) && ((op_sim_time () + move_time) > stop_time) )
		move_time = stop_time - op_sim_time();
	
	/* Schedule an interrupt for the end of the next pause		*/
	/* state.													*/		
	op_intrpt_schedule_call (op_sim_time () + move_time, 
		PAUSE, mobility_random_waypoint_process_intrpt, mobile_node_ptr);	   
	
	/* Write a debug message if tracing is active.				*/
	if (trace_active)
		{
		sprintf (odb_msg1, "Objid: %d\tLat: %f\tLong: %f\t", mobile_node_ptr->mobile_entity_objid, longitude, latitude);
		sprintf (odb_msg2, "Objid: %d\tTarget Lat:%f\tTarget Long: %f\n", mobile_node_ptr->mobile_entity_objid, longitude_target, latitude_target);		
		op_prg_odb_print_minor (odb_msg1, odb_msg2, OPC_NIL);
		}	
	
	FOUT;
	}


static void
mobility_random_waypoint_process_update (MobilityT_Mobile_Node_Rec* mobile_node_ptr)
	{
	double	latitude, longitude, altitude, x_pos, y_pos, z_pos;
	
	/** This function updates the location of the node    **/
	/** for the purpose of visualization                  **/
	FIN (mobility_random_waypoint_process_update (mobile_node_ptr));
	
	/* Obtain the global positions of the node*/
	op_ima_obj_pos_get (mobile_node_ptr->mobile_entity_objid, &latitude, &longitude, &altitude, &x_pos, &y_pos, &z_pos);

	/* The node can start to move after a pause time */
	op_intrpt_schedule_call (op_sim_time () + update_freq, UPDATE, mobility_random_waypoint_process_intrpt, mobile_node_ptr);	   
	
	FOUT;
	}


static void
mobility_random_waypoint_error (const char* str1, const char* str2, const char* str3)
	{
	/** This function generates an error and	**/
	/** terminates the simulation				**/
	FIN (random_waypoint_error (str1, str2, str3));
	
	op_sim_end ("Random Waypoint error : ", str1, str2, str3);
	
	FOUT;
	}

static void
mobility_random_waypoint_end_processing (void * PRG_ARG_UNUSED (v_state_ptr), int PRG_ARG_UNUSED(code))
	{
	MobilityT_Mobile_Node_Rec*		mobile_node_ptr = OPC_NIL;
	List*							node_list_ptr;
	int 							i=0;
	int								node_count;
	char 							trj_file_output_string [OMSC_HNAME_MAX_LEN];
	OpT_Ima_Obj_Position_Unit_Type	cur_subnet_unit;
	char	units_str [64];
	
	/* At the end of simulation, write header of trajectory files */
	FIN (mobility_random_waypoint_end_processing);

    /* Get the list of node ids for this profile. */
	node_list_ptr = profile_ptr->Mobile_entity_id_list;
	node_count    = op_prg_list_size (node_list_ptr);

	/* Open each node's trajectory file and set its trajectory file's header */
    for(i = 0; i < node_count; i++)
		{
		/* Access the mobile node's record.		*/
		mobile_node_ptr = (MobilityT_Mobile_Node_Rec *) op_prg_list_access (node_list_ptr, i);

		op_ima_obj_attr_get (op_topo_parent (mobile_node_ptr->mobile_entity_objid), "unit", &cur_subnet_unit);

		/* Convert the unit type into a PrgC value.	*/
		switch (cur_subnet_unit)
			{
			case OpC_Ima_Obj_Position_Unit_Meters:
				strcpy (units_str, "Meters");
				break;
			case OpC_Ima_Obj_Position_Unit_Kilometers:
				strcpy (units_str, "Kilometers");
				break;
			case OpC_Ima_Obj_Position_Unit_Feet:
				strcpy (units_str, "Feet");
				break;
			case OpC_Ima_Obj_Position_Unit_Miles:
				strcpy (units_str, "Miles");
				break;
			case OpC_Ima_Obj_Position_Unit_Degrees:
				strcpy (units_str, "Degrees");
				break;
			default:
				break;
			};
		
		/* Write the node movement count into the files */		
		sprintf (trj_file_output_string,
			"Version: 2\n"
			"Position_Unit: %s\n"
			"Altitude_Unit: Meters\n"
			"Coordinate_Method: absolute\n"
			"Altitude_Method: absolute\n"
			"locale: C\n"
			"Coordinate_Count: %d\n", units_str, mobile_node_ptr->num_transit_points);
		
		Oms_Ext_File_Info_Prepend (mobile_node_ptr->trj_fhndl, trj_file_output_string);
		}

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
	void random_waypoint (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_random_waypoint_init (int * init_block_ptr);
	void _op_random_waypoint_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_random_waypoint_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_random_waypoint_alloc (VosT_Obtype, int);
	void _op_random_waypoint_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
random_waypoint (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (random_waypoint ());

		{


		FSM_ENTER_NO_VARS ("random_waypoint")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "random_waypoint [init enter execs]")
				FSM_PROFILE_SECTION_IN ("random_waypoint [init enter execs]", state0_enter_exec)
				{
				/* Initialize the state variables  */
				mobility_random_waypoint_sv_init ();
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "random_waypoint [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "idle", "tr_0", "random_waypoint [init -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "random_waypoint [idle enter execs]")
				FSM_PROFILE_SECTION_IN ("random_waypoint [idle enter execs]", state1_enter_exec)
				{
				/* Initialize the random waypoint mobility for each node  */
				mobility_random_waypoint_initialize_node_movement ();
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"random_waypoint")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "random_waypoint [idle exit execs]")


			/** state (idle) transition processing **/
			FSM_TRANSIT_MISSING ("idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"random_waypoint")
		}
	}




void
_op_random_waypoint_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_random_waypoint_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_random_waypoint_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_random_waypoint_svar function. */
#undef profile_ptr
#undef my_objid
#undef my_prohandle
#undef start_time_dist
#undef stop_time
#undef xmin
#undef xmax
#undef ymin
#undef ymax
#undef speed_dist
#undef pause_time_dist
#undef trace_active
#undef update_freq
#undef use_mobility_domain
#undef record_trajectory

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_random_waypoint_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_random_waypoint_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (random_waypoint)",
		sizeof (random_waypoint_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_random_waypoint_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	random_waypoint_state * ptr;
	FIN_MT (_op_random_waypoint_alloc (obtype))

	ptr = (random_waypoint_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "random_waypoint [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_random_waypoint_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	random_waypoint_state		*prs_ptr;

	FIN_MT (_op_random_waypoint_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (random_waypoint_state *)gen_ptr;

	if (strcmp ("profile_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->profile_ptr);
		FOUT
		}
	if (strcmp ("my_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_objid);
		FOUT
		}
	if (strcmp ("my_prohandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_prohandle);
		FOUT
		}
	if (strcmp ("start_time_dist" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->start_time_dist);
		FOUT
		}
	if (strcmp ("stop_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->stop_time);
		FOUT
		}
	if (strcmp ("xmin" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->xmin);
		FOUT
		}
	if (strcmp ("xmax" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->xmax);
		FOUT
		}
	if (strcmp ("ymin" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ymin);
		FOUT
		}
	if (strcmp ("ymax" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ymax);
		FOUT
		}
	if (strcmp ("speed_dist" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->speed_dist);
		FOUT
		}
	if (strcmp ("pause_time_dist" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pause_time_dist);
		FOUT
		}
	if (strcmp ("trace_active" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->trace_active);
		FOUT
		}
	if (strcmp ("update_freq" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->update_freq);
		FOUT
		}
	if (strcmp ("use_mobility_domain" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->use_mobility_domain);
		FOUT
		}
	if (strcmp ("record_trajectory" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->record_trajectory);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

