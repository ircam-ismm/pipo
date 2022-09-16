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

    WHEN ("Defining a specific subset of columns to select as int")
    {
      h.setAttr("select.columns", std::vector<int>{1, 2});

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 2);
        CHECK (std::strcmp(outSa.labels[0], "Spread") == 0);
        CHECK (std::strcmp(outSa.labels[1], "Skewness") == 0);
      }
    }

    WHEN ("Defining a specific subset of columns to select as symbols")
    {
      auto columns_attr = h.getAttr("select.columns");
      columns_attr->set(0, "Spread");
      columns_attr->set(1, "Skewness");
      columns_attr->set(2, "Spread");

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 3);
        CHECK (std::string(outSa.labels[0]) == "Spread");
        CHECK (std::string(outSa.labels[1]) == "Skewness");
        CHECK (std::string(outSa.labels[2]) == "Spread");
      }
    }

    WHEN ("Deprecated cols attribute is still used")
    {
      auto cols_attr = h.getAttr("select.cols");
      cols_attr->set(0, "Skewness");
      cols_attr->set(1, "Spread");

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 2);
        CHECK (std::string(outSa.labels[0]) == "Skewness");
        CHECK (std::string(outSa.labels[1]) == "Spread");
      }
    }
    
    WHEN ("Both attributes are given, columns takes precedence")
    {
      auto cols_attr = h.getAttr("select.cols");
      cols_attr->set(0, "Skewness");
      cols_attr->set(1, "Spread");
      cols_attr->set(2, "Skewness");

      auto columns_attr = h.getAttr("select.columns");
      columns_attr->set(0, 0);
      columns_attr->set(1, "Kurtosis");

      CHECK (cols_attr->getSize() == 3);
      CHECK (columns_attr->getSize() == 2);

      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 2);
        CHECK (std::string(outSa.labels[0]) == "Centroid");
        CHECK (std::string(outSa.labels[1]) == "Kurtosis");
      }
    }

    WHEN ("Illegal column values are ignored")
    {
      auto cols_attr = h.getAttr("select.cols");
      auto columns_attr = h.getAttr("select.columns");
      columns_attr->set(0, -1);
      columns_attr->set(1, "Spread");
      columns_attr->set(2, 99);
      columns_attr->set(3, "non-existing-column-name");
      columns_attr->set(4, "Skewness");
      CHECK (cols_attr->getSize() == 0);
      CHECK (columns_attr->getSize() == 5);
      
      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 2);
        CHECK (std::string(outSa.labels[0]) == "Spread");
        CHECK (std::string(outSa.labels[1]) == "Skewness");
      }
    }

    WHEN ("Changing selected columns")
    {
      auto cols_attr = h.getAttr("select.cols");
      auto columns_attr = h.getAttr("select.columns");
      columns_attr->set(0, -1);
      columns_attr->set(1, "Spread");
      columns_attr->set(2, 99);
      columns_attr->set(3, "non-existing-column-name");
      columns_attr->set(4, "Skewness");
      CHECK (cols_attr->getSize() == 0);
      CHECK (columns_attr->getSize() == 5);
      
      THEN ("Output stream attributes and selected columns should match")
      {
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 2);
        CHECK (std::string(outSa.labels[0]) == "Spread");
        CHECK (std::string(outSa.labels[1]) == "Skewness");

	columns_attr->setSize(0);
	columns_attr->set(0, 0);
	CHECK (cols_attr->getSize() == 0);
	CHECK (columns_attr->getSize() == 1);

	outSa = h.getOutputStreamAttributes();
        REQUIRE (outSa.numLabels == 1);
        CHECK (std::string(outSa.labels[0]) == "Centroid");
      }
    }
  }
}
