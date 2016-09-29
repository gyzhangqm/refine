
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_EGADS
#include "egads.h"
#endif

#include "ref_geom.h"

#include "ref_export.h"
#include "ref_grid.h"
#include "ref_node.h"
#include "ref_cell.h"

#include "ref_dict.h"

#include "ref_malloc.h"

REF_STATUS ref_geom_create( REF_GEOM *ref_geom_ptr )
{
  REF_GEOM ref_geom;
  REF_INT geom;
  ( *ref_geom_ptr ) = NULL;

  ref_malloc( *ref_geom_ptr, 1, REF_GEOM_STRUCT );

  ref_geom = ( *ref_geom_ptr );

  ref_geom_n(ref_geom) = 0;
  ref_geom_max(ref_geom) = 10;

  ref_malloc( ref_geom->descr, 3*ref_geom_max(ref_geom), REF_INT);
  ref_malloc( ref_geom->param, 2*ref_geom_max(ref_geom), REF_DBL);

  for ( geom = 0; geom < ref_geom_max(ref_geom); geom++ )
    {
      ref_geom_type(ref_geom,geom) = REF_EMPTY;
      ref_geom_id(ref_geom,geom) = geom+1;
    }
  ref_geom_id(ref_geom,ref_geom_max(ref_geom)-1) = REF_EMPTY;
  ref_geom_blank(ref_geom) = 0;

  RSS( ref_adj_create( &( ref_geom->ref_adj ) ), "create ref_adj for ref_geom" );
  
  return REF_SUCCESS;
}

REF_STATUS ref_geom_free( REF_GEOM ref_geom )
{
  if ( NULL == (void *)ref_geom )
    return REF_NULL;
  RSS( ref_adj_free( ref_geom->ref_adj ), "adj free" );
  ref_free( ref_geom->param );
  ref_free( ref_geom->descr );
  ref_free( ref_geom );
  return REF_SUCCESS;
}

REF_STATUS ref_geom_deep_copy( REF_GEOM *ref_geom_ptr, REF_GEOM original )
{
  REF_GEOM ref_geom;
  REF_INT geom, i;
  ( *ref_geom_ptr ) = NULL;

  ref_malloc( *ref_geom_ptr, 1, REF_GEOM_STRUCT );

  ref_geom = ( *ref_geom_ptr );

  ref_geom_n(ref_geom) = ref_geom_n(original);
  ref_geom_max(ref_geom) = ref_geom_max(original);

  ref_malloc( ref_geom->descr, 3*ref_geom_max(ref_geom), REF_INT);
  ref_malloc( ref_geom->param, 2*ref_geom_max(ref_geom), REF_DBL);

  for ( geom = 0; geom < ref_geom_max(ref_geom); geom++ )
    for ( i = 0; i < 3; i++ )
      ref_geom_descr(ref_geom,i,geom) = ref_geom_descr(original,i,geom);
  ref_geom_blank(ref_geom) = ref_geom_blank(original);
  for ( geom = 0; geom < ref_geom_max(ref_geom); geom++ )
    for ( i = 0; i < 2; i++ )
      ref_geom_param(ref_geom,i,geom) = ref_geom_param(original,i,geom);

  RSS( ref_adj_deep_copy( &( ref_geom->ref_adj ), original->ref_adj ),
       "deep copy ref_adj for ref_geom" );
  
  return REF_SUCCESS;
}

REF_STATUS ref_geom_inspect( REF_GEOM ref_geom )
{
  REF_INT geom;
  printf("ref_geom = %p\n",(void *)ref_geom);
  printf(" n = %d, max = %d\n",ref_geom_n(ref_geom),ref_geom_max(ref_geom));
  for ( geom = 0 ; geom < ref_geom_max( ref_geom ) ; geom++ )
    {
      switch(ref_geom_type(ref_geom,geom))
	{
	case REF_GEOM_NODE:
	  printf("%d node: %d global, %d id\n",
		 geom,
		 ref_geom_id(ref_geom,geom),
		 ref_geom_node(ref_geom,geom));
	  break;
	case REF_GEOM_EDGE:
	  printf("%d edge: %d id, %d global, t=%e\n",
		 geom,
		 ref_geom_id(ref_geom,geom),
		 ref_geom_node(ref_geom,geom),
		 ref_geom_param(ref_geom,0,geom));
	  break;
	case REF_GEOM_FACE:
	  printf("%d face: %d id, %d global, uv= %e %e\n",
		 geom,
		 ref_geom_id(ref_geom,geom),
		 ref_geom_node(ref_geom,geom),
		 ref_geom_param(ref_geom,0,geom),
		 ref_geom_param(ref_geom,1,geom) );
	  break;
	}
    }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_add( REF_GEOM ref_geom, REF_INT node,
			 REF_INT type, REF_INT id,
			 REF_DBL *param )
{
  REF_INT geom;
  REF_INT orig, chunk;
  REF_INT max_limit = REF_INT_MAX/3;
  REF_STATUS status;

  if ( type < 0 || 2 < type )
    return REF_INVALID;

  status = ref_geom_find( ref_geom, node, type, id, &geom );
  RXS( status, REF_NOT_FOUND, "find failed");

  if ( REF_SUCCESS == status )
    {
      if ( type > 0 ) ref_geom_param(ref_geom,0,geom) = param[0];
      if ( type > 1 ) ref_geom_param(ref_geom,1,geom) = param[1];
      return REF_SUCCESS;
    }
    
  if ( REF_EMPTY == ref_geom_blank(ref_geom) )
    {
      RAS( ref_geom_max(ref_geom) != max_limit,
           "the number of geoms is too large for integers, cannot grow");
      orig = ref_geom_max(ref_geom);
      /* geometric growth for efficiency */
      chunk = MAX(1000,(REF_INT)( 1.5*(REF_DBL)orig ));

      /* try to keep under 32-bit limit */
      RAS( max_limit-orig > 0, "chunk limit at max");
      chunk = MIN( chunk, max_limit-orig );

      ref_geom_max(ref_geom) = orig + chunk;

      ref_realloc( ref_geom->descr, 3*ref_geom_max(ref_geom), REF_INT );
      ref_realloc( ref_geom->param, 2*ref_geom_max(ref_geom), REF_DBL );

      for (geom = orig; geom < ref_geom_max(ref_geom); geom++ )
        {
          ref_geom_type(ref_geom,geom) = REF_EMPTY;
          ref_geom_id(ref_geom,geom) = geom+1;
        }
      ref_geom_id(ref_geom,ref_geom_max(ref_geom)-1) = REF_EMPTY;
      ref_geom_blank(ref_geom) = orig;
    }

  geom = ref_geom_blank(ref_geom);
  ref_geom_blank(ref_geom) = ref_geom_id(ref_geom,geom);

  ref_geom_type(ref_geom,geom) = type;
  ref_geom_id(ref_geom,geom) = id;
  ref_geom_node(ref_geom,geom) = node;

  if ( type > 0 ) ref_geom_param(ref_geom,0,geom) = param[0];
  if ( type > 1 ) ref_geom_param(ref_geom,1,geom) = param[1];
  
  RSS( ref_adj_add(ref_geom->ref_adj, node, geom),"register geom" );

  ref_geom_n(ref_geom)++;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_remove( REF_GEOM ref_geom, REF_INT node,
			    REF_INT type, REF_INT id)
{
  REF_INT geom;
  REF_STATUS status;
  
  status = ref_geom_find( ref_geom, node, type, id, &geom );
  RXS( status, REF_NOT_FOUND, "find failed");

  if ( REF_SUCCESS == status )
    {
      RSS( ref_adj_remove(ref_geom_adj(ref_geom),
			  ref_geom_node(ref_geom,geom), geom),
	   "unregister geom" );
	  
      ref_geom_type(ref_geom,geom) = REF_EMPTY;
      ref_geom_id(ref_geom,geom) = ref_geom_blank(ref_geom);
      ref_geom_blank(ref_geom) = geom;
      ref_geom_n(ref_geom)--;
    }
  
  return status;
}

REF_STATUS ref_geom_find( REF_GEOM ref_geom, REF_INT node,
			  REF_INT type, REF_INT id,
			  REF_INT *found )
{
  REF_INT item, geom;
  *found = REF_EMPTY;
  each_ref_adj_node_item_with_ref( ref_geom_adj(ref_geom), node, item, geom)
    {
      if ( type == ref_geom_type(ref_geom,geom) &&
	   id   == ref_geom_id(ref_geom,geom) )
	{
	  *found = geom;
	  return REF_SUCCESS;
	}   
    }
  return REF_NOT_FOUND;
}

REF_STATUS ref_geom_tuv( REF_GEOM ref_geom, REF_INT node,
			 REF_INT type, REF_INT id,
			 REF_DBL *param )
{
  REF_INT geom;
  
  RSS( ref_geom_find( ref_geom, node, type, id, &geom ), "not found");

  if ( type > 0 ) param[0] = ref_geom_param(ref_geom,0,geom);
  if ( type > 1 ) param[1] = ref_geom_param(ref_geom,1,geom);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_egads_fixture( char *filename )
{
#ifdef HAVE_EGADS
  ego context;
  int stype;
  double data[6];
  ego body;
  ego model = NULL;
  
  REIS( EGADS_SUCCESS, EG_open(&context), "EG open");
  stype = 1; /* box */
  data[0]=0.0; /* corner */
  data[1]=0.0;
  data[2]=0.0;
  data[3]=1.0; /* length of sides */
  data[4]=1.0;
  data[5]=1.0;
  REIS( EGADS_SUCCESS,
	EG_makeSolidBody(context, stype, data, &body), "EG box");
  REIS( EGADS_SUCCESS,
	EG_makeTopology(context, NULL, MODEL, 0,
			NULL, 1, &body,
			NULL, &model), "topo");
  REIS( EGADS_SUCCESS,
	EG_saveModel(model, filename), "save");
  REIS( EGADS_SUCCESS, EG_close(context), "EG close");
  
#else
  printf("No EGADS linked for %s\n",filename);
#endif
  
  return REF_SUCCESS;
}

REF_STATUS ref_geom_tetgen_volume( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  char *smesh_name = "ref_geom_test.smesh";
  char *node_name = "ref_geom_test.1.node";
  char *ele_name = "ref_geom_test.1.ele";
  char command[1024];
  FILE *file;
  REF_INT nnode, ndim, attr, mark;
  REF_INT ntet, node_per;
  REF_INT node, nnode_surface, item, new_node;
  REF_DBL xyz[3], dist;
  REF_INT cell, new_cell, nodes[REF_CELL_MAX_SIZE_PER];
  RSS( ref_export_smesh( ref_grid, smesh_name ), "smesh" );
  sprintf( command, "tetgen -pYq1.0/0z %s", smesh_name );
  printf(" %s\n", command);
  REIS(0, system( command ), "epstopdf failed");

  file = fopen(node_name,"r");
  if (NULL == (void *)file) printf("unable to open %s\n",node_name);
  RNS(file, "unable to open file" );

  REIS( 1, fscanf( file, "%d", &nnode ), "node header nnode" );
  REIS( 1, fscanf( file, "%d", &ndim ), "node header ndim" );
  REIS( 3, ndim, "not 3D");
  REIS( 1, fscanf( file, "%d", &attr ), "node header attr" );
  REIS( 0, attr, "nodes have attribute 3D");
  REIS( 1, fscanf( file, "%d", &mark ), "node header mark" );
  REIS( 0, mark, "nodes have mark");

  /* verify surface nodes */
  nnode_surface = ref_node_n(ref_node);
  for( node=0; node<nnode_surface ; node++ ) 
    {
      REIS( 1, fscanf( file, "%d", &item ), "node item" );
      RES( node, item, "node index");
      RES( 1, fscanf( file, "%lf", &(xyz[0]) ), "x" );
      RES( 1, fscanf( file, "%lf", &(xyz[1]) ), "y" );
      RES( 1, fscanf( file, "%lf", &(xyz[2]) ), "z" );
      dist = sqrt( (xyz[0]-ref_node_xyz( ref_node, 0, node )) *
		   (xyz[0]-ref_node_xyz( ref_node, 0, node )) +
		   (xyz[1]-ref_node_xyz( ref_node, 1, node )) *
		   (xyz[1]-ref_node_xyz( ref_node, 1, node )) +
		   (xyz[2]-ref_node_xyz( ref_node, 2, node )) *
		   (xyz[2]-ref_node_xyz( ref_node, 2, node )) );
      if ( dist > 1.0e-12 )
	{
	  printf("node %d off by %e\n",node,dist);
	  THROW("tetgen moved node");
	}
    }

  /* interior nodes */
  for( node=nnode_surface; node<nnode ; node++ ) 
    {
      REIS( 1, fscanf( file, "%d", &item ), "node item" );
      RES( node, item, "node index");
      RSS( ref_node_add(ref_node, node, &new_node ), "new_node");
      RES( node, new_node, "node index");
      RES( 1, fscanf( file, "%lf", &(xyz[0]) ), "x" );
      RES( 1, fscanf( file, "%lf", &(xyz[1]) ), "y" );
      RES( 1, fscanf( file, "%lf", &(xyz[2]) ), "z" );
      ref_node_xyz( ref_node, 0, new_node ) = xyz[0];
      ref_node_xyz( ref_node, 1, new_node ) = xyz[1];
      ref_node_xyz( ref_node, 2, new_node ) = xyz[2];
    }
  
  fclose( file );
  
  /* check faces when paranoid, but -z should not mess with them */

  file = fopen(ele_name,"r");
  if (NULL == (void *)file) printf("unable to open %s\n",ele_name);
  RNS(file, "unable to open file" );

  REIS( 1, fscanf( file, "%d", &ntet ), "ele header ntet" );
  REIS( 1, fscanf( file, "%d", &node_per ), "ele header node_per" );
  REIS( 4, node_per, "expected tets");
  REIS( 1, fscanf( file, "%d", &mark ), "ele header mark" );
  REIS( 0, mark, "ele have mark");

  ref_cell = ref_grid_tet(ref_grid);
  for( cell = 0; cell < ntet ; cell++ )
    {
      REIS( 1, fscanf( file, "%d", &item ), "tet item" );
      RES( cell, item, "node index");
      for ( node = 0 ; node < 4 ; node++ )  
	RES( 1, fscanf( file, "%d", &(nodes[node]) ), "tet" );
      RSS( ref_cell_add(ref_cell, nodes, &new_cell ), "new tet");
      RES( cell, new_cell, "tet index");
    }
  
  fclose( file );
  
  return REF_SUCCESS;
}
  
REF_STATUS ref_geom_grid_from_egads( REF_GRID *ref_grid_ptr, char *filename )
{
  REF_GRID ref_grid;
  RSS( ref_geom_brep_from_egads( ref_grid_ptr, filename ), "brep" );
  ref_grid = (*ref_grid_ptr);
  RSS( ref_geom_tetgen_volume( ref_grid ), "tetgen volume" );
  return REF_SUCCESS;
}
  
REF_STATUS ref_geom_brep_from_egads( REF_GRID *ref_grid_ptr, char *filename )
{
#ifdef HAVE_EGADS
  REF_GRID ref_grid;
  REF_NODE ref_node;
  REF_GEOM ref_geom;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tri, new_cell;
  REF_DBL param[2];
  ego context;
  ego model = NULL;
  double params[3], box[6], size;
  ego geom, *bodies, *dum;
  int oclass, mtype, nbody, *senses, j;
  ego solid, tess, *faces, *edges;
  int tess_status, nvert;
  int face, nface, edge, nedge, plen, tlen;
  const double *points, *uv, *t;
  const int    *ptype, *pindex, *tris, *tric;
  int node, new_node, pty, pin;
  double verts[3];
  
  printf("EGAGS project %s\n",filename);
  RSS( ref_grid_create( ref_grid_ptr ), "create grid");
  ref_grid = (*ref_grid_ptr);
  ref_node = ref_grid_node(ref_grid);
  ref_geom = ref_grid_geom(ref_grid);
			  
  REIS( EGADS_SUCCESS, EG_open(&context), "EG open");
  REIS( EGADS_SUCCESS, EG_loadModel(context, 0, filename, &model), "EG load");
  REIS( EGADS_SUCCESS, EG_getBoundingBox(model, box), "EG bounding box");
  size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
	      (box[1]-box[4])*(box[1]-box[4]) +
	      (box[2]-box[5])*(box[2]-box[5]));
  printf(" box %f %f %f %f %f %f\n",box[0],box[1],box[2],box[3],box[4],box[5]);

  params[0] =  0.25*size; /*spacing*/
  params[1] =  0.001*size;
  params[2] = 15.0;

  REIS( EGADS_SUCCESS,
	EG_getTopology(model, &geom, &oclass, &mtype, NULL,
		       &nbody, &bodies, &senses), "EG topo bodies");
  printf(" %d bodies\n",nbody);
  REIS( 1, nbody, "expected 1 body" );
  solid = bodies[0];
  REIS( EGADS_SUCCESS,
	EG_getTopology(bodies[0], &geom, &oclass, &mtype,
		       NULL, &j, &dum, &senses), "EG topo body type");
  REIS( SOLIDBODY, mtype, "expected SOLIDBODY" );
  
  REIS( EGADS_SUCCESS,
	EG_makeTessBody(solid, params, &tess), "EG tess");

  REIS( EGADS_SUCCESS,
	EG_getBodyTopos(solid, NULL, FACE, &nedge, &edges), "EG edge typo");
  EG_free(edges);
  REIS( EGADS_SUCCESS,
	EG_getBodyTopos(solid, NULL, FACE, &nface, &faces), "EG face typo");
  EG_free(faces);

  REIS( EGADS_SUCCESS,
	EG_statusTessBody(tess, &geom, &tess_status, &nvert), "EG tess");
  REIS( 1, tess_status, "tess not closed" );
  printf(" %d global vertex\n",nvert);

  for (node = 0; node < nvert; node++) {
    REIS( EGADS_SUCCESS,
	  EG_getGlobal(tess, node+1, &pty, &pin, verts), "global node info");
    RSS( ref_node_add(ref_node, node, &new_node ), "new_node");
    RES( node, new_node, "node index");
    ref_node_xyz( ref_node, 0, node ) = verts[0];
    ref_node_xyz( ref_node, 1, node ) = verts[1];
    ref_node_xyz( ref_node, 2, node ) = verts[2];
    if ( 0 == pty )
      RSS( ref_geom_add( ref_geom, node, REF_GEOM_NODE, node, NULL), "node");
  }

  for (edge = 0; edge < nedge; edge++) {
    REIS( EGADS_SUCCESS,
	  EG_getTessEdge(tess, edge+1, &plen, &points, &t), "tess query edge" );
    printf(" edge %d has %d nodes\n",edge,plen);
    for ( node = 0; node<plen; node++ ) {
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, -(edge+1), node+1, &(nodes[0])), "l2g0");
      param[0] = t[node];
      RSS( ref_geom_add( ref_geom, nodes[0], REF_GEOM_EDGE, edge+1, param),
	   "edge t");
    }
    for ( node = 0; node<(plen-1); node++ ) {
      /* assue edge index is 1-bias */
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, -(edge+1), node+1, &(nodes[0])), "l2g0");
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, -(edge+1), node+2, &(nodes[1])), "l2g1");
      nodes[0] -= 1;
      nodes[1] -= 1;
      nodes[2] = edge + 1;
      RSS( ref_cell_add(ref_grid_edg(ref_grid), nodes, &new_cell ), "new edge");
    }
  }
  
  for (face = 0; face < nface; face++) {
    REIS( EGADS_SUCCESS,
	  EG_getTessFace(tess, face+1, &plen, &points, &uv, &ptype, &pindex,
			 &tlen, &tris, &tric), "tess query face" );
    printf(" face %d has %d triangles\n",face,tlen);
    for ( node = 0; node<plen; node++ ) {
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, face+1, node+1, &(nodes[0])), "l2g0");
      param[0] = uv[0+2*node];
      param[1] = uv[1+2*node];
      RSS( ref_geom_add( ref_geom, nodes[0], REF_GEOM_FACE, face+1, param),
	   "face uv");
    }
    for ( tri = 0; tri<tlen; tri++ ) {
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, face+1, tris[0+3*tri], &(nodes[0])), "l2g0");
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, face+1, tris[1+3*tri], &(nodes[1])), "l2g1");
      REIS( EGADS_SUCCESS,
	    EG_localToGlobal(tess, face+1, tris[2+3*tri], &(nodes[2])), "l2g2");
      nodes[0] -= 1;
      nodes[1] -= 1;
      nodes[2] -= 1;
      nodes[3] = face + 1;
      RSS( ref_cell_add(ref_grid_tri(ref_grid), nodes, &new_cell ), "new tri");
    }
  }
  
  REIS( EGADS_SUCCESS, EG_close(context), "EG close");

#else
  printf("returning empty grid, No EGADS linked for %s\n",filename);
  RSS( ref_grid_create( ref_grid_ptr ), "create grid");  
#endif
  
  return REF_SUCCESS;
}

REF_STATUS ref_geom_face_tec_zone( REF_GRID ref_grid, REF_INT id, FILE *file )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DICT ref_dict;
  REF_INT geom, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, local, node;
  REF_INT nnode, ntri;

  RSS( ref_dict_create( &ref_dict ), "create dict" ); 

  each_ref_geom_face( ref_geom, geom )
    if ( id == ref_geom_id(ref_geom,geom) )
      RSS( ref_dict_store( ref_dict, ref_geom_node(ref_geom,geom), geom ),
	   "mark nodes");
  nnode = ref_dict_n( ref_dict );
  
  ntri = 0;
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    if ( id == nodes[3] )
      ntri++;
    
  fprintf(file,
	  "zone t=face%d, nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
	  id, nnode, ntri, "point", "fetriangle" );

  each_ref_dict_key_value( ref_dict, item, node, geom )
    {
      fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e\n",
	      ref_node_xyz(ref_node,0,node),
	      ref_node_xyz(ref_node,1,node),
	      ref_node_xyz(ref_node,2,node),
	      ref_geom_param(ref_geom,0,geom),
	      ref_geom_param(ref_geom,1,geom),
	      0.0 ) ;
    }
  ref_dict_inspect(ref_dict);
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
    if ( id == nodes[3] )
      {
	for ( node = 0; node < 3; node++ )
	  {
	    RSS( ref_dict_location( ref_dict, nodes[node], &local),
		 "localize");
	    fprintf(file," %d",local+1);
	  }
	fprintf(file,"\n");
      }

  RSS( ref_dict_free( ref_dict ), "free dict" ); 

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tec( REF_GRID ref_grid, char *filename  )
{
  FILE *file;
  REF_INT id;
  
  file = fopen(filename,"w");
  if (NULL == (void *)file) printf("unable to open %s\n",filename);
  RNS(file, "unable to open file" );

  fprintf(file, "title=\"refine cad coupling in tecplot format\"\n");
  fprintf(file, "variables = \"x\" \"y\" \"z\" \"u\" \"v\" \"t\"\n");

  id = 1;
  RSS( ref_geom_face_tec_zone( ref_grid, id, file ), "surf" );

  fclose(file);
  return REF_SUCCESS;
}