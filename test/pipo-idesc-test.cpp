/* -*- mode: c; c-basic-offset:2 -*- */

// test needs ircamdescriptor, so we only include it within mubu's maxpipo project

#if 0 //def MAXAPI_DIR  // disabled for now, needs too many libs (see iaeom binding)

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"
#include "PiPoCollection.h"
#include "PiPoIdesc.h"

TEST_CASE ("idesc")
{
  // initialise pipo factory and add idesc to collection
  PiPoCollection::init();
  PiPoCollection::addToCollection("ircamdescriptor", new PiPoCreator<PiPoIdesc>);

  const double sr = 44100;
  const int    n_samp = sr / 2; // 0.5 s
  const double t_samp = n_samp  / sr * 1000;	// duration in ms
  std::vector<float> vals(n_samp);
  
  // generate test audio: noise
  for (unsigned int i = 0; i < n_samp; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);

  PiPoTestHost host;
  REQUIRE(host.setGraph("ircamdescriptor(d)"));
  
  PiPoStreamAttributes sa;
  sa.rate = sr;

  WHEN ("Set zcr en")
  {
    REQUIRE(host.setAttr("d.descriptors", "SignalZeroCrossingRate TotalEnergy"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.dims[0] == 2);
      CHECK(sa.dims[1] == 1);

      // check typical values for zerocrossing rate and energy for noise input
      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.receivedFrames[0][0] > 8000);
      CHECK(host.receivedFrames[0][1] < 0.5);
    }
  }
}

#else
// no error, disabled for now
//#error want maxpipo
#endif
