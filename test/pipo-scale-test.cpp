#include <sstream>
#include <string>
#include <cstddef>

#include <vector>
#include <algorithm>

#include "catch.hpp"

#include "PiPoScale.h"
#include "PiPoTestHost.h"
#include "PiPoTestReceiver.h"

TEST_CASE ("PiPoScale")
{
  // PiPoTestHost host;
  // host.setGraph("scale");

  PiPoTestReceiver receiver(NULL);
  PiPoScale scale(NULL); // TODO: there should be a parent
  scale.setReceiver(&receiver);

  std::vector<PiPoValue> inputFrame;

  // PiPoStreamAttributes sa;
  // sa.maxFrames = 100;

  for(double sampleRate = 100.; sampleRate <= 1000.; sampleRate *= 10.)
  {
    for(unsigned int width = 1; width <= 10; width *= 3)
    {
      for(unsigned int height = 1; height <= 10; height *= 4)
      {
        // sa.rate = sampleRate;
        // sa.dims[0] = width;
        // sa.dims[1] = height;

        int check;
        // check = host.setInputStreamAttributes(sa);
        check = scale.streamAttributes(false, sampleRate, 0, width, height, NULL, 0, 0, 100);
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
            scale.func.set("lin");
            scale.inMin.set(0, 1.);
            scale.inMax.set(0, 2.);
            scale.outMin.set(0, 3.);
            scale.outMax.set(0, 4.);

            // host.setAttr("func", "lin");
            // host.setAttr("scale.inmin", 1.);
            // host.setAttr("scale.inmax", 2.);
            // host.setAttr("scale.outmin", 3.);
            // host.setAttr("scale.outmax", 4.);

            // TODO: no parent: no automatic propagation
            check = scale.streamAttributes(false, sampleRate, 0, width, height, NULL, 0, 0, 100);
            // check = host.setInputStreamAttributes(sa); // no need for this with a host
            REQUIRE (check == 0);

            std::vector<std::vector<PiPoValue> > values = {
              {-1. , 1. },
              { 0. , 2. },
              { 1. , 3. },
              { 2. , 4. },
              { 3. , 5. }
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);
              check = scale.frames(0., 1., &inputFrame[0], size, 1);
              // check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);
              // REQUIRE (receiver.values != NULL);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (receiver.values[sample] == Approx(outputExpected) );
              }
            }

          }  // Scaling from [1. ; 2. ] to [3. ; 4.]

          WHEN ("Scaling from [0. ; 1. ] to [0. ; 127.]")
          {
            scale.func.set("lin");
            scale.inMin.set(0, 0.);
            scale.inMax.set(0, 1.);
            scale.outMin.set(0, 0.);
            scale.outMax.set(0, 127.);

            // TODO: no parent: no automatic propagation
            check = scale.streamAttributes(false, sampleRate, 0, width, height, NULL, 0, 0, 100);
            REQUIRE (check == 0);

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
              check = scale.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);
              REQUIRE (receiver.values != NULL);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (receiver.values[sample] == Approx(outputExpected) );
              }
            }
          } // Scaling from [0. ; 1. ] to [0. ; 127.]

          WHEN ("Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping")
          {
            scale.func.set("lin");
            scale.inMin.set(0, 0.5);
            scale.inMax.set(0, 0.9);
            scale.outMin.set(0, 10.);
            scale.outMax.set(0, 100.);
            scale.clip.set(true);

            // TODO: no parent: no automatic propagation
            check = scale.streamAttributes(false, sampleRate, 0, width, height, NULL, 0, 0, 100);
            REQUIRE (check == 0);

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
              check = scale.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);
              REQUIRE (receiver.values != NULL);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (receiver.values[sample] == Approx(outputExpected) );
              }
            }

          }  // Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping

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
