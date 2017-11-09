
#include "catch.hpp"

#define PIPO_WRONG_VERSION 0.001f

// redefine to simulate a wrong version
#define PIPO_SDK_VERSION PIPO_WRONG_VERSION

// with this defined, we can override the getVersion method
#define PIPO_TESTING 1

// reset include guards
#undef _PIPO_
#undef _PIPO_HOST_
#undef _PIPO_COLLECTION_
#include "PiPo.h"
#include "PiPoCollection.h"
#include "PiPoHost.h"
#include "PiPoTestReceiver.h"


// pipo test class that does nothing but report a wrong version
class pipo_version_test : public PiPo
{
public:
  virtual float getVersion()
  {
    printf("Hey, you forced me to lie!  Returning wrong pipo sdk version %f\n", PIPO_WRONG_VERSION);
    return PIPO_WRONG_VERSION;
  }

  pipo_version_test (PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent)
  { }

  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxFrames);
  }

  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    return propagateFrames(time, weight, values, size, num);
  }
};


TEST_CASE("Test pipo version")
{
  PiPoCollection::init();
  PiPoCollection::addToCollection("version_test", new PiPoCreator<pipo_version_test>);
  
  PiPoTestReceiver rx(NULL);	// is also parent
  PiPo *p = PiPoCollection::create("version_test");

  //  if (p != NULL)
//    CHECK(p->getVersion() == PIPO_WRONG_VERSION);

  // creation should fail, since we forced pipo to report too small a version
  REQUIRE(p == NULL);
}

#undef PIPO_SDK_VERSION
#undef _PIPO_

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
