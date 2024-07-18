/* -*- mode: c; c-basic-offset:2 -*- */

#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestHost.h"

TEST_CASE ("segment2", "[seg]")
{
  // define test audio input: 2 x silence / noise segments:
  //     ________________                        _______________
  // ____|              |________________________|             |_________________________
  // :   :              :                        :             :                         :
  // :   t_onset1       t_onset1 + t_duration1   t_onset2      t_onset2 + t_duration2    t_samp
  // 0   100            300                      500           900                       1000
  
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
  
  // generate test audio with 2 x silence / noise:
  for (unsigned int i = n_onset1; i < n_offset1; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX) - 0.5;
  for (unsigned int i = n_onset2; i < n_offset2; ++i)
    vals[i] = std::rand() / static_cast<float>(RAND_MAX) - 0.5;

  PiPoTestHost host;
  host.setGraph("descr:segment:segmarker");
  host.setAttr("segment.columns", "Loudness");

  PiPoStreamAttributes sa;
  sa.rate = sr;

  REQUIRE(host.setInputStreamAttributes(sa) == 0);
#if 0
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

      REQUIRE(host.receivedFrames.size() == 5);
      CHECK(host.received_times_[0] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset1 + t_duration1)).epsilon(0.5));
      CHECK(host.received_times_[2] == Approx(t_expected(t_onset2)).epsilon(0.1));
      CHECK(host.received_times_[3] == Approx(t_expected(t_onset2 + t_duration2)).epsilon(0.1));
      CHECK(host.received_times_[4] == Approx(t_expected(t_samp)).epsilon(0.1)); //???
    }
  }
  
  WHEN ("with duration output")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("descr:segment:segduration"));
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 1);
      CHECK(sa.dims[1] == 1); // expect duration column

      REQUIRE(host.receivedFrames.size() == 3);
      CHECK(host.received_times_[0] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset2)).epsilon(0.1));
      CHECK(host.received_times_[2] == Approx(t_expected(t_samp)).epsilon(0.1)); //???
      CHECK(host.receivedFrames[0][0] == Approx(t_duration1 + t_win).epsilon(0.01)); // duration is enlarged by ~~ 1 window
      CHECK(host.receivedFrames[1][0] == Approx(t_duration2 + t_win).epsilon(0.01)); 
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

      REQUIRE(host.receivedFrames.size() >= 4); //????? == 6);
      CHECK(host.received_times_[0] == Approx(t_win / 2 - t_hop).epsilon(0.1)); // expect first frame as onset timetagged at middle of window
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[2] == Approx(t_expected(t_onset2)).epsilon(0.1));
      CHECK(host.received_times_[3] == Approx(t_expected(t_samp)).epsilon(0.1)); //??? why is end time reported?
      CHECK(host.receivedFrames[0][0] == Approx(t_expected(t_onset1) - t_win / 2).epsilon(0.1));
      CHECK(host.receivedFrames[1][0] == Approx(t_duration1 + t_win).epsilon(0.01)); // duration is enlarged by ~~ 1 window
      CHECK(host.receivedFrames[2][0] == Approx(t_duration2 + t_win).epsilon(0.01)); 
      CHECK(host.receivedFrames[3][0] == Approx(t_samp - host.received_times_[3])); //??? what's this duration?
    }
  }

  WHEN ("check loudness values")
  {
    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("descr:segment:<segduration,segmean>"));
    REQUIRE(host.setAttr("segment.startisonset", 1));
    REQUIRE(host.setAttr("segment.columns", "Loudness"));
    REQUIRE(host.setAttr("segmean.columns", "Loudness")); // we just want loudness in dB
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 2); // expect duration and loudness column
      CHECK(sa.dims[1] == 1); 

      REQUIRE(host.receivedFrames.size() == 3); // why not 4 as above???
      CHECK(host.received_times_[0] == Approx(t_win / 2 - t_hop).epsilon(0.1)); // expect first frame as onset timetagged at middle of window
      CHECK(host.received_times_[1] == Approx(t_expected(t_onset1)).epsilon(0.1));
      CHECK(host.received_times_[2] == Approx(t_expected(t_onset2)).epsilon(0.1));
      CHECK(host.receivedFrames[0][0] == Approx(t_expected(t_onset1) - t_win / 2).epsilon(0.1));
      CHECK(host.receivedFrames[1][0] == Approx(t_duration1 + t_win).epsilon(0.01)); // duration is enlarged by ~~ 1 window
      CHECK(host.receivedFrames[2][0] == Approx(t_duration2 + t_win).epsilon(0.01)); 

      CHECK(host.receivedFrames[0][1] < -99); // silence
      CHECK(host.receivedFrames[1][1] > -6);  // noise
      CHECK(host.receivedFrames[2][1] > -6);
    }
  }
#endif
  WHEN ("use parallel segmentation")
  {
    const int    n_winseg = 256;
    const int    n_hopseg = 64;	
    const double t_winseg = n_winseg / sr * 1000.; 
    const double t_hopseg = n_hopseg / sr * 1000.; 
#   define t_expected_seg(t) ((t) - (t_winseg / 2 + t_hopseg))	// expected reported onset (- framesize etc.)

    host.reset(); // clear stored received frames
    REQUIRE(host.setGraph("<descr,loudness:segment><segduration,segmean>"));
    REQUIRE(host.setAttr("segment.startisonset", 1));
    REQUIRE(host.setAttr("segment.outputmode",   0));
    REQUIRE(host.setAttr("loudness.hopsize",    n_hopseg));	// more precise
    REQUIRE(host.setAttr("loudness.winsize",    n_winseg));	// more precise
    REQUIRE(host.setAttr("segmean.columns", "Loudness"));       // we just want loudness from descr. in dB
    REQUIRE(host.setInputStreamAttributes(sa) == 0);

    REQUIRE(host.frames(0, 1, &vals[0], 1, n_samp) == 0);
    REQUIRE(host.finalize(t_samp) == 0);

    THEN ("result is ok")
    {
      PiPoStreamAttributes &sa = host.getOutputStreamAttributes();
      CHECK(sa.rate == sr / n_hop);  // output frame rate of descr
      CHECK(sa.dims[0] == 2); // duration and loudness
      CHECK(sa.dims[1] == 1);

      REQUIRE(host.receivedFrames.size() == 3);
      CHECK(host.received_times_[0] == Approx(t_winseg / 2 - t_hopseg).epsilon(0.2)); // expect first frame as onset timetagged at middle of window
      CHECK(host.received_times_[1] == Approx(t_expected_seg(t_onset1)).epsilon(0.01));
      CHECK(host.received_times_[2] == Approx(t_expected_seg(t_onset2)).epsilon(0.01));

      CHECK(host.receivedFrames[0][0] == Approx(t_expected_seg(t_onset1) - t_hopseg).epsilon(0.01));
      CHECK(host.receivedFrames[1][0] == Approx(t_duration1 + t_winseg).epsilon(0.01)); // duration is enlarged by ~~ 1 window
      CHECK(host.receivedFrames[2][0] == Approx(t_duration2 + t_winseg).epsilon(0.01)); 

      CHECK(host.receivedFrames[0][1] < -99); // silence
      CHECK(host.receivedFrames[1][1] > -6); CHECK(host.receivedFrames[1][1] !=  0);  // noise
      CHECK(host.receivedFrames[2][1] > -6); CHECK(host.receivedFrames[2][1] !=  0);  // noise
    }
  }
}
