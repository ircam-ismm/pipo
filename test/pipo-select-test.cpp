#include "catch.hpp"

#include "PiPoTestHost.h"

SCENARIO ("Testing PiPoSelect")
{
  PiPoTestHost h;
  PiPoStreamAttributes sa;

  GIVEN ("A host with a graph ending with \"select\"")
  {
    h.setGraph("slice:moments:select");
    h.setAttr("slice.size", 10);
    h.setAttr("slice.hop", 5);
    h.setInputStreamAttributes(sa);

    PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();

    WHEN ("Defining a specific subset of columns to select")
    {
      h.setAttr("select.columns", std::vector<int>{1, 2});

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        std::vector<int> c = h.getIntArrayAttr("select.columns");

        REQUIRE (outSa.numLabels == 2);
        REQUIRE (std::strcmp(outSa.labels[0], "Spread") == 0);
        REQUIRE (std::strcmp(outSa.labels[1], "Skewness") == 0);
      }
    }
  }
}
