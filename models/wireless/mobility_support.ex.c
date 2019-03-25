/** mobility_support.ex.c				**/

/****************************************/
/*      Copyright (c) 1987-2008		*/
/*     by OPNET Technologies, Inc.		*/
/*       (A Delaware Corporation)      	*/
/*    7255 Woodmont Av., Suite 250     	*/
/*     Bethesda, MD 20814, U.S.A.       */
/*       All Rights Reserved.          	*/
/****************************************/


/** Include directives.	**/
#include "opnet.h"
#include "mobility_support.h"

void
mobility_support_print_profile (Mobility_Profile_Desc* profile_defn_ptr)
	{
	int			num_nodes;
	int			i_th_node;
	Objid*		node_id_ptr;
	char		node_name [512];
	
	FIN (mobility_support_print_profile (profile_defn_ptr));
	
	num_nodes = op_prg_list_size (profile_defn_ptr->Mobile_entity_id_list);
	
	printf ("Profile Name : %s Mobility Model : %s \n",profile_defn_ptr->Profile_Name,profile_defn_ptr->Mobility_Model);
	mobility_support_print_RW_profile ((Mobility_Random_Waypoint_Desc *) profile_defn_ptr->Mobility_Desc_ptr);
	printf ("Printing Node Information: ");
	
	if (num_nodes == 0)
		{
		printf ("No nodes belong to this profile.");
		}
	else
		{
		for (i_th_node = 0; i_th_node < num_nodes; i_th_node++)
			{
			node_id_ptr = (Objid *) op_prg_list_access (profile_defn_ptr->Mobile_entity_id_list, i_th_node);
			op_ima_obj_attr_get (*(node_id_ptr), "name", node_name);
			if (i_th_node == 0)
				printf ("Node Object ID     Node Name");
			printf ("%-4d              %s", *(node_id_ptr), node_name);
			}
		}
	
	printf ("\n");
	
	FOUT;
	}


void
mobility_support_print_RW_profile (Mobility_Random_Waypoint_Desc* RW_desc_ptr)
	{	
	FIN (mobility_support_print_RW_profile (RW_desc_ptr));
	
  	printf ("Printing RW Details: start_time Stop_time\n");
	printf ("                     %8.2f %8.2f \n",
		(double) oms_dist_nonnegative_outcome (RW_desc_ptr->Start_Time), RW_desc_ptr->Stop_Time);
  	printf ("Printing RW Details: x_min  x_max  y_min  y_max\n");	
	printf ("                     %f   %f   %f   %f\n",
		RW_desc_ptr->X_min, RW_desc_ptr->X_max, RW_desc_ptr->Y_min, RW_desc_ptr->Y_max);

	FOUT;
	}
	
   
