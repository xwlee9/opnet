/* dra_error_all_stats.ps.c */                                                       
/* Alternative error allocation model for radio link Transceiver Pipeline.	*/
/* In contrast to default model, this one computes all the bit errors in	*/
/* the packet, even though the errors found so far already exceed the		*/
/* receiver's error correction threshold. Hence, this model may provide a	*/
/* slower execution of the error pipeline stage compared to the default		*/
/* model, but it should be used if the accuracy of the statistics "number	*/
/* of bit errors" and "actual bit-error-rate" collected for each packet are	*/
/* important for the conducted study.										*/                                                        

/****************************************/
/*		 Copyright (c) 1993-2008		*/
/*		by OPNET Technologies, Inc.		*/
/*		 (A Delaware Corporation)		*/
/*	   7255 Woodmont Av., Suite 250  	*/
/*      Bethesda, MD 20814, U.S.A.      */
/*		   All Rights Reserved.			*/
/****************************************/

#include "opnet.h"
#include <math.h>


/* Define a convenient macro for computing factorials using the gamma		*/
/* function Pick the mt-safe version for parallel execution, which is		*/
/* available only for Solaris.												*/	
#if defined (OPD_PARALLEL) && !defined (HOST_PC_INTEL_WIN32)
#define log_factorial(n)		lgamma_r ((double) n + 1.0, &signgam)
extern double lgamma_r (double, int *);
#else
#define log_factorial(n)		lgamma ((double) n + 1.0)
extern double lgamma (double);
#endif


#if defined (__cplusplus)
extern "C"
#endif
void
dra_error_all_stats_mt (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet* pkptr)
	{
	double		pe, r, p_accum, p_exact;
	double		data_rate, elap_time;
	double		log_p1, log_p2, log_arrange;
	int			seg_size, num_errs;
	int			invert_errors;
#if defined (OPD_PARALLEL) && !defined (HOST_PC_INTEL_WIN32)
	int			signgam;
#endif

	/** Compute the number of errors assigned to a segment of bits within	**/
	/** a packet based on its length and the bit error probability.			**/
	FIN_MT (dra_error_all_stats (pkptr));

	/* Obtain the expected Bit-Error-Rate 'pe' */
	pe = op_td_get_dbl (pkptr, OPC_TDA_RA_BER);

	/* Calculate time elapsed since last BER change */
	elap_time = op_sim_time () - op_td_get_dbl (pkptr, OPC_TDA_RA_SNR_CALC_TIME);

	/* Use datarate to determine how many bits in the segment. */
	data_rate = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_DRATE);
	seg_size = elap_time * data_rate;

    
	/* Case 1: if the bit error rate is zero, so is the number of errors. 	*/
    if (pe == 0.0 || seg_size == 0)
		num_errs = 0;

	/* Case 2: if the bit error rate is 1.0, then all the bits are in error.	*/
	/* (note however, that bit error rates should not normally exceed 0.5).		*/
	else if (pe >= 1.0)
		num_errs = seg_size;

	/* Case 3: The bit error rate is not zero or one. */
	else
		{
		/* If the bit error rate is greater than 0.5 and less than 1.0, invert	*/
		/* the problem to find instead the number of bits that are not in error	*/
		/* in order to accelerate the performance of the algorithm.  Set a flag	*/
		/* to indicate that the result will then have to be inverted.			*/
		if (pe > 0.5)
			{
			pe = 1.0 - pe;
			invert_errors = OPC_TRUE;	
			}
		else
			invert_errors = OPC_FALSE;
	
		/* The error count can be obtained by mapping a uniform random number 	*/
		/* in [0, 1[ via the inverse of the cumulative mass function (CMF) 		*/
		/* for the bit error count distribution. 								*/
	
		/* Obtain a uniform random number in [0, 1[ to represent 		*/
		/* the value of the CDF at the outcome that will be produced. 	*/
		r = op_dist_uniform (1.0);
	
		/* Integrate probability mass over possible outcomes until r is exceeded. 	*/
		/* The loop iteratively corresponds to "inverting" the CMF since it finds	*/
		/* the bit error count at which the CMF first meets or exceeds the value r.	*/
		for (p_accum = 0.0, num_errs = 0; num_errs <= seg_size; num_errs++)
			{
			/* Compute the probability of exactly 'num_errs' bit errors occurring.	*/
	
			/* The probability that the first 'num_errs' bits will be in error 		*/
			/* is given by pow (pe, num_errs). Here it is obtained in logarithmic	*/
			/* form to avoid underflow for small 'pe' or large 'num_errs'.			*/
			log_p1 = (double) num_errs * log (pe);
	
			/* Similarly, obtain the probability that the remaining bits will not 	*/
			/* be in error. The combination of these two events represents one 		*/
			/* possible configuration of bits yielding a total of 'num_errs' errors.*/
			log_p2 = (double) (seg_size - num_errs) * log (1.0 - pe);
	
			/* Compute the number of arrangements that are possible with the same 	*/
			/* number of bits in error as the particular case above. Again obtain 	*/
			/* this number in logarithmic form (to avoid overflow in this case).	*/
			/* This result is expressed as the logarithmic form of the formula for	*/
			/* the number N of combinations of k items from n:  N = n!/(n-k)!k!		*/
			log_arrange = 	log_factorial (seg_size) -
							log_factorial (num_errs) - 
							log_factorial (seg_size - num_errs);
	
			/* Compure the probability that exactly 'num_errs' are present 	*/
			/* in the segment of bits, in any arrangement.					*/
			p_exact = exp (log_arrange + log_p1 + log_p2);
		
			/* Add this to the probability mass accumulated so far for previously 	*/
			/* tested outcomes to obtain the value of the CMF at outcome = num_errs.*/
			p_accum += p_exact;
	
			/*'num_errs' is the outcome for this trial if the CMF meets or exceeds 	*/
			/* the uniform random value selected earlier. 							*/
			if (p_accum >= r)
				break;
			}
	
		/* If the bit error rate was inverted to compute correct bits instead, then */
		/* Reinvert the result to obtain the number of bits in error. 				*/
		if (invert_errors == OPC_TRUE)
			num_errs = seg_size - num_errs;
		}
	
	
	/* Increase number of bit errors in packet transmission data attribute. */
	op_td_increment_int (pkptr, OPC_TDA_RA_NUM_ERRORS, num_errs);

	/* Assign actual (allocated) bit-error rate over tested segment. */
	if (seg_size != 0)
		op_td_set_dbl (pkptr, OPC_TDA_RA_ACTUAL_BER, (double) num_errs / seg_size);
	else 
		op_td_set_dbl (pkptr, OPC_TDA_RA_ACTUAL_BER, pe);

	FOUT
	}

