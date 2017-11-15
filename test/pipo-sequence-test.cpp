#include "catch.hpp"
#include "PiPoTestReceiver.h"

#include "PiPoSequence.h"
#include "PiPoConst.h"
#include "PiPoScale.h"

TEST_CASE("Test PiPoSequence")
{
  PiPo::Parent *parent = NULL;
  PiPoTestReceiver rx(parent);
  PiPoSequence seq(parent);
  PiPoConst con(parent);
  PiPoScale sca(parent);

  seq.add(con, false);	// use reference
  seq.add(&sca, false); // use pointer
  seq.connect(&rx);

  SECTION("setup")
  {
    con.value.set(99);
    int ret = seq.streamAttributes(false, 11, 22, 1, 1, NULL, false, 33, 44);

    CHECK(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 11);
    CHECK(rx.sa.offset == 22);
    CHECK(rx.sa.dims[0] == 2);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels != NULL);
    CHECK(rx.sa.domain  == 33);
    CHECK(rx.sa.maxFrames == 1);
/*}

  SECTION("data")
  {*/
    float vals = 333;
    int ret2 = seq.frames(111, 222, &vals, 1, 1);

    CHECK(ret2 == 0);
    CHECK(rx.count_frames == 1);
    CHECK(rx.time == 111);
    CHECK(rx.values[0] == 333);
    CHECK(rx.values[1] == 99);
  }
}


TEST_CASE("Test PiPoSequence autoconnect")
{
  PiPo::Parent *parent = NULL;
  PiPoTestReceiver rx(parent);
  PiPoSequence seq(parent);
  PiPoConst con(parent);
  PiPoScale sca(parent);

  seq.add(&con);	// use pointer
  seq.add(sca);		// use reference
  seq.setReceiver(&rx);

  SECTION("setup autoconnected")
  {
    con.value.set(99);
    int ret = seq.streamAttributes(false, 11, 22, 1, 1, NULL, false, 33, 44);

    CHECK(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 11);
    CHECK(rx.sa.offset == 22);
    CHECK(rx.sa.dims[0] == 2);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels != NULL);
    CHECK(rx.sa.domain  == 33);
    CHECK(rx.sa.maxFrames == 1);
/*}

  SECTION("data")
  {*/
    float vals = 333;
    int ret2 = seq.frames(111, 222, &vals, 1, 1);

    CHECK(ret2 == 0);
    CHECK(rx.count_frames == 1);
    CHECK(rx.time == 111);
    CHECK(rx.values[0] == 333);
    CHECK(rx.values[1] == 99);
  }
}


TEST_CASE("Test PiPoSequence arg list")
{
  PiPo::Parent *parent = NULL;
  PiPoTestReceiver rx(parent);
  PiPoConst con(parent);
  PiPoScale sca(parent);
  PiPoSequence seq(parent, con, sca);

  seq.setReceiver(&rx);

  SECTION("setup with arg list")
  {
    con.value.set(99);
    int ret = seq.streamAttributes(false, 11, 22, 1, 1, NULL, false, 33, 44);

    CHECK(ret == 0);
    CHECK(rx.count_streamAttributes == 1);
    CHECK(rx.sa.rate == 11);
    CHECK(rx.sa.offset == 22);
    CHECK(rx.sa.dims[0] == 2);
    CHECK(rx.sa.dims[1] == 1);
    CHECK(rx.sa.labels != NULL);
    CHECK(rx.sa.domain  == 33);
    CHECK(rx.sa.maxFrames == 1);
/*}

  SECTION("data")
  {*/
    float vals = 333;
    int ret2 = seq.frames(111, 222, &vals, 1, 1);

    CHECK(ret2 == 0);
    CHECK(rx.count_frames == 1);
    CHECK(rx.time == 111);
    CHECK(rx.values[0] == 333);
    CHECK(rx.values[1] == 99);
  }
}

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
