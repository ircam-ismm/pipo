/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("segment-list", "[seg]")
{
  const std::vector<double> t_expected  = {  0, 100, 200, 300, 400 };   // expected chop times
  const std::vector<double> v_expected  = { 45, 145, 245, 345, 445 };   // expected chop mean values for regular seg.

  const std::vector<double> t_segtimes   = {   0, 200, 300, 400 };	// given and expected chop times
  const std::vector<double> t_durations0 = { 200, 100, 100, 100 };	// given segdurations
  const std::vector<double> t_durations1 = {  -1, 100,   0, -99 };      // given neg. segdurations
  const std::vector<double> t_durations2 = {  20,  50,  -1, -99 };      // given shorter segdurations

  const std::vector<double> v_expected0  = {  95, 245, 345, 445 };	// expected chop mean values for dur.0
  const std::vector<double> v_expected1  = {  95, 245, 345, 445 };      // expected chop mean values for dur.1
  const std::vector<double> v_expected2  = {   5, 220, 345, 445 };      // expected chop mean values for dur.2

  const std::vector<double> t_segtimes3  = { 100, 300 };	// given and expected chop times not starting at 0
  const std::vector<double> t_durations3 = { 100, 100 };	// given segdurations
  const std::vector<double> v_expected3  = { 145, 345 };	// expected chop mean values

  const std::vector<double> t_segtimes4  = { -200, -50, 400 };	// given chop times starting before 0
  const std::vector<double> t_durations4 = {  100, 100, 200 };	// given segdurations
  const std::vector<double> t_expected4  = {         0, 400 };	// expected chop times: 1st seg is dropped, 2nd clipped
  const std::vector<double> v_expected4  = {        45, 445 };	// expected chop mean values

  //TODO: test segments shorter than 1 frame (no data), after end

  int n_samp = 50;
  int t_samp = 500;	// time of last input sample
  std::vector<float> vals(n_samp);
  
  // generate test input: ramp by 10 every 10ms 
  for (unsigned int i = 0; i < n_samp; ++i)
    vals[i] = i * 10;

  PiPoTestHost host;
  host.setGraph("segment:segmean");

  PiPoStreamAttributes sa;
  sa.rate = 100;

/*
  WHEN ("regular chop size")
  {
    REQUIRE(host.setInputStreamAttributes(sa) == 0);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == t_expected.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_expected[i]);
	CHECK(host.receivedFrames[i][0] == v_expected[i]);
      }
    }
  }
  */
  WHEN ("with regular segdurations")
  {
    host.reset(); // clear stored received frames
    host.setAttr("segment.segtimes",     t_segtimes);
    host.setAttr("segment.segdurations", t_durations0);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("segment.segtimes");
    REQUIRE(segt != NULL);
    CHECK(segt->getSize() == t_segtimes.size());

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      REQUIRE(host.receivedFrames.size() == t_segtimes.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_segtimes[i]);
	CHECK(host.receivedFrames[i][0] == v_expected0[i]);
      }
    }
  }

  WHEN ("with negative segdurations")
  {
    host.reset(); // clear stored received frames
    host.setAttr("segment.segtimes",     t_segtimes);
    host.setAttr("segment.segdurations", t_durations1);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("segment.segtimes");
    REQUIRE(segt != NULL);
    CHECK(segt->getSize() == t_segtimes.size());

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      REQUIRE(host.receivedFrames.size() == t_segtimes.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_segtimes[i]);
	CHECK(host.receivedFrames[i][0] == v_expected1[i]);
      }
    }
  }

  WHEN ("with shorter segdurations")
  {
    host.reset(); // clear stored received frames
    host.setAttr("segment.segtimes",     t_segtimes);
    host.setAttr("segment.segdurations", t_durations2);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("segment.segtimes");
    REQUIRE(segt != NULL);
    CHECK(segt->getSize() == t_segtimes.size());

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      REQUIRE(host.receivedFrames.size() == t_segtimes.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_segtimes[i]);
	CHECK(host.receivedFrames[i][0] == v_expected2[i]);
      }
    }
  }

  WHEN ("with seglist starting late")
  {
    host.reset(); // clear stored received frames
    host.setAttr("segment.segtimes",     t_segtimes3);
    host.setAttr("segment.segdurations", t_durations3);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("segment.segtimes");
    REQUIRE(segt != NULL);
    CHECK(segt->getSize() == t_segtimes3.size());

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      REQUIRE(host.receivedFrames.size() == t_segtimes3.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_segtimes3[i]);
	CHECK(host.receivedFrames[i][0] == v_expected3[i]);
      }
    }
  }

  WHEN ("with seglist larger than data (starting before zero)")
  {
    host.reset(); // clear stored received frames
    host.setAttr("segment.segtimes",     t_segtimes4);
    host.setAttr("segment.segdurations", t_durations4);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("segment.segtimes");
    REQUIRE(segt != NULL);
    CHECK(segt->getSize() == t_segtimes4.size());

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      REQUIRE(host.receivedFrames.size() == t_expected4.size());
      for (unsigned int i = 0; i < host.receivedFrames.size(); ++i)
      {
	CHECK(host.received_times_[i]   == t_expected4[i]);
	CHECK(host.receivedFrames[i][0] == v_expected4[i]);
      }
    }
  }
}
