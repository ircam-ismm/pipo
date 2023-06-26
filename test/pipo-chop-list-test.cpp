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
  const int    n_expected = 5;
  const double t_expected[]  = { 0, 100, 200, 300, 400 }; // expected chop times
  const double v_expected1[] = { 45, 145, 245, 345, 445 }; // expected chop mean values
  const double v_expected2[] = {  5, 145, 225, 345, 445 }; // expected chop mean values

  const double t_durations0[] = { 100, 100, 100, 100, 100 };   // given segdurations
  const double t_durations1[] = { -1, 100, 0, 100, -1 };   // given segdurations
  const double t_durations2[] = { 20, -1, 50, -99, 1000 };   // given segdurations

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

  REQUIRE(host.setInputStreamAttributes(sa) == 0);

  WHEN ("regular")
  {
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == n_expected);
      for (unsigned int i = 0; i < n_expected; ++i)
      {
	CHECK(host.received_times_[i]   == t_expected[i]);
	CHECK(host.receivedFrames[i][0] == v_expected1[i]);
      }
    }
  }
  
  WHEN ("with regular segdurations")
  {
    host.reset(); // clear stored received frames
    host.setAttr("chop.segtimes",     t_expected);
    host.setAttr("chop.segdurations", t_durations0);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == n_expected);
      for (unsigned int i = 0; i < n_expected; ++i)
      {
	CHECK(host.received_times_[i]   == t_expected[i]);
	CHECK(host.receivedFrames[i][0] == v_expected1[i]);
      }
    }
  }
}
