#!/usr/bin/env ruby
#
# Mobility test for intersect c lib
#


Dir.chdir ENV['srcdir'] if ENV['srcdir']
require 'RubyExtensionBuilder'
RubyExtensionBuilder.new('Intersect').build

require 'test/unit'
require 'GridMath/GridMath'
require 'Intersect/Intersect'

class TestIntersect < Test::Unit::TestCase

 def set_up
  @intersect = Intersect.new
 end
 def setup ; set_up ; end

 def test_center_cross_segement_segment_intersection
  s0_n0 = [0,0,0]
  s0_n1 = [1,1,0]
  s1_n0 = [0,1,0]
  s1_n1 = [1,0,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta( 0.5, ans[0], tol)
  assert_in_delta( 0.5, ans[1], tol)
  assert_equal true, ans[2]
 end

 def test_cross_off_center_segement_segment_intersection
  s0_n0 = [0,0,0]
  s0_n1 = [1,1,0]
  s1_n0 = [0.25,0,1]
  s1_n1 = [0.25,1,1]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta( 0.25, ans[0], tol)
  assert_in_delta( 0.25, ans[1], tol)
  assert_equal true, ans[2]
 end

 def test_missed_plus_plus_segement_segment_intersection
  s0_n0 = [2,0,0]
  s0_n1 = [1,0,0]
  s1_n0 = [0,3,0]
  s1_n1 = [0,2,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta(  2.0, ans[0], tol)
  assert_in_delta(  3.0, ans[1], tol)
  assert_equal false, ans[2]
 end

 def test_missed_plus_minus_segement_segment_intersection
  s0_n0 = [1.0,0,0]
  s0_n1 = [0.5,0,0]
  s1_n0 = [0,2,0]
  s1_n1 = [0,3,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta(  2.0, ans[0], tol)
  assert_in_delta( -2.0, ans[1], tol)
  assert_equal false, ans[2]
 end

 def test_missed_minus_plus_segement_segment_intersection
  s0_n0 = [2.0,0,0]
  s0_n1 = [3.0,0,0]
  s1_n0 = [0,-20,0]
  s1_n1 = [0,-10,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta( -2.0, ans[0], tol)
  assert_in_delta(  2.0, ans[1], tol)
  assert_equal false, ans[2]
 end

 def test_missed_minus_minus_segement_segment_intersection
  s0_n0 = [10.0,0,0]
  s0_n1 = [12.0,0,0]
  s1_n0 = [0,1,0]
  s1_n1 = [0,2,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta( -5.0, ans[0], tol)
  assert_in_delta( -1.0, ans[1], tol)
  assert_equal false, ans[2]
 end

 def test_colinear_segement_segment_intersection
  s0_n0 = [0,0,0]
  s0_n1 = [1,0,0]
  s1_n0 = [-1,0,0]
  s1_n1 = [ 1,0,0]
  ans = @intersect.segmentSegment(s0_n0,s0_n1,s1_n0,s1_n1)
  tol = 1.0e-14
  assert_in_delta( 0.0, ans[0], tol)
  assert_in_delta( 0.5, ans[1], tol)
  assert_equal false, ans[2]
 end

 def testNodeAboveTrianglePlane
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  nn = [0,0,-1]
  n0 = [0,0,0]
  np = [0,0,1]
  assert_equal false, @intersect.above(v0,v1,v2,nn)
  assert_equal false, @intersect.above(v0,v1,v2,n0)
  assert_equal true,  @intersect.above(v0,v1,v2,np)
 end

 def testTriangleAndNodeInAndOut
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  assert_equal true, @intersect.triangleNode(v0,v1,v2,[0.3,0.3,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[0,-0.5,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[1,1,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[-0.5,0,0])
 end

 def testTriangleAndNodeNick
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[0,0,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[1,0,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[0,1,0])
  assert_equal false, @intersect.triangleNode(v0,v1,v2,[0.5,0.5,0])
 end

 def testTriangleAndSegmentThroughMiddle
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  n0 = [0.3,0.3,1]
  n1 = [0.3,0.3,1]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0.3,0.3,-1]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0.3,0.3,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n1 = [0.3,0.3,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0.3,0.3,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTriangleAndSegmentThroughMiddleSlant
  v0 = [1,0,0]
  v1 = [0,1,0]
  v2 = [0,0,1]
  n0 = [2,2,2]
  n1 = [1,1,1]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [1,1,1]
  n1 = [0,0,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTriangleAndSegmentThroughNick
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  n0 = [0.5,0.5,10]
  n1 = [0.5,0.5,-1]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0,0,18]
  n1 = [0,0,-3]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTriangleAndSegmentThroughMiss
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  n0 = [1,1,10]
  n1 = [1,1,-1]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0,-1,18]
  n1 = [0,-1,-3]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTriangleAndSegmentAdjecent
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  n0 = [0,0,0]
  n1 = [1,0,0]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [1,0,0]
  n1 = [0,1,0]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [0,1,0]
  n1 = [0,0,0]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTriangleAndSegmentCoplanarPerce
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  n0 = [0.3,0.3,0]
  n1 = [1,1,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [1,1,0]
  n1 = [0.3,0.3,0]
  assert_equal true, @intersect.triangleSegment(v0,v1,v2,n0,n1)
  n0 = [1,1,0]
  n1 = [2,2,0]
  assert_equal false, @intersect.triangleSegment(v0,v1,v2,n0,n1)
 end

 def testTetAndNode
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  v3 = [0,0,1]
  n = [0.1,0.1,0.1]
  assert_equal true, @intersect.insideTet(v0,v1,v2,v3,n)
  n = [1,1,1]
  assert_equal false, @intersect.insideTet(v0,v1,v2,v3,n)
  n = [-1,0.3,0.3]
  assert_equal false, @intersect.insideTet(v0,v1,v2,v3,n)
  n = [0.3,-1,0.3]
  assert_equal false, @intersect.insideTet(v0,v1,v2,v3,n)
  n = [0.3,0.3,-1]
  assert_equal false, @intersect.insideTet(v0,v1,v2,v3,n)
 end

 def testTetAndSegmentNodeInside
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  v3 = [0,0,1]
  n0 = [0.1,0.1,0.1]
  n1 = [0.2,0.2,0.2]
  assert_equal true, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
  n0 = [1,1,1]
  assert_equal true, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
  n0 = [0.1,0.1,0.1]
  n1 = [2,2,2]
  assert_equal true, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
  n0 = [1,1,1]
  n1 = [2,2,2]
  assert_equal false, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
 end

 def testTetAndSegmentPerce
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  v3 = [0,0,1]
  n0 = [1,1,1]
  n1 = [2,2,2]
  assert_equal false, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
  n0 = [-1,-1,-1]
  n1 = [2,2,2]
  assert_equal true, @intersect.tetSegment(v0,v1,v2,v3,n0,n1)
 end

 def testTetInsideTet
  v0 = [0,0,0]
  v1 = [1,0,0]
  v2 = [0,1,0]
  v3 = [0,0,1]
  n0 = [0.1,0.1,0.1]
  n1 = [0.9,0.1,0.1]
  n2 = [0.1,0.9,0.1]
  n3 = [0.1,0.1,0.9]
  assert_equal true, @intersect.tetTet(v0,v1,v2,v3,n0,n1,n2,n3)
  assert_equal true, @intersect.tetTet(n0,n1,n2,n3,v0,v1,v2,v3)
 end

 def testTetSliceTet
  v0 = [1,1,1]
  v1 = [4,1,1]
  v2 = [1,4,1]
  v3 = [1,1,4]
  n0 = [2,2,0]
  n1 = [3,2,0]
  n2 = [2,3,0]
  n3 = [2,2,5]
  assert_equal true, @intersect.tetTet(v0,v1,v2,v3,n0,n1,n2,n3)
  assert_equal true, @intersect.tetTet(n0,n1,n2,n3,v0,v1,v2,v3)
 end

end
