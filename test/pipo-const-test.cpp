#include "catch.hpp"

#include "PiPoTestHost.h"

SCENARIO ("Testing PiPoConst")
{
  PiPoTestHost h;
  PiPoStreamAttributes sa;
  std::vector<PiPoValue> inputFrame;

  GIVEN ("A host with a graph ending with \"const\"")
  {
    // h.setGraph("slice:moments:const");
    // h.setAttr("slice.size", 10);
    // h.setAttr("slice.hop", 5);
    h.setGraph("moments:const");
    h.setInputStreamAttributes(sa);

    PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();

    WHEN ("Setting the const value")
    {
      h.setAttr("const.value", 3.14);

      THEN ("Output frames should have the const value appended")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        std::vector<int> c = h.getIntArrayAttr("select.columns");

        REQUIRE (outSa.dims[0] == 5);

        std::fill(inputFrame.begin(), inputFrame.end(), 10.);
        h.reset();
        h.frames(0., 1., &inputFrame[0], 1, inputFrame.size());

        REQUIRE (h.receivedFrames[0].size() == outSa.dims[0]);
        REQUIRE (h.receivedFrames[0][4] == 3.14);
      }
    }
  }
}
