
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MPI
#include "mpi.h"
#endif

#include "ref_mpi.h"

#include "ref_malloc.h"

REF_INT ref_mpi_n = 1;
REF_INT ref_mpi_id = 0;

#ifdef HAVE_MPI

#define ref_type_mpi_type(macro_ref_type,macro_mpi_type)		\
  switch (macro_ref_type)						\
    {									\
    case REF_INT_TYPE: (macro_mpi_type) = MPI_INT; break;		\
    case REF_DBL_TYPE: (macro_mpi_type) = MPI_DOUBLE; break;		\
    case REF_BYTE_TYPE: (macro_mpi_type) = MPI_UNSIGNED_CHAR; break;	\
    default: (macro_mpi_type) = MPI_DATATYPE_NULL;			\
      RSS( REF_IMPLEMENT, "data type");					\
    }

#endif

static REF_DBL mpi_stopwatch_start_time;
static REF_DBL mpi_stopwatch_first_time;

#ifdef HAVE_MPI
#define ref_mpi_voidptr2comm(ref_mpi)		\
  (*(MPI_Comm *)(( ref_mpi )->comm))
#define ref_mpi_comm(ref_mpi)				\
  ( MPI_COMM_NULL==ref_mpi_voidptr2comm(ref_mpi) ?	\
    MPI_COMM_WORLD : ref_mpi_voidptr2comm(ref_mpi) )
#endif

REF_STATUS ref_mpi_create( REF_MPI *ref_mpi_ptr )
{
  REF_MPI ref_mpi;

  ref_malloc( *ref_mpi_ptr, 1, REF_MPI_STRUCT );
  ref_mpi = ( *ref_mpi_ptr );

#ifdef HAVE_MPI  
  ref_malloc( (MPI_Comm *)(( ref_mpi )->comm), 1, MPI_Comm );
  ref_mpi_voidptr2comm(ref_mpi) = MPI_COMM_NULL;
#else
  ref_mpi->comm = NULL;
#endif

  ref_mpi->id = 0;
  ref_mpi->n = 1;

  ref_mpi->first_time = (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC);
  ref_mpi->start_time = ref_mpi->first_time;

#ifdef HAVE_MPI  
  MPI_Comm_size(ref_mpi_comm(ref_mpi),&(ref_mpi->n));
  MPI_Comm_rank(ref_mpi_comm(ref_mpi),&(ref_mpi->id));

  ref_mpi->first_time = (REF_DBL)MPI_Wtime();
  ref_mpi->start_time = ref_mpi->first_time;
#endif

  ref_mpi_id = ref_mpi->id;
  ref_mpi_n = ref_mpi->n;
  mpi_stopwatch_first_time = ref_mpi->first_time;
  mpi_stopwatch_start_time = ref_mpi->start_time;

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_free( REF_MPI ref_mpi )
{
  if ( NULL == (void *)ref_mpi )
    return REF_NULL;
#ifdef HAVE_MPI
  ref_free( ref_mpi->comm );
#endif
  ref_free( ref_mpi );
  return REF_SUCCESS;
}

REF_STATUS ref_mpi_deep_copy( REF_MPI *ref_mpi_ptr, REF_MPI original )
{
  REF_MPI ref_mpi;

  ref_malloc( *ref_mpi_ptr, 1, REF_MPI_STRUCT );
  ref_mpi = ( *ref_mpi_ptr );

#ifdef HAVE_MPI
  ref_malloc( (MPI_Comm *)(( ref_mpi )->comm), 1, MPI_Comm );
  ref_mpi_voidptr2comm(ref_mpi) = MPI_COMM_NULL;
#else
  ref_mpi->comm = NULL;
#endif
  ref_mpi->id = original->id;
  ref_mpi->n = original->n;

  ref_mpi->first_time = original->first_time;
  ref_mpi->start_time = original->start_time;

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_start( int argc, char *argv[] )
{

#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#else
  SUPRESS_UNUSED_COMPILER_WARNING(argc);
  SUPRESS_UNUSED_COMPILER_WARNING(argv);
#endif

  RSS( ref_mpi_initialize(  ), "init");

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_initialize( void )
{

#ifdef HAVE_MPI

  MPI_Comm_size(MPI_COMM_WORLD,&ref_mpi_n);
  MPI_Comm_rank(MPI_COMM_WORLD,&ref_mpi_id);

  if ( ref_mpi_n > 1 )
    MPI_Barrier( MPI_COMM_WORLD ); 
  mpi_stopwatch_first_time = (REF_DBL)MPI_Wtime();
#else
  ref_mpi_n = 1;
  ref_mpi_id = 0;

  mpi_stopwatch_first_time = (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_stop( )
{

#ifdef HAVE_MPI
  MPI_Finalize( );
#endif

  ref_mpi_n = 1;
  ref_mpi_id = 0;

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_stopwatch_start( void )
{
#ifdef HAVE_MPI
  if ( ref_mpi_n > 1 )
    MPI_Barrier( MPI_COMM_WORLD ); 
  mpi_stopwatch_start_time = (REF_DBL)MPI_Wtime();
#else
  mpi_stopwatch_start_time = (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_stopwatch_stop( const char *message )
{

#ifdef HAVE_MPI
  REF_DBL before_barrier, after_barrier, elapsed;
  REF_DBL first, last;
  before_barrier = (REF_DBL)MPI_Wtime()-mpi_stopwatch_start_time;
  if ( ref_mpi_n > 1 )
    MPI_Barrier( MPI_COMM_WORLD ); 
  after_barrier = (REF_DBL)MPI_Wtime();
  elapsed = after_barrier - mpi_stopwatch_first_time;
  after_barrier = after_barrier-mpi_stopwatch_start_time;
  RSS( ref_mpi_min( &before_barrier, &first, REF_DBL_TYPE), "min");
  RSS( ref_mpi_min( &after_barrier, &last, REF_DBL_TYPE), "max");
  if ( ref_mpi_master )
    {
      printf("%9.4f: %16.12f (%16.12f) %6.2f%% load balance %s\n",
	     elapsed,
	     last,
	     first,
	     ( last>0.0 ? first/last*100.0 : 100.0 ),
	     message );
      fflush(stdout);
    }
  RSS( ref_mpi_stopwatch_start(), "restart" );
#else
  printf("%9.4f: %16.12f (%16.12f) %6.2f%% load balance %s\n",
	 (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC) - 
	 mpi_stopwatch_first_time,
	 (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC) - 
	 mpi_stopwatch_start_time,
	 (REF_DBL)clock(  )/((REF_DBL)CLOCKS_PER_SEC) - 
	 mpi_stopwatch_start_time,
	 110.0,
	 message );
  fflush(stdout);
  RSS( ref_mpi_stopwatch_start(), "restart" );
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_bcast( void *data, REF_INT n, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;

  if ( 1 == ref_mpi_n ) return REF_SUCCESS;

 ref_type_mpi_type(type,datatype);

  MPI_Bcast(data, n, datatype, 0, MPI_COMM_WORLD);
#else
  SUPRESS_UNUSED_COMPILER_WARNING(data);
  SUPRESS_UNUSED_COMPILER_WARNING(n);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_send( void *data, REF_INT n, REF_TYPE type, REF_INT dest )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;
  REF_INT tag;

  ref_type_mpi_type(type,datatype);

  tag = ref_mpi_n*dest+ref_mpi_id;

  MPI_Send(data, n, datatype, dest, tag, MPI_COMM_WORLD);
#else
  SUPRESS_UNUSED_COMPILER_WARNING(data);
  SUPRESS_UNUSED_COMPILER_WARNING(n);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  SUPRESS_UNUSED_COMPILER_WARNING(dest);
  return REF_IMPLEMENT;
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_recv( void *data, REF_INT n, REF_TYPE type, REF_INT source )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;
  REF_INT tag;
  MPI_Status status;

  ref_type_mpi_type(type,datatype);

  tag = ref_mpi_n*ref_mpi_id+source;

  MPI_Recv(data, n, datatype, source, tag, MPI_COMM_WORLD, &status);
#else
  SUPRESS_UNUSED_COMPILER_WARNING(data);
  SUPRESS_UNUSED_COMPILER_WARNING(n);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  SUPRESS_UNUSED_COMPILER_WARNING(source);
  return REF_IMPLEMENT;
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_alltoall( void *send, void *recv, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;

  ref_type_mpi_type(type,datatype);

  MPI_Alltoall(send, 1, datatype, 
	       recv, 1, datatype, 
	       MPI_COMM_WORLD );
#else
  SUPRESS_UNUSED_COMPILER_WARNING(send);
  SUPRESS_UNUSED_COMPILER_WARNING(recv);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  return REF_IMPLEMENT;
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_alltoallv( void *send, REF_INT *send_size, 
			      void *recv, REF_INT *recv_size, 
			      REF_INT n, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;
  REF_INT *send_disp;
  REF_INT *recv_disp;
  REF_INT *send_size_n;
  REF_INT *recv_size_n;
  REF_INT part;

  ref_type_mpi_type(type,datatype);

  send_size_n =(REF_INT *)malloc(ref_mpi_n*sizeof(REF_INT));
  RNS(send_size_n,"malloc failed");
  for ( part = 0; part<ref_mpi_n ; part++ )
    send_size_n[part] = n*send_size[part];

  recv_size_n =(REF_INT *)malloc(ref_mpi_n*sizeof(REF_INT));
  RNS(recv_size_n,"malloc failed");
  for ( part = 0; part<ref_mpi_n ; part++ )
    recv_size_n[part] = n*recv_size[part];

  send_disp =(REF_INT *)malloc(ref_mpi_n*sizeof(REF_INT));
  RNS(send_disp,"malloc failed");
  send_disp[0] = 0;
  for ( part = 1; part<ref_mpi_n ; part++ )
    send_disp[part] = send_disp[part-1]+send_size_n[part-1];

  recv_disp =(REF_INT *)malloc(ref_mpi_n*sizeof(REF_INT));
  RNS(recv_disp,"malloc failed");
  recv_disp[0] = 0;
  for ( part = 1; part<ref_mpi_n ; part++ )
    recv_disp[part] = recv_disp[part-1]+recv_size_n[part-1];

  MPI_Alltoallv(send, send_size_n, send_disp, datatype, 
		recv, recv_size_n, recv_disp, datatype, 
		MPI_COMM_WORLD );

  free(recv_disp);
  free(send_disp);
  free(recv_size_n);
  free(send_size_n);

#else
  SUPRESS_UNUSED_COMPILER_WARNING(send);
  SUPRESS_UNUSED_COMPILER_WARNING(send_size);
  SUPRESS_UNUSED_COMPILER_WARNING(recv);
  SUPRESS_UNUSED_COMPILER_WARNING(recv_size);
  SUPRESS_UNUSED_COMPILER_WARNING(n);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  return REF_IMPLEMENT;
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_min( void *input, void *output, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;

  if ( ref_mpi_n == 1 )
    {
      switch (type)
	{
	case REF_INT_TYPE: *(REF_INT *)output = *(REF_INT *)input; break;
	case REF_DBL_TYPE: *(REF_DBL *)output = *(REF_DBL *)input; break;
	default: RSS( REF_IMPLEMENT, "data type");
	}
      return REF_SUCCESS;
    }
  ref_type_mpi_type(type,datatype);

  MPI_Reduce( input, output, 1, datatype, MPI_MIN, 0, MPI_COMM_WORLD);

#else
  switch (type)
    {
    case REF_INT_TYPE: *(REF_INT *)output = *(REF_INT *)input; break;
    case REF_DBL_TYPE: *(REF_DBL *)output = *(REF_DBL *)input; break;
    default: RSS( REF_IMPLEMENT, "data type");
    }
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_all_or( REF_BOOL *boolean )
{
#ifdef HAVE_MPI
  REF_BOOL output;

  if ( 1 == ref_mpi_n ) return REF_SUCCESS;

  MPI_Allreduce( boolean, &output, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  *boolean = MIN(output,1);

#else
  SUPRESS_UNUSED_COMPILER_WARNING(boolean);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_max( void *input, void *output, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;

  if ( ref_mpi_n == 1 )
    {
      switch (type)
	{
	case REF_INT_TYPE: *(REF_INT *)output = *(REF_INT *)input; break;
	case REF_DBL_TYPE: *(REF_DBL *)output = *(REF_DBL *)input; break;
	default: RSS( REF_IMPLEMENT, "data type");
	}
      return REF_SUCCESS;
    }
  ref_type_mpi_type(type,datatype);

  MPI_Reduce( input, output, 1, datatype, MPI_MAX, 0, MPI_COMM_WORLD);

#else
  switch (type)
    {
    case REF_INT_TYPE: *(REF_INT *)output = *(REF_INT *)input; break;
    case REF_DBL_TYPE: *(REF_DBL *)output = *(REF_DBL *)input; break;
    default: RSS( REF_IMPLEMENT, "data type");
    }
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_sum( void *input, void *output, REF_INT n, REF_TYPE type )
{
  REF_INT i;
#ifdef HAVE_MPI
  MPI_Datatype datatype;

  if ( 1 == ref_mpi_n )
    {
      switch (type)
	{
	case REF_INT_TYPE: 
	  for (i=0;i<n;i++)
	    ((REF_INT *)output)[i] = ((REF_INT *)input)[i]; 
	  break;
	case REF_DBL_TYPE:
	  for (i=0;i<n;i++)
	    ((REF_DBL *)output)[i] = ((REF_DBL *)input)[i]; 
	  break;
	default: RSS( REF_IMPLEMENT, "data type");
	}
    }
  else
    {
      ref_type_mpi_type(type,datatype);
      MPI_Reduce( input, output, n, datatype, MPI_SUM, 0, MPI_COMM_WORLD);
    }

#else
  switch (type)
    {
    case REF_INT_TYPE: 
      for (i=0;i<n;i++)
	((REF_INT *)output)[i] = ((REF_INT *)input)[i]; 
      break;
    case REF_DBL_TYPE:
      for (i=0;i<n;i++)
	((REF_DBL *)output)[i] = ((REF_DBL *)input)[i]; 
      break;
    default: RSS( REF_IMPLEMENT, "data type");
    }
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_allgather( void *scalar, void *array, REF_TYPE type )
{
#ifdef HAVE_MPI
  MPI_Datatype datatype;
  
  if ( 1 == ref_mpi_n ) 
    {
      switch (type)
	{
	case REF_INT_TYPE: *(REF_INT *)array = *(REF_INT *)scalar; break;
	case REF_DBL_TYPE: *(REF_DBL *)array = *(REF_DBL *)scalar; break;
	default: RSS( REF_IMPLEMENT, "data type");
	}
      return REF_SUCCESS;
    }

  ref_type_mpi_type(type,datatype);

  MPI_Allgather( scalar, 1, datatype, 
		 array, 1, datatype, 
		 MPI_COMM_WORLD);

#else
  switch (type)
    {
    case REF_INT_TYPE: *(REF_INT *)array = *(REF_INT *)scalar; break;
    case REF_DBL_TYPE: *(REF_DBL *)array = *(REF_DBL *)scalar; break;
    default: RSS( REF_IMPLEMENT, "data type");
    }
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_mpi_allgatherv( void *local_array, REF_INT *counts, 
			       void *concatenated_array, REF_TYPE type )
{
#ifdef HAVE_MPI
  REF_INT proc;
  REF_INT *displs;
  MPI_Datatype datatype;
  REF_INT i;
  ref_type_mpi_type(type,datatype);

  if ( 1 == ref_mpi_n ) 
    {
      switch (type)
	{
	case REF_INT_TYPE: 
	  for (i=0;i<counts[0];i++)
	    ((REF_INT *)concatenated_array)[i] = ((REF_INT *)local_array)[i]; 
	  break;
	case REF_DBL_TYPE:
	  for (i=0;i<counts[0];i++)
	    ((REF_DBL *)concatenated_array)[i] = ((REF_DBL *)local_array)[i]; 
	  break;
	default: RSS( REF_IMPLEMENT, "data type");
	}
      return REF_SUCCESS;
    }

  ref_malloc( displs, ref_mpi_n, REF_INT );

  displs[0] = 0;
  for (proc=1;proc<ref_mpi_n;proc++)
    displs[proc] = displs[proc-1] + counts[proc-1];

  MPI_Allgatherv( local_array, counts[ref_mpi_id], datatype, 
		  concatenated_array, counts, displs, datatype, 
		  MPI_COMM_WORLD);

  ref_free( displs );

#else
  REF_INT i;
  switch (type)
    {
    case REF_INT_TYPE: 
      for (i=0;i<counts[0];i++)
	((REF_INT *)concatenated_array)[i] = ((REF_INT *)local_array)[i]; 
      break;
    case REF_DBL_TYPE:
      for (i=0;i<counts[0];i++)
	((REF_DBL *)concatenated_array)[i] = ((REF_DBL *)local_array)[i]; 
      break;
    default: RSS( REF_IMPLEMENT, "data type");
    }
  SUPRESS_UNUSED_COMPILER_WARNING(type);
#endif

  return REF_SUCCESS;
}

