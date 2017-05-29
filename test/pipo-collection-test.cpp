#include "catch.hpp"
#include "PiPoCollection.h"

TEST_CASE ("Test pipo collection")
{
  PiPoCollection::init();

  WHEN ("Trying to instantiate a pipo chain")
  {
    PiPo *seg = PiPoCollection::create("slice:fft:sum:scale:onseg");
    PiPo *lpcf = PiPoCollection::create("lpcformants");

    THEN ("Chains / modules should be instantiated")
    {
      // Chains / modules are not null
      //seg->setAttr(0, 12);
      
      REQUIRE (seg != NULL);
      REQUIRE (lpcf != NULL);
    }
  }
}
