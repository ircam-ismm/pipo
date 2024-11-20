/* -*- mode: c++; c-basic-offset:2 -*- */
#include "catch.hpp"
#include <stdio.h>
#include "PiPoTestHost.h"

TEST_CASE ("fluidsynth")
{
  PiPoTestHost host;
  PiPoStreamAttributes sa;

  SECTION ("check output attrs")
  {
    sa.hasTimeTags = true;
    sa.dims[0] = 4; // 4 column of note event data
    sa.dims[1] = 1;
    REQUIRE(host.setGraph("fluidsynth"));
    REQUIRE(host.setInputStreamAttributes(sa) == 0);
    REQUIRE(host.setAttr("fluidsynth.soundfont", "/Users/schwarz/src/mubu-git/maxmubu/patches/test/GM.sf2"));
    
    PiPoStreamAttributes &out_sa = host.getOutputStreamAttributes();
    CHECK(out_sa.dims[0] == 1);	// PiPoStreamAttributes is initialised with 1 x 1 dims input
    CHECK(out_sa.dims[1] == 1);
    CHECK(out_sa.rate == 44100);

    WHEN ("send data")
    {
      // event columns: pitch, duration, [velocity, [channel]],
      float vals[] = { 60, 100, 64, 1 };

      REQUIRE(host.frames(100, 1, &vals[0], 4, 1) == 0);      
      REQUIRE(host.finalize(300) == 0);

      THEN ("output is correct")
      {
	CHECK(host.receivedFrames.size() >= ceil(299. / 1000. * 44100.) );
	REQUIRE(host.receivedFrames[0].size() == 1);

	// now write audio for checking
	FILE *f = fopen("fluidout.raw", "wb");
	for (int i = 0; i < host.receivedFrames.size(); i++)
	  fwrite(host.receivedFrames[i].data(), sizeof(float), 1, f);
	fclose(f);
      }
    }
  }
}
