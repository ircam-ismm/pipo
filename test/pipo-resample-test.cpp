// -*- mode: c++; c-basic-offset:2 -*-

#include <sstream>
#include <string>
#include <cstddef>

#include <vector>
#include <algorithm>

#include "catch.hpp"

#include "PiPoResample.h"
#include "PiPoTestHost.h"


template<typename T>
std::string vector_to_string (std::vector<T> vec)
{
  return "[" + std::accumulate(vec.begin() + 1, vec.end(), std::to_string(vec[0]),
			       [](const std::string& a, PiPoValue b)
				 { return a + ", " + std::to_string(b); }) + "]";
}

TEST_CASE ("resample")
{
  PiPoTestHost host;
  PiPoStreamAttributes sa;

  host.setGraph("resample");
  host.setAttr("resample.mode", 1);
  sa.maxFrames = 32;

  int check;
  double insamplerate   = 1000.;
  double insampleperiod = 1000. / insamplerate;
  std::vector<PiPoValue> inputframe;
  int numframes = 8;
  
  for (int timetaggedinput = 1; timetaggedinput >= 0; timetaggedinput -= 1)
  {
    for (double factor = 0.25; factor <= 4.; factor *= 2.)
    {
      for (unsigned int width = 1; width <= 2; width++) //later: check empty frames
      {
	for (unsigned int height = 1; height <= 3; height *= 3)
	{
	  double outsamplerate   = insamplerate * factor;
	  double outsampleperiod = 1000. / outsamplerate;
	  sa.rate = insamplerate;
	  sa.hasTimeTags = timetaggedinput;
	  sa.dims[0] = width;
	  sa.dims[1] = height;
	  const unsigned int size = width * height;
	  inputframe.resize(size);

	  const std::string setup = (std::stringstream("Setup: ") <<
				     "timetaggedinput = " << timetaggedinput << ", " <<
				     "factor = " << factor << ", " <<
				     "width = "  << width  << ", " <<
				     "height = " << height).str();

	  GIVEN (setup)
	  {
	    WHEN ("giving targetrate")
	    {
	      double time = 0;
	      double targetrate = insamplerate * factor;
	      INFO("targetrate " << targetrate);
	      host.reset(); // clear stored received frames
	      host.setAttr("resample.targetrate", targetrate);
	      check = host.setInputStreamAttributes(sa);
	      REQUIRE (check == 0);

	      std::cout << "Input for " << setup << std::endl;
	  
	      for (std::size_t i = 0; i < numframes; i++)
	      {
		if (size > 0)
		{ // generate vector i, ii, iii, ...
		  inputframe[0] = i + 1;
		  for (int j = 1; j < size; j++)
		    inputframe[j] = inputframe[j - 1] * 10 + inputframe[0];

		  std::cout << time << " " << vector_to_string(inputframe) << std::endl;
		}
		
		check = host.frames(time, 1., inputframe.data(), size, 1);///todo num>1
		REQUIRE (check == 0);

		time += insampleperiod;
	      }

	      // check received frames and time tags
	      double expected_time = insampleperiod; // output is second frame of decimated input group (???)
	      CHECK (host.receivedFrames.size() == numframes * factor);
              for (unsigned int i = 0; i < host.receivedFrames.size(); i++)
              {
		std::cout << "received(rate) @" << host.received_times_[i] << ": " << vector_to_string(host.receivedFrames[i]) << std::endl;
		CHECK (host.received_times_[i] == Approx(expected_time));
                //CHECK (host.receivedFrames[i][j] == ????);
		expected_time += outsampleperiod;
              }
            } // when

	    WHEN ("giving factor")
	    {
	      double time = 0;
	      host.reset(); // clear stored received frames
	      //host.setAttr("resample.factor", factor);
	      host.setAttr("resample.factor", 1 / factor); // invert for classic PiPoResample.h downsampling factor
	      check = host.setInputStreamAttributes(sa);
	      REQUIRE (check == 0);

	      for (std::size_t i = 0; i < numframes; i++)
	      {
		if (size > 0)
		{ // generate vector i, ii, iii, ...
		  inputframe[0] = i + 1;
		  for (int j = 1; j < size; j++)
		    inputframe[j] = inputframe[j - 1] * 10 + inputframe[0];
		}
		
		check = host.frames(time, 1., inputframe.data(), size, 1);///todo num>1
		REQUIRE (check == 0);

		time += insampleperiod;
	      }

	      double expected_time = insampleperiod; // output is second frame of decimated input group (???)
	      CHECK (host.receivedFrames.size() == numframes * factor);
              for (unsigned int i = 0; i < host.receivedFrames.size(); i++)
              {
		std::cout << "received(factor) @" << host.received_times_[i] << ": " << vector_to_string(host.receivedFrames[i]) << std::endl;

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
