// -*- mode: c++; c-basic-offset:2 -*-

#include <sstream>
#include <string>
#include <cstddef>

#include <vector>
#include <algorithm>

#include "catch.hpp"

#include "PiPoResample.h"
#include "PiPoTestHost.h"


TEST_CASE ("resample")
{
  PiPoTestHost host;
  host.setGraph("resample");

  int check;
  double insamplerate   = 1000.;
  double insampleperiod = 1000. / insampleperiod;
  std::vector<PiPoValue> inputframe;
  PiPoStreamAttributes sa;
  int numframes = 10;
  sa.maxFrames = 20;
  
  for (int timetaggedinput = 0; timetaggedinput <= 1; timetaggedinput += 1)
  {
    for (double factor = 0.125; factor <= 8.; factor *= 2.)
    {
      for (unsigned int width = 0; width <= 2; width++)
      {
	for (unsigned int height = 1; height <= 3; height *= 3)
	{
	  double outsamplerate   = insamplerate * factor;
	  double outsampleperiod = 1000. / outsamplerate;
	  sa.rate = insamplerate;
	  sa.hasTimeTags = timetaggedinput;
	  sa.dims[0] = width;
	  sa.dims[1] = height;
	  
	  check = host.setInputStreamAttributes(sa);
	  REQUIRE (check == 0);

	  const unsigned int size = width * height;
	  inputframe.resize(size);

	  const std::string setup = (std::stringstream("Setup: ") <<
				     "timetaggedinput=" << timetaggedinput << ", " <<
				     "factor=" << factor << ", " <<
				     "width="  << width  << ", " <<
				     "height=" << height).str();
	  
	  GIVEN (setup)
	  {
	    WHEN ("giving factor")
	    {
	      double time = 0;
	      host.reset(); // clear stored received frames
	      host.setAttr("resample.factor", factor);

	      for (std::size_t i = 0; i < numframes; i++)
	      {
		if (size > 0)
		{
		  inputframe[0] = i + 1;
		  for (int j = 1; j < size; j++)
		    inputframe[j] = inputframe[j - 1] * (1 + pow(10, i));
		}
		
		check = host.frames(time, 1., inputframe.data(), size, 1);///todo num>1
		REQUIRE (check == 0);

		time += insampleperiod;
	      }

	      double expected_time = 0;
	      CHECK (host.receivedFrames.size() == numframes * factor);
              for (unsigned int i = 0; i < host.receivedFrames.size(); i++)
              {
		CHECK (host.received_times_[i] == Approx(expected_time));
                //CHECK (host.receivedFrames[i][j] == ????);
		expected_time += outsampleperiod;
              }
            } // when
          } // given
	} // height
      } // width
    } // factor
  } // timetagged
} // PiPoScale test case

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
