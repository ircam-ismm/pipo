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

// defaults, can be overridden by some tests
const int inframesize = 1;
const int outframesize = 1;
const int numframes = 1;
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
    js.expr_attr_.set("a[0] * 2");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("Scalar Data")
    {
      float vals[numframes] = {42.42};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      rx.zero();
    }
  }

  SECTION ("Setup Scalar expr")
  {
    js.expr_attr_.set("var temp = a[0] * 2; temp;");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("Scalar expr Data")
    {
      float vals[numframes] = {42.42};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      rx.zero();
    }
  }

  SECTION ("Setup Scalar complex expr")
  {
    js.expr_attr_.set("if (a[0] > 0) { var temp = a[0] * 2; } else { var temp = -1; }; temp;");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("Scalar complex expr Data")
    {
      float vals[numframes] = {42.42};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      rx.zero();
    }
  }

  SECTION ("Setup Scalar->Vector")
  {
    const int outframesize = 3;
    js.expr_attr_.set("[ a[0] * 2, a[0] * 3, a[0] * 4 ]");
	
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);

    SECTION ("Scalar->Vector Data")
    {
      float vals[numframes] = {2.22};
	
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      CHECK(rx.values[1] == vals[numframes - 1] * 3);
      CHECK(rx.values[2] == vals[numframes - 1] * 4);
      rx.zero();
    }
  }
    
  SECTION ("Setup Scalar->Float32 Array")
  {
    const int outframesize = 3;
    js.expr_attr_.set("new Float32Array([a[0] * 2, a[0] * 3, a[0] * 4])");
	
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);

    SECTION ("Scalar->Float32 Array Data")
    {
      float vals[numframes] = {2.22};
	
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      CHECK(rx.values[1] == vals[numframes - 1] * 3);
      CHECK(rx.values[2] == vals[numframes - 1] * 4);
      rx.zero();
    }
  }
    
  SECTION ("Setup Vector->Vector")
  {
    const int inframesize = 2;
    const int outframesize = 3;
    js.expr_attr_.set("[ a[0] * 2, a[1] * 3, a[0] + a[1] ]");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);
  
    SECTION ("Vector->Vector Data")
    {
      float vals[numframes * inframesize] = {1.11, 2.22};
      
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[0] * 2);
      CHECK(rx.values[1] == vals[1] * 3);
      CHECK(rx.values[2] == vals[0] + vals[1]);
      rx.zero();
    }
  }
  
  SECTION ("Setup Vector map")
  {
    const int inframesize = 2;
    const int outframesize = 2;
    js.expr_attr_.set("a.map(function(x) { return x * 2; })");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);
  
    SECTION ("Vector map Data")
    {
      float vals[numframes * inframesize] = {1.11, 2.22};
      
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[0] * 2);
      CHECK(rx.values[1] == vals[1] * 2);
      rx.zero();
    }
  }
  
  SECTION ("Setup Vector map ES6")
  {
    const int inframesize = 2;
    const int outframesize = 2;
    js.expr_attr_.set("a.map(x => x * 2)");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);
  
    SECTION ("Vector map Data ES6")
    {
      float vals[numframes * inframesize] = {1.11, 2.22};
      
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[0] * 2);
      CHECK(rx.values[1] == vals[1] * 2);
      rx.zero();
    }
  }

  SECTION ("Setup Math expr")
  {
    js.expr_attr_.set("Math.sin(a[0] * 2 * Math.PI)");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);
  
    SECTION ("Vector map Data ES6")
    {
      float vals[numframes * inframesize] = {0.75};
            int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == Approx(-1.0)); // sin(3/4 * 2 Pi) = -1
      rx.zero();
    }
  }

  SECTION ("Setup external expr")
  {
    const int outframesize = 4;
    const double testval = 2.0;
    js.expr_attr_.set("var db = atodb(a[0]); var hz = mtof(69); [db, dbtoa(db), hz, ftom(hz)]");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);
  
    SECTION ("Vector map Data ES6")
    {
      float vals[numframes * inframesize] = { (float) testval };
      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == Approx(PiPoJs::atodb(testval)));
      CHECK(rx.values[1] == Approx(testval));
      CHECK(rx.values[2] == Approx(440.0));
      CHECK(rx.values[3] == Approx(69));
      rx.zero();
    }
  }
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
