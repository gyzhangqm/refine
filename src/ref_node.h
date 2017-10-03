
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

#ifndef REF_NODE_H
#define REF_NODE_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_NODE_STRUCT REF_NODE_STRUCT;
typedef REF_NODE_STRUCT * REF_NODE;
END_C_DECLORATION

#include "ref_list.h"

BEGIN_C_DECLORATION

struct REF_NODE_STRUCT {
  REF_INT n, max;
  REF_INT blank;
  REF_INT *global;
  REF_INT *sorted_global;
  REF_INT *sorted_local;
  REF_INT *part;
  REF_INT *age;
  REF_INT *guess;
  REF_DBL *real;
  REF_INT naux;
  REF_DBL *aux;
  REF_LIST unused_global_list;
  REF_INT old_n_global, new_n_global;
  REF_DBL twod_mid_plane;
  REF_INT tet_quality;
  REF_INT tri_quality;
};

#define REF_NODE_REAL_PER (9) /* x,y,z, m[6] */

#define REF_NODE_EPIC_QUALITY (1)
#define REF_NODE_JAC_QUALITY (2)

#define ref_node_n(ref_node) ((ref_node)->n)
#define ref_node_max(ref_node) ((ref_node)->max)

#define ref_node_n_global(ref_node) ((ref_node)->old_n_global)

#define ref_node_valid(ref_node,node) \
  ( (node) > -1 && (node) < ref_node_max(ref_node) && \
    (ref_node)->global[(node)] >= 0 )

#define ref_node_global(ref_node,node)			\
  ( ( (node) > -1 && (node) < ref_node_max(ref_node) && \
      (ref_node)->global[(node)] >= 0) ?		\
    (ref_node)->global[(node)]:REF_EMPTY )

#define each_ref_node_valid_node( ref_node, node )			\
  for ( (node) = 0 ;							\
	(node) < ref_node_max(ref_node);				\
	(node)++ )							\
    if ( ref_node_valid( ref_node, node ) )

#define ref_node_xyz(ref_node,ixyz,node) \
  ((ref_node)->real[(ixyz)+REF_NODE_REAL_PER*(node)])
#define ref_node_xyz_ptr(ref_node,node)			\
  (&((ref_node)->real[REF_NODE_REAL_PER*(node)]))

#define ref_node_metric(ref_node,im,node)		\
  ((ref_node)->real[(im+3)+REF_NODE_REAL_PER*(node)])
#define ref_node_metric_ptr(ref_node,node)		\
  (&((ref_node)->real[3+REF_NODE_REAL_PER*(node)]))

#define ref_node_real(ref_node,ireal,node)		\
  ((ref_node)->real[(ireal)+REF_NODE_REAL_PER*(node)])

#define ref_node_part(ref_node,node) ((ref_node)->part[(node)])
#define ref_node_age(ref_node,node) ((ref_node)->age[(node)])

#define ref_node_raw_guess(ref_node,node) ((ref_node)->guess[(node)])
#define ref_node_guess(ref_node,node)					\
  ( NULL==(( ref_node )->guess) ?					\
    REF_EMPTY :	ref_node_raw_guess(ref_node,node) )
#define ref_node_guess_allocated(ref_node) \
  ( NULL != (( ref_node )->guess) )

#define ref_node_naux(ref_node) ((ref_node)->naux)
#define ref_node_aux(ref_node,iaux,node)		\
  ((ref_node)->aux[(iaux)+ref_node_naux(ref_node)*(node)])

#define ref_node_twod_mid_plane(ref_node) ((ref_node)->twod_mid_plane)

REF_STATUS ref_node_create( REF_NODE *ref_node );
REF_STATUS ref_node_free( REF_NODE ref_node );

REF_STATUS ref_node_allocate_guess( REF_NODE ref_node );

REF_STATUS ref_node_deep_copy( REF_NODE *ref_node_ptr, REF_NODE original );

REF_STATUS ref_node_inspect( REF_NODE ref_node );
REF_STATUS ref_node_location( REF_NODE ref_node, REF_INT node );
REF_STATUS ref_node_tattle_global( REF_NODE ref_node, REF_INT global );

REF_STATUS ref_node_local( REF_NODE ref_node, REF_INT global, REF_INT *node );

REF_STATUS ref_node_initialize_n_global_from_locals(  REF_NODE ref_node );
REF_STATUS ref_node_initialize_n_global(  REF_NODE ref_node, REF_INT n_global );
REF_STATUS ref_node_next_global( REF_NODE ref_node, REF_INT *global );

REF_STATUS ref_node_synchronize_globals( REF_NODE ref_node );
REF_STATUS ref_node_shift_new_globals( REF_NODE ref_node );
REF_STATUS ref_node_eliminate_unused_globals( REF_NODE ref_node );

REF_STATUS ref_node_add( REF_NODE ref_node, REF_INT global, REF_INT *node );
REF_STATUS ref_node_add_many( REF_NODE ref_node, REF_INT n, REF_INT *global );

REF_STATUS ref_node_remove( REF_NODE ref_node, REF_INT node );
REF_STATUS ref_node_remove_without_global( REF_NODE ref_node, REF_INT node );
REF_STATUS ref_node_rebuild_sorted_global( REF_NODE ref_node );

REF_STATUS ref_node_compact( REF_NODE ref_node, REF_INT **o2n, REF_INT **n2o );
REF_STATUS ref_node_in_bounding_box( REF_NODE ref_node, REF_DBL *bounding_box,
				     REF_INT *n, REF_INT **o2n, REF_INT **n2o );

REF_STATUS ref_node_ghost_real( REF_NODE ref_node );
REF_STATUS ref_node_ghost_int( REF_NODE ref_node, REF_INT *scalar );

REF_STATUS ref_node_edge_twod( REF_NODE ref_node, 
			       REF_INT node0, REF_INT node1, 
			       REF_BOOL *twod );

REF_STATUS ref_node_node_twod( REF_NODE ref_node, 
			       REF_INT node, 
			       REF_BOOL *twod );

REF_STATUS ref_node_ratio( REF_NODE ref_node, REF_INT node0, REF_INT node1, 
			   REF_DBL *ratio );
REF_STATUS ref_node_dratio_dnode0( REF_NODE ref_node, 
				   REF_INT node0, REF_INT node1, 
				   REF_DBL *ratio, REF_DBL *dratio_dnode0 );

REF_STATUS ref_node_tri_normal( REF_NODE ref_node, 
				REF_INT *nodes,  
				REF_DBL *normal );

REF_STATUS ref_node_tri_y_projection( REF_NODE ref_node, 
				      REF_INT *nodes,  
				      REF_DBL *y_projection );
REF_STATUS ref_node_tri_twod_orientation( REF_NODE ref_node, 
					  REF_INT *nodes,  
					  REF_BOOL *valid );
REF_STATUS ref_node_tri_node_angle( REF_NODE ref_node, 
				    REF_INT *nodes, REF_INT node,  
				    REF_DBL *angle );
REF_STATUS ref_node_tri_area( REF_NODE ref_node, 
			      REF_INT *nodes,  
			      REF_DBL *area );
REF_STATUS ref_node_tri_darea_dnode0( REF_NODE ref_node, 
				      REF_INT *nodes,  
				      REF_DBL *area, REF_DBL *darea_dnode0);

REF_STATUS ref_node_tri_quality( REF_NODE ref_node, 
				 REF_INT *nodes,  
				 REF_DBL *quality );
REF_STATUS ref_node_tri_dquality_dnode0( REF_NODE ref_node, 
					 REF_INT *nodes,  
					 REF_DBL *quality, 
					 REF_DBL *dquality_dnode0 );

REF_STATUS ref_node_tet_vol( REF_NODE ref_node, 
			     REF_INT *nodes,  
			     REF_DBL *volume );
REF_STATUS ref_node_tet_dvol_dnode0( REF_NODE ref_node, 
				     REF_INT *nodes,  
				     REF_DBL *vol, REF_DBL *dvol_dnode0 );

REF_STATUS ref_node_tet_quality( REF_NODE ref_node, 
				 REF_INT *nodes,  
				 REF_DBL *quality );
REF_STATUS ref_node_tet_dquality_dnode0( REF_NODE ref_node, 
					 REF_INT *nodes,  
					 REF_DBL *quality,
					 REF_DBL *dquality_dnode0);

REF_STATUS ref_node_twod_clone( REF_NODE ref_node, 
				REF_INT original, REF_INT *clone );
REF_STATUS ref_node_interpolate_edge( REF_NODE ref_node, 
				      REF_INT node0, REF_INT node1, 
				      REF_INT new_node );
REF_STATUS ref_node_interpolate_face( REF_NODE ref_node, 
				      REF_INT node0, REF_INT node1,
				      REF_INT node2, REF_INT new_node );
REF_STATUS ref_node_resize_aux( REF_NODE ref_node );

REF_STATUS ref_node_bary3( REF_NODE ref_node, REF_INT *nodes,
			   REF_DBL *xyz, REF_DBL *bary );
REF_STATUS ref_node_bary4( REF_NODE ref_node, REF_INT *nodes,
			   REF_DBL *xyz, REF_DBL *bary );

END_C_DECLORATION

#endif /* REF_NODE_H */