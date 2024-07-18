/* 
- run just this test from xcode build
  common/pipo/build/xcode/DerivedData/Build/Products/Debug/pipo_test --success pipo.js
  maxpipo/build/osx-macho/build/Debug/pipo-test pipo.js

- quick compile with:
  clang pipo-js-test2.cpp -I .. -I ../../sdk/src -I ../../modules -I ../../modules/javascript-engine/include/ -L ../../modules/javascript-engine/lib/ -ljerry-core -ljerry-ext -ljerry-port-default  &&  ./a.out
*/

#include "../catch.hpp"
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

  SECTION ("Alloc/Dealloc")
  {
    REQUIRE(&js != NULL);
  }

  SECTION ("Just parse")
  {
    js.expr_attr_.set("1;");
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    REQUIRE(ret == 0);
  }
  
  SECTION ("Catch Syntax Error")
  {
    js.expr_attr_.set("a[0] invalid js (*&^%*!");

    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);

    REQUIRE(ret == -1);	// expect failure and error message
  }
  
  SECTION ("Catch Label Syntax Error")
  {
    js.expr_attr_.set("a");
    js.label_expr_attr_.set("asdf 0 jlkl");

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

  SECTION ("Setup external func")
  {
    const int outframesize = 4;
    const double testval = 2.0;
    js.expr_attr_.set("var db = atodb(a[0]); var hz = mtof(69); [db, dbtoa(db), hz, ftom(hz)]");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, NULL, 0, 0, 100);
  
    SECTION ("Vector external func")
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

  SECTION ("Setup with param")
  {
    js.expr_attr_.set("a[0] * p[0]");
    js.param_attr_.set(0, 2);
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("Data with param ")
    {
      float vals[numframes] = {42.42};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1] * 2);
      rx.zero();

      SECTION ("Param size change")
      {
	js.param_attr_.set(0, 3);
	js.param_attr_.set(1, 4); // param size changes
	
	int ret2 = js.frames(0, 1, vals, inframesize, numframes);
	CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
	CHECK(rx.values[0] == vals[numframes - 1] * 3);
	rx.zero();
      }

      SECTION ("Setup with param overshoot")
      {
	js.expr_attr_.set("1 * p[99]"); // using uninitialized value
	
	int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
	CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);
	
	int ret2 = js.frames(0, 1, vals, inframesize, numframes);
	CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
	//CHECK(rx.values[0] == 0);
	WARN("uninitialized value is: " << rx.values[0]); // uninitialized, not zero...
	rx.zero();
      }
    }    
  }

  SECTION ("Setup expr using input label object")
  {
    js.expr_attr_.set("print('c is ' + c); a[c.scalar] * c.scalar");

    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("Data with labels ")
    {
      float vals[numframes] = {0};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, numframes);
      CHECK(rx.values[0] == vals[numframes - 1]);
      rx.zero();
    }
  }

  SECTION ("Setup output labels")
  {
    const char *outlab[]  = { "column_1", "column_2", "column_3", "" };

    js.expr_attr_.set("a");
    js.label_expr_attr_.set("'column_1'");

    SECTION ("one label replace")
    {
      int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
      CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, outlab, 0, 0, 100);
    }

    SECTION ("one label NULL")
    {
      int ret2 = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
      CHECK_STREAMATTRIBUTES(ret2, rx, false, 1000, 0, outframesize, 1, outlab, 0, 0, 100);
    }
    
    SECTION ("three labels")
    {
      const int outframesize = 3;
      js.expr_attr_.set("[ a[0], 2, 3 ]");
      js.label_expr_attr_.set("[ 'column_1', 'column_2', 'column_3' ]");
      int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
      CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, outlab, 0, 0, 100);
    }
        
    SECTION ("numlabels/width mismatch")
    {
      const int outframesize = 4;
      js.expr_attr_.set("[ a[0], 2, 3, 4 ]");
      js.label_expr_attr_.set("[ 'column_1', 'column_2', 'column_3' ]");
      int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
      CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, outlab, 0, 0, 100);
    }

    SECTION ("extend labels")
    {
      const int outframesize = 2;
      js.expr_attr_.set("[ a[0], a[0] * 2 ]");
      js.label_expr_attr_.set("l.concat('column_2')");

      SECTION ("extend labels concat")
      {
	const char *outlab2[] = { "scalar", "column_2" };
	int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
	CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, outlab2, 0, 0, 100);
      }
      
      SECTION ("extend NULL labels")
      {
	const char *outlab2[] = { "column_2", "" };
	int ret2 = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
	CHECK_STREAMATTRIBUTES(ret2, rx, false, 1000, 0, outframesize, 1, outlab2, 0, 0, 100);
      }

      js.label_expr_attr_.set("l[1] = 'column_2'; l");

      SECTION ("extend labels out of bounds")
      {
	const char *outlab2[] = { "scalar", "column_2" };
	int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
	CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, outlab2, 0, 0, 100);
      }
    
      SECTION ("extend NULL labels out of bounds")
      {
	const char *outlab2[] = { "undefined", "column_2" };
	int ret2 = js.streamAttributes(false, 1000, 0, inframesize, 1, NULL, 0, 0, 100);
	CHECK_STREAMATTRIBUTES(ret2, rx, false, 1000, 0, outframesize, 1, outlab2, 0, 0, 100);
      }
    }
  }

  SECTION ("Setup expr with undefined")
  { // always returns undefined !@#$%^&*(:
    // function x(a) { a ? 1 : undefined; }
    js.expr_attr_.set("time === undefined || a[0] > 0.5 ? a : undefined;");
    
    int ret = js.streamAttributes(false, 1000, 0, inframesize, 1, labels_scalar, 0, 0, 100);
    CHECK_STREAMATTRIBUTES(ret, rx, false, 1000, 0, outframesize, 1, labels_scalar, 0, 0, 100);

    SECTION ("filtered expr data")
    {
      const int numframes = 3;
      float vals[numframes] = { 0.1, 10, 0.2};

      int ret2 = js.frames(0, 1, vals, inframesize, numframes);
      CHECK_FRAMES(ret2, rx, 0, outframesize, 1); // num output frames == 1; 2 should be ignored
      CHECK(rx.values[0] == 10);
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
