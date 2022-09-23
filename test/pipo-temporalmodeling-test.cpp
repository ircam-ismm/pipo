/* -*- mode: c++; c-basic-offset:2 -*- */
#include "catch.hpp"

#include "PiPoTestHost.h"

TEST_CASE ("temporalmodeling")
{
  PiPoTestHost h;
  PiPoStreamAttributes sa;

  SECTION ("check 2 col const output")
  {
    h.setGraph("const");
    h.setAttr("const.name",  std::vector<const char *>{"a", "b"});
    h.setAttr("const.value", std::vector<int> {3, 4});
    h.setInputStreamAttributes(sa);
    PiPoStreamAttributes &out_sa = h.getOutputStreamAttributes();
    CHECK(out_sa.dims[0] == 3);	// PiPoStreamAttributes is initialised with 1 x 1 dims input
    CHECK(out_sa.dims[1] == 1);
  }
  
  SECTION ("setup 2 col const input to segmean")
  {
    h.setGraph("const:segmean");
    h.setAttr("const.name",  std::vector<const char *>{"a", "b"});
    h.setAttr("const.value", std::vector<int> {1, 2});

    WHEN("no column selection")
    {
      h.setInputStreamAttributes(sa);
      PiPoStreamAttributes &out_sa = h.getOutputStreamAttributes();

      THEN("all columns are passed through")
      {
	REQUIRE(out_sa.dims[0]   == 3);
	REQUIRE(out_sa.numLabels == 3);
	CHECK(out_sa.dims[1] == 1);
	CHECK (std::string(out_sa.labels[0]) ==  "Mean");
	CHECK (std::string(out_sa.labels[1]) ==  "aMean");
	CHECK (std::string(out_sa.labels[2]) ==  "bMean");
      }
    }
    
    WHEN ("Defining a specific subset of columns to select as int")
    {
      h.setAttr("segmean.columns", std::vector<int>{1, 99, -99, 2});
      h.setInputStreamAttributes(sa);

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &out_sa = h.getOutputStreamAttributes();
        REQUIRE (out_sa.numLabels == 2);
        CHECK (std::string(out_sa.labels[0]) ==  "aMean");
        CHECK (std::string(out_sa.labels[1]) ==  "bMean");
      }
    }
  }
}
