// -*- mode: c++; c-basic-offset:2 -*-

#include <sstream>
#include <iomanip>
#include <string>
#include <cstddef>

#include <vector>
#include <span>
#include <algorithm>

#include "catch.hpp"

#include "PiPoResample.h"
#include "PiPoTestHost.h"


template<typename T>
std::string vector_to_string (std::span<T> vec)
{
  return "[" + std::accumulate(vec.begin() + 1, vec.end(), std::to_string(vec[0]),
			       [](const std::string& a, PiPoValue b)
				 { return a + ", " + std::to_string(b); }) + "]";
}

TEST_CASE ("resample")
{
  PiPoTestHost host;
  PiPoStreamAttributes sa;

  int maxframes = 8;
  int numframes = 9;
  host.setGraph("resample");
  host.setAttr("resample.mode", 1);
  sa.maxFrames = maxframes;

  int check;
  double insamplerate   = 1000.;
  double insampleperiod = 1000. / insamplerate;
  std::vector<PiPoValue> inputframe;
  
  for (int timetaggedinput = 0; timetaggedinput <= 1; timetaggedinput++)
  {
    for (double srfactor = 0.25; srfactor <= 4.; srfactor *= 2.)
    {
      for (unsigned int width = 1; width <= 2; width++) //later: check empty frames
      {
	for (unsigned int height = 1; height <= 3; height *= 3)
	{
	  for (int rate_or_factor = 0; rate_or_factor <= 1; rate_or_factor++)
	  {
	    double outsamplerate   = insamplerate * srfactor;
	    double outsampleperiod = 1000. / outsamplerate;
	    double targetrate = insamplerate * srfactor;
	    sa.rate = insamplerate;
	    sa.hasTimeTags = timetaggedinput;
	    sa.dims[0] = width;
	    sa.dims[1] = height;
	    const unsigned int size = width * height;	    
	    inputframe.resize(numframes * size);

	    const std::string setup = (std::stringstream("Setup: ") <<
				       "timetaggedinput = " << timetaggedinput << ", " <<
				       "factor = "          << srfactor        << ", " <<
				       "targetrate = "      << targetrate      << ", " <<
				       "width = "  	    << width  	       << ", " <<
				       "height = " 	    << height 	       << ", " <<
				       "giving "            << (rate_or_factor ? "rate" : "factor")).str();
	    
	    GIVEN (setup)
	    {
	      WHEN ("input")
	      {
		host.reset(); // clear stored received frames

		if (rate_or_factor)
		{ // rate
		  host.setAttr("resample.targetrate", targetrate);
		}
		else
		{ // factor
		  //host.setAttr("resample.factor", srfactor);
		  host.setAttr("resample.factor", 1 / srfactor); // invert for classic PiPoResample.h downsampling factor behaviour
		}
		check = host.setInputStreamAttributes(sa);
		REQUIRE (check == 0);
		
		std::cout << "Input for " << setup << std::endl;

		double time = 0;
		for (std::size_t i = 0; i < numframes; i++)
		{
		  if (size > 0)
		  { // generate vector i, ii, iii, ...
		    inputframe[i * size] = i + 1;
		    for (int j = 1; j < size; j++)
		      inputframe[i * size + j] = inputframe[i * size + j - 1] * 10 + inputframe[i * size];
		    
		    std::cout << time << " " << vector_to_string(std::span(inputframe).subspan(i * size, size)) << std::endl;
		    time += insampleperiod;
		  }
		}

		time = 0;
		if (timetaggedinput)
		{
		  for (std::size_t i = 0; i < numframes; i++)
		  {
		    check = host.frames(time, 1., inputframe.data() + i * size, size, 1);///todo num>1?
		    REQUIRE (check == 0);
		    
		    time += insampleperiod;
		  }
		}
		else
		{ // sampled
		  for (int i = 0; i < numframes; i += maxframes)
		  {
		    check = host.frames(time, 1., inputframe.data() + i * size, size, std::min(numframes - i, maxframes)); // push one block of sampled frames at a time
		    REQUIRE (check == 0);
		    time += insampleperiod * maxframes;
		  }
		}

		// check received frames and time tags
		double expected_time = timetaggedinput
		  ?  insampleperiod // with timetags, output is second frame of decimated input group (???)
		  :  0;
		CHECK (abs(host.receivedFrames.size() - numframes * srfactor) < 2);
		INFO ("received frames" << host.receivedFrames.size() << " expected frames " << numframes * srfactor);

		for (unsigned int i = 0; i < host.receivedFrames.size(); i++)
		{
		  std::cout << "received(rate) @" << std::fixed << std::setprecision(3) << host.received_times_[i] << ": " << vector_to_string(std::span(host.receivedFrames[i])) << std::endl;
		  CHECK (host.received_times_[i] == Approx(expected_time));
		  //CHECK (host.receivedFrames[i][j] == ????);
		  expected_time += timetaggedinput  ?  outsampleperiod  :  0; // sampled output doesn't incr. timetags
		}
	      } // when
	    } // given
	  } // rate_or_factor
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
