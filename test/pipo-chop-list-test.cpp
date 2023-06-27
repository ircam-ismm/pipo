/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("chop-list", "[seg]")
{
  const std::vector<double> t_expected  = {  0, 100, 200, 300, 400 };   // expected chop times
  const std::vector<double> v_expected  = { 45, 145, 245, 345, 445 };   // expected chop mean values for regular seg.

  const std::vector<double> t_segtimes   = {   0, 200, 300, 400 };	// given and expected chop times
  const std::vector<double> t_durations0 = { 200, 100, 100, 100 };	// given segdurations
  const std::vector<double> t_durations1 = {  -1, 100,   0, -99 };      // given neg. segdurations
  const std::vector<double> t_durations2 = {  20,  50,  -1, -99 };      // given shorter segdurations

  const std::vector<double> v_expected0  = {  95, 245, 345, 445 };	// expected chop mean values for dur.0/1
  const std::vector<double> v_expected1  = {  95, 245, 345, 445 };   // expected chop mean values for dur.0/1
  const std::vector<double> v_expected2  = {   5, 225, 345, 445 };   // expected chop mean values for dur.2


  int n_samp = 50;
  int t_samp = 500;	// time of last input sample
  std::vector<float> vals(n_samp);
  
  // generate test input: ramp by 10 every 10ms 
  for (unsigned int i = 0; i < n_samp; ++i)
    vals[i] = i * 10;

  PiPoTestHost host;
  host.setGraph("chop");
  host.setAttr("chop.size", 100);
  host.setAttr("chop.duration", 0);
  host.setAttr("chop.mean", 1);

  PiPoStreamAttributes sa;
  sa.rate = 100;


  WHEN ("regular")
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
  
  WHEN ("with regular segdurations")
  {
    host.reset(); // clear stored received frames
    host.setAttr("chop.segtimes",     t_segtimes);
    host.setAttr("chop.segdurations", t_durations0);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("chop.segtimes");
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
    host.setAttr("chop.segtimes",     t_segtimes);
    host.setAttr("chop.segdurations", t_durations1);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("chop.segtimes");
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
    host.setAttr("chop.segtimes",     t_segtimes);
    host.setAttr("chop.segdurations", t_durations2);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    PiPo::Attr *segt = host.getAttr("chop.segtimes");
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
}
