/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("chop", "[seg]")
{
  const double sr = 44100;
  const int    n_samp = sr / 2; // 0.5 s
  const int    n_win = 1710;	// descr default
  const int    n_hop = 128;	// descr default
  const int    n_onset = 250. / 1000. * sr; // onset at 250ms

  const double t_win      = n_win   / sr * 1000.; 
  const double t_hop      = n_hop   / sr * 1000.; 
  const double t_samp     = n_samp  / sr * 1000;	// duration in ms
  const double t_onset    = n_onset / sr * 1000;	// true onset time
  const double t_expected[] = { 0, 200, 400 }; // expected chop times
  std::vector<float> vals(n_samp);
  
  // generate test audio with 0.25s silence, then 0.25s noise
  for (unsigned int i = n_onset; i < n_samp; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);

  PiPoTestHost host;
  host.setGraph("descr:chop");
  host.setAttr("chop.size", 200);

  PiPoStreamAttributes sa;
  sa.rate = sr;

  REQUIRE(host.setInputStreamAttributes(sa) == 0);

  WHEN ("no duration")
  {
    host.setAttr("chop.duration", 0);
    host.setAttr("chop.mean", 0);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 0); // descr outputs 9 columns, but expect just marker, no data
      CHECK(sa.dims[1] == 1); // still 1 row(?)

      REQUIRE(host.receivedFrames.size() == 3); // 3 segments of 200, 200, 100 ms
      CHECK(host.last_time == Approx(t_expected[2]).epsilon(0.1));
      CHECK(host.received_times_[0] == Approx(t_expected[0]).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected[1]).epsilon(0.1));
      CHECK(host.received_times_[2] == Approx(t_expected[2]).epsilon(0.1));
    }
  }
  
  WHEN ("just 1 output")
  {
    host.reset(); // clear stored received frames
    host.setAttr("chop.duration", 0);
    host.setAttr("chop.mean", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 9); // descr outputs 9 columns, expect 9 mean column
      CHECK(sa.dims[1] == 1); // expect 1 row

      REQUIRE(host.receivedFrames.size() == 3);
      CHECK(host.receivedFrames[0][4] < -99); // expect dB value for silence in Loudness column
      CHECK(host.receivedFrames[1][4] > -40); // expect dB value for partly sound
      CHECK(host.receivedFrames[2][4] > -10); // expect dB value for full scale noise
    }
  }

  WHEN ("with duration output")
  {
    host.reset(); // clear stored received frames
    host.setAttr("chop.duration", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 10); // expect 9 mean columns + duration
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() == 3);
      CHECK(host.receivedFrames[0][0] == Approx(t_expected[1] - t_expected[0]).epsilon(0.1)); // first column is duration
      CHECK(host.receivedFrames[1][0] == Approx(t_expected[2] - t_expected[1]).epsilon(0.1)); // 
      CHECK(host.receivedFrames[2][0] == Approx(t_samp        - t_expected[2]).epsilon(0.1)); // duration until end
    }
  }

  WHEN ("bad size")
  {
    host.reset(); // clear stored received frames
    host.setAttr("chop.duration", 1);
    host.setAttr("chop.mean", 0);
    host.setAttr("chop.size", -99);
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("graceful handling")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1); // expect duration column
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == 1);
      CHECK(host.received_times_[0] == 0); // Approx(t_win / 2).epsilon(0.01)); // expect first frame as onsete timetagged at middle of window
      CHECK(host.receivedFrames[0][0] == Approx(t_samp).epsilon(0.1)); // graceful handling: just 1 segment
    }
  }

  WHEN ("one input column")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("loudness:chop"));
    host.setAttr("chop.size", 200);
    host.setAttr("chop.duration", 0);
    host.setAttr("chop.mean", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 1); // expect loudness mean column
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == 3); // 3 segments of 200, 200, 100 ms
      CHECK(host.last_time == Approx(t_expected[2]).epsilon(0.1));
      CHECK(host.received_times_[0] == Approx(t_expected[0]).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected[1]).epsilon(0.1));
      CHECK(host.received_times_[2] == Approx(t_expected[2]).epsilon(0.1));
    }
  }

  WHEN ("one input column, size 0")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("loudness:chop"));
    host.setAttr("chop.size", 0);
    host.setAttr("chop.duration", 0);
    host.setAttr("chop.mean", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 1); // expect loudness mean column
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == 1); // expect 1 segment of whole duration 500
      CHECK(host.last_time == 0);
      CHECK(host.received_times_[0] == 0);
      CHECK(host.receivedFrames[0][0] < 0); // expect some dB value
    }
  }

  WHEN ("chain with undefined sync")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("mfcc<chop,thru>")); // undefined: chop not in sync with thru frames from mfcc
    REQUIRE(host.setAttr("chop.size", 100));
    REQUIRE(host.setAttr("chop.duration", 1));

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("host did not crash")
    {
      REQUIRE(host.receivedFrames.size() > 0);
    }
  }
}
