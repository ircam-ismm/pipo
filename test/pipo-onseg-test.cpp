/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("onseg")
{
  PiPoTestHost host;
  host.setGraph("descr:onseg");
  host.setAttr("onseg.columns", "Loudness");
  host.setAttr("onseg.duration", 0);

  const double sr = 44100;
  const int    winsize = 1710;	// descr default
  const int    hopsize = 128;	// descr default
  const int    numsamples = sr / 2;
  const int    onset = numsamples / 2;
  const double onsettime = onset / sr * 1000;	// true onset
  const double onsetexpected = onsettime - (winsize / 2 + hopsize) / sr * 1000.;	// expected reported onset (- framesize etc.)
  const double endtime   = numsamples / sr * 1000;
  std::vector<float> vals(numsamples);
  
  // generate test audio with 0.25s silence, then 0.25s noise
  for (unsigned int i = onset; i < numsamples; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);

  PiPoStreamAttributes sa;
  sa.rate = sr;

  REQUIRE(host.setInputStreamAttributes(sa) == 0);
  
  WHEN ("no duration")
  {
    REQUIRE(host.frames(0, 1, &vals[0], 1, numsamples) == 0);
    REQUIRE(host.finalize(endtime) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / hopsize);  // output frame rate of descr
      CHECK(sa.dims[0] == 0);
      CHECK(sa.dims[1] == 0); // expect just marker, no data

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.last_time == Approx(onsetexpected).epsilon(0.1));
    }
  }
  
  WHEN ("with duration output")
  {
    host.reset(); // clear stored received frames
    host.setAttr("onseg.duration", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, numsamples) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / hopsize);  // output frame rate of descr
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.last_time == Approx(onsetexpected).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(onsettime)); // duration until end
    }
  }
}
