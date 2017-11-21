#include <sstream>
#include <string>
#include <cstddef>

#include <vector>
#include <algorithm>

#include "catch.hpp"

#include "PiPoScale.h"
#include "PiPoTestHost.h"

TEST_CASE ("PiPoScale")
{
  PiPoTestHost host;
  host.setGraph("scale");

  std::vector<PiPoValue> inputFrame;
  int check;

  PiPoStreamAttributes sa;
  sa.maxFrames = 100;

  for(double sampleRate = 100.; sampleRate <= 1000.; sampleRate *= 10.)
  {
    for(unsigned int width = 1; width <= 10; width *= 3)
    {
      for(unsigned int height = 1; height <= 10; height *= 4)
      {
        sa.rate = sampleRate;
        sa.dims[0] = width;
        sa.dims[1] = height;

        check = host.setInputStreamAttributes(sa);
        REQUIRE (check == 0);

        const unsigned int size = width * height;
        inputFrame.resize(size);

        const std::string setup = (std::stringstream("Setup: ") <<
                                   "sampleRate=" << sampleRate << ", " <<
                                   "width=" << width << ", " <<
                                   "height=" << height).str();
        GIVEN (setup)
        {
          WHEN ("Scaling from [1. ; 2. ] to [3. ; 4.]")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 1.);
            host.setAttr("scale.inmax", 2.);
            host.setAttr("scale.outmin", 3.);
            host.setAttr("scale.outmax", 4.);

            std::vector<std::vector<PiPoValue> > values = {
              {-1. , 1. },
              { 0. , 2. },
              { 1. , 3. },
              { 2. , 4. },
              { 3. , 5. }
            };

            for (std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for (unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }

          }  // Scaling from [1. ; 2. ] to [3. ; 4.]

          WHEN ("Scaling from [0. ; 1. ] to [0. ; 127.]")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 0.);
            host.setAttr("scale.inmax", 1.);
            host.setAttr("scale.outmin", 0.);
            host.setAttr("scale.outmax", 127.);

            std::vector<std::vector<PiPoValue> > values = {
              {-1.  , -127. },
              {-0.5 ,  -63.5},
              {-0.  ,    0. },
              { 0.  ,    0. },
              { 0.1 ,   12.7},
              { 0.5 ,   63.5},
              { 1.  ,  127. },
              { 2.  ,  254. }
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }
          } // Scaling from [0. ; 1. ] to [0. ; 127.]

          //*
          WHEN ("Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 0.5);
            host.setAttr("scale.inmax", 0.9);
            host.setAttr("scale.outmin", 10.);
            host.setAttr("scale.outmax", 100.);
            host.setAttr("scale.clip", true);

            std::vector<std::vector<PiPoValue> > values = {
              {-1.  ,  10.  },
              { 0.  ,  10.  },
              { 0.5 ,  10.  },
              { 0.6 ,  32.5 },
              { 0.65,  43.75},
              { 0.9 , 100.  },
              { 1.  , 100.  }
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }

          }  // Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping
          //*/
        } // Setup: sample-rate, width, and height

      } // height
    } // width
  } // sampleRate

} // PiPoScale test case

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
