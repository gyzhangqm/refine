#!/usr/bin/env ruby
#
# $Id$
#
# Mobility test for grid c lib

Dir.chdir ENV['srcdir'] if ENV['srcdir']

require 'RubyExtensionBuilder'

RubyExtensionBuilder.new('GridCAD').build

require 'test/unit'
require 'Adj/Adj'
require 'Line/Line'
require 'Grid/Grid'
require 'GridMath/GridMath'
require 'GridMetric/GridMetric'
require 'GridCAD/GridCAD'

class Grid
 include GridMetric
 include GridCAD
 def totalVolume
  vol = 0.0
  ncell.times { |cellId| vol += volume(cell(cellId)) }
  vol
 end
 def silentMinVolume
  vol = 999.0
  ncell.times do |cellId| 
   vol = volume(cell(cellId)) if volume(cell(cellId))<vol 
  end
  vol
 end
end

class TestGridCAD < Test::Unit::TestCase

 def testEdgeProjection
  assert_not_nil              grid = Grid.new(3,0,0,2)
  assert_equal 0,             grid.addNode(0.0,0.0,0.0)
  assert_equal 1,             grid.addNode(0.5,0.1,0.1)
  assert_nil                  grid.projectNodeToEdge(1,1)
  grid.addEdge(0,1,1,0.0,0.55)
  assert_equal grid,          grid.projectNodeToEdge(1,1)
  assert_equal [0.5,0.0,0.0], grid.nodeXYZ(1)
  assert_equal 0.0,           grid.nodeT(0,1)
  assert_equal 0.5,           grid.nodeT(1,1)
 end

 def testFaceProjection
  assert_not_nil              grid = Grid.new(4,0,2,0)
  assert_equal 0,             grid.addNode(0.0,0.0,0.1)
  assert_equal 1,             grid.addNode(1.0,0.0,0.0)
  assert_equal 2,             grid.addNode(0.0,1.0,0.0)
  assert_equal 3,             grid.addNode(1.0,1.0,0.0)
  assert_nil                  grid.projectNodeToFace(0,1)
  grid.addFaceUV(0, 10.1, 20.1,
					     3, 11.0, 21.0,
					     1, 11.0, 20.0,
					     1)
  grid.addFaceUV(0, 10.1, 20.1,
					     3, 11.0, 21.0,
					     2, 10.0, 21.0,
					     1)
  assert_equal grid,          grid.projectNodeToFace(0,1)
  assert_equal [0.0,0.0,0.0], grid.nodeXYZ(0)
  assert_equal [10.0,20.0],   grid.nodeUV(0,1)
 end

 def testEvaluateEdgeTvalue
  assert_not_nil  grid = Grid.new(4,1,1,1)
  assert_equal 0, grid.addNode(0,0,0)
  assert_equal 1, grid.addNode(1,0,0)
  assert_equal 2, grid.addNode(0,1,0)
  assert_equal 3, grid.addNode(0,0,1)
  grid.addFace(0,1,2,7)
  grid.addEdge(0,1,5,-2,-3)
  assert_equal grid, grid.setNGeomNode(1)
  assert_nil         grid.evaluateEdgeAtT(-1,0)
  assert_nil         grid.evaluateEdgeAtT(4,0)
  assert_nil         grid.evaluateEdgeAtT(0,0) 
  assert_equal(-2,  grid.nodeT(0,5))
  assert_equal(-3,  grid.nodeT(1,5))
  assert_equal grid, grid.evaluateEdgeAtT(1,0.5)
  assert_equal 0.5,  grid.nodeT(1,5)
  assert_equal [0.5,0,0],  grid.nodeXYZ(1)
 end

 def testEvaluateFaceUVvalue
  assert_not_nil  grid = Grid.new(4,1,1,1)
  assert_equal 0, grid.addNode(0,0,0)
  assert_equal 1, grid.addNode(1,0,0)
  assert_equal 2, grid.addNode(0,1,0)
  assert_equal 3, grid.addNode(0,0,1)
  grid.addFaceUV(0,-2,-3,
				    1,0,0,
				    2,0,0,
				    7)
  grid.addEdge(1,2,5,0,0);
  assert_nil         grid.evaluateFaceAtUV(-1,0,0)
  assert_nil         grid.evaluateFaceAtUV(4,0,0)
  assert_nil         grid.evaluateFaceAtUV(1,0,0)
  assert_equal [-2,-3],  grid.nodeUV(0,7)
  assert_equal grid, grid.evaluateFaceAtUV(0,10.1,20.2)
  assert_equal [10.1,20.2],  grid.nodeUV(0,7)
  gold = [0.1,0.2,0.0]
  xyz  = grid.nodeXYZ(0)
  assert_in_delta gold[0], xyz[0], 1e-15
  assert_in_delta gold[1], xyz[1], 1e-15
  assert_in_delta gold[2], xyz[2], 1e-15
 end

 def testUpdateFaceParameters
  assert_not_nil     grid = Grid.new(4,1,2,1)
  assert_equal 0,    grid.addNode(0,0,0)
  assert_equal 1,    grid.addNode(1,0,0)
  assert_equal 2,    grid.addNode(0,1,0)
  assert_equal 3,    grid.addNode(1,1,0)
  grid.addFace(0,1,2,7)
  grid.addFace(1,3,2,8)
  assert_equal grid, grid.updateFaceParameter(1)
  assert_equal [11,20], grid.nodeUV(1,7)
  assert_equal [11,20], grid.nodeUV(1,8)
 end

 def testProjectionToGeometry
  assert_not_nil grid = Grid.new(5,1,1,1)
  assert_equal 0, grid.addNode(5.0,5.0,5.0)
  assert_equal 1, grid.addNode(0.0,0.1,0.1)
  assert_equal 2, grid.addNode(1.0,0.1,0.1)
  assert_equal 3, grid.addNode(0.0,1.0,0.1)
  assert_equal 4, grid.addNode(0.0,0.0,1.0)
  grid.addCell(1,2,3,4)
  grid.addFace(1,2,3,10)
  grid.addEdge(1,2,20,0.0,1.0)
  assert_equal grid, grid.setNGeomNode(1)
  5.times do |i| 
   assert_equal grid, grid.safeProjectNode(i,1.0), "project node #{i}"
  end
  assert_equal [5.0,5.0,5.0], grid.nodeXYZ(0)
  assert_equal [0.0,0.0,0.0], grid.nodeXYZ(1)
  assert_equal [1.0,0.0,0.0], grid.nodeXYZ(2)
  assert_equal [0.0,1.0,0.0], grid.nodeXYZ(3)
  assert_equal [0.0,0.0,1.0], grid.nodeXYZ(4)
  assert_equal 0.0,           grid.nodeT(1,20)
  assert_equal 1.0,           grid.nodeT(2,20)
  assert_equal [10.0,20.0],   grid.nodeUV(1,10)
  assert_equal [11.0,20.0],   grid.nodeUV(2,10)
  assert_equal [10.0,21.0],   grid.nodeUV(3,10)
 end

 def unprojectableTet
  assert_not_nil grid = Grid.new(4,1,1,1)
  assert_equal 0, grid.addNode(0.0,0.0,-0.5)
  assert_equal 1, grid.addNode(1.0,0.0,0.0)
  assert_equal 2, grid.addNode(0.0,1.0,0.0)
  assert_equal 3, grid.addNode(0.0,0.0,-0.1)
  grid.addCell(0,1,2,3)
  grid.addFace(0,1,2,10)
  grid
 end

 def testSafeProjectionForPositiveVolume
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s  
  assert_nil   grid.safeProjectNode(0,1.0)
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
  grid.addEdge(0,1,20,0.0,1.0)
  assert_nil   grid.safeProjectNode(0,1.0)
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
 end

 def testSafeProjectionForBackOffRatioFace
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s  
  assert_nil   grid.safeProjectNode(0,0.5)
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.25], grid.nodeXYZ(0)
 end

 def testSafeProjectionForBackOffRatioEdge
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  grid.addEdge(0,1,20,0.0,1.0)
  assert_nil   grid.safeProjectNode(0,0.5)
  assert_nil   grid.project
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.25], grid.nodeXYZ(0)
 end

 def testSafeProjectionForBackOffRatioFaceTimeOut
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s  
  assert_nil   grid.safeProjectNode(0,0.0)
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
 end

 def testSafeProjectionForBackOffRatioEdgeTimeOut
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  grid.addEdge(0,1,20,0.0,1.0)
  assert_nil   grid.safeProjectNode(0,0.0)
  assert_nil   grid.project
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
 end

 def testProjectForPositiveVolume
  assert_not_nil grid = unprojectableTet
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s  
  assert_nil   grid.project
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
  grid.addEdge(0,1,20,0.0,1.0)
  assert_nil   grid.project
  assert grid.minVolume>0.0, "negative volume cell "+grid.minVolume.to_s
  assert_equal [0.0,0.0,-0.5], grid.nodeXYZ(0)
 end

 def isoTet(xpert = 0.0, zpert = 0.0, edge = nil)
  grid = Grid.new(4,1,1,1)
  grid.addNode( xpert, 0.000, 0.000 )
  grid.addNode( 1.000, 0.000, 0.000 )
  grid.addNode( 0.500, 0.866, 0.000 )
  grid.addNode( 0.500, 0.289, 0.823+zpert ) 
  grid.addCell(0,1,2,3)
  grid.addFaceUV(0,10.0+xpert,20.0,
		 1,11.0,20.0,
		 2,10.5,20.866,
		 10)
  grid.addEdge(0, 1, 20, 0.0+xpert, 1.0) if edge
  grid
 end

 def testIsotropicTet
  assert_in_delta 1.000, isoTet.minAR, 1.0e-4
  assert_in_delta 0.9797, isoTet(-0.2).minAR, 1.0e-4
 end

 def testOptimizeTDispacement
  assert_not_nil grid = isoTet(-0.2,0.0,true)
  assert_in_delta 0.9797, grid.minAR, 1.0e-3
  assert_equal grid, grid.optimizeT(0,1.0)
  assert_in_delta 0.999, grid.minAR, 1.0e-3
  assert_in_delta 0.0, grid.nodeT(0,20), 5.0e-2
 end

 def testSmoothEdge
  assert_not_nil grid = isoTet(-0.2,0.0,true)
  assert_in_delta 0.9797, grid.minAR, 1.0e-3
  assert_equal grid, grid.smoothNode(0)
  assert_in_delta 0.999, grid.minAR, 1.0e-3
 end

 def testOptimizeUVDispacement
  assert_not_nil grid = isoTet(-0.2)
  assert_equal grid, grid.optimizeUV(0,[1.0,0.0])
  assert_in_delta 0.999, grid.minAR, 1.0e-3
  assert_in_delta 10.0, grid.nodeUV(0,10)[0], 5.0e-2
  assert_in_delta 20.0, grid.nodeUV(0,10)[1], 1.0e-15
 end

 def testOptimizeFaceUVDispacement
  assert_not_nil grid = isoTet(-0.2)
  assert_equal grid, grid.optimizeFaceUV(0,[1.0,0.0])
  assert_in_delta 0.999, grid.nodeFaceMR(0), 1.0e-3
  assert_in_delta 10.0, grid.nodeUV(0,10)[0], 5.0e-2
  assert_in_delta 20.0, grid.nodeUV(0,10)[1], 1.0e-15
 end

 def testSmoothSurf
  assert_not_nil grid = isoTet(-0.2)
  assert_equal grid, grid.smoothNode(0)
  assert_in_delta 0.999, grid.minAR, 1.0e-3
 end

 def testSmoothNodeFaceMR
  assert_not_nil grid = isoTet(-0.2)
  assert_equal grid, grid.smoothNodeFaceMR(0)
  assert_in_delta 0.999, grid.faceMR(0,1,2), 1.0e-3
 end

 def testOptimizeXYZDispacement
  assert_not_nil grid = isoTet( 0.0, 1.0 )
  assert_equal grid, grid.optimizeXYZ(3,[0.0,0.0,-1.0])
  assert_in_delta 1.00, grid.minAR, 1.0e-2
  assert_equal grid, grid.optimizeXYZ(3,[0.0,0.0,1.0])
  assert_in_delta 1.00, grid.minAR, 1.0e-4
 end

 def testSmoothVol
  assert_not_nil grid = isoTet(0.0,1.0)
  assert_equal grid, grid.smoothNode(3)
  assert_in_delta 1.00, grid.minAR, 1.0e-1
  assert_equal grid, grid.smoothNode(3)
  assert_in_delta 1.00, grid.minAR, 1.0e-3
  assert_equal grid, grid.smoothNode(3)
  assert_in_delta 1.00, grid.minAR, 1.0e-4
 end

 def testSmooth
  assert_not_nil grid = isoTet(-4.0)
  assert_in_delta 0.252, grid.minAR, 1.0e-3
  assert_equal grid, grid.freezeAll
  assert_equal grid, grid.smooth
  assert_in_delta 0.252, grid.minAR, 1.0e-3
  assert_equal grid, grid.thawAll
  assert_equal grid, grid.smooth
  assert_in_delta 0.999, grid.minAR, 1.0e-3
  assert_in_delta 0.999, grid.minFaceMR, 1.0e-3
 end

 def testSmoothFaceMR
  assert_not_nil grid = isoTet(-4.0)
  node3 = grid.nodeXYZ(3)
  assert_in_delta 0.3191, grid.faceMR(0,1,2), 1.0e-3
  assert_equal grid, grid.smoothFaceMR(0.1)
  assert_in_delta 0.3191, grid.faceMR(0,1,2), 1.0e-3
  assert_equal grid, grid.smoothFaceMR(0.35)
  assert_in_delta 0.9053, grid.faceMR(0,1,2), 1.0e-3
  assert_equal grid, grid.smoothFaceMR(0.99)
  assert_in_delta 0.9999, grid.faceMR(0,1,2), 1.0e-3
  assert_equal node3, grid.nodeXYZ(3)
 end

 def gemGrid(nequ=4,disp=0.2,dent=nil, dentratio=-0.5)
  grid = Grid.new(nequ+2+1,nequ*2,nequ*2,0)
  n = Array.new
  n.push grid.addNode( 1.0,0.0,0.0)
  n.push grid.addNode(-1.0,0.0,0.0)
  nequ.times do |i| 
   angle = 2.0*Math::PI*(i-1)/(nequ)
   s = if (dent==i) then dentratio else 1.0 end
   n.push grid.addNode(0.0,s*Math.sin(angle),s*Math.cos(angle)) 
  end
  n.push 2
  center = grid.addNode(disp,disp,disp)
  nequ.times do |i|
   faceId = i+1
   grid.addCell(n[0],center,n[i+2],n[i+3])
   grid.addFace(n[0],n[i+2],n[i+3],faceId) # 0,2,3
   grid.addCell(center,n[1],n[i+2],n[i+3])
   grid.addFace(n[1],n[i+3],n[i+2],faceId) # 1,3,2
  end
  grid  
 end

 def testSmartLaplacianSmooth
  ngem =6
  assert_not_nil  grid=gemGrid(ngem)
  assert_in_delta 0.6041, grid.minAR, 1.0e-3
  assert_equal grid, grid.smartLaplacian(ngem+2)
  assert_in_delta 0.8585, grid.minAR, 1.0e-3
  assert_in_delta 0.0, grid.nodeXYZ(ngem+2)[0], 1.0e-15
  assert_in_delta 0.0, grid.nodeXYZ(ngem+2)[1], 1.0e-15
  assert_in_delta 0.0, grid.nodeXYZ(ngem+2)[2], 1.0e-15
 end

 def isoTet4 h=0.1
  grid = Grid.new(5,4,4,0)
  grid.addNode(0,0,0)
  grid.addNode(1,0,0)
  grid.addNode(0.5,0.866,0)
  grid.addNode(0.5,0.35,0.8)
  grid.addNode(0.5,0.35,0.8*h)

  grid.addCell(0,1,2,4)
  grid.addCell(0,3,1,4)
  grid.addCell(1,3,2,4)
  grid.addCell(0,2,3,4)

  grid.addFace(0,1,2,10)
  grid.addFace(0,3,1,10)
  grid.addFace(1,3,2,10)
  grid.addFace(0,2,3,10)
  grid
 end

 def testSmartLaplacianVolumeImprovement4
  grid = isoTet4( -1.0)
  assert_equal grid, grid.smartVolumeLaplacian(4)
  avgVol = grid.totalVolume/grid.ncell.to_f
  assert_in_delta avgVol, grid.minVolume, 1.0e-8 
 end

 def testSmartLaplacianVolumeImprovement8
  grid = gemGrid 4, 5.0
  grid.smartVolumeLaplacian(6)
  avgVol = grid.totalVolume/grid.ncell.to_f
  assert_in_delta avgVol, grid.minVolume, 1.0e-8 
 end

 def testImproveMinVolumeForConcaveGemWithBadLapacian
  grid = gemGrid 4, 5.0, 0
  avgVol = grid.totalVolume/grid.ncell.to_f
  assert_nil grid.smoothNodeVolume(0)
  assert_equal grid, grid.smoothNodeVolume(6)
  assert_in_delta avgVol, grid.minVolume, 1.0e-4
 end

 def testImproveMinVolumeForInvalidConcaveGem
  grid = gemGrid 4, 5.0, 0, -1.5
  avgVol = grid.totalVolume/grid.ncell.to_f
  grid.smoothNodeVolume(6)
  assert_in_delta avgVol, grid.silentMinVolume, 1.0e-4
 end

 def testRelaxNegativeCellsForConcaveGemWithBadLapacian
  grid = gemGrid 4, 5.0, 0
  avgVol = grid.totalVolume/grid.ncell.to_f
  assert_equal grid, grid.relaxNegativeCells
  assert_in_delta avgVol, grid.minVolume, 1.0e-4
 end

 def rightTet3
  grid = Grid.new(5,3,6,0)

  nodeXY = 1.0
  grid.addNode(nodeXY,nodeXY,0.0)
  grid.addNode(0.0,0.0,0.0)
  grid.addNode(1.0,0.0,0.0)
  grid.addNode(0.0,1.0,0.0)
  grid.addNode(0.0,0.0,1.0)

  visualizationId = 99
  grid.addFace(1,4,2,visualizationId)
  grid.addFace(2,4,3,visualizationId)
  grid.addFace(1,3,4,visualizationId)

  grid.addCell(1,4,2,0)
  grid.addCell(2,4,3,0)
  grid.addCell(1,3,4,0)

  realFaceId = 10
  grid.addFace(0,1,2,realFaceId)
  grid.addFace(0,2,3,realFaceId)
  grid.addFace(0,3,1,realFaceId)
  grid.projectNodeToFace(0,realFaceId)
  grid
 end

 def testRelaxNegativeCellsOnSurface
  grid = rightTet3
  #grid.exportFAST
  assert_equal grid, grid.smoothNodeVolumeWithSurf(0)
  assert grid.minVolume>0.0
 end

 def testRelaxNegativeCellsOnSurfaceWithGeneralSmooth
  grid = rightTet3
  #grid.exportFAST
  assert_equal grid, grid.smoothNodeVolume(0)
  assert grid.minVolume>0.0
 end

 def testComputeFaceAreaInParameterSpaceHalf
  grid = Grid.new(3,0,1,0)
  grid.addNode(0,0,0)
  grid.addNode(1,0,0)
  grid.addNode(0,1,0)
  # FAKEGeom: U = X+10; V = Y+20;
  grid.addFaceUV(0,10.0,20.0,
		 1,11.0,20.0,
		 2,10.0,21.0,
		 55)
  tol = 1.0e-15
  area = 0.5
  assert_in_delta area, grid.faceAreaUV(0), tol
 end

 def testComputeFaceAreaInParameterSpaceTwo
  grid = Grid.new(3,0,1,0)
  grid.addNode(0,0,0)
  grid.addNode(2,0,0)
  grid.addNode(0,2,0)
  # FAKEGeom: U = X+10; V = Y+20;
  grid.addFaceUV(0,10.0,20.0,
		 1,12.0,20.0,
		 2,10.0,22.0,
		 55)
  tol = 1.0e-15
  area = 2.0
  assert_in_delta area, grid.faceAreaUV(0), tol
 end

 def threeSurfaceTriangles( x=1, y=1)
  grid = Grid.new(5,1,3,0)
  grid.addNode(0,0,0)
  grid.addNode(1,0,0)
  grid.addNode(0,1,0)
  grid.addNode(x,y,0)
  faceId = 55
  grid.addFace(0,1,3,faceId)
  grid.addFace(1,2,3,faceId)
  grid.addFace(2,0,3,faceId)
  4.times { |node| grid.projectNodeToFace(node,faceId) }
  grid.addCell(0,1,2,3) # needed for gridAverageEdgeLength in Simplex
  grid # note ruby returns result of last line if return is missing
 end

 def testComputeMinFaceAreaInParameterSpaceForNode
  grid = threeSurfaceTriangles
  tol = 1.0e-15
  area = -0.5
  assert_in_delta area, grid.minFaceAreaUV(3), tol
 end

 def testSmoothFaceAreaInParameterSpaceForNode
  grid = threeSurfaceTriangles
  tol = 1.0e-7
  optimalArea = 0.5/3.0
  assert grid, grid.smoothNodeFaceAreaUV(3)
  assert_in_delta optimalArea, grid.minFaceAreaUV(3), tol
 end

 def testSmoothFaceAreaInParameterSpaceForNodeWith2BadFaces
  grid = threeSurfaceTriangles(-0.9,-1.1)
  tol = 1.0e-7
  optimalArea = 0.5/3.0
  assert grid, grid.smoothNodeFaceAreaUV(3)
  assert_in_delta optimalArea, grid.minFaceAreaUV(3), tol
 end

end
