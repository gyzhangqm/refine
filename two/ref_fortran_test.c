#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_fortran.h"
#include "ref_test.h"
#include "ref_grid.h"
#include "ref_node.h"
#include "ref_cell.h"
#include "ref_adj.h"

int main( void )
{
  REF_INT nnodes;
  REF_INT nnodesg;
  REF_INT *l2g;
  REF_INT *part; 
  REF_INT partition;
  REF_DBL *x;
  REF_DBL *y; 
  REF_DBL *z;

  nnodes = 4;
  nnodesg = 4;

  l2g  = (REF_INT *) malloc( sizeof(REF_INT) * nnodes );
  part = (REF_INT *) malloc( sizeof(REF_INT) * nnodes );
  x = (REF_DBL *) malloc( sizeof(REF_DBL) * nnodes );
  y = (REF_DBL *) malloc( sizeof(REF_DBL) * nnodes );
  z = (REF_DBL *) malloc( sizeof(REF_DBL) * nnodes );

  l2g[0] = 3; part[0] = 1; x[0] = 0; y[0] = 1; z[0] = 0;
  l2g[1] = 4; part[0] = 1; x[1] = 0; y[1] = 0; z[1] = 1;
  l2g[2] = 1; part[0] = 2; x[2] = 0; y[2] = 0; z[2] = 0;
  l2g[3] = 2; part[0] = 2; x[3] = 1; y[3] = 0; z[3] = 0;

  partition = 1;

  TSS(ref_init_node__(&nnodes, &nnodesg,
		      l2g, part, &partition,
		      x, y, z),"init node");

  TSS(ref_free_(),"free");

  free(z);
  free(y);
  free(x);
  free(part);
  free(l2g);
  return 0;
}
