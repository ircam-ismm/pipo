#include "catch.hpp"
#include "PiPoSlice.h"
#include "PiPoFft.h"
#include "PiPoTestReceiver.h"

const double sr = 44100;
const int fftsize = 512;
const int winsize = 1764;
const int hopsize = 441;
const int numsamp = hopsize * 8;

TEST_CASE ("Test pipo fft")
{
  PiPoTestReceiver rx(NULL);  // is also parent
  PiPoSlice slice(NULL);
  PiPoFft fft(NULL);

  slice.setReceiver(&fft);
  fft.setReceiver(&rx);

  SECTION ("Setup")
  {
    slice.size.set(winsize);
    slice.hop.set(hopsize);
    fft.size.set(fftsize);
    fft.mode.set(3);
    fft.norm.set(0);
    int ret = slice.streamAttributes(false, 44100, 0, 1, 1, NULL, 0, 0, 100);

    CHECK(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == sr / hopsize);
    CHECK(rx.sa.dims[0] == 1);
    CHECK(rx.sa.dims[1] == fftsize / 2  + 1);
    CHECK(rx.sa.labels != NULL);
    CHECK(rx.sa.domain  == sr / 2);
    CHECK(rx.sa.maxFrames == 1);

    SECTION ("Data")
    {
      float vals[numsamp];

      WHEN ("input is noise")
      {
        for (int i = 0; i < numsamp; i++)
          vals[i] = random() / (1 << 30) - 1.0; // returns successive pseudo-random numbers in the range from 0 to (2**31)-1.

        int ret2 = slice.frames(0, 1, vals, 1, numsamp);

        THEN ("output is ...")
        {
          CHECK(ret2 == 0);
          REQUIRE(rx.values != NULL);
          CHECK(rx.count_frames == 8);
          rx.zero();
        }
      }
    }
  }
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
