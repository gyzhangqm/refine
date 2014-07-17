
#ifndef REF_SMOOTH_H
#define REF_SMOOTH_H

#include "ref_defs.h"

#include "ref_grid.h"

BEGIN_C_DECLORATION

REF_STATUS ref_smooth_tri_steepest_descent( REF_GRID ref_grid, REF_INT node );

REF_STATUS ref_smooth_tri_quality_around( REF_GRID ref_grid,
                                          REF_INT node,
                                          REF_DBL *min_quality );

REF_STATUS ref_smooth_tri_ideal( REF_GRID ref_grid,
                                 REF_INT node,
                                 REF_INT tri,
                                 REF_DBL *ideal_location );

END_C_DECLORATION

#endif /* REF_SMOOTH_H */
