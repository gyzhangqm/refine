
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov
 */

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include "gridgeom.h"
#ifdef HAVE_SDK
#include "CADGeom/CADGeom.h"
#else
#include "FAKEGeom.h"
#endif

Grid *gridParallelGeomLoad( Grid *grid, char *project )
{
  int vol=1;
  int nGeomNode, nGeomEdge, nGeomFace, nGeomGroups;
  UGridPtr ugrid;
  int i, iedge, inode;
  int nedgenode;
  double trange[2];
  int edgeEndPoint[2];
  CADCurvePtr edge;
  int face, localNode, globalNode, partitionNode;
  int patchDimensions[3];
  UGPatchPtr  localPatch, globalPatch;
  Iterator patchIterator;

  if ( ! CADGeom_Start( ) ){
    printf("ERROR: CADGeom_Start broke.\n%s\n",ErrMgr_GetErrStr());
    return NULL;
  }  

  if ( ! GeoMesh_LoadPart( project ) ){
    printf("ERROR: GeoMesh_LoadPart broke.\n%s\n",ErrMgr_GetErrStr());
    return NULL;
  }

  if( !CADGeom_GetVolume(vol,&nGeomNode,&nGeomEdge,&nGeomFace,&nGeomGroups) ) {
    printf("ERROR: CADGeom_GetVolume. \n%s\n",ErrMgr_GetErrStr());
  }

  if ( 0==gridPartId(grid) )
    printf("Geometry: %d nodes %d edges %d faces %d boundaries\n",
	   nGeomNode,nGeomEdge,nGeomFace,nGeomGroups);

  gridSetNGeomNode( grid, nGeomNode );
  gridSetNGeomEdge( grid, nGeomEdge );
  gridSetNGeomFace( grid, nGeomFace );

  inode = nGeomNode;

  for( iedge=1; iedge<=nGeomEdge; iedge++ ) {
    if( (edge=CADGeom_EdgeGrid(vol,iedge)) == NULL ) 
      printf("ERROR: CADGeom_EdgeGrid(%d).\n%s\n",iedge,ErrMgr_GetErrStr());
 
    nedgenode = CADCURVE_NUMPTS(edge);

    CADGeom_GetEdge( vol, iedge, trange, edgeEndPoint );

    edgeEndPoint[0]--; /* convert from fortran to c numbers */
    edgeEndPoint[1]--;

    gridAddGeomEdge( grid, iedge, edgeEndPoint[0], edgeEndPoint[1]);

    if (nedgenode == 2) {
      gridAddEdgeInGlobal(grid, edgeEndPoint[0], edgeEndPoint[1], 
			  iedge, trange[0], trange[1]);
    }else{
      gridAddEdgeInGlobal(grid, edgeEndPoint[0], inode, iedge,
			  edge->param[0], edge->param[1]);
      for( i=1 ; i < (nedgenode-2) ; i++ ) { // skip end segments  
	gridAddEdgeInGlobal(grid, inode, inode+1, iedge,
			    edge->param[i], edge->param[i+1]);
	inode++;
      }
      gridAddEdgeInGlobal(grid, inode, edgeEndPoint[1], iedge,
			  edge->param[nedgenode-2], 
			  edge->param[nedgenode-1]);
      inode++;
    }
  }

  /* get uv vals for surface(s) */
  /* we use globalPatch to track with the localPatch so that we can get global
   * node numbering relative the volume grid and NOT the face grid as would
   * be the case of global index of upp
   */

  if (NULL == (ugrid = CADGeom_VolumeGrid(vol)) ) {
    printf("ERROR: Can not find grid in restart. \n%s\n",ErrMgr_GetErrStr());
    return NULL;
  }

  globalPatch = DList_SetIteratorToHead(UGrid_PatchList(ugrid),&patchIterator);

  for( face=1; face<=nGeomFace; face++ ) {
    localPatch = CADGeom_FaceGrid(vol,face);
    UGPatch_GetDims(localPatch,patchDimensions);
    for( localNode=0; localNode<patchDimensions[0]; localNode++ ) {
      globalNode = UGPatch_GlobalIndex(globalPatch,localNode);
      partitionNode = gridGlobal2Local(grid, globalNode);
      if (partitionNode>EMPTY) 
	gridSetNodeUV( grid, partitionNode, face,
		       UGPatch_Parameter(localPatch,localNode,0), 
		       UGPatch_Parameter(localPatch,localNode,1));
    }
    globalPatch = DList_GetNextItem(&patchIterator);
  }

  return grid;
}

Grid *gridParallelGeomSave( Grid *grid, char *project )
{
  printf("DUMMY: gridParallelGeomSave for project %s\n",project);
  return grid;
}
