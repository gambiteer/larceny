/* Copyright 1998 Lars T Hansen.
 *
 * $Id$
 *
 * Larceny run-time system (Unix) -- main file.
 * On-line manual available at http://www.ccs.neu.edu/home/lth/larceny.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config.h"
#include "larceny.h"
#include "gc.h"

/* Argument parsing structure */

typedef struct opt opt_t;
struct opt {
  int        maxheaps;		        /* length of size[] member */
  int        size[ MAX_HEAPS ];	        /* area 1 at loc 0, etc */
  gc_param_t gc_info;                   /* detailed info about areas */
  unsigned   timerval;                  /* timer value */
  bool       enable_singlestep;         /* enable/disable single stepping */
  bool       enable_breakpoints;        /* enable/disable breakpoints */
  bool       enable_timer;              /* enable/disable timer */
  bool       show_heapstats;            /* unparse this structure */
  char       *heapfile;                 /* name of heap file */
  bool       quiet;                     /* do not print informative msgs */
  bool       annoying;                  /* print many informative msgs */
  bool       supremely_annoying;        /* print massively many msgs */
  bool       flush;                     /* force icache flushing */
  bool       noflush;                   /* disable icache flushing */
  bool       reorganize_and_dump;       /* split text and data and dump */
  int        restc;                     /* number of extra arguments */
  char       **restv;                   /* vector of extra arguments */
};

static void param_error( char *s );
static void invalid( char *s );
static void usage( void );
static void help( void );
static void parse_options( int argc, char **argv, opt_t *opt );
static int  getsize( char *s, int *p );
static void dump_options( opt_t *o );

static bool quiet = 0;
  /* 'quiet' controls consolemsg() 
     */

static bool annoying = 0;
  /* 'annoying' controls annoying_msg() 
     */

static bool supremely_annoying = 0;
  /* 'supremely_annoying' controls supremely_annoyingmsg()
     */

int main( argc, argv )
int argc;
char **argv;
{
  opt_t o;

  memset( &o, 0, sizeof( o ) );
  o.maxheaps = MAX_HEAPS;
  o.timerval = 0xFFFFFFFF;
  o.heapfile = 0;
  o.enable_breakpoints = 1;
  o.restv = 0;
  o.gc_info.ephemeral_info = 0;
  o.gc_info.use_static_area = 1;
  o.gc_info.globals = globals;
#if defined( BDW_GC )
  o.gc_info.is_conservative_system = 1;
#endif

  cache_setup();
  consolemsg( "%s v%d.%d%s (%s:%s:%s) (%s %s)",
	      larceny_system_name,
	      larceny_major_version, 
	      larceny_minor_version,
	      larceny_version_qualifier,
	      larceny_gc_technology,
	      osname, 
	      (globals[ G_CACHE_FLUSH ] ? "split" : "unified"),
	      user, date );

  parse_options( argc, argv, &o );
  if (o.heapfile == 0)
    o.heapfile = larceny_heap_name;

  quiet = o.quiet;
  annoying = o.annoying;
  supremely_annoying = o.supremely_annoying;

  if (annoying || supremely_annoying)
    dump_options( &o );

  if (o.flush)
    globals[ G_CACHE_FLUSH ] = 1;
  else if (o.noflush)
    globals[ G_CACHE_FLUSH ] = 0;

  if (o.reorganize_and_dump && !o.gc_info.is_stopcopy_system) {
    o.gc_info.is_conservative_system = 0;
    o.gc_info.is_generational_system = 0;
    o.gc_info.is_stopcopy_system = 1;
    o.gc_info.use_static_area = 1;
    o.gc_info.use_non_predictive_collector = 0;
    o.gc_info.use_incremental_bdw_collector = 0;
    o.gc_info.sc_info.size_bytes = DEFAULT_STOPCOPY_SIZE;
    o.gc_info.sc_info.load_factor = DEFAULT_LOAD_FACTOR;
  }

  if (!create_memory_manager( &o.gc_info ))
    panic( "Unable to set up the garbage collector." );

  if (!load_heap_image_from_file( o.heapfile ))
    panic( "Unable to load the heap image." );

  if (o.reorganize_and_dump) {
    char buf[ PATH_MAX ];

    sprintf( buf, "%s.split", o.heapfile );
    if (!reorganize_and_dump_static_heap( buf ))
      panic( "Failed heap reorganization." );
    goto end;
  }

  /* initialize some policy globals */
  globals[ G_BREAKPT_ENABLE ] =
    (o.enable_breakpoints ? TRUE_CONST : FALSE_CONST);
  globals[ G_SINGLESTEP_ENABLE ] =
    (o.enable_singlestep ? TRUE_CONST : FALSE_CONST );
  globals[ G_TIMER_ENABLE ] =
    (o.enable_timer ? TRUE_CONST : FALSE_CONST );
  globals[ G_TIMER ] = 0;
  globals[ G_TIMER2 ] = o.timerval;
  globals[ G_RESULT ] = fixnum( 0 );  /* No arguments */

  setup_signal_handlers();
  init_stats( o.show_heapstats );

  /* Allocate vector of command line arguments and pass it as an
   * argument to the startup procedure.
   */
#if 0
  globals[ G_REG1 ] = allocate_argument_vector( o.restc, o.restv );
  globals[ G_RESULT ] = fixnum( 1 );
  scheme_start();
  /* control does not usually reach this point */

  consolemsg( "Scheme_start() returned with value %08lx", globals[ G_RESULT ]);
#else
  { word args[1], res;

    args[0] = allocate_argument_vector( o.restc, o.restv );
    larceny_call( globals[ G_STARTUP ], 1, args, &res );
    consolemsg( "Startup procedure returned with value %08lx", (long)res );
  }
#endif

 end:
  return 0;
}


/***************************************************************************
 *
 * Console messages
 *
 */

int panic( const char *fmt, ... )
{
  static int in_panic = 0;
  va_list args;

  va_start( args, fmt );
  fprintf( stderr, "Larceny Panic: " );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );

  if (in_panic) _exit( 1 );
  in_panic = 1;
  exit( 1 );
  /* Never returns. Return type is 'int' to facilitate an idiom. */
  return 0;
}

int panic_abort( const char *fmt, ... )
{
  static int in_panic = 0;
  va_list args;

  va_start( args, fmt );
  fprintf( stderr, "Larceny Panic: " );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );

  if (in_panic) _exit( 1 );
  in_panic = 1;
  abort();
  /* Never returns. Return type is 'int' to facilitate an idiom. */
  return 0;
}

void annoyingmsg( const char *fmt, ... )
{
  va_list args;

  if (!annoying) return;

  va_start( args, fmt );
  if (!quiet) {
    vfprintf( stderr, fmt, args );
    fprintf( stderr, "\n" );
  }
  va_end( args );
}

void supremely_annoyingmsg( const char *fmt, ... )
{
  va_list args;

  if (!supremely_annoying) return;

  va_start( args, fmt );
  if (!quiet) {
    vfprintf( stderr, fmt, args );
    fprintf( stderr, "\n" );
  }
  va_end( args );
}

void consolemsg( const char *fmt, ... )
{
  va_list args;

  if (quiet) return;

  va_start( args, fmt );
  vfprintf( stdout, fmt, args );
  fprintf( stdout, "\n" );
  va_end( args );
  fflush( stdout );
}

void hardconsolemsg( const char *fmt, ... )
{
  va_list args;

  va_start( args, fmt );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );
}

/****************************************************************************
 *
 * Command line parsing.
 */

static int sizearg( char *str, int *argc, char ***argv, int *var );
static int hsizearg( char *str, int *argc, char ***argv, int *var, int *loc );
static int doublearg( char *str, int *argc, char ***argv, double *var );
static int numbarg( char *str, int *argc, char ***argv, int *var );
static int hnumbarg( char *str, int *argc, char ***argv, int *var, int *loc );
static void compute_np_parameters( opt_t *o, int suggested_size );

static void init_generational( opt_t *o, int areas, char *name )
{
  if (areas < 2)
    invalid( name );

  if (o->gc_info.ephemeral_info != 0) {
    consolemsg( "Error: Number of areas re-specified with '%s'", name );
    consolemsg( "Type \"larceny -help\" for help." );
    exit( 1 );
  }

  o->gc_info.is_generational_system = 1;
  o->gc_info.ephemeral_info = 
    (sc_info_t*)must_malloc( sizeof( sc_info_t )*areas-2 );
  o->gc_info.ephemeral_area_count = areas-2;
}

static void
parse_options( int argc, char **argv, opt_t *o )
{
  int i, loc, heaps, prev_size, areas = DEFAULT_AREAS;
#if !defined( BDW_GC )
  double load_factor = DEFAULT_LOAD_FACTOR;
#else
  double load_factor = 0.0;	/* Ignore it. */
#endif
  double feeling_lucky = 0.0;
  double phase_detection = -1.0;
  int dynamic_max = 0;
  int dynamic_min = 0;
  int divisor = 0;
  int val;

  while (--argc) {
    ++argv;
#if !defined( BDW_GC )
    if (strcmp( *argv, "-stopcopy" ) == 0)
      o->gc_info.is_stopcopy_system = 1;
    else if (numbarg( "-areas", &argc, &argv, &areas ))
      init_generational( o, areas, "-areas" );
    else if (strcmp( *argv, "-gen" ) == 0)
      init_generational( o, areas, "-gen" );
    else if (strcmp( *argv, "-np" ) == 0) {
      o->gc_info.is_generational_system = 1;
      o->gc_info.use_non_predictive_collector = 1;
    }
    else if (strcmp( *argv, "-nostatic" ) == 0)
      o->gc_info.use_static_area = 0;
    else if (hsizearg( "-size", &argc, &argv, &val, &loc )) {
      if (loc > 1) o->gc_info.is_generational_system = 1;
      if (loc < 0 || loc > o->maxheaps)
	invalid( "-size" );
      else if (loc > 0)
	o->size[loc-1] = val;
      else 
	for ( i=1 ; i < o->maxheaps ; i++ )
	  if (o->size[i-1] == 0) o->size[i-1] = val;
    }
    else if (numbarg( "-steps", &argc, &argv,
		     &o->gc_info.dynamic_np_info.steps )) {
      o->gc_info.is_generational_system = 1;
      o->gc_info.use_non_predictive_collector = 1;
    }
    else if (sizearg( "-stepsize", &argc, &argv,
		     &o->gc_info.dynamic_np_info.stepsize )) {
      o->gc_info.is_generational_system = 1;
      o->gc_info.use_non_predictive_collector = 1;
    }
    else if (sizearg( "-rhash", &argc, &argv, (int*)&o->gc_info.rhash ))
      ;
    else if (sizearg( "-ssb", &argc, &argv, (int*)&o->gc_info.ssb ))
      ;
    else if (doublearg( "-feeling-lucky", &argc, &argv, &feeling_lucky ))
      ;
    else if (doublearg( "-phase-detection", &argc, &argv, &phase_detection ))
      ;
    else 
#endif
    if (numbarg( "-ticks", &argc, &argv, (int*)&o->timerval ))
      ;
    else if (doublearg( "-load", &argc, &argv, &load_factor ))
      ;
    else if (sizearg( "-min", &argc, &argv, &dynamic_min ))
      ;
    else if (sizearg( "-max", &argc, &argv, &dynamic_max ))
      ;
    else if (strcmp( *argv, "-nobreak" ) == 0)
      o->enable_breakpoints = 0;
    else if (strcmp( *argv, "-step" ) == 0)
      o->enable_singlestep = 1;
    else if (strcmp( *argv, "-stats" ) == 0)
      o->show_heapstats = 1;
    else if (strcmp( *argv, "-help" ) == 0 || strcmp( *argv, "-h" ) == 0)
      help();
    else if (strcmp( *argv, "-quiet" ) == 0) 
      o->quiet = 1;
    else if (strcmp( *argv, "-annoy-user" ) == 0)
      o->annoying = 1;
    else if (strcmp( *argv, "-annoy-user-greatly" ) == 0) {
      o->annoying = 1;
      o->supremely_annoying = 1;
    }
    else if (strcmp( *argv, "-flush" ) == 0)
      o->flush = 1;
    else if (strcmp( *argv, "-noflush" ) == 0)
      o->noflush = 1;
    else if (strcmp( *argv, "-reorganize-and-dump" ) == 0)
      o->reorganize_and_dump = 1;
    else if (strcmp( *argv, "-args" ) == 0) {
      o->restc = argc-1;
      o->restv = argv+1;
      break;
    }
#if defined(BDW_GC)
    else if (numbarg( "-divisor", &argc, &argv, &divisor ))
      ;
#endif
    else if (**argv == '-') {
      consolemsg( "Error: Invalid option '%s'", *argv );
      usage();
    }
    else if (o->heapfile != NULL) {
      consolemsg( "Error: Only one heap file allowed." );
      usage();
    }
    else
      o->heapfile = *argv;
  }

  /* Initial validation */
  if (o->gc_info.is_conservative_system &&
      (o->gc_info.is_generational_system || o->gc_info.is_stopcopy_system))
    param_error( "Both precise and conservative gc selected." );
    
  if (o->gc_info.is_generational_system && o->gc_info.is_stopcopy_system)
    param_error( "Both generational and non-generational gc selected." );

  if (!o->gc_info.is_stopcopy_system && !o->gc_info.is_conservative_system
      && o->gc_info.ephemeral_info == 0)
    init_generational( o, areas, "*invalid*" );

  if (dynamic_max && dynamic_min && dynamic_max < dynamic_min)
    param_error( "Expandable MAX is less than expandable MIN." );

  if (dynamic_max || dynamic_min) {
    int n = (o->gc_info.is_generational_system ? areas-1 : 0);

    if (o->size[n] && dynamic_max && o->size[n] > dynamic_max)
      param_error( "Size of expandable area is larger than selected max." );
    if (o->size[n] && dynamic_min && o->size[n] < dynamic_min)
      param_error( "Size of expandable area is smaller than selected min." );
  }

  /* Complete parameter structure by computing the not-specified values. */
  if (o->gc_info.is_generational_system) {
    int n = areas-1;		/* Index of dynamic generation */

    /* Nursery */
    o->gc_info.nursery_info.size_bytes =
      (o->size[0] > 0 ? o->size[0] : DEFAULT_NURSERY_SIZE);

    /* Ephemeral generations */
    prev_size = o->gc_info.nursery_info.size_bytes;

    for ( i = 1 ; i <= areas-2 ; i++ ) {
      if (o->size[i] == 0)
	o->size[i] = prev_size + DEFAULT_EPHEMERAL_INCREMENT;
      o->gc_info.ephemeral_info[i-1].size_bytes = o->size[i];
      prev_size = o->size[i];
    }

    /* Dynamic generation */
    if (o->gc_info.use_non_predictive_collector) {
      int size;

      o->gc_info.dynamic_np_info.load_factor = load_factor;
      o->gc_info.dynamic_np_info.dynamic_max = dynamic_max;
      o->gc_info.dynamic_np_info.dynamic_min = dynamic_min;
      if (o->size[n] != 0)
	o->gc_info.dynamic_np_info.size_bytes = o->size[n];
      size = prev_size + DEFAULT_DYNAMIC_INCREMENT;
      if (dynamic_min) size = max( dynamic_min, size );
      if (dynamic_max) size = min( dynamic_max, size );
      compute_np_parameters( o, size );
      if (feeling_lucky < 0.0 || feeling_lucky > 1.0)
	param_error( "NP luck parameter (-feeling-lucky) out of range." );
      else
	o->gc_info.dynamic_np_info.luck = feeling_lucky;
      if (phase_detection != -1.0 &&
	  (phase_detection < 0.0 || phase_detection > 1.0))
	param_error( "NP phase detection paramater out of range." );
      else
	o->gc_info.dynamic_np_info.phase_detection = phase_detection;
    }
    else {
      o->gc_info.dynamic_sc_info.load_factor = load_factor;
      o->gc_info.dynamic_sc_info.dynamic_max = dynamic_max;
      o->gc_info.dynamic_sc_info.dynamic_min = dynamic_min;
      if (o->size[n] == 0) {
	int size = prev_size + DEFAULT_DYNAMIC_INCREMENT;
	if (dynamic_min) size = max( dynamic_min, size );
	if (dynamic_max) size = min( dynamic_max, size );
	o->gc_info.dynamic_sc_info.size_bytes = size;
      }
      else
	o->gc_info.dynamic_sc_info.size_bytes = o->size[n];
    }
  }
  else if (o->gc_info.is_stopcopy_system) {
    if (o->size[0] == 0) {
      int size = DEFAULT_STOPCOPY_SIZE;

      if (dynamic_min) size = max( dynamic_min, size );
      if (dynamic_max) size = min( dynamic_max, size );
      o->gc_info.sc_info.size_bytes = size;
    }
    else
      o->gc_info.sc_info.size_bytes = o->size[0]; /* Already validated */
    o->gc_info.sc_info.load_factor = load_factor;
    o->gc_info.sc_info.dynamic_min = dynamic_min;
    o->gc_info.sc_info.dynamic_max = dynamic_max;
  }
  else if (o->gc_info.is_conservative_system) {
    o->gc_info.bdw_info.load_factor = load_factor;
    o->gc_info.bdw_info.divisor = divisor;
    o->gc_info.bdw_info.dynamic_min = dynamic_min;
    o->gc_info.bdw_info.dynamic_max = dynamic_max;
  }
}

/* Note that by design, we do not take the load factor into account. */
static void compute_np_parameters( opt_t *o, int suggested_size )
{
  int steps = o->gc_info.dynamic_np_info.steps;
  int stepsize = o->gc_info.dynamic_np_info.stepsize;
  int size = o->gc_info.dynamic_np_info.size_bytes;

  if (steps == 0 && stepsize == 0) {
    if (size == 0)
      size = suggested_size;
    stepsize = DEFAULT_STEPSIZE;
    steps = ceildiv( size, stepsize );
  }
  else if (steps != 0 && stepsize == 0) {
    if (size == 0) {
      stepsize = DEFAULT_STEPSIZE;
      size = stepsize * steps;
    }
    else
      stepsize = size / steps;
  }
  else if (steps == 0 && stepsize != 0) {
    if (size == 0) {
      steps = DEFAULT_STEPS;
      size = stepsize * steps;
    }
    else
      steps = size / stepsize;
  }
  else 
    size = stepsize * steps;

  o->gc_info.dynamic_np_info.steps = steps;
  o->gc_info.dynamic_np_info.stepsize = stepsize;
  o->gc_info.dynamic_np_info.size_bytes = size;
}

static int 
sizearg( char *str, int *argc, char ***argv, int *loc ) 
{
  if (strcmp( **argv, str ) == 0) {
    if (*argc == 1 || !getsize( *(*argv+1), loc )) invalid( str );
    ++*argv; --*argc;
    return 1;
  }
  else
    return 0;
}

static int 
hsizearg( char *str, int *argc, char ***argv, int *var, int *loc ) 
{
  int l = strlen(str);

  if (strncmp( **argv, str, l ) == 0) {
    if (*(**argv+l) != 0) {
      if (sscanf( **argv+strlen(str), "%d", loc ) != 1) invalid( str );
    }
    else
      *loc = 0;
    if (*argc == 1 || !getsize( *(*argv+1), var )) invalid( str );
    ++*argv; --*argc;
    return 1;
  }
  else
    return 0;
}

static int 
numbarg( char *str, int *argc, char ***argv, int *loc )
{
  if (strcmp( **argv, str ) == 0) {
    if (*argc == 1 || sscanf( *(*argv+1), "%d", loc ) != 1 ) invalid( str );
    ++*argv; --*argc;
    return 1;
  }
  else
    return 0;
}

static int 
doublearg( char *str, int *argc, char ***argv, double *loc )
{
  if (strcmp( **argv, str ) == 0) {
    if (*argc == 1 || sscanf( *(*argv+1), "%lf", loc ) != 1 ) invalid( str );
    ++*argv; --*argc;
    return 1;
  }
  else
    return 0;
}

static int 
hnumbarg( char *str, int *argc, char ***argv, int *var, int *loc )
{
  int l = strlen(str);

  if (strncmp( **argv, str, l ) == 0) {
    if (*(**argv+l) != 0) {
      if (sscanf( **argv+strlen(str), "%d", loc ) != 1) invalid( str );
    }
    else
      *loc = 0;
    if (*argc == 1 || sscanf( *(*argv+1), "%d", var ) != 1 ) invalid( str );
    ++*argv; --*argc;
    return 1;
  }
  else
    return 0;
}

static int getsize( char *s, int *p )
{
  int r;
  char c, d;

  r = sscanf( s, "%i%c%c", p, &c, &d );
  if (r == 0) return 0;
  if (r == 1) return 1;
  if (r == 3) return 0;
  if (c == 'M' || c == 'm') {
    *p *= 1024*1024;
    return 1;
  }
  if (c == 'K' || c == 'k') {
    *p *= 1024;
    return 1;
  }
  return 0;
}

static void dump_options( opt_t *o )
{
  int i;

  consolemsg( "" );
  consolemsg( "Command line parameter dump" );
  consolemsg( "---------------------------" );
  consolemsg( "Stepping: %d", o->enable_singlestep );
  consolemsg( "Breakpoints: %d", o->enable_breakpoints );
  consolemsg( "Timer: %d (val=%d)", o->enable_timer, o->timerval );
  consolemsg( "Heap file: %s", o->heapfile );
  consolemsg( "Quiet: %d", o->quiet );
  consolemsg( "Annoying: %d", o->annoying );
  consolemsg( "Supremely annoying: %d", o->supremely_annoying );
  consolemsg( "Flush/noflush: %d/%d", o->flush, o->noflush );
  consolemsg( "Reorganize and dump: %d", o->reorganize_and_dump );
  consolemsg( "" );
  if (o->gc_info.is_conservative_system) {
    consolemsg( "Using conservative garbage collector." );
    consolemsg( "  Incremental: %d", o->gc_info.use_incremental_bdw_collector);
    consolemsg( "  Inverse load factor: %f", o->gc_info.bdw_info.load_factor );
    consolemsg( "  Divisor: %d", o->gc_info.bdw_info.divisor );
    consolemsg( "  Min size: %d", o->gc_info.bdw_info.dynamic_min );
    consolemsg( "  Max size: %d", o->gc_info.bdw_info.dynamic_max );
  }
  else if (o->gc_info.is_stopcopy_system) {
    consolemsg( "Using stop-and-copy garbage collector." );
    consolemsg( "  Size (bytes): %d", o->gc_info.sc_info.size_bytes );
    consolemsg( "  Inverse load factor: %f", o->gc_info.sc_info.load_factor );
    consolemsg( "  Using static area: %d", o->gc_info.use_static_area );
    consolemsg( "  Min size: %d", o->gc_info.sc_info.dynamic_min );
    consolemsg( "  Max size: %d", o->gc_info.sc_info.dynamic_max );
  }
  else if (o->gc_info.is_generational_system) {
    consolemsg( "Using generational garbage collector." );
    consolemsg( "  Nursery" );
    consolemsg( "    Size (bytes): %d", o->gc_info.nursery_info.size_bytes );
    for ( i=1 ; i<= o->gc_info.ephemeral_area_count ; i++ ) {
      consolemsg( "  Ephemeral area %d", i );
      consolemsg( "    Size (bytes): %d",
		  o->gc_info.ephemeral_info[i-1].size_bytes );
    }
    if (o->gc_info.use_non_predictive_collector ) {
      np_info_t *i = &o->gc_info.dynamic_np_info;
      consolemsg( "  Dynamic area (nonpredictive copying)" );
      consolemsg( "    Steps: %d", i->steps );
      consolemsg( "    Step size (bytes): %d", i->stepsize );
      consolemsg( "    Total size (bytes): %d", i->size_bytes );
      consolemsg( "    Inverse load factor: %f", i->load_factor );
      consolemsg( "    Min size: %d", i->dynamic_min );
      consolemsg( "    Max size: %d", i->dynamic_max );
      consolemsg( "    Luck: %f", i->luck );
    }
    else {
      sc_info_t *i = &o->gc_info.dynamic_sc_info;
      consolemsg( "  Dynamic area (normal copying)" );
      consolemsg( "    Size (bytes): %d", i->size_bytes );
      consolemsg( "    Inverse load factor: %f", i->load_factor );
      consolemsg( "    Min size: %d", i->dynamic_min );
      consolemsg( "    Max size: %d", i->dynamic_max );
    }
    consolemsg( "  Using non-predictive dynamic area: %d",
	       o->gc_info.use_non_predictive_collector );
    consolemsg( "  Using static area: %d", o->gc_info.use_static_area );
  }
  else {
    consolemsg( "ERROR: inconsistency: GC type not known." );
    exit( 1 );
  }
  consolemsg( "---------------------------" );
}

static void param_error( char *s )
{
  consolemsg( "Error: %s", s );
  usage();
}

static void invalid( char *s )
{
  consolemsg( "Error: Invalid argument to option '%s'", s );
  consolemsg( "Type \"larceny -help\" for help." );
  exit( 1 );
}

static void usage( void )
{
  consolemsg( "" );
  consolemsg( "Usage: larceny [ options ][ heapfile ][-args arguments]" );
  consolemsg( "Type \"larceny -help\" for help." );
  exit( 1 );
}

static void help( void )
{
  consolemsg("Usage: larceny [options][heapfile][-args arg-to-scheme ...]");
  consolemsg("" );
  consolemsg("Options:" );
#ifndef BDW_GC
  consolemsg("\t-stopcopy       Select stop-and-copy collector." );
  consolemsg("\t-areas    n     Select generational collector with n areas." );
  consolemsg("\t-gen            Select generational collector with %d areas.",
	     DEFAULT_AREAS );
  consolemsg("\t                  (This is the default selection)." );
  consolemsg("\t-nostatic       Don't use the static area." );
  consolemsg("\t-size#    nnnn  Area number '#' is given size 'nnnn' bytes." );
  consolemsg("\t                  (Selects generational collector if # > 1.)");
  consolemsg("\t-np             Use the non-predictive dynamic area." );
  consolemsg("\t                  (Selects generational collector.)" );
  consolemsg("\t-steps    n     Number of steps in the non-predictive gc." );
  consolemsg("\t                  (Selects generational collector.)" );
  consolemsg("\t-stepsize nnnn  Size of each step in non-predictive gc." );
  consolemsg("\t                  (Selects generational collector.)" );
#endif
  consolemsg("\t-min      nnnn  Lower limit on the expandable area." );
  consolemsg("\t-max      nnnn  Upper limit on the expandable area." );
  consolemsg("\t-load     d     Use inverse load factor d for dynamic area");
  consolemsg("\t                  or for stop-and-copy heap." );
  consolemsg("\t-feeling-lucky d  NP gc fudge factor 0.0..1.0.  Default=0.0");
  consolemsg("\t                  (Doesn't select anything else.)");
  consolemsg("\t-phase-detection d  NP gc remset growth factor 0.0..1.0.");
  consolemsg("\t                    Off by default.  Selects nothing else.");
#ifdef BDW_GC
  consolemsg("\t-divisor  n     Allocation divisor s.t. live/n bytes are" );
  consolemsg("\t                 allocated before next GC." );
#endif
  consolemsg("\t-stats          Print startup memory statistics." );
  consolemsg("\t-quiet          Suppress nonessential messages." );
  consolemsg("\t-annoy-user     Print annoying messages." );
  consolemsg("\t-annoy-user-greatly  Print very annoying messages." );
  consolemsg("\t-help           Print this message." );
  consolemsg("");
#ifndef BDW_GC
  consolemsg("\t-rhash    nnnn  Remembered-set hash table size, in elements");
  consolemsg("\t                 (The size must be a power of 2)");
  consolemsg("\t-ssb      nnnn  Remembered-set Sequential Store Buffer (SSB)");
  consolemsg("\t                 size in elements" );
#endif
  consolemsg("\t-ticks    nnnn  Initial timer interval value" );
  consolemsg("\t-nobreak        Disable breakpoints" );
  consolemsg("\t-step           Enable single-stepping" );
#ifndef BDW_GC
  consolemsg("\t-reorganize-and-dump  Split static heap." );
#endif
  consolemsg("" );
  consolemsg("Values can be decimal, octal (0nnn), hex (0xnnn), or suffixed");
  consolemsg("with K (for KB) or M (for MB) when that makes sense." );
  consolemsg("");
  consolemsg("The Larceny User's Manual is available on the web at");
  consolemsg("  http://www.ccs.neu.edu/home/lth/larceny/manual.html");
  exit( 0 );
}

int memfail( int code, char *fmt, ... )
{
  va_list args;
  static char *code_str[] = { "malloc", "heap", "realloc", "calloc", "rts" };

  va_start( args, fmt );
  fprintf( stderr, "Allocation failed (code '%s').\n", code_str[code] );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );
  exit( 1 );
  /* Never returns; return type is `int' to facilitate an idiom. */
  return 0;
}

#if defined(SUNOS4)
int strncasecmp( char*, char*, int ); /* should be in string.h but isn't */
#endif

void conditional_abort( void )
{
  char buf[ 10 ];

  while (1) {
    hardconsolemsg( "Abort (yes/no)?" );
    if (fgets( buf, 10, stdin ) == NULL) {
      hardconsolemsg( "EOF -- exiting." );
      exit(1);
    }
    if (strncasecmp( buf, "yes", 3 ) == 0) abort();
    if (strncasecmp( buf, "no", 2 ) == 0) return;
  }
}


/* eof */
