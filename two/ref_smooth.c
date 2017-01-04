
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ref_smooth.h"
#include "ref_adapt.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_cell.h"
#include "ref_mpi.h"
#include "ref_twod.h"

REF_STATUS ref_smooth_tri_steepest_descent( REF_GRID ref_grid, REF_INT node )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL f, d[3];
  REF_DBL dcost, dcost_dl, dl;
  REF_BOOL verbose = REF_FALSE;

  each_ref_cell_having_node( ref_cell, node, item, cell )
  {
    RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
    RSS( ref_node_tri_dquality_dnode0(ref_node, nodes,
                                      &f, d), "qual deriv" );
    if (verbose)
      printf("cost %10.8f : %12.8f %12.8f %12.8f\n",f,d[0],d[1],d[2]);
  }

  dcost = 1.0-f;
  dcost_dl = sqrt(d[0]*d[0]+d[1]*d[1]*d[2]*d[2]);
  dl = dcost/dcost_dl;

  ref_node_xyz(ref_node,0,node) += dl*d[0];
  ref_node_xyz(ref_node,1,node) += dl*d[1];
  ref_node_xyz(ref_node,2,node) += dl*d[2];

  each_ref_cell_having_node( ref_cell, node, item, cell )
  {
    RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
    RSS( ref_node_tri_dquality_dnode0(ref_node, nodes,
                                      &f, d), "qual deriv" );
  }

  if (verbose)
    printf("rate %12.8f dcost %12.8f dl %12.8f\n",
           (1.0-f)/dcost,dcost,dl);


  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_quality_around( REF_GRID ref_grid,
                                          REF_INT node,
                                          REF_DBL *min_quality )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL none_found = REF_TRUE;
  REF_DBL quality;

  *min_quality = 1.0;
  each_ref_cell_having_node( ref_cell, node, item, cell )
  {
    RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
    none_found = REF_FALSE;
    RSS( ref_node_tri_quality( ref_node,
                               nodes,
                               &quality ), "qual" );
    *min_quality = MIN( *min_quality, quality );
  }

  if ( none_found )
    {
      *min_quality = -2.0;
      return REF_NOT_FOUND;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_outward_norm( REF_GRID ref_grid, 
				    REF_INT node,
				    REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL normal[3];

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_having_node( ref_cell, node, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );

      RSS( ref_node_tri_normal( ref_node,nodes,normal ), "norm");

      if ( ( ref_node_xyz(ref_node,1,nodes[0]) > 0.5 &&
	     normal[1] >= 0.0 ) ||
	   ( ref_node_xyz(ref_node,1,nodes[0]) < 0.5 &&
	     normal[1] <= 0.0 ) ) return REF_SUCCESS;
    }

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_ideal( REF_GRID ref_grid,
                                 REF_INT node,
                                 REF_INT tri,
                                 REF_DBL *ideal_location )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT n0, n1;
  REF_INT ixyz;
  REF_DBL dn[3];
  REF_DBL dt[3];
  REF_DBL tangent_length, projection, scale, length_in_metric;

  RSS(ref_cell_nodes(ref_grid_tri(ref_grid), tri, nodes ), "get tri");
  n0 = REF_EMPTY; n1 = REF_EMPTY;
  if ( node == nodes[0])
    {
      n0 = nodes[1];
      n1 = nodes[2];
    }
  if ( node == nodes[1])
    {
      n0 = nodes[2];
      n1 = nodes[0];
    }
  if ( node == nodes[2])
    {
      n0 = nodes[0];
      n1 = nodes[1];
    }
  if ( n0==REF_EMPTY || n1==REF_EMPTY)
    THROW("empty triangle side");

  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] = 0.5*( ref_node_xyz(ref_node,ixyz,n0) +
                                 ref_node_xyz(ref_node,ixyz,n1) );
  for (ixyz = 0; ixyz<3; ixyz++)
    dn[ixyz] = ref_node_xyz(ref_node,ixyz,node) - ideal_location[ixyz];
  for (ixyz = 0; ixyz<3; ixyz++)
    dt[ixyz] = ref_node_xyz(ref_node,ixyz,n1) - ref_node_xyz(ref_node,ixyz,n0);

  tangent_length = ref_math_dot(dt,dt);
  projection = ref_math_dot(dn,dt);

  if ( ref_math_divisible(projection,tangent_length) )
    {
      for (ixyz = 0; ixyz<3; ixyz++)
        dn[ixyz] -=  (projection/tangent_length) * dt[ixyz];
    }
  else
    {
      printf("projection = %e tangent_length = %e\n",
             projection,tangent_length);
      return REF_DIV_ZERO;
    }

  RSS( ref_math_normalize( dn ), "normalize direction" );
  /* would an averaged metric be more appropriate? */
  length_in_metric =
    ref_matrix_sqrt_vt_m_v( ref_node_metric_ptr(ref_node,node), dn );

  scale = 0.5*sqrt(3.0); /* altitude of equilateral triangle */
  if ( ref_math_divisible(scale,length_in_metric) )
    {
      scale = scale/length_in_metric;
    }
  else
    {
      printf(" length_in_metric = %e, not invertable\n", length_in_metric);
      return REF_DIV_ZERO;
    }

  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] += scale*dn[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_ideal_uv( REF_GRID ref_grid,
				    REF_INT node,
				    REF_INT tri,
				    REF_DBL *ideal_uv )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT n0, n1;
  REF_INT id;
  REF_DBL r0, r1;
  REF_DBL uv_orig[2];
  REF_DBL uv0[2], uv1[2];
  REF_DBL q0, q1;
  REF_DBL omega = 0.25;
  
  RSS(ref_cell_nodes(ref_grid_tri(ref_grid), tri, nodes ), "get tri");
  n0 = REF_EMPTY; n1 = REF_EMPTY;
  if ( node == nodes[0])
    {
      n0 = nodes[1];
      n1 = nodes[2];
    }
  if ( node == nodes[1])
    {
      n0 = nodes[2];
      n1 = nodes[0];
    }
  if ( node == nodes[2])
    {
      n0 = nodes[0];
      n1 = nodes[1];
    }
  if ( n0==REF_EMPTY || n1==REF_EMPTY)
    THROW("empty triangle side");

  RSS( ref_geom_unique_id( ref_geom, node, REF_GEOM_FACE, &id ), "id");
  RSS( ref_geom_tuv( ref_geom, node, REF_GEOM_FACE, id, uv_orig ), "uv" );
  RSS( ref_geom_tuv( ref_geom, n0, REF_GEOM_FACE, id, uv0 ), "uv0" );
  RSS( ref_geom_tuv( ref_geom, n1, REF_GEOM_FACE, id, uv1 ), "uv1" );
  uv0[0] = uv_orig[0] - uv0[0];
  uv0[1] = uv_orig[1] - uv0[1];
  uv1[0] = uv_orig[0] - uv1[0];
  uv1[1] = uv_orig[1] - uv1[1];
  
  RSS( ref_node_ratio(ref_node,n0,node,&r0), "get r0" );
  RSS( ref_node_ratio(ref_node,n1,node,&r1), "get r1" );
  RSS( ref_node_tri_quality( ref_node,
			     nodes,
			     &q0 ), "qual" );
  
  printf(" orig  r %f %f q %f\n",r0,r1,q0);

  ideal_uv[0]= uv_orig[0] + omega*uv0[0]*(1.0-r0)/r0 + omega*uv1[0]*(1.0-r1)/r1;
  ideal_uv[1]= uv_orig[1] + omega*uv0[1]*(1.0-r0)/r0 + omega*uv1[1]*(1.0-r1)/r1;

  RSS( ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, ideal_uv ), "set uv");
  RSS( ref_geom_constrain(ref_grid, node ), "constrain");

  RSS( ref_node_ratio(ref_node,n0,node,&r0), "get r0" );
  RSS( ref_node_ratio(ref_node,n1,node,&r1), "get r1" );
  RSS( ref_node_tri_quality( ref_node,
			     nodes,
			     &q1 ), "qual" );
  
  printf(" ideal r %f %f q %f\n",r0,r1,q1);

  RSS( ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv_orig ), "set uv");

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_weighted_ideal( REF_GRID ref_grid,
					  REF_INT node,
					  REF_DBL *ideal_location )
{
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ixyz;
  REF_DBL tri_ideal[3];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] = 0.0;

  each_ref_cell_having_node( ref_grid_tri(ref_grid), node, item, cell )
    {
      RSS( ref_smooth_tri_ideal( ref_grid, node, cell, 
				 tri_ideal ), "tri ideal");
      RSS( ref_cell_nodes( ref_grid_tri(ref_grid), cell, nodes ), "nodes" );
      RSS( ref_node_tri_quality( ref_grid_node(ref_grid), 
				 nodes,  
				 &quality ), "tri qual");
      quality = MAX(quality,ref_adapt_smooth_min_quality);
      weight = 1.0/quality;
      normalization += weight;
      for (ixyz = 0; ixyz<3; ixyz++)
	ideal_location[ixyz] += weight*tri_ideal[ixyz];
    }

  if ( ref_math_divisible(1.0,normalization) )
    {
      for (ixyz = 0; ixyz<3; ixyz++)
        ideal_location[ixyz] =  (1.0/normalization) * ideal_location[ixyz];
    }
  else
    {
      printf("normalization = %e\n",normalization);
      return REF_DIV_ZERO;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_weighted_ideal_uv( REF_GRID ref_grid,
					     REF_INT node,
					     REF_DBL *ideal_uv )
{
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT iuv;
  REF_DBL tri_uv[2];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (iuv = 0; iuv<2; iuv++)
    ideal_uv[iuv] = 0.0;

  each_ref_cell_having_node( ref_grid_tri(ref_grid), node, item, cell )
    {
      RSS( ref_smooth_tri_ideal_uv( ref_grid, node, cell, 
				    tri_uv ), "tri ideal");
      RSS( ref_cell_nodes( ref_grid_tri(ref_grid), cell, nodes ), "nodes" );
      RSS( ref_node_tri_quality( ref_grid_node(ref_grid), 
				 nodes,  
				 &quality ), "tri qual");
      quality = MAX(quality,ref_adapt_smooth_min_quality);
      weight = 1.0/quality;
      normalization += weight;
      for (iuv = 0; iuv<2; iuv++)
	ideal_uv[iuv] += weight*tri_uv[iuv];
    }

  if ( ref_math_divisible(1.0,normalization) )
    {
      for (iuv = 0; iuv<2; iuv++)
        ideal_uv[iuv] =  (1.0/normalization) * ideal_uv[iuv];
    }
  else
    {
      printf("normalization = %e\n",normalization);
      return REF_DIV_ZERO;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_improve( REF_GRID ref_grid,
				   REF_INT node )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT tries;
  REF_DBL ideal[3], original[3];
  REF_DBL backoff, quality0, quality;
  REF_INT ixyz, opposite;
  REF_BOOL allowed;

  /* can't handle boundaries yet */
  if ( !ref_cell_node_empty( ref_grid_qua( ref_grid ), node ) )
    return REF_SUCCESS;

  for (ixyz = 0; ixyz<3; ixyz++)
    original[ixyz] = ref_node_xyz(ref_node,ixyz,node);

  RSS( ref_smooth_tri_weighted_ideal( ref_grid, node, ideal ), "ideal" );

  RSS( ref_smooth_tri_quality_around( ref_grid, node, &quality0),"q");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++)
    {
      for (ixyz = 0; ixyz<3; ixyz++)
	ref_node_xyz(ref_node,ixyz,node) = backoff*ideal[ixyz] +
	  (1.0 - backoff) * original[ixyz];
      RSS( ref_smooth_outward_norm( ref_grid, node, &allowed ), "normals" );
      if ( allowed )
	{
	  RSS( ref_smooth_tri_quality_around( ref_grid, node, &quality),"q");
	  if ( quality > quality0 )
	    {
	      /* update opposite side: X and Z only */
	      RSS( ref_twod_opposite_node( ref_grid_pri(ref_grid), 
					   node, &opposite ),"opp");
	      ref_node_xyz(ref_node,0,opposite) = ref_node_xyz(ref_node,0,node);
	      ref_node_xyz(ref_node,2,opposite) = ref_node_xyz(ref_node,2,node);
	      return REF_SUCCESS;
	    }
	}
      backoff *= 0.5;
    }

  for (ixyz = 0; ixyz<3; ixyz++)
    ref_node_xyz(ref_node,ixyz,node) = original[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_local_pris_about( REF_GRID ref_grid, 
					REF_INT about_node, 
					REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed =  REF_FALSE;

  ref_cell = ref_grid_pri(ref_grid);

  each_ref_cell_having_node( ref_cell, about_node, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;

  *allowed =  REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_twod_pass( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_BOOL allowed;

  each_ref_node_valid_node( ref_node, node )
    {
      RSS( ref_node_node_twod( ref_node, node, &allowed ), "twod" );
      if ( !allowed ) continue;

      /* can't handle boundaries yet */
      allowed = ref_cell_node_empty( ref_grid_qua( ref_grid ), node );
      if ( !allowed ) continue;

      RSS( ref_smooth_local_pris_about( ref_grid, node, &allowed ), "para" );
      if ( !allowed )
	{
	  ref_node_age(ref_node,node)++;
	  continue;
	}

      ref_node_age(ref_node,node) = 0;
      RSS( ref_smooth_tri_improve( ref_grid, node ), "improve" );
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_quality_around( REF_GRID ref_grid,
                                          REF_INT node,
                                          REF_DBL *min_quality )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL none_found = REF_TRUE;
  REF_DBL quality;

  *min_quality = 1.0;
  each_ref_cell_having_node( ref_cell, node, item, cell )
  {
    RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
    none_found = REF_FALSE;
    RSS( ref_node_tet_quality( ref_node,
                               nodes,
                               &quality ), "qual" );
    *min_quality = MIN( *min_quality, quality );
  }

  if ( none_found )
    {
      *min_quality = -2.0;
      return REF_NOT_FOUND;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_ideal( REF_GRID ref_grid,
                                 REF_INT node,
                                 REF_INT tet,
                                 REF_DBL *ideal_location )
{
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tet_node, tri_node;
  REF_INT ixyz;
  REF_DBL dn[3];
  REF_DBL scale, length_in_metric;

  RSS(ref_cell_nodes(ref_cell, tet, nodes ), "get tri");
  tri_nodes[0] = REF_EMPTY; tri_nodes[1] = REF_EMPTY; tri_nodes[2] = REF_EMPTY;
  for ( tet_node=0 ; tet_node<4; tet_node++ )
    if ( node == nodes[tet_node])
      {
	for ( tri_node=0 ; tri_node<3; tri_node++ )
	  tri_nodes[tri_node] = 
	    nodes[ref_cell_f2n_gen(ref_cell,tri_node,tet_node)];
      }
  if ( tri_nodes[0] == REF_EMPTY ||
       tri_nodes[1] == REF_EMPTY ||
       tri_nodes[2] == REF_EMPTY )
    THROW("empty tetrahedra face");

  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] = ( ref_node_xyz(ref_node,ixyz,tri_nodes[0]) +
			     ref_node_xyz(ref_node,ixyz,tri_nodes[1]) +
			     ref_node_xyz(ref_node,ixyz,tri_nodes[2]) ) / 3.0;

  RSS( ref_node_tri_normal( ref_node, tri_nodes, dn ), "tri normal" );

  RSS( ref_math_normalize( dn ), "normalize direction" );
  /* would an averaged metric be more appropriate? */
  length_in_metric =
    ref_matrix_sqrt_vt_m_v( ref_node_metric_ptr(ref_node,node), dn );

  scale = sqrt(6.0)/3.0; /* altitude of regular tetrahedra */
  if ( ref_math_divisible(scale,length_in_metric) )
    {
      scale = scale/length_in_metric;
    }
  else
    {
      printf(" length_in_metric = %e, not invertable\n", length_in_metric);
      return REF_DIV_ZERO;
    }

  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] += scale*dn[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_weighted_ideal( REF_GRID ref_grid,
					  REF_INT node,
					  REF_DBL *ideal_location )
{
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ixyz;
  REF_DBL tet_ideal[3];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (ixyz = 0; ixyz<3; ixyz++)
    ideal_location[ixyz] = 0.0;

  each_ref_cell_having_node( ref_grid_tet(ref_grid), node, item, cell )
    {
      RSS( ref_smooth_tet_ideal( ref_grid, node, cell, 
				 tet_ideal ), "tet ideal");
      RSS( ref_cell_nodes( ref_grid_tet(ref_grid), cell, nodes ), "nodes" );
      RSS( ref_node_tet_quality( ref_grid_node(ref_grid), 
				 nodes,  
				 &quality ), "tet qual");
      quality = MAX(quality,ref_adapt_smooth_min_quality);
      weight = 1.0/quality;
      normalization += weight;
      for (ixyz = 0; ixyz<3; ixyz++)
	ideal_location[ixyz] += weight*tet_ideal[ixyz];
    }

  if ( ref_math_divisible(1.0,normalization) )
    {
      for (ixyz = 0; ixyz<3; ixyz++)
        ideal_location[ixyz] =  (1.0/normalization) * ideal_location[ixyz];
    }
  else
    {
      printf("normalization = %e\n",normalization);
      return REF_DIV_ZERO;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_improve( REF_GRID ref_grid,
				   REF_INT node )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT tries;
  REF_DBL ideal[3], original[3];
  REF_DBL backoff, quality0, quality;
  REF_INT ixyz;

  /* can't handle boundaries yet */
  if ( !ref_cell_node_empty( ref_grid_tri( ref_grid ), node ) ||
       !ref_cell_node_empty( ref_grid_qua( ref_grid ), node ) )
    return REF_SUCCESS;

  for (ixyz = 0; ixyz<3; ixyz++)
    original[ixyz] = ref_node_xyz(ref_node,ixyz,node);

  RSS( ref_smooth_tet_weighted_ideal( ref_grid, node, ideal ), "ideal" );

  RSS( ref_smooth_tet_quality_around( ref_grid, node, &quality0),"q");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++)
    {
      for (ixyz = 0; ixyz<3; ixyz++)
	ref_node_xyz(ref_node,ixyz,node) = backoff*ideal[ixyz] +
	  (1.0 - backoff) * original[ixyz];
      RSS( ref_smooth_tet_quality_around( ref_grid, node, &quality),"q");
      if ( quality > quality0 )
	{
	  return REF_SUCCESS;
	}
      backoff *= 0.5;
    }

  for (ixyz = 0; ixyz<3; ixyz++)
    ref_node_xyz(ref_node,ixyz,node) = original[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_local_tet_about( REF_GRID ref_grid, 
				       REF_INT about_node, 
				       REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed =  REF_FALSE;

  ref_cell = ref_grid_tet(ref_grid);

  each_ref_cell_having_node( ref_cell, about_node, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;

  *allowed =  REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_geom_edge( REF_GRID ref_grid,
				 REF_INT node )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL edg = ref_grid_edg(ref_grid);
  REF_BOOL geom_node, geom_edge;
  REF_INT id;
  REF_INT nodes[2], nnode;
  REF_DBL t_orig, t0, t1;
  REF_DBL r0, r1;
  REF_DBL q_orig;
  REF_DBL s_orig, rsum;

  REF_DBL t,st,sr,q,backoff,t_target;
  REF_INT tries;
  REF_BOOL verbose = REF_FALSE;
  
  RSS( ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node check");
  RSS( ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "edge check");
  RAS( !geom_node, "geom node not allowed" );
  RAS( geom_edge, "geom edge required" );

  RSS( ref_geom_unique_id(ref_geom,node,REF_GEOM_EDGE,&id), "get id" );
  RSS( ref_cell_node_list_around( edg, node, 2,
				  &nnode, nodes ), "edge neighbors");
  REIS( 2, nnode, "expected two nodes" );

  RSS( ref_node_ratio(ref_node,nodes[0],node,&r0), "get r0" );
  RSS( ref_node_ratio(ref_node,nodes[1],node,&r1), "get r1" );

  rsum = r1+r0;
  if ( ref_math_divisible(r0,rsum) )
    {
      s_orig = r0/rsum;
      /* one percent imblance is good enough */
      if ( ABS(s_orig-0.5) < 0.01 ) return REF_SUCCESS;
    }
  else
    {
      printf("div zero %e r0 %e r1\n",r1,r0);
      return REF_DIV_ZERO;
    }

  RSS( ref_geom_tuv(ref_geom,nodes[0],REF_GEOM_EDGE, id, &t0), "get t0" );
  RSS( ref_geom_tuv(ref_geom,node,REF_GEOM_EDGE, id, &t_orig), "get t_orig" );
  RSS( ref_geom_tuv(ref_geom,nodes[1],REF_GEOM_EDGE, id, &t1), "get t1" );
  RSS( ref_smooth_tet_quality_around( ref_grid, node, &q_orig ), "q_orig");
  
  if (verbose) printf("edge %d t %f %f %f r %f %f q %f\n",
		      id,t0,t_orig,t1,r0,r1,q_orig);

  sr = r0/(r1+r0);
  st = (t_orig-t0)/(t1-t0);
  st = st + (0.5-sr);
  t_target = st*t1+(1.0-st)*t0; 

  if (verbose) printf("t_target %f sr %f st %f %f \n",
		      t_target, sr, (t_orig-t0)/(t1-t0), st );
  
  backoff = 1.0;
  for (tries = 0; tries < 8; tries++)
    {
      t = backoff * t_target + (1.0-backoff) * t_orig;
  
      RSS( ref_geom_add(ref_geom, node, REF_GEOM_EDGE, id, &t ), "set t");
      RSS( ref_geom_constrain(ref_grid, node ), "constrain");
      RSS( ref_node_ratio(ref_node,nodes[0],node,&r0), "get r0" );
      RSS( ref_node_ratio(ref_node,nodes[1],node,&r1), "get r1" );
      RSS( ref_smooth_tet_quality_around( ref_grid, node, &q ), "q");
  
      if (verbose) printf("t %f r %f %f q %f \n", t, r0, r1, q );
      if ( q > ref_adapt_smooth_min_quality )
	{
	  return REF_SUCCESS;
	}
      backoff *= 0.5;
    }

  RSS( ref_geom_add(ref_geom, node, REF_GEOM_EDGE, id, &t_orig ), "set t");
  RSS( ref_geom_constrain(ref_grid, node ), "constrain");
  RSS( ref_smooth_tet_quality_around( ref_grid, node, &q ), "q");
  
  if (verbose) printf("undo q %f\n",q);

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_geom_face( REF_GRID ref_grid,
				 REF_INT node )
{
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_BOOL geom_node, geom_edge, geom_face, no_quads;
  REF_INT id;
  REF_DBL uv_orig[2], uv_ideal[2];
  REF_DBL qtet_orig, qtri_orig;
  REF_DBL qtri, qtet;
  REF_DBL backoff, uv[2];
  REF_INT tries, iuv;
  RSS( ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node check");
  RSS( ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "edge check");
  RSS( ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &geom_face), "face check");
  no_quads = ref_cell_node_empty( ref_grid_qua( ref_grid ), node );
  RAS( !geom_node, "geom node not allowed" );
  RAS( !geom_edge, "geom edge not allowed" );
  RAS( geom_face, "geom face required" );
  RAS( no_quads,  "quads not allowed" );

  RSS( ref_geom_unique_id(ref_geom,node,REF_GEOM_FACE,&id), "get id" );
  RSS( ref_geom_tuv(ref_geom,node,REF_GEOM_FACE, id, uv_orig), "get uv_orig" );
  RSS( ref_smooth_tet_quality_around( ref_grid, node, &qtet_orig ), "q tet");
  RSS( ref_smooth_tri_quality_around( ref_grid, node, &qtri_orig ), "q tri");

  printf("uv %f %f tri %f tet %f\n",uv_orig[0],uv_orig[1],qtri_orig,qtet_orig);

  RSS( ref_smooth_tri_weighted_ideal_uv( ref_grid, node,uv_ideal ),"ideal");
  
  backoff = 1.0;
  for (tries = 0; tries < 8; tries++)
    {
      for (iuv = 0; iuv<2; iuv++)
	uv[iuv] = backoff*uv_ideal[iuv] +
	  (1.0 - backoff) * uv_orig[iuv];
      
      RSS( ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv ), "set uv");
      RSS( ref_geom_constrain(ref_grid, node ), "constrain");
      RSS( ref_smooth_tet_quality_around( ref_grid, node, &qtet ), "q tet");
      RSS( ref_smooth_tri_quality_around( ref_grid, node, &qtri ), "q tri");
      if ( qtri >= qtri_orig && qtet > ref_adapt_smooth_min_quality )
	{
	  printf("better qtri %f qtet %f\n", qtri, qtet );
	  return REF_SUCCESS;
	}
      backoff *= 0.5;
    }

  RSS( ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv_orig ), "set t");
  RSS( ref_geom_constrain(ref_grid, node ), "constrain");
  RSS( ref_smooth_tet_quality_around( ref_grid, node, &qtet ), "q tet");
  RSS( ref_smooth_tri_quality_around( ref_grid, node, &qtri ), "q tri");

  printf("undo qtri %f qtet %f\n", qtri, qtet );

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_threed_pass( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_BOOL allowed, geom_node, geom_edge, geom_face, interior, no_quads;

  /* sort worst to best? */
  /* requeue neighbors with large changes? particulary quality drops */
  
  each_ref_node_valid_node( ref_node, node )
    {
      /* don't move geom nodes */
      RSS( ref_geom_is_a(ref_grid_geom(ref_grid), node, REF_GEOM_NODE,
			 &geom_node), "node check");
      if ( geom_node ) continue;

      /* next to ghost node, can't move */
      RSS( ref_smooth_local_tet_about( ref_grid, node, &allowed ), "para" );
      if ( !allowed )
	{
	  ref_node_age(ref_node,node)++;
	  continue;
	}

      RSS( ref_geom_is_a(ref_grid_geom(ref_grid), node, REF_GEOM_EDGE,
			 &geom_edge), "edge check");
      if ( geom_edge )
	{
	  RSS( ref_smooth_geom_edge( ref_grid, node ), "ideal node for edge" );
	  ref_node_age(ref_node,node) = 0;
	  continue;
	}

      no_quads = ref_cell_node_empty( ref_grid_qua( ref_grid ), node );
      RSS( ref_geom_is_a(ref_grid_geom(ref_grid), node, REF_GEOM_FACE,
			 &geom_face), "face check");
      if ( geom_face && no_quads )
	{
	  RSS( ref_smooth_geom_face( ref_grid, node ), "ideal node for face" );
	  ref_node_age(ref_node,node) = 0;
	  continue;
	}

      interior =
	ref_cell_node_empty( ref_grid_tri( ref_grid ), node ) &&
	ref_cell_node_empty( ref_grid_qua( ref_grid ), node );
      if ( interior )
	{
	  RSS( ref_smooth_tet_improve( ref_grid, node ), "ideal tet node" );
	  ref_node_age(ref_node,node) = 0;
	}
    }

  return REF_SUCCESS;
}

