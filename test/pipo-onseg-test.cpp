/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("onseg", "[seg]")
{
  const double sr = 44100;
  const int    n_samp = sr / 2; // 0.5 s
  const int    n_win = 1710;	// descr default
  const int    n_hop = 128;	// descr default
  const int    n_onset = 200. / 1000. * sr; // onset at 200ms

  const double t_win      = n_win   / sr * 1000.; 
  const double t_hop      = n_hop   / sr * 1000.; 
  const double t_samp     = n_samp  / sr * 1000;	// duration in ms
  const double t_onset    = n_onset / sr * 1000;	// true onset time
  const double t_expected = t_onset - (t_win / 2 + t_hop);	// expected reported onset (- framesize etc.)
  std::vector<float> vals(n_samp);
  
  // generate test audio with 0.25s silence, then 0.25s noise
  for (unsigned int i = n_onset; i < n_samp; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);

  PiPoTestHost host;
  host.setGraph("descr:onseg");
  host.setAttr("onseg.columns", "Loudness");
  host.setAttr("onseg.duration", 0);

  PiPoStreamAttributes sa;
  sa.rate = sr;

  REQUIRE(host.setInputStreamAttributes(sa) == 0);
  
  WHEN ("no duration")
  {
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 0);
      CHECK(sa.dims[1] == 0); // expect just marker, no data

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.last_time == Approx(t_expected).epsilon(0.1));
    }
  }
  
  WHEN ("with duration output")
  {
    host.reset(); // clear stored received frames
    host.setAttr("onseg.duration", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.last_time == Approx(t_expected).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(t_samp - t_expected - t_hop).epsilon(0.5)); // duration until end
    }
  }
  
  WHEN ("bad column index/number")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("loudness:onseg"));
    host.setAttr("onseg.colindex", 99);
    host.setAttr("onseg.numcols", 1);
    host.setAttr("onseg.mean", 1);

    THEN ("error is caught")
    {
      // force streamAttributes call, should fail
      int ret = host.setInputStreamAttributes(sa);
      CHECK(ret != 0);
      // but we try to push data anyway, to catch follow-up crashes
      REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
      REQUIRE(host.finalize(t_samp) == 0);
      
      THEN ("output doesn't crash")
      {
	PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
	CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
	CHECK(sa.dims[0] == 0);
	CHECK(sa.dims[1] == 0); // expect no data
	
	REQUIRE(host.receivedFrames.size() == 0);
      }
    }
  }

  WHEN ("chain with undefined sync")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("mfcc<onseg,thru>")); // undefined: onseg not in sync with thru frames from mfcc
    REQUIRE(host.setAttr("onseg.columns", 0));
    REQUIRE(host.setAttr("onseg.duration", 1));

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("host did not crash")
    {
      REQUIRE(host.receivedFrames.size() > 0);
    }
  }
}
