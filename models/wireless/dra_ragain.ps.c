/* dra_ragain.ps.c */                                                       
/* Default receiver antenna gain model for radio link Transceiver Pipeline */

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
dra_ragain_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
	{
	double		tx_x, tx_y, tx_z;
	double		rx_x, rx_y, rx_z;
	double		dif_x, dif_y, dif_z, dist_xy;
	double		rot1_x, rot1_y, rot1_z;
	double		rot2_x, rot2_y, rot2_z;
	double		rot3_x, rot3_y, rot3_z;
	double		rx_phi, rx_theta, point_phi, point_theta;
	double		bore_phi, bore_theta, lookup_phi, lookup_theta, gain;
	Vartype		pattern_table;
	
	/** Compute the gain associated with the receiver's antenna. 	**/
	FIN_MT (dra_ragain (pkptr));

	/* Obtain handle on receiving antenna's gain. */
	pattern_table = op_td_get_ptr (pkptr, OPC_TDA_RA_RX_PATTERN);

	/* Special case: by convention a nil table address indicates an  */
	/* isotropic antenna pattern. Thus no calculations are necessary.*/
	if (pattern_table == OPC_NIL)
		{
		/* Assign zero dB gain regardless of transmission direction. */
		op_td_set_dbl (pkptr, OPC_TDA_RA_RX_GAIN, 0.0);
		FOUT;
		}

	/* Obtain the geocentric coordinates of the transmitter. */
	tx_x = op_td_get_dbl (pkptr, OPC_TDA_RA_TX_GEO_X);
	tx_y = op_td_get_dbl (pkptr, OPC_TDA_RA_TX_GEO_Y);
	tx_z = op_td_get_dbl (pkptr, OPC_TDA_RA_TX_GEO_Z);

	/* Obtain the geocentric coordinates of the receiver. */
	rx_x = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_GEO_X);
	rx_y = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_GEO_Y);
	rx_z = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_GEO_Z);
	
	/* Compute the vector from the receiver to the transmitter. */
	dif_x = tx_x - rx_x;
	dif_y = tx_y - rx_y;
	dif_z = tx_z - rx_z;
	
	/* Special case: If transmitter and receiver are the same  */
	/* then calculations are unnecessary.  We set gain = 0     */
	if ((dif_x == 0) && (dif_y == 0) && (dif_z == 0))
		{
		op_td_set_dbl(pkptr, OPC_TDA_RA_RX_GAIN, 0.0);
		FOUT;
		}

	/* Obtain phi, theta deflections from receiver to the 		*/
	/* antenna's target which is computed by the kernel.				*/
	/* These are computed based on the target point of the antenna		*/
	/* module and the position of the receiver. The frame of reference	*/
	/* is the Earth														*/
	point_phi = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_PHI_POINT);
	point_theta = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_THETA_POINT);
	
	/* Determine antenna pointing reference direction				*/
	/* (usually boresight cell of pattern).							*/
	/* Note that the difference in selected coordinate systems		*/
	/* between the antenna definiton and the geocentric axes,		*/
	/* is accomodated for here by modifying the given phi value.	*/
	bore_phi = 90 - op_td_get_dbl (pkptr, OPC_TDA_RA_RX_BORESIGHT_PHI);
	bore_theta = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_BORESIGHT_THETA);

	{
	// Setup a new coordinate system originating at the antenna location
	// where x axis is pointing at the antenna target 
	// and z axis is pointing to the "sky"
	// Deterministic z-axis definition is required to make rotation
	// of pattern assymetrical around the boresigh independent of the antenna location 
	// [more strictly, "sky" means that tangental projection of Z-axis to the surface
	// tangetal to Earth at the antenna location is covered by the tangental
	// projection of the X-axis to the same surface]
	double cos_pt_th = cos (VOSC_NA_DEG_TO_RAD * point_theta);
	double sin_pt_th = sin (VOSC_NA_DEG_TO_RAD * point_theta);
	double cos_pt_ph = cos (VOSC_NA_DEG_TO_RAD * point_phi);
	double sin_pt_ph = sin (VOSC_NA_DEG_TO_RAD * point_phi);

	// rotate about z axis by -point_theta 
	double rot_x = dif_x * cos_pt_th + dif_y * sin_pt_th;
	double rot_y = -dif_x * sin_pt_th + dif_y * cos_pt_th;
	double rot_z = dif_z;	
	// rotate about y axis by -point_phi
	rot1_x = rot_x * cos_pt_ph + rot_z * sin_pt_ph;
	rot1_y = rot_y;
	rot1_z = rot_z * cos_pt_ph - rot_x * sin_pt_ph;
	}
	{	
	// now  roll around x axis to make z axis point to the "sky"
	double		r = sqrt (tx_x * tx_x + tx_y * tx_y + tx_z * tx_z);
	double		sin_lat = tx_z / r;
	double		cos_lat = sqrt (1 - sin_lat * sin_lat);

	rot2_x = rot1_x;
	rot2_y = rot1_y * sin_lat + rot1_z * cos_lat;
	rot2_z = rot1_z * sin_lat - rot1_y * cos_lat;
	}
	{
	// now rotate vector by the pattern's boresight angles
	double cos_b_th = cos (VOSC_NA_DEG_TO_RAD * bore_theta);
	double cos_b_ph = cos (VOSC_NA_DEG_TO_RAD * bore_phi);
	double sin_b_th = sin (VOSC_NA_DEG_TO_RAD * bore_theta);
	double sin_b_ph = sin (VOSC_NA_DEG_TO_RAD * bore_phi);

	// first by +boresigh_phi about y axis
	double rot_x = rot2_x * cos_b_ph - rot2_z * sin_b_ph;
	double rot_y = rot2_y;
	double rot_z = rot2_x * sin_b_ph + rot2_z * cos_b_ph;
	// then by +boresigh_theta about the z axis
	rot3_x = rot_x * cos_b_th - rot_y * sin_b_th;
	rot3_y = rot_x * sin_b_th + rot_y * cos_b_th;
	rot3_z = rot_z;	
	}

	/* Determine x-y projected distance. */
	dist_xy = sqrt (rot3_x * rot3_x + rot3_y * rot3_y);

	/* For the vector to the receiver, determine phi-deflection from	*/
	/* the x-y plane (in degress) and determine theta deflection from 	*/
	/* the positive x axis.												*/
	if (dist_xy == 0.0)
		{
		if (rot3_z < 0.0)
			rx_phi = -90.0;
		else
			rx_phi = 90.0;
		rx_theta = 0.0;	
		}
	else
		{
		rx_phi = VOSC_NA_RAD_TO_DEG * atan (rot3_z / dist_xy);
		rx_theta = VOSC_NA_RAD_TO_DEG * atan2 (rot3_y, rot3_x);
		}

	/* Setup the angles at which to lookup gain.				*/
	/* In the rotated coordinate system, these are really		*/
	/* just the angles of the transmission vector. However,		*/
	/* note that here again the difference in the coordinate 	*/
	/* systems of the antenna and the geocentric axes is		*/
	/* accomodated for by modiftying the phi angle.				*/
	lookup_phi = 90.0 - rx_phi;
	lookup_theta = rx_theta;

	/* Obtain gain of antenna pattern at given angles. */
	gain = op_tbl_pat_gain (pattern_table, lookup_phi, lookup_theta);

	/* Set the rx antenna gain in the packet's transmission data attribute. */
	op_td_set_dbl (pkptr, OPC_TDA_RA_RX_GAIN, gain);
	
	FOUT;
	}
