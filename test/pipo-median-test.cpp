#include "catch.hpp"

#include "PiPoMedian.h"
#include "PiPoTestReceiver.h"

TEST_CASE ("pipo.median")
{
  PiPoTestReceiver rx(NULL);  // is also parent
  PiPoMedian median(NULL);
  median.setReceiver(&rx);

  SECTION ("setup small window")
  {
    median.size.set(3);
    int ret = median.streamAttributes(false, 1000, 0, 1, 1, NULL, false, 0, 10);

    REQUIRE(ret == 0);
    CHECK(rx.count_streamAttributes == 1);

    SECTION ("data")
    {
      std::vector<PiPoValue> input    = { 1, 0,   1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1 };
      std::vector<PiPoValue> expected = { 1, 0.5, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1 };
      int numerr = 0;

      //for (auto it = input.begin(); it != input.end(); it++)

      for (int i = 0; i < input.size(); i++)
      {
	int ret2 = median.frames(0, 1, &input[i], 1, 1);
	CHECK(ret2 == 0);
	REQUIRE(rx.count_frames == 1);
	REQUIRE(rx.values != NULL);

	INFO("i " << i << " input " << input[i] << " output " << rx.values[0]<< " expected " << expected[i]);
	CHECK(rx.values[0] == expected[i]);
	numerr += rx.values[0] != expected[i];

	rx.zero();
      }
      
      CHECK(numerr == 0);	    
    } 
  }
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
