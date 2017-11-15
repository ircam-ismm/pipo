#include "catch.hpp"
#include "PiPoTestReceiver.h"

#include "PiPoParallel.h"
#include "PiPoConst.h"
#include "PiPoScale.h"


TEST_CASE("Test PiPoParallel")
{
  PiPo::Parent *parent = NULL;
  PiPoTestReceiver rx(parent);
  PiPoParallel par(parent);
  PiPoConst con(parent);
  PiPoScale sca(parent);

  par.add(con);	// use reference
  par.add(&sca); // use pointer
  par.setReceiver(&rx);

  WHEN ("streamAttributes done") {
    con.value.set(444);
    int ret = par.streamAttributes(false, 11, 22, 1, 1, NULL, false, 33, 44);

    THEN ("Output is configured") {
      CHECK(ret == 0);
      CHECK(rx.count_error == 0);
      CHECK(rx.count_streamAttributes == 1);
      CHECK(rx.sa.rate == 11);
      CHECK(rx.sa.offset == 22);
      CHECK(rx.sa.dims[0] == 3);	// width = #columns
      CHECK(rx.sa.dims[1] == 1);
      CHECK(rx.sa.labels != NULL);
      CHECK(rx.sa.domain  == 33);
      CHECK(rx.sa.maxFrames == 1);
    }

    WHEN ("Streaming data") {
      float vals = 333;
      int ret2 = par.frames(111, 222, &vals, 1, 1);

      THEN ("Results are as expected") {
	CHECK(ret2 == 0);
	CHECK(rx.count_frames == 1);
	CHECK(rx.time == 111);
	CHECK(rx.values[0] == 333);
	CHECK(rx.values[1] == 444);
	CHECK(rx.values[2] == 333);
      }
    }
  }
}

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
