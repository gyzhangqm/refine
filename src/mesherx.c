
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov
 */

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>         /* Needed in some systems for DBL_MAX definition */
#include <float.h>
#include "mesherx.h"
#include "grid.h"
#include "gridfiller.h"
#include "CADGeom/CADGeom.h"
#include "CADGeom/CADTopo.h"
#include "Goolache/MeshMgr.h"
#include "Goolache/UGPatch.h"
#include "MeatLib/ErrMgr.h"
#include "MeatLib/GeoBC.h"

int MesherX_DiscretizeVolume( int maxNodes, double scale, char *project,
			      bool mixedElement )
{
  char outputProject[256];
  int vol=1;
  Grid *grid;
  Layer *layer;
  int i;
  double h;
  double rate;
  int nLayer;


  nLayer = (int)(10.0/scale);
  nLayer = 15;
  rate = exp(scale*log(1.2));
  printf("rate is set to %10.5f for %d layers\n",rate,nLayer);

  grid = gridFillFromPart( vol, maxNodes );

  MeshMgr_SetElementScale( scale );
  //CAPrIMesh_TetVolume( vol );

  //layer = layerFormAdvancingLayerWithCADGeomBCS( 1, grid );
  layer = formAdvancingFront( grid, project);
  if (mixedElement) layerToggleMixedElementMode(layer);

  /* only needed for formAdvancingFront freeze distant volume nodes */
  gridThawAll(grid); 
  layerFindParentGeomEdges(layer);
  i=0;
  layerLaminarInitialHeight(layer, 1000.0, -0.05 );
  layerScaleNormalHeight(layer,scale);
  while (i<nLayer && layerNNormal(layer)>layerTerminateNormalWithBGSpacing(layer, 0.5)) {
    //layerVisibleNormals(layer);
    layerAdvance(layer);
    layerScaleNormalHeight(layer,rate);
    i++;
  }

  printf(" -- REBUILD EDGES\n");
  if ( layer != layerRebuildEdges(layer,vol) ) {
    sprintf(outputProject,"%s_MXerror.fgrid",project);
    printf("writing ERROR FAST file %s\n",outputProject);
    gridExportFAST( grid, outputProject  );
    return 0;
  }

  printf(" -- REBUILD FACES\n");
  if ( layer != layerRebuildFaces(layer,vol) ) {
    sprintf(outputProject,"%s_MXerror.fgrid",project);
    printf("writing ERROR FAST file %s\n",outputProject);
    gridExportFAST( grid, outputProject  );
    return 0;
  }

  printf(" -- REBUILD VOLUME\n");
  if ( layer != layerRebuildVolume(layer,vol) ) {
    sprintf(outputProject,"%s_MXerror.fgrid",project);
    printf("writing ERROR FAST file %s\n",outputProject);
    gridExportFAST( grid, outputProject  );
    return 0;
  }

  printf(" -- DUMP PART\n");

  sprintf(outputProject,"%s_MX.fgrid",project);
  printf("writing output FAST file %s\n",outputProject);
  gridExportFAST( grid, outputProject  );

  sprintf(outputProject,"%s_MX.ugrid",project);
  printf("writing output AFL3R file %s\n",outputProject);
  gridExportAFLR3( grid, outputProject  );

  sprintf(outputProject,"%s_MX",project);
  printf("writing DEBUG output project %s\n",outputProject);
  gridSavePart( grid, outputProject );

  return 1;
}

int layerTerminateNormalWithBGSpacing(Layer *layer, double ratio)
{
  int normal, nterm;
  int root;
  double xyz[3];
  double spacing[3];
  double direction[9];
  double height;

  if (layerNNormal(layer) == 0 ) return EMPTY;

  nterm = 0;
  for (normal=0;normal<layerNNormal(layer);normal++){
    layerGetNormalHeight(layer,normal,&height);

    root = layerNormalRoot(layer, normal );
    gridNodeXYZ(layerGrid(layer),root,xyz);
    MeshMgr_GetSpacing(&(xyz[0]),&(xyz[1]),&(xyz[2]),spacing,direction);

    if (height > ratio*spacing[0]) {     /* Assume Isotropic for now */
      nterm++;
      layerTerminateNormal(layer, normal);
    }
  }
  printf("normals %d of %d terminated\n",nterm,layerNNormal(layer) );
  return nterm;
}

Layer *layerFormAdvancingLayerWithCADGeomBCS( int vol, Grid *grid )
{
  int nFrontFaces, frontFaces[10000];
  int face;
  UGPatchPtr upp;

  int    i,loop,edge,current;
  int    nloop;
  int    *nedge;
  int    *edges;
  double uv[4];
  int    self,other;
  int    left,right;

  Layer *layer;

  layer = layerCreate( grid );
  if (NULL == layer) printf("ERROR layerCreate failed: %s: %d\n",__FILE__,__LINE__);
  nFrontFaces =0;
  for (face=1;face<=gridNGeomFace(grid);face++){
    upp = CADGeom_FaceGrid(vol,face);
    if (NULL == upp) printf("ERROR CADGeom_FaceGrid(%d,%d) failed: %s: %d\n",vol,face,__FILE__,__LINE__);
    if ( BC_NOSLIP == GeoBC_GenericType(UGPatch_BC(upp))){
      printf("face %d is added to front.\n",face);
      frontFaces[nFrontFaces] = face;
      nFrontFaces++;
    }
  }

  layerPopulateAdvancingFront(layer, nFrontFaces, frontFaces);
  
  for (face=1;face<=gridNGeomFace(grid);face++){
    upp = CADGeom_FaceGrid(vol,face);
    if ( BC_NOSLIP != GeoBC_GenericType(UGPatch_BC(upp))){
      
      if( !CADGeom_GetFace(vol,face,uv,&nloop,&nedge,&edges) ) {  /* Face Info */
        ErrMgr_Set(__FILE__,__LINE__,"%s\nCould NOT get Volume %d, Face %d Info",ErrMgr_GetErrStr(),vol,face);
        return( NULL );
      }

      for( loop=0; loop<nloop; loop++ ) {                         /* Each Loop */
        for( current=0,i=0; current<nedge[loop]; current++,i+=2 ) { /* Each Edge */
	  if( edges[i+1] > 0 )
            CADTopo_EdgeFaces(vol,edges[i],&self,&other);
          else
            CADTopo_EdgeFaces(vol,edges[i],&other,&self);
	  upp = CADGeom_FaceGrid(vol,other);
	  if ( BC_NOSLIP == GeoBC_GenericType(UGPatch_BC(upp))){
	    layerConstrainNormal(layer,self);
	    printf("face %d is used to constrain normals.\n",face);
	  }
        }
      }
    }
  }

  for (edge=1;edge<=gridNGeomEdge(grid);edge++){
    CADTopo_EdgeFaces(vol,edge,&left,&right);
    if ( layerConstrainingGeometry(layer,left) &&
	 layerConstrainingGeometry(layer,right) ){
      layerConstrainNormal(layer,-edge);
      printf("edge %d is used to constrain normals.\n",edge);
    }
  }

  layerVisibleNormals(layer);

  return layer;
}


Layer *layerRebuildEdges(Layer *layer, int vol){

  int i, edgeId, edgeEndPoints[2];

  double edgexyz[6],tRange[2];
  int nedgenode;
  double *newxyz;
  double *newt;
  int *newnodes;
  int i0, i1;

  Grid *grid;
  grid = layerGrid(layer);

  for (edgeId=1;edgeId<=gridNGeomEdge(grid);edgeId++) {
    if ( layerConstrainingGeometry(layer,-edgeId) ){
      edgeEndPoints[0]=gridGeomEdgeStart(grid,edgeId);
      edgeEndPoints[1]=gridGeomEdgeEnd(grid,edgeId);
      printf("rebuild edge %4d:  %10d <-> %10d\n",
	     edgeId,edgeEndPoints[0],edgeEndPoints[1]);
      edgeEndPoints[0] = gridFrozenEdgeEndPoint(grid,edgeId,edgeEndPoints[0]);
      edgeEndPoints[1] = gridFrozenEdgeEndPoint(grid,edgeId,edgeEndPoints[1]);
      printf("rebuild endpoints:  %10d <-> %10d\n",
	     edgeEndPoints[0],edgeEndPoints[1]);
      gridNodeXYZ(grid, edgeEndPoints[0], &edgexyz[0]);
      gridNodeXYZ(grid, edgeEndPoints[1], &edgexyz[3]);
      gridNodeT(grid, edgeEndPoints[0], edgeId, &tRange[0]);
      gridNodeT(grid, edgeEndPoints[1], edgeId, &tRange[1] );

      /* WTJ will change this to generic with DSO */
      if( !MeshMgr_MeshEdge(vol, edgeId, edgexyz, tRange, &nedgenode, &newxyz, &newt) ) {
	printf("Could NOT mesh Edge %d\n",edgeId);
	return NULL;
      }

      printf("number of rebuild edge points:  %d\n",nedgenode);

      newnodes = malloc( nedgenode * sizeof(int));
      newnodes[0] = edgeEndPoints[0];
      newnodes[nedgenode-1] = edgeEndPoints[1];
      for(i=1;i<(nedgenode-1);i++){
	newnodes[i]=gridAddNode(grid,newxyz[0+3*i],newxyz[1+3*i],newxyz[2+3*i]);
      }

      gridDeleteThawedEdgeSegments(grid,edgeId);
      for(i=1;i<nedgenode;i++){
	i0 = i-1; i1 = i;
	gridAddEdge(grid,newnodes[i0],newnodes[i1],edgeId,newt[i0],newt[i1]);
      }

      free(newxyz);
      free(newt);
      free(newnodes);
      
    }
  }

  return layer;
}

Layer *layerRebuildFaces(Layer *layer, int vol){

  int maxnode, nnode;
  int edgeId;
  int faceId;
  double uv[4];
  int loop, nloop;
  int *loopLength;
  int *loopEdge;
  int edge;
  int nedge;
  int nshell;
  int nparent;
  int nthaw;
  int orient;
  int *shell;
  int nodes[3], triangle, side;
  int ncurve, *curve;
  int i,j;
  int n0,n1;
  int *l2g, *g2l;
  int node;
  double *shellxyz, *shelluv, resolved[3];
  int nfacenode, nfacetri, *newface;
  double *newxyz, *newuv;
  int l0,l1,l2,g0,g1,g2;

  Grid *grid;
  grid = layerGrid(layer);

  maxnode = gridMaxNode(grid);

  l2g = malloc(maxnode*sizeof(int));
  g2l = malloc(maxnode*sizeof(int));

  for (faceId=1;faceId<=gridNGeomFace(grid);faceId++){
    if (layerConstrainingGeometry(layer,faceId)) {
      printf("faceId %4d is a rebuild face.\n",faceId);    
      CADGeom_GetFace(vol, faceId, uv, &nloop, &loopLength, &loopEdge);
      nedge = 0;
      for (loop=0;loop<nloop;loop++) nedge += loopLength[loop];
      nshell =0;
      for(edge=0;edge<nedge;edge++){
	edgeId = loopEdge[0+2*edge];
	orient = loopEdge[1+2*edge];
	nparent = layerNParentGeomEdgeSegments(layer,edgeId);
	if (nparent > 0 ) {
	  nshell += nparent;
	  printf(" edge %4d, edgeId %4d %2d has %4d phantom.\n",
		 edge,edgeId,orient,nparent);
	} else if ( layerConstrainingGeometry(layer,-edgeId) ) {
	  nthaw = gridNThawedEdgeSegments(grid,edgeId);
	  nshell += nthaw;
	  printf(" edge %4d, edgeId %4d %2d has %4d rebuild.\n",
		 edge,edgeId,orient,nthaw);
	} else {
	  nthaw = gridGeomEdgeSize(grid,edgeId)-1;
	  nshell += nthaw;
	  printf(" edge %4d, edgeId %4d %2d has %4d original.\n",
		 edge,edgeId,orient,nthaw);
	}
      }
      printf("faceId %4d has %4d segments\n",faceId,nshell);
      shell = malloc(2*nshell*sizeof(int));
      nshell =0;
      for(edge=0;edge<nedge;edge++){
	edgeId = loopEdge[0+2*edge];
	orient = loopEdge[1+2*edge];
	nparent = layerNParentGeomEdgeSegments(layer,edgeId);
	if (nparent > 0 ) {
	  for(triangle=0;triangle<layerNTriangle(layer);triangle++){
	    for(side=0;side<3;side++){
	      if (edgeId == layerParentGeomEdge(layer,triangle,side)){
		n0 = side;
		n1 = side+1; if (n1>2) n1 = 0;
		layerTriangle(layer,triangle,nodes);
		n0 = nodes[n0];
		n1 = nodes[n1];
		shell[0+2*nshell] = n0;
		shell[1+2*nshell] = n1;
		nshell++;
 	      }
	    }
	  }
	  printf("phantom %4d %4d edge added. nshell %8d\n",edge,edgeId,nshell);
	} else if ( layerConstrainingGeometry(layer,-edgeId) ) {
	  ncurve = gridGeomEdgeSize(grid,edgeId);
	  curve = malloc( ncurve * sizeof(int) );
	  gridGeomEdge( grid, edgeId, curve );
	  for(i=1;i<ncurve;i++){
	    if ( !gridNodeFrozen(grid,curve[i-1]) || 
		 !gridNodeFrozen(grid,curve[i])   ){
	      if (orient>0){
		shell[0+2*nshell] = curve[i-1];
		shell[1+2*nshell] = curve[i];
	      }else{
		shell[0+2*nshell] = curve[i];
		shell[1+2*nshell] = curve[i-1];
	      }
	      nshell++;
	    }	      
	  }
	  free(curve);
	  printf("rebuild %4d %4d edge added. nshell %8d\n",edge,edgeId,nshell);
	} else {
	  ncurve = gridGeomEdgeSize(grid,edgeId);
	  curve = malloc( ncurve * sizeof(int) );
	  gridGeomEdge( grid, edgeId, curve );
	  for(i=1;i<ncurve;i++){
	    if (orient>0){
	      shell[0+2*nshell] = curve[i-1];
	      shell[1+2*nshell] = curve[i];
	    }else{
	      shell[0+2*nshell] = curve[i];
	      shell[1+2*nshell] = curve[i-1];
	    }
	    nshell++;	      
	  }
	  free(curve);
	  printf("original%4d %4d edge added. nshell %8d\n",edge,edgeId,nshell);
	}
      }
      printf("faceId %4d has %4d segments\n",faceId,nshell);

      for(i=0;i<maxnode;i++) l2g[i]=EMPTY;
      for(i=0;i<maxnode;i++) g2l[i]=EMPTY;
      nnode = 0;
      for(i=0;i<nshell;i++){
	if (EMPTY == g2l[shell[0+2*i]] ) { 
	  g2l[shell[0+2*i]] = nnode; 
	  nnode++;
	}
	if (EMPTY == g2l[shell[1+2*i]] ) { 
	  g2l[shell[1+2*i]] = nnode; 
	  nnode++;
	}
	shell[0+2*i] = g2l[shell[0+2*i]];
	shell[1+2*i] = g2l[shell[1+2*i]];
      }
      printf("the shell has %8d nodes.\n",nnode);
      for(i=0;i<maxnode;i++)if (EMPTY != g2l[i]) l2g[g2l[i]]=i;
      shellxyz = malloc(3*nnode*sizeof(double));
      shelluv  = malloc(2*nnode*sizeof(double));
      for(i=0;i<nnode;i++) gridNodeXYZ(grid,l2g[i],&shellxyz[3*i]);
      for(i=0;i<nnode;i++) {
	gridNodeUV(grid,l2g[i],faceId, &shelluv[2*i]);
	/* project all nodes for safety. rebuilt edge has no uv */
	CADGeom_ResolveOnFace(vol,faceId,&shellxyz[3*i],&shelluv[2*i],resolved);
      }

      nfacenode = EMPTY;
      nfacetri  = EMPTY;
      if( !MeshMgr_MeshTriFace(vol, faceId, 
			       nnode, shellxyz, shelluv,
			       nshell, shell,
			       0, NULL,
			       &nfacenode, &nfacetri, 
			       &newface, &newxyz, &newuv) ) {
	printf("%s\nCould NOT mesh Face %d\n",ErrMgr_GetErrStr(),faceId);
	return NULL;
      }
      printf("rebuild face has %d nodes %d faces\n",nfacenode,nfacetri);

      gridDeleteThawedFaces(grid, faceId);

      for(i=nnode;i<nfacenode;i++){
	l2g[i]=gridAddNode(grid,newxyz[0+3*i],newxyz[1+3*i],newxyz[2+3*i]);
      }
      for(i=0;i<nfacetri;i++){
	l0 = newface[0+3*i]; 
	l1 = newface[1+3*i]; 
	l2 = newface[2+3*i];
	g0 = l2g[l0];
	g1 = l2g[l1];
	g2 = l2g[l2];
	gridAddFaceUV(grid,
		      g0, newuv[0+2*l0], newuv[1+2*l0],
		      g1, newuv[0+2*l1], newuv[1+2*l1],
		      g2, newuv[0+2*l2], newuv[1+2*l2],
		      faceId );
      }

      free(shell);
      free(newface);
      free(newxyz);
      free(newuv);
    }
  }

  free(g2l);
  free(l2g);

  return layer;
}

Layer *layerRebuildVolume(Layer *layer, int vol){

  int faceId;
  int nshell, *shell;
  int *l2g, *g2l;
  int maxnode, nnode;
  int maxface, face;
  int nodes[3];
  int triangle;
  int i;
  double *shellxyz;

  int nvolnode, nvolcell;
  int *newcell;
  double *newxyz;

  Grid *grid;
  grid = layerGrid(layer);

  maxnode = gridMaxNode(grid);
  maxface = gridMaxFace(grid);

  l2g = malloc(maxnode*sizeof(int));
  g2l = malloc(maxnode*sizeof(int));

  nshell =0;

  for (faceId=1;faceId<=gridNGeomFace(grid);faceId++){
    if (!layerParentGeomFace(layer,faceId)) {
      // use total faces for origial faces not thawed, but they should be thawed
      nshell += gridNThawedFaces(grid,faceId);
    }
  }
  nshell += layerNTriangle(layer);
  printf("allocating  %10d faces for shell.\n",nshell);

  shell = malloc(3*nshell*sizeof(int));

  nshell =0;
  for (face=0;face<maxface;face++){
    if (grid==gridFace(grid,face,nodes,&faceId)){
      if (!layerParentGeomFace(layer,faceId) && 
	  ( !gridNodeFrozen(grid,nodes[0]) ||
	    !gridNodeFrozen(grid,nodes[1]) ||
	    !gridNodeFrozen(grid,nodes[2])    ) ){
	shell[0+3*nshell] = nodes[0];
	shell[1+3*nshell] = nodes[1];
	shell[2+3*nshell] = nodes[2];
	nshell++;
      }
    }
  }
  for(triangle=0;triangle<layerNTriangle(layer);triangle++){
    layerTriangle(layer, triangle, nodes);
    shell[0+3*nshell] = nodes[0];
    shell[1+3*nshell] = nodes[1];
    shell[2+3*nshell] = nodes[2];
    nshell++;
  }
  printf("inserted  %12d faces into shell.\n",nshell);

  for(i=0;i<maxnode;i++) l2g[i]=EMPTY;
  for(i=0;i<maxnode;i++) g2l[i]=EMPTY;
  nnode = 0;
  for(i=0;i<nshell;i++){
    if (EMPTY == g2l[shell[0+3*i]] ) { 
      g2l[shell[0+3*i]] = nnode; 
      nnode++;
    }
    if (EMPTY == g2l[shell[1+3*i]] ) { 
      g2l[shell[1+3*i]] = nnode; 
      nnode++;
    }
    if (EMPTY == g2l[shell[2+3*i]] ) { 
      g2l[shell[2+3*i]] = nnode; 
      nnode++;
    }
    shell[0+3*i] = g2l[shell[0+3*i]];
    shell[1+3*i] = g2l[shell[1+3*i]];
    shell[2+3*i] = g2l[shell[2+3*i]];
  }
  printf("the shell has %8d nodes.\n",nnode);
  for(i=0;i<maxnode;i++)if (EMPTY != g2l[i]) l2g[g2l[i]]=i;
  shellxyz = malloc(3*nnode*sizeof(double));
  for(i=0;i<nnode;i++) gridNodeXYZ(grid,l2g[i],&shellxyz[3*i]);

  if( !MeshMgr_MeshTetVolume(vol, 
			     nnode, shellxyz,
			     &nshell, &shell,
			     0, NULL, 0, NULL,
			     &nvolnode, &nvolcell, 
			     &newcell, &newxyz) ) {
    printf("%s\nCould NOT mesh Volume %d\n",ErrMgr_GetErrStr(),vol);
    return NULL;
  }
  printf("rebuild volume has %d nodes %d cells\n",nvolnode,nvolcell);

  gridDeleteThawedCells(grid);
  gridDeleteNodesNotUsed(grid);

  for(i=nnode;i<nvolnode;i++){
    l2g[i]=gridAddNode(grid,newxyz[0+3*i],newxyz[1+3*i],newxyz[2+3*i]);
  }
  for(i=0;i<nvolcell;i++){
    gridAddCell(grid,
		l2g[newcell[0+4*i]],
		l2g[newcell[1+4*i]],
		l2g[newcell[2+4*i]],
		l2g[newcell[3+4*i]]);
  }

  free(l2g);
  free(g2l);
  free(newcell);
  free(newxyz);

  return layer;
}
