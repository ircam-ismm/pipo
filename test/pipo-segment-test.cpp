/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("segment", "[seg]")
{
  // test audio (off-on) params
  const double sr = 44100;
  const int    n_samp = sr / 2; // 0.5 s
  const int    n_win = 1710;	// descr default
  const int    n_hop = 128;	// descr default
  const int    n_onset = 200. / 1000. * sr; // onset at 200ms

  const double t_win      = n_win   / sr * 1000.; 
  const double t_hop      = n_hop   / sr * 1000.; 
  const double t_samp     = n_samp  / sr * 1000.;	// duration in ms
  const double t_onset    = n_onset / sr * 1000.;	// true sample onset time
  const double t_expected = t_onset - (t_win / 2 + t_hop);	// expected reported onset (- framesize etc.)
  std::vector<float> vals(n_samp);
  
  // generate test audio with 0.25s silence, then 0.25s noise
  for (unsigned int i = n_onset; i < n_samp; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);

  PiPoTestHost host;
  PiPoStreamAttributes sa;
  sa.rate = sr;

  REQUIRE(host.setInputStreamAttributes(sa) == 0);
  
  WHEN ("markers only - no duration")
  {
    REQUIRE(host.setGraph("descr:segment:segmarker"));
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);
    
    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 0);
      CHECK(sa.dims[1] == 0); // expect just marker, no data

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.received_times_[0] == Approx(t_expected).epsilon(0.1));
    }
  }
  
  WHEN ("with duration output")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("descr:segment:segduration"));
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    host.setAttr("segment.duration", 1);
    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.received_times_[0] == Approx(t_expected).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(t_samp - t_expected - t_hop).epsilon(0.1)); // duration until end
    }
  }

  WHEN ("with multiple outputs")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("descr:segment<segduration,segmean,segstddev,segmeanstd>"));
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1 + 4 * 9);
      CHECK(sa.dims[1] == 1); 

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.received_times_[0] == Approx(t_expected).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(t_samp - t_expected).epsilon(0.1)); // duration until end

      CAPTURE(host.receivedFrames.size());
      for (int j = 0; j < host.receivedFrames.size(); j++)
      {
	CAPTURE(host.receivedFrames[j].size());
	for (int i = 0; i < host.receivedFrames[j].size(); i++)
	  INFO(j << i << host.receivedFrames[j][i]);
      }
    }
  }
  
  WHEN ("with startisonset 1")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("descr:segment:segduration")); 
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setAttr("segment.startisonset", 1));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() > 0);
      CHECK(host.received_times_[0] == Approx(t_win / 2 - t_hop).epsilon(0.1)); // expect first frame as onsete timetagged at middle of window
      CHECK(host.receivedFrames[0][0] == Approx(t_expected - t_hop).epsilon(0.1)); // duration until first true segment
    }
  }
}
