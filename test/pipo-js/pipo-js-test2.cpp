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

const int inframesize = 1;
const char *labels_scalar[1] = { "scalar" };

TEST_CASE ("pipo.js")
{
  PiPoTestReceiver rx(NULL);  // is also parent
  PiPoJs js(NULL);

  js.setReceiver(&rx);

  SECTION ("Catch Syntax Error")
  {
    js.expr_attr_.set("a[0] invalid js (*&^%*!");

    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == -1);	// expect failure and error message
  }
  
  SECTION ("Setup Scalar")
  {
    const int outframesize = 1;
    js.expr_attr_.set("a[0] * 2");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 1000);
    CHECK(rx.sa.dims[0] == outframesize);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels == labels_scalar);
    CHECK(rx.sa.domain  == 0);
    CHECK(rx.sa.maxFrames == 100);

    SECTION ("Scalar Data")
    {
      WHEN ("input is scalar")
      {
        const int numframes = 1;
        float vals[numframes] = {42.42};

        int ret2 = js.frames(0, 1, vals, inframesize, numframes);

        THEN ("output is ...")
        {
          CHECK(ret2 == 0);
          REQUIRE(rx.values != NULL);
          CHECK(rx.count_frames == numframes);
          CHECK(rx.values[0] == vals[numframes - 1] * 2);
          rx.zero();
        }
      }
    }

  }

  
  SECTION ("Setup Scalar->Vector")
  {
    const int outframesize = 3;
    js.expr_attr_.set("[ a[0] * 2, a[0] * 3, a[0] * 4 ]");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 1000);
    CHECK(rx.sa.dims[0] == outframesize);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels == labels_scalar);
    CHECK(rx.sa.domain  == 0);
    CHECK(rx.sa.maxFrames == 100);
  
    SECTION ("Vector Data")
    {
      WHEN ("input is scalar->vector")
      {
        const int numframes = 1;
	float vals[numframes] = {2.22};
	
        int ret2 = js.frames(0, 1, vals, inframesize, numframes);

        THEN ("output is ...")
        {
          CHECK(ret2 == 0);
          REQUIRE(rx.values != NULL);
          CHECK(rx.count_frames == numframes);
          CHECK(rx.size == outframesize);
	  CHECK(rx.values[0] == vals[numframes - 1] * 2);
	  CHECK(rx.values[1] == vals[numframes - 1] * 3);
	  CHECK(rx.values[2] == vals[numframes - 1] * 4);
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
