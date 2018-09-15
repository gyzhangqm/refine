
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_recon.h"

#include "ref_cell.h"
#include "ref_edge.h"
#include "ref_grid.h"
#include "ref_node.h"

#include "ref_interp.h"

#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"

#include "ref_dict.h"
#include "ref_twod.h"

#define REF_RECON_MAX_DEGREE (1000)

/* Alauzet and A. Loseille doi:10.1016/j.jcp.2009.09.020
 * section 2.2.4.1. A double L2-projection */
static REF_STATUS ref_recon_l2_projection_grad(REF_GRID ref_grid,
                                               REF_DBL *scalar, REF_DBL *grad) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT i, node, cell, group, cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL div_by_zero;
  REF_DBL cell_vol, cell_grad[3];
  REF_DBL *vol;

  ref_malloc_init(vol, ref_node_max(ref_node), REF_DBL, 0.0);

  each_ref_node_valid_node(ref_node, node) for (i = 0; i < 3; i++)
      grad[i + 3 * node] = 0.0;

  each_ref_grid_ref_cell(ref_grid, group, ref_cell)
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    switch (ref_cell_node_per(ref_cell)) {
      case 4:
        RSS(ref_node_tet_vol(ref_node, nodes, &cell_vol), "vol");
        RSS(ref_node_tet_grad(ref_node, nodes, scalar, cell_grad), "grad");
        for (cell_node = 0; cell_node < 4; cell_node++)
          for (i = 0; i < 3; i++)
            grad[i + 3 * nodes[cell_node]] += cell_vol * cell_grad[i];
        for (cell_node = 0; cell_node < 4; cell_node++)
          vol[nodes[cell_node]] += cell_vol;
        break;
      default:
        RSS(REF_IMPLEMENT, "implement cell type");
        break;
    }
  }

  div_by_zero = REF_FALSE;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_math_divisible(grad[0 + 3 * node], vol[node]) &&
        ref_math_divisible(grad[1 + 3 * node], vol[node]) &&
        ref_math_divisible(grad[2 + 3 * node], vol[node])) {
      for (i = 0; i < 3; i++) grad[i + 3 * node] /= vol[node];
    } else {
      div_by_zero = REF_TRUE;
      for (i = 0; i < 3; i++) grad[i + 3 * node] = 0.0;
    }
  }
  RSS(ref_mpi_all_or(ref_grid_mpi(ref_grid), &div_by_zero), "mpi all or");
  RSS(ref_node_ghost_dbl(ref_node, grad, 3), "update ghosts");

  ref_free(vol);

  return (div_by_zero ? REF_DIV_ZERO : REF_SUCCESS);
}

static REF_STATUS ref_recon_l2_projection_hessian(REF_GRID ref_grid,
                                                  REF_DBL *scalar,
                                                  REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL *grad, *dsdx, *gradx, *grady, *gradz;
  REF_DBL diag_system[12];

  ref_malloc_init(grad, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(dsdx, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradx, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grady, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradz, 3 * ref_node_max(ref_node), REF_DBL, 0.0);

  RSS(ref_recon_l2_projection_grad(ref_grid, scalar, grad), "l2 grad");

  i = 0;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, gradx), "gradx");

  i = 1;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, grady), "grady");

  i = 2;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, gradz), "gradz");

  /* average off-diagonals */
  each_ref_node_valid_node(ref_node, node) {
    hessian[0 + 6 * node] = gradx[0 + 3 * node];
    hessian[1 + 6 * node] = 0.5 * (gradx[1 + 3 * node] + grady[0 + 3 * node]);
    hessian[2 + 6 * node] = 0.5 * (gradx[2 + 3 * node] + gradz[0 + 3 * node]);
    hessian[3 + 6 * node] = grady[1 + 3 * node];
    hessian[4 + 6 * node] = 0.5 * (grady[2 + 3 * node] + gradz[1 + 3 * node]);
    hessian[5 + 6 * node] = gradz[2 + 3 * node];
  }

  /* positive eignevalues to make symrecon positive definite */
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_matrix_diag_m(&(hessian[6 * node]), diag_system), "eigen decomp");
    ref_matrix_eig(diag_system, 0) = ABS(ref_matrix_eig(diag_system, 0));
    ref_matrix_eig(diag_system, 1) = ABS(ref_matrix_eig(diag_system, 1));
    ref_matrix_eig(diag_system, 2) = ABS(ref_matrix_eig(diag_system, 2));
    RSS(ref_matrix_form_m(diag_system, &(hessian[6 * node])), "re-form hess");
  }

  ref_free(gradz);
  ref_free(grady);
  ref_free(gradx);
  ref_free(dsdx);
  ref_free(grad);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_kexact_hessian_at_cloud(REF_GRID ref_grid,
                                                    REF_DBL *scalar,
                                                    REF_INT center_node,
                                                    REF_DICT ref_dict,
                                                    REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL geom[9], ab[90];
  REF_DBL dx, dy, dz, dq;
  REF_DBL *a, *q, *r;
  REF_INT m, n, cloud_node;
  REF_INT i2, im, i, j;
  /* solve A with QR factorization size m x n */
  m = ref_dict_n(ref_dict) - 1;     /* skip self */
  if (ref_grid_twod(ref_grid)) m++; /* add mid node */
  n = 9;
  ref_malloc(a, m * n, REF_DBL);
  ref_malloc(q, m * n, REF_DBL);
  ref_malloc(r, n * n, REF_DBL);
  i = 0;
  if (ref_grid_twod(ref_grid)) {
    dx = 0;
    dy = ref_node_twod_mid_plane(ref_node);
    dz = 0;
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
    }
    i++;
  }
  each_ref_dict_key(ref_dict, i2, cloud_node) {
    if (center_node == cloud_node) continue; /* skip self */
    dx = ref_node_xyz(ref_node, 0, cloud_node) -
         ref_node_xyz(ref_node, 0, center_node);
    dy = ref_node_xyz(ref_node, 1, cloud_node) -
         ref_node_xyz(ref_node, 1, center_node);
    dz = ref_node_xyz(ref_node, 2, cloud_node) -
         ref_node_xyz(ref_node, 2, center_node);
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
    }
    i++;
  }
  REIS(m, i, "A row miscount");
  RSS(ref_matrix_qr(m, n, a, q, r), "kexact lsq hess qr");
  for (i = 0; i < 90; i++) ab[i] = 0.0;
  for (i = 0; i < 9; i++) {
    for (j = 0; j < 9; j++) {
      ab[i + 9 * j] += r[i + 9 * j];
    }
  }
  i = 0;
  if (ref_grid_twod(ref_grid)) {
    dq = 0;
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
  }
  each_ref_dict_key(ref_dict, i2, cloud_node) {
    if (center_node == cloud_node) continue; /* skip self */
    dq = scalar[cloud_node] - scalar[center_node];
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
  }
  REIS(m, i, "b row miscount");
  RSS(ref_matrix_solve_ab(9, 10, ab), "solve rx=qtb");
  j = 9;
  for (im = 0; im < 6; im++) {
    hessian[im] = ab[im + 9 * j];
  }
  ref_free(r);
  ref_free(q);
  ref_free(a);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_grow_dict_one_layer(REF_DICT ref_dict,
                                                REF_CELL ref_cell) {
  REF_DICT copy;
  REF_INT node_list[REF_RECON_MAX_DEGREE];
  REF_INT max_node = REF_RECON_MAX_DEGREE;
  REF_INT nnode, key_index, cloud_node, i;
  RSS(ref_dict_deep_copy(&copy, ref_dict), "copy");
  each_ref_dict_key(copy, key_index, cloud_node) {
    RXS(ref_cell_node_list_around(ref_cell, cloud_node, max_node, &nnode,
                                  node_list),
        REF_INCREASE_LIMIT, "first halo of nodes");
    for (i = 0; i < nnode; i++) {
      RSS(ref_dict_store(ref_dict, node_list[i], REF_EMPTY), "store node");
    }
  }
  ref_dict_free(copy);
  return REF_SUCCESS;
}

static REF_STATUS ref_recon_kexact_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                                           REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT node, im;
  REF_DICT ref_dict;
  REF_DBL node_hessian[6];
  REF_STATUS status;
  if (ref_grid_twod(ref_grid)) ref_cell = ref_grid_pri(ref_grid);
  each_ref_node_valid_node(ref_node, node) {
    /* use ref_dict to get a unique list of halo(2) nodes */
    RSS(ref_dict_create(&ref_dict), "create ref_dict");
    RSS(ref_dict_store(ref_dict, node, REF_EMPTY), "store node0");
    RSS(ref_recon_grow_dict_one_layer(ref_dict, ref_cell), "grow");
    RSS(ref_recon_grow_dict_one_layer(ref_dict, ref_cell), "grow");
    status = ref_recon_kexact_hessian_at_cloud(ref_grid, scalar, node, ref_dict,
                                               node_hessian);
    if (REF_DIV_ZERO == status) {
      printf(" caught %s, adding third layer to kexact cloud and retry\n",
             "REF_DIV_ZERO");
      RSS(ref_recon_grow_dict_one_layer(ref_dict, ref_cell), "grow");
      status = ref_recon_kexact_hessian_at_cloud(ref_grid, scalar, node,
                                                 ref_dict, node_hessian);
    }
    RSS(status, "kexact qr node");
    for (im = 0; im < 6; im++) {
      hessian[im + 6 * node] = node_hessian[im];
    }
    if (ref_grid_twod(ref_grid)) {
      node_hessian[1] = 0.0;
      node_hessian[3] = 0.0;
      node_hessian[4] = 0.0;
    }
    RSS(ref_dict_free(ref_dict), "free ref_dict");
  }

  /* positive eignevalues to make symrecon positive definite */
  each_ref_node_valid_node(ref_node, node) {
    REF_DBL diag_system[12];
    RSS(ref_matrix_diag_m(&(hessian[6 * node]), diag_system), "eigen decomp");
    ref_matrix_eig(diag_system, 0) = ABS(ref_matrix_eig(diag_system, 0));
    ref_matrix_eig(diag_system, 1) = ABS(ref_matrix_eig(diag_system, 1));
    ref_matrix_eig(diag_system, 2) = ABS(ref_matrix_eig(diag_system, 2));
    RSS(ref_matrix_form_m(diag_system, &(hessian[6 * node])), "re-form hess");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_recon_extrapolate_boundary_multipass(REF_DBL *recon,
                                                    REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tris = ref_grid_tri(ref_grid);
  REF_CELL tets = ref_grid_tet(ref_grid);
  REF_INT node;
  REF_INT max_node = REF_RECON_MAX_DEGREE, nnode;
  REF_INT node_list[REF_RECON_MAX_DEGREE];
  REF_INT i, neighbor, nint;
  REF_BOOL *needs_donor;
  REF_INT pass, remain;

  if (ref_grid_twod(ref_grid)) RSS(REF_IMPLEMENT, "2D not implmented");

  ref_malloc_init(needs_donor, ref_node_max(ref_node), REF_BOOL, REF_FALSE);

  /* each boundary node */
  each_ref_node_valid_node(ref_node,
                           node) if (!ref_cell_node_empty(tris, node)) {
    needs_donor[node] = REF_TRUE;
  }

  RSS(ref_node_ghost_int(ref_node, needs_donor), "update ghosts");

  for (pass = 0; pass < 10; pass++) {
    each_ref_node_valid_node(
        ref_node,
        node) if (ref_node_owned(ref_node, node) && needs_donor[node]) {
      RXS(ref_cell_node_list_around(tets, node, max_node, &nnode, node_list),
          REF_INCREASE_LIMIT, "unable to build neighbor list ");
      nint = 0;
      for (neighbor = 0; neighbor < nnode; neighbor++)
        if (!needs_donor[node_list[neighbor]]) nint++;
      if (0 < nint) {
        for (i = 0; i < 6; i++) recon[i + 6 * node] = 0.0;
        for (neighbor = 0; neighbor < nnode; neighbor++)
          if (!needs_donor[node_list[neighbor]]) {
            for (i = 0; i < 6; i++)
              recon[i + 6 * node] += recon[i + 6 * node_list[neighbor]];
          }
        /* use Euclidean average, these are derivatives */
        for (i = 0; i < 6; i++) recon[i + 6 * node] /= (REF_DBL)nint;
        needs_donor[node] = REF_FALSE;
      }
    }

    RSS(ref_node_ghost_int(ref_node, needs_donor), "update ghosts");
    RSS(ref_node_ghost_dbl(ref_node, recon, 6), "update ghosts");

    remain = 0;
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_owned(ref_node, node) && needs_donor[node]) {
        remain++;
      }
    }
    RSS(ref_mpi_allsum(ref_grid_mpi(ref_grid), &remain, 1, REF_INT_TYPE),
        "sum updates");

    if (0 == remain) break;
  }

  ref_free(needs_donor);

  REIS(0, remain, "untouched boundary nodes remain");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_roundoff_limit(REF_DBL *recon, REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL radius, dist;
  REF_DBL round_off_jitter = 1.0e-12;
  REF_DBL eig_floor;
  REF_INT nnode, node_list[REF_RECON_MAX_DEGREE],
      max_node = REF_RECON_MAX_DEGREE;
  REF_DBL diag_system[12];

  if (ref_grid_twod(ref_grid)) ref_cell = ref_grid_tri(ref_grid);

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_cell_node_list_around(ref_cell, node, max_node, &nnode, node_list),
        "first halo of nodes");
    radius = 0.0;
    for (i = 0; i < nnode; i++) {
      dist = sqrt(pow(ref_node_xyz(ref_node, 0, node_list[i]) -
                          ref_node_xyz(ref_node, 0, node),
                      2) +
                  pow(ref_node_xyz(ref_node, 1, node_list[i]) -
                          ref_node_xyz(ref_node, 1, node),
                      2) +
                  pow(ref_node_xyz(ref_node, 2, node_list[i]) -
                          ref_node_xyz(ref_node, 2, node),
                      2));
      if (i == 0) radius = dist;
      radius = MIN(radius, dist);
    }
    /* 2nd order central finite difference */
    eig_floor = 4 * round_off_jitter / radius / radius;

    RSS(ref_matrix_diag_m(&(recon[6 * node]), diag_system), "eigen decomp");
    ref_matrix_eig(diag_system, 0) =
        MAX(ref_matrix_eig(diag_system, 0), eig_floor);
    ref_matrix_eig(diag_system, 1) =
        MAX(ref_matrix_eig(diag_system, 1), eig_floor);
    ref_matrix_eig(diag_system, 2) =
        MAX(ref_matrix_eig(diag_system, 2), eig_floor);
    RSS(ref_matrix_form_m(diag_system, &(recon[6 * node])), "re-form hess");
  }

  RSS(ref_node_ghost_dbl(ref_node, recon, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_gradient(REF_GRID ref_grid, REF_DBL *scalar, REF_DBL *grad,
                              REF_RECON_RECONSTRUCTION recon) {
  switch (recon) {
    case REF_RECON_L2PROJECTION:
      RSS(ref_recon_l2_projection_grad(ref_grid, scalar, grad), "l2");
      break;
    default:
      THROW("reconstruction not available");
  }

  return REF_SUCCESS;
}
REF_STATUS ref_recon_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                             REF_DBL *hessian, REF_RECON_RECONSTRUCTION recon) {
  switch (recon) {
    case REF_RECON_L2PROJECTION:
      RSS(ref_recon_l2_projection_hessian(ref_grid, scalar, hessian), "l2");
      RSS(ref_recon_extrapolate_boundary_multipass(hessian, ref_grid),
          "bound extrap");
      break;
    case REF_RECON_KEXACT:
      RSS(ref_recon_kexact_hessian(ref_grid, scalar, hessian), "k-exact");
      break;
    default:
      THROW("reconstruction not available");
  }

  return REF_SUCCESS;
}