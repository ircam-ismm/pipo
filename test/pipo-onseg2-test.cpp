/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("onseg2", "[seg]")
{
# define t_expected(t) ((t) - (t_win / 2 + t_hop))	// expected reported onset (- framesize etc.)
  const double t_onset1    = 100;
  const double t_duration1 = 200; // 1st segment end at 300 ms
  const double t_onset2    = 500;
  const double t_duration2 = 400; // 2nd segment end at 900 ms
  
  const double sr = 44100;
  const int    n_samp = sr;  // 1 s
  const int    n_win = 1710;	// descr default
  const int    n_hop = 128;	// descr default
  const int    n_onset1  =  t_onset1 / 1000. * sr;
  const int    n_offset1 = (t_onset1 + t_duration1) / 1000. * sr;
  const int    n_onset2  =  t_onset2 / 1000. * sr;
  const int    n_offset2 = (t_onset2 + t_duration2) / 1000. * sr;  

  const double t_win      = n_win   / sr * 1000.; 
  const double t_hop      = n_hop   / sr * 1000.; 
  const double t_samp     = n_samp  / sr * 1000;	// duration in ms

  std::vector<float> vals(n_samp); // init with zeros
  
  // generate test audio with 2 x silence / noise
  for (unsigned int i = n_onset1; i < n_offset1; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);
  for (unsigned int i = n_onset2; i < n_offset2; ++i)
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

      REQUIRE(host.receivedFrames.size() == 2);
      CHECK(host.received_times_[0] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset2)).epsilon(0.1));
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

      REQUIRE(host.receivedFrames.size() == 2);
      CHECK(host.received_times_[0] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset2)).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(t_duration1 + t_win).epsilon(0.01)); // duration is enlarged by ~~ 1 window
      CHECK(host.receivedFrames[1][0] == Approx(t_duration2 + t_win).epsilon(0.01)); 
    }
  }
}
