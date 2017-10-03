
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

#ifndef REF_SUBDIV_H
#define REF_SUBDIV_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_SUBDIV_STRUCT REF_SUBDIV_STRUCT;
typedef REF_SUBDIV_STRUCT * REF_SUBDIV;
END_C_DECLORATION

#include "ref_grid.h"
#include "ref_edge.h"

BEGIN_C_DECLORATION

struct REF_SUBDIV_STRUCT {
  REF_GRID grid;
  REF_EDGE edge;
  REF_INT *mark;
  REF_INT *node;
};

#define ref_subdiv_grid( ref_subdiv ) ((ref_subdiv)->grid)
#define ref_subdiv_edge( ref_subdiv ) ((ref_subdiv)->edge)

REF_STATUS ref_subdiv_create( REF_SUBDIV *ref_subdiv, REF_GRID ref_grid );
REF_STATUS ref_subdiv_free( REF_SUBDIV ref_subdiv );

#define ref_subdiv_mark( ref_subdiv, edge )	\
  ((ref_subdiv)->mark[edge])

#define ref_subdiv_node( ref_subdiv, edge )	\
  ((ref_subdiv)->node[edge])

REF_STATUS ref_subdiv_inspect( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_mark_n( REF_SUBDIV ref_subdiv, REF_INT *n );

REF_STATUS ref_subdiv_mark_to_split( REF_SUBDIV ref_subdiv, 
				     REF_INT node0, REF_INT node1 );

REF_STATUS ref_subdiv_mark_all( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_mark_prism_by_metric( REF_SUBDIV ref_subdiv );
REF_STATUS ref_subdiv_mark_prism_by_ratio( REF_SUBDIV ref_subdiv, 
					   REF_DBL *node_ratio );
REF_STATUS ref_subdiv_mark_prism_sides( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_mark_relax( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_split( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_mark_verify( REF_SUBDIV ref_subdiv );

REF_STATUS ref_subdiv_undo_impossible_marks( REF_SUBDIV ref_subdiv );

END_C_DECLORATION

#endif /* REF_SUBDIV_H */