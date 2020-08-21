/* 
- run just this test from xcode build
  cd pipo/test
  ../build/xcode/DerivedData/Build/Products/Debug/pipo_test --success pipo.js

- quick compile with:
  clang pipo-js-test2.cpp -I .. -I ../../sdk/src -I ../../modules -I ../../modules/javascript-engine/include/ -L ../../modules/javascript-engine/lib/ -ljerry-core -ljerry-ext -ljerry-port-default  &&  ./a.out
*/

#include "catch.hpp"
#include "PiPoTestReceiver.h"

#include "PiPoJs.h"

const int framesize = 3;
const char *labels_scalar[1] = { "scalar" };

TEST_CASE ("pipo.js")
{
  PiPoTestReceiver rx(NULL);  // is also parent
  PiPoJs js(NULL);

  js.setReceiver(&rx);

  SECTION ("Catch Syntax Error")
  {
    js.expr_attr_.set("a[0] invalid js (*&^%*!");

    int ret = js.streamAttributes(false, 1000, 0, 1, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == -1);	// expect failure and error message
  }
  
  SECTION ("Setup Scalar")
  {
    js.expr_attr_.set("a[0] * 2");
    
    int ret = js.streamAttributes(false, 1000, 0, 1, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 1000);
    CHECK(rx.sa.dims[0] == 1);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels == labels_scalar);
    CHECK(rx.sa.domain  == 0);
    CHECK(rx.sa.maxFrames == 100);
  }

  
  SECTION ("Setup Scalar->Vector")
  {
    js.expr_attr_.set("[ a[0] * 2, a[0] * 3, a[0] * 4 ]");
    
    int ret = js.streamAttributes(false, 1000, 0, 1, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 1000);
    CHECK(rx.sa.dims[0] == 3);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels == labels_scalar);
    CHECK(rx.sa.domain  == 0);
    CHECK(rx.sa.maxFrames == 100);
  
/*
    SECTION ("Data")
    {
      WHEN ("input is noise")
      {
        for (int i = 0; i < numsamp; i++)
          vals[i] = random() / (1 << 30) - 1.0; // returns successive pseudo-random numbers in the range from 0 to (2**31)-1.

        int ret2 = slice.frames(0, 1, vals, 1, numsamp);

        THEN ("output is ...")
        {
          CHECK(ret2 == 0);
          REQUIRE(rx.values != NULL);
          CHECK(rx.count_frames == numframes);
          rx.zero();
        }
      }
    }
*/
  }
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
