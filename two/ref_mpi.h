
#ifndef REF_MPI_H
#define REF_MPI_H

#include "ref_defs.h"

BEGIN_C_DECLORATION

extern REF_INT ref_mpi_n;
extern REF_INT ref_mpi_id;

#define ref_mpi_master (0 == ref_mpi_id)

typedef int REF_TYPE;
#define REF_INT_TYPE (1)

REF_STATUS ref_mpi_start( int argc, char *argv[] );
REF_STATUS ref_mpi_stop( );

REF_STATUS ref_mpi_bcast( void *data, REF_INT n, REF_TYPE type );
REF_STATUS ref_mpi_send( void *data, REF_INT n, REF_TYPE type, REF_INT dest );
REF_STATUS ref_mpi_recv( void *data, REF_INT n, REF_TYPE type, REF_INT source );

END_C_DECLORATION

#endif /* REF_MPI_H */
