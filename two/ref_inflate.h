
#ifndef REF_INFLATE_H
#define REF_INFLATE_H

#include "ref_defs.h"

#include "ref_grid.h"
#include "ref_node.h"
#include "ref_dict.h"

BEGIN_C_DECLORATION

REF_STATUS ref_inflate_pri_min_dot( REF_NODE ref_node, 
				    REF_INT *nodes,  
				    REF_DBL *min_dot );

REF_STATUS ref_inflate_face( REF_GRID ref_grid, 
			     REF_DICT faceids, 
			     REF_DBL thickness, REF_DBL xshift );

REF_STATUS ref_inflate_fix( REF_GRID ref_grid, 
			    REF_DICT faceids, 
			    REF_INT *o2n );

END_C_DECLORATION

#endif /* REF_INFLATE_H */