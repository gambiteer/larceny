/* Rts/Sys/barrier.h
 * Larceny run-time system -- write barrier interface.
 *
 * $Id: barrier.h,v 1.6 1997/09/23 19:57:44 lth Exp lth $
 *
 * See Rts/Sys/barrier.c and Rts/Sparc/barrier.s for more information.
 */

#ifndef INCLUDED_BARRIER_H
#define INCLUDED_BARRIER_H

#include "larceny-types.h"

/* Initialize the write barrier for a generational system. */

void wb_setup( unsigned *genv,      /* map from page to generation */
	       unsigned *pagebase,  /* fixed: address of lowest page */
	       int generations,     /* fixed: number of generations */
	       word *globals,       /* fixed: globals vector */
	       word **ssbtopv,      /* fixed: SSB top pointers */
	       word **ssblimv,      /* fixed: SSB lim pointers */
	       int  np_young_gen,   /* -1 or generation # for NP young */
	       int  np_ssbidx       /* -1 or idx in vectors for magic remset */
	      );

/* Disable the write barrier. */
void wb_disable_barrier( void );

/* If the descriptor tables change, notify the barrier */
void wb_re_setup( void *pagebase, unsigned *genv );


#endif
/* eof */
