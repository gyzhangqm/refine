#!/usr/bin/env ruby
#

#
# Mobility test for grid c lib

Dir.chdir ENV['srcdir'] if ENV['srcdir']

$:.push "." # ruby 1.9.2
require 'RubyExtensionBuilder'

RubyExtensionBuilder.new('GridMetric').build

require 'test/unit'
require 'Adj/Adj'
require 'Line/Line'
require 'Grid/Grid'
require 'GridMath/GridMath'
require 'GridMetric/GridMetric'

class Grid
 include GridMetric
end

class TestGridMetric < Test::Unit::TestCase

 def rightTet
  grid = Grid.new(4,1,2,0)
  grid.addCell( 
	       grid.addNode(0.0,0.0,0.0), 
	       grid.addNode(1.0,0.0,0.0), 
	       grid.addNode(0.0,1.0,0.0), 
	       grid.addNode(0.0,0.0,1.0) )
  grid
 end

 def rightTet3
  grid = Grid.new(5,2,2,0)
  grid.addCell( 
	       grid.addNode(0.0,0.0,0.0), 
	       grid.addNode(1.0,0.0,0.0), 
	       grid.addNode(0.0,1.0,0.0), 
	       grid.addNode(0.0,0.0,1.0) )
  grid.addNode(-1.0,0.0,0.0)
  grid.addCell(4,0,2,3)
  grid.addCell( 
	       1, 
	       grid.addNode(2.0,0.0,0.0), 
	       grid.addNode(1.0,1.0,0.0), 
	       grid.addNode(1.0,0.0,1.0) )
  grid
 end

 def isoTet
  grid = Grid.new(4,1,0,0)
  grid.addNode( 0.000, 0.000, 0.000 )
  grid.addNode( 1.000, 0.000, 0.000 )
  grid.addNode( 0.500, 0.866, 0.000 )
  grid.addNode( 0.500, 0.289, 0.823 ) 
  grid.addCell(0,1,2,3)
  grid
 end

 def testIsotropicTet
  assert_in_delta 1.000, isoTet.minAR, 1.0e-4
 end

 def testEdgeLength
  assert_not_nil grid = Grid.new(2,0,0,0)
  assert_equal 0, grid.addNode(0.0,0.0,0.0)
  assert_equal 1, grid.addNode(0.0,0.0,2.0)
  assert_in_delta 2.0, grid.edgeLength(0,1), 1.0e-15
 end
 
 def testMatrixSpaceMap
  assert_not_nil grid = Grid.new(5,1,0,0)
  assert_equal 0, grid.addNode(-1.0,-1.0,-1.0)
  grid.addCell( grid.addNode(0.0,0.0,0.0), 
	       grid.addNode(1.0,0.0,0.0), 
	       grid.addNode(0.0,2.0,0.0), 
	       grid.addNode(0.0,0.0,5.0) )
  1.upto(4) do |n| 
   assert_equal grid, grid.setMap(n, 1.00, 0.00, 0.00,
	 		                   0.25, 0.00,
			                         0.04)
  end
  assert_equal grid, grid.removeNode(0)
  assert_equal grid, grid.sortNodeGridEx
  assert_in_delta 1.0, grid.edgeRatio(0,1), 1.0e-15
  assert_in_delta 1.0, grid.edgeRatio(0,2), 1.0e-15
  assert_in_delta 1.0, grid.edgeRatio(0,3), 1.0e-15
  assert_in_delta 1.0, grid.edgeLength(0,1), 1.0e-15
  assert_in_delta 2.0, grid.edgeLength(0,2), 1.0e-15
  assert_in_delta 5.0, grid.edgeLength(0,3), 1.0e-15
 end

 def testSetSpacing
  h = 0.125
  assert_not_nil grid = Grid.new(2,0,0,0)
  assert_equal 0, grid.addNode(1,0,0)
  assert_equal grid, grid.setMap(0,64,0,0,64,0,64)
  assert_equal [1.0/h/h,0.0,0.0,1.0/h/h,0.0,1.0/h/h], grid.map(0)
  assert_equal h, grid.spacing(0)
 end

 def testCopySpacing
  assert_not_nil grid = Grid.new(2,0,0,0)
  assert_equal 0, grid.nnode
  assert_nil      grid.copySpacing(0,1)
  assert_equal 0, grid.addNode(0,0,0)
  assert_nil      grid.copySpacing(0,1)
  assert_equal 1, grid.addNode(1,0,0)
  assert_equal grid, grid.setMap(0, 1, 0, 0, 1, 0, 1)
  assert_equal grid, grid.setMap(1, 2, 0, 0, 2, 0, 2)
  assert_equal 1, grid.spacing(0)
  assert_in_delta Math::sqrt(0.5), grid.spacing(1), 1.0e-14
  assert_equal grid, grid.copySpacing(0,1)
  assert_equal 1, grid.spacing(1)  
 end

 def testAverageSpacing
  assert_not_nil grid = Grid.new(3,0,0,0)
  grid.addNode(0,0,0)
  grid.addNode(0,0,0)
  grid.addNode(0,0,0)
  assert_equal grid, grid.setMap(0, 1, 2, 3, 4, 5, 6)
  assert_equal grid, grid.setMap(1,11,12,13,14,15,16)
  assert_equal grid, grid.setMap(2,20,20,20,20,20,20)
  assert_equal [20,20,20,20,20,20], grid.map(2)
  assert_equal grid, grid.setMapMatrixToAverageOfNodes(2,0,1)
  assert_equal [6,7,8,9,10,11], grid.map(2)
 end

 def testMatrixSpaceMapRotate
  assert_not_nil grid = Grid.new(3,1,0,0)
  assert_equal 0, grid.addNode( 0.0,0.0,0.0)
  assert_equal 1, grid.addNode( 2.0,2.0,0.0)
  assert_equal 2, grid.addNode(-1.0,1.0,0.0)
  sr2  = Math::sqrt(2.0)
  0.upto(2) do |n| 
   assert_equal grid, grid.setMap(n, 0.3125, -0.1875,  0.0000,
	 		                      0.3125,  0.0000,
		                               1.0000)
  end
  assert_equal grid, grid.sortNodeGridEx
  assert_in_delta 2*sr2, grid.edgeLength(0,1), 1.0e-15
  assert_in_delta sr2,   grid.edgeLength(0,2), 1.0e-15
  assert_in_delta 1.0, grid.edgeRatio(0,1), 1.0e-15
  assert_in_delta 1.0, grid.edgeRatio(0,2), 1.0e-15
 end

 def testConvertMetricToJacobian111
  assert_not_nil grid = Grid.new(1,0,0,0)
  assert_not_nil jacob = grid.convertMetricToJacobian( [ 1.0, 0.0, 0.0, 
                                                              1.0, 0.0, 
                                                                   1.0 ] )
  assert_in_delta 1.0, jacob[0], 1.0e-15
  assert_in_delta 0.0, jacob[1], 1.0e-15
  assert_in_delta 0.0, jacob[2], 1.0e-15
  assert_in_delta 0.0, jacob[3], 1.0e-15
  assert_in_delta 1.0, jacob[4], 1.0e-15
  assert_in_delta 0.0, jacob[5], 1.0e-15
  assert_in_delta 0.0, jacob[6], 1.0e-15
  assert_in_delta 0.0, jacob[7], 1.0e-15
  assert_in_delta 1.0, jacob[8], 1.0e-15
 end

 def testConvertMetricToJacobian149
  assert_not_nil grid = Grid.new(1,0,0,0)
  assert_not_nil jacob = grid.convertMetricToJacobian( [ 1.0, 0.0, 0.0, 
                                                              4.0, 0.0, 
                                                                   9.0 ] )
  assert_in_delta 0.0, jacob[0], 1.0e-15
  assert_in_delta 0.0, jacob[1], 1.0e-15
  assert_in_delta 3.0, jacob[2], 1.0e-15
  assert_in_delta 0.0, jacob[3], 1.0e-15
  assert_in_delta(-2.0, jacob[4], 1.0e-15)
  assert_in_delta 0.0, jacob[5], 1.0e-15
  assert_in_delta 1.0, jacob[6], 1.0e-15
  assert_in_delta 0.0, jacob[7], 1.0e-15
  assert_in_delta 0.0, jacob[8], 1.0e-15
 end

 def testConvertMetricToJacobian2x2plus1
  assert_not_nil grid = Grid.new(1,0,0,0)
  assert_not_nil jacob = grid.convertMetricToJacobian([0.3125, -0.1875,  0.0,
                                                                0.3125,  0.0,
                                                                         1.0])
  assert_in_delta 0.0, jacob[0], 1.0e-15
  assert_in_delta 0.0, jacob[1], 1.0e-15
  assert_in_delta(1.0, jacob[2], 1.0e-15)
  assert_in_delta 0.5, jacob[3], 1.0e-15
  assert_in_delta(-0.5, jacob[4], 1.0e-15)
  assert_in_delta 0.0, jacob[5], 1.0e-15
  assert_in_delta 0.25, jacob[6], 1.0e-15
  assert_in_delta 0.25, jacob[7], 1.0e-15
  assert_in_delta 0.0, jacob[8], 1.0e-15
 end

 def testFindLargestRatioEdge
  assert_not_nil grid = isoTet.resetSpacing
  grid.scaleSpacing(0,0.50)
  grid.scaleSpacing(1,0.95)

  assert_equal 1, grid.largestRatioEdge(0)
  assert_equal 0, grid.largestRatioEdge(1)
  assert_equal 0, grid.largestRatioEdge(2)
  assert_equal 0, grid.largestRatioEdge(3)
 end

 def testFindSmallestRatioEdge
  assert_not_nil grid = isoTet.resetSpacing
  grid.scaleSpacing(0,2.00)
  grid.scaleSpacing(1,1.05)

  assert_equal 1, grid.smallestRatioEdge(0)
  assert_equal 0, grid.smallestRatioEdge(1)
  assert_equal 0, grid.smallestRatioEdge(2)
  assert_equal 0, grid.smallestRatioEdge(3)
 end

# need this?
 def testAverageEdgeLength
  assert_not_nil grid = isoTet
  4.times do |i| 
   assert_in_delta 1.0, grid.averageEdgeLength(i), 3.0e-3, "node #{i} length"
  end
 end

 def testSpacingFunction
  assert_not_nil grid = isoTet
  4.times do |i| 
   assert_in_delta 1.0, grid.spacing(i), 1.0e-15, "node #{i} spacing"
  end
  assert_equal grid, grid.resetSpacing
  4.times do |i| 
   assert_in_delta grid.averageEdgeLength(i), grid.spacing(i), 1.0e-15, "n#{i}"
  end
  assert_equal grid, grid.scaleSpacing(0,0.5)
  assert_in_delta grid.averageEdgeLength(0)*0.5, grid.spacing(0), 1.0e-15
 end

 def testEdgeLengthRatioInMetricForRightTet
  assert_not_nil grid = rightTet
  nodes = [0,1,2,3]
  assert_in_delta 1.0, grid.edgeRatio(0,1), 1.0e-10
  assert_in_delta 1.0, grid.edgeRatio(0,2), 1.0e-10
  assert_in_delta 1.0, grid.edgeRatio(0,3), 1.0e-10
  length = Math::sqrt(2)
  assert_in_delta length, grid.edgeRatio(1,2), 1.0e-10
  assert_in_delta length, grid.edgeRatio(2,3), 1.0e-10
  assert_in_delta length, grid.edgeRatio(3,1), 1.0e-10
 end

 def testVolumeMetrics
  assert_not_nil grid = rightTet
  nodes = [0,1,2,3]
  ar = 0.8399473666
  assert_in_delta 1.0/6.0, grid.volume(nodes), 1.0e-15
  assert_in_delta 1.0/6.0, grid.minVolume, 1.0e-15
  assert_in_delta ar, grid.ar(nodes), 1.0e-10
  assert_in_delta ar, grid.minAR, 1.0e-10
  assert_in_delta ar, grid.nodeAR(0), 1.0e-10
  assert_in_delta ar, grid.nodeAR(1), 1.0e-10
  assert_in_delta ar, grid.nodeAR(2), 1.0e-10
  assert_in_delta ar, grid.nodeAR(3), 1.0e-10
 end

 def testARDerivatives
  assert_not_nil grid = Grid.new(4,1,0,0)

  grid.addCell( 
	       grid.addNode(0.0,0.0,0.0), 
	       grid.addNode(1.0,0.0,0.0), 
	       grid.addNode(0.0,1.0,0.0), 
	       grid.addNode(0.0,0.0,1.0) )
  nodes = [0,1,2,3]
  ar    = 0.8399473666
  tol   = 1.0e-10
  deriv = -0.3733099407

  ans = grid.cellARDerivative(nodes)
  assert_in_delta ar,    ans[0], tol
  assert_in_delta deriv, ans[1], tol
  assert_in_delta deriv, ans[2], tol
  assert_in_delta deriv, ans[3], tol  

  ans = grid.nodeARDerivative(0)
  assert_in_delta ar,    ans[0], tol
  assert_in_delta deriv, ans[1], tol
  assert_in_delta deriv, ans[2], tol
  assert_in_delta deriv, ans[3], tol  

  deriv = 0.1866549704

  ans = grid.nodeARDerivative(1)
  assert_in_delta ar,    ans[0], tol
  assert_in_delta 0.0,   ans[1], tol
  assert_in_delta deriv, ans[2], tol
  assert_in_delta deriv, ans[3], tol  

  ans = grid.nodeARDerivative(2)
  assert_in_delta ar,    ans[0], tol
  assert_in_delta deriv, ans[1], tol
  assert_in_delta 0.0,   ans[2], tol
  assert_in_delta deriv, ans[3], tol  

  ans = grid.nodeARDerivative(3)
  assert_in_delta ar,    ans[0], tol
  assert_in_delta deriv, ans[1], tol
  assert_in_delta deriv, ans[2], tol
  assert_in_delta 0.0,   ans[3], tol  

 end

 def testRightHandedFaces
  assert_not_nil grid = Grid.new(4,1,2,0)

  grid.addCell( grid.addNode(0.0,0.0,0.0), grid.addNode(1.0,0.0,0.0), 
	        grid.addNode(0.0,1.0,0.0), grid.addNode(0.0,0.0,1.0) )
  grid.addFace(0,1,2,11)
  assert_equal true,  grid.rightHandedFace(0)
  grid.addFace(0,2,3,11)
  assert_equal true,  grid.rightHandedBoundary
  assert_equal grid,  grid.removeFace(grid.findFace(0,1,2))
  grid.addFace(0,2,1,11)
  assert_equal false, grid.rightHandedFace(0)
  assert_equal false, grid.rightHandedBoundary
 end

 def testComputeFaceAreaInParameterSpacePositiveHalf
  grid = Grid.new(4,1,1,0)
  grid.addNode(0,0,0)
  grid.addNode(1,0,0)
  grid.addNode(0,1,0)
  grid.addNode(0,0,1)
  grid.addCell(0,1,2,3)
  # FAKEGeom: U = X+10; V = Y+20;
  grid.addFaceUV(0,10.0,20.0,
		 1,11.0,20.0,
		 2,10.0,21.0,
		 55)
  tol = 1.0e-15
  area = 0.5
  assert_in_delta area, grid.faceAreaUV(0), tol
  assert_in_delta area, grid.minFaceAreaUV(0), tol
  assert_in_delta area, grid.minCellFaceAreaUV([0,1,2,3]), tol
 end

 def testComputeFaceAreaInParameterSpaceNegativeHalf
  grid = Grid.new(4,1,1,0)
  grid.addNode(0,0,0)
  grid.addNode(1,0,0)
  grid.addNode(0,1,0)
  grid.addNode(0,0,1)
  grid.addCell(0,1,2,3)
  # FAKEGeom: U = X+10; V = Y+20;
  grid.addFaceUV(0,10.0,20.0,
		 2,10.0,21.0,
		 1,11.0,20.0,
		 55)
  tol = 1.0e-15
  area = -0.5
  assert_in_delta area, grid.faceAreaUV(0), tol
  assert_in_delta area, grid.minFaceAreaUV(0), tol
  assert_in_delta area, grid.minCellFaceAreaUV([0,1,2,3]), tol
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
  assert_in_delta area, grid.minFaceAreaUV(0), tol
 end

 def testFaceMetrics
  assert_not_nil grid = rightTet
  assert_in_delta 0.5, grid.faceArea(0,1,2), 1.0e-15
  assert_in_delta 1.0, grid.faceAR(1,2,3), 1.0e-15
  ar = 0.8284271247
  assert_in_delta ar, grid.faceAR(0,1,2), 1.0e-8
  mr = 0.8660254038
  assert_in_delta 1.0, grid.faceMR(1,2,3), 1.0e-14
  assert_in_delta mr,  grid.faceMR(0,1,2), 1.0e-8

  deriv = -0.433
  ans = grid.faceMRDerivative([0,1,2])
  assert_in_delta mr,    ans[0], 1.0e-8
  assert_in_delta deriv, ans[1], 1.0e-4
  assert_in_delta deriv, ans[2], 1.0e-4
  assert_in_delta 0.0,   ans[3], 1.0e-14

  grid.addFace(0,1,2,10)
  grid.addFace(1,2,3,10)
  assert_in_delta mr,  grid.nodeFaceMR(0), 1.0e-8
  assert_in_delta mr,  grid.nodeFaceMR(1), 1.0e-8
  assert_in_delta 1.0, grid.nodeFaceMR(3), 1.0e-8
  assert_in_delta mr,  grid.minFaceMR, 1.0e-8

  ans = grid.nodeFaceMRDerivative(0)
  assert_in_delta mr,    ans[0], 1.0e-8
  assert_in_delta deriv, ans[1], 1.0e-4
  assert_in_delta deriv, ans[2], 1.0e-4
  assert_in_delta 0.0,   ans[3], 1.0e-15  
  ans = grid.nodeFaceMRDerivative(1)
  assert_in_delta mr,    ans[0], 1.0e-8
  assert_in_delta 0.0, ans[1], 1.0e-4
  assert_in_delta( -deriv, ans[2], 1.0e-4)
  assert_in_delta 0.0,   ans[3], 1.0e-15  
  ans = grid.nodeFaceMRDerivative(3)
  assert_in_delta 1.0,    ans[0], 1.0e-8
  assert_in_delta 0.0, ans[1], 1.0e-4
  assert_in_delta 0.0, ans[2], 1.0e-4
  assert_in_delta 0.0,   ans[3], 1.0e-15  
 end
 
 def testFaceMRDerivative
  x1 =0.061
  y1 =0.045
  z1 =0.077

  x2 =1.561
  y2 =0.145
  z2 =0.277

  x3 =-0.161
  y3 =2.545
  z3 =-0.0177

  assert_not_nil grid = Grid.new(3,0,0,0)
  assert_equal 0, grid.addNode(x1,y1,z1)
  assert_equal 1, grid.addNode(x2,y2,z2)
  assert_equal 2, grid.addNode(x3,y3,z3)

  delta = 1.0e-7
  ans = grid.FaceMRDerivative(x1,y1,z1,x2,y2,z2,x3,y3,z3)

  dx = grid.FaceMRDerivative(x1+delta,y1,z1,x2,y2,z2,x3,y3,z3)
  dx = (dx[0]-ans[0])/delta
  assert_in_delta dx, ans[1], 10.0*delta, "dx"

  dy = grid.FaceMRDerivative(x1,y1+delta,z1,x2,y2,z2,x3,y3,z3)
  dy = (dy[0]-ans[0])/delta
  assert_in_delta dy, ans[2], 10.0*delta, "dy"

  dz = grid.FaceMRDerivative(x1,y1,z1+delta,x2,y2,z2,x3,y3,z3)
  dz = (dz[0]-ans[0])/delta
  assert_in_delta dz, ans[3], 10.0*delta, "dz"

  assert_in_delta grid.faceMR(0,1,2),ans[0],1e-14
 end

 def testCellMR

  nodes = [0,1,2,3]

  grid = rightTet
  node0 = grid.nodeXYZ(0)
  node1 = grid.nodeXYZ(1)
  node2 = grid.nodeXYZ(2)
  node3 = grid.nodeXYZ(3)
  assert_in_delta 0.840, grid.cellMeanRatio(node0,node1,node2,node3), 1.0e-4

  grid = isoTet
  node0 = grid.nodeXYZ(0)
  node1 = grid.nodeXYZ(1)
  node2 = grid.nodeXYZ(2)
  node3 = grid.nodeXYZ(3)
  assert_in_delta 1.0, grid.cellMeanRatio(node0,node1,node2,node3), 1.0e-4

 end

 def testCellMetricConformity_iso
  grid = isoTet
  node0 = grid.nodeXYZ(0)
  node1 = grid.nodeXYZ(1)
  node2 = grid.nodeXYZ(2)
  node3 = grid.nodeXYZ(3)
  m = [1.0, 0.0, 0.0, 1.0, 0.0, 1.0 ]
  assert_in_delta 1.0, 
        grid.cellMetricConformity(node0,node1,node2,node3,m), 4.0e-4
 end

 def testCellMetricConformity_right
  grid = rightTet
  node0 = grid.nodeXYZ(0)
  node1 = grid.nodeXYZ(1)
  node2 = grid.nodeXYZ(2)
  node3 = grid.nodeXYZ(3)
  m = [1.0, 0.5, 0.5, 1.0, 0.5, 1.0 ]
  assert_in_delta 1.0, 
        grid.cellMetricConformity(node0,node1,node2,node3,m), 1.0e-14
 end

 def testCellMRDerivative

  x0=0.1
  y0=0.2
  z0=0.3
  node0 = [x0,y0,z0]
  node1 = [1.12,-0.1,-0.2]
  node2 = [0.01,1.05,0.12]
  node3 = [0.34,0.22,0.99]
  assert_not_nil grid = Grid.new(4,0,0,0)
  grid.addNode(node0[0],node0[1],node0[2])
  grid.addNode(node1[0],node1[1],node1[2])
  grid.addNode(node2[0],node2[1],node2[2])
  grid.addNode(node3[0],node3[1],node3[2])

  delta = 1.0e-7
  ans = grid.cellMeanRatioDerivative(node0,node1,node2,node3)

  dx = grid.cellMeanRatioDerivative([x0+delta,y0,z0],node1,node2,node3)
  dx = (dx[0]-ans[0])/delta
  assert_in_delta dx, ans[1], 10.0*delta, "dx"

  dy = grid.cellMeanRatioDerivative([x0,y0+delta,z0],node1,node2,node3)
  dy = (dy[0]-ans[0])/delta
  assert_in_delta dy, ans[2], 10.0*delta, "dy"

  dz = grid.cellMeanRatioDerivative([x0,y0,z0+delta],node1,node2,node3)
  dz = (dz[0]-ans[0])/delta
  assert_in_delta dz, ans[3], 10.0*delta, "dz"

  assert_in_delta grid.cellMeanRatio(node0,node1,node2,node3), 
   ans[0], 1.0e-15, "function"
 end

 def XtestCostAfterCollapse
  grid = rightTet3
  grid.setCostFunction(2)
  tol = 1.0e-10

  ratio = Math::sqrt(2)
  err = (1.0-ratio)/(1.0+ratio)
  origCost = 1.0/(1.0+err.abs)

  ans = grid.collapseCost(0,1)
  assert_equal origCost, ans[0], tol

  ratio = 2.0
  err = (1.0-ratio)/(1.0+ratio)
  cost = 1.0/(1.0+err.abs)

  assert_equal cost, ans[1], tol
  assert_equal cost, ans[2], tol

  ans = grid.collapseCost(0,1)
  assert_equal origCost, ans[0], tol
 end

end
