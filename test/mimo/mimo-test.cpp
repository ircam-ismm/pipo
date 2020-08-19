/* run this test only with
../../../maxpipo/build/osx-macho/DerivedData/Build/Products/Debug/pipo-test mimo
*/

#include "catch.hpp"
#include <stdlib.h>
#include "mimo.h"
#include "mimo_stats.h"
#include "PiPoTestReceiver.h"

class MimoTestReceiver : public Mimo
{
public:
  MimoTestReceiver (PiPo::Parent *parent)
    : Mimo(parent), prx(parent)
  {  }

  ~MimoTestReceiver()
  {  }

  mimo_model_data *getmodel () override { return NULL; }

  // called by mimo module's propagateSetup via setupChain
  int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[]) override
  {
    const PiPoStreamAttributes *at = streamattr[0];

    char str[1000];
    printf("%s: received mimo setup output stream attributes\n%s", __PRETTY_FUNCTION__, at->to_string(str, 1000));

    // store stream attributes in outputTrackDescr via base PiPoProcReceiver's method
    return 0; // propagateStreamAttributes(at->hasTimeTags, at->rate, at->offset, at->dims[0], at->dims[1], at->labels, at->hasVarSize, at->domain, at->maxFrames);
  }

  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[]) override
  {
    return -1;
  }

  // forwarding to PiPoTestReceiver:
  void zero ()
  {
    prx.zero();
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames) override
  {
    return prx.streamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxFrames);
  }

  int frames (double _time, double _weight, PiPoValue *_values, unsigned int _size, unsigned int _num) override
  {
    return prx.frames(_time, _weight, _values, _size, _num);
  }

  int finalize (double inputEnd) override
  {
    return prx.finalize(inputEnd);
  }

  void signalError(PiPo *pipo, std::string errorMsg)
  {
    prx.signalError(pipo, errorMsg);
  }

  void signalWarning(PiPo *pipo, std::string errorMsg)
  {
    prx.signalWarning(pipo, errorMsg);
  }

  PiPoTestReceiver prx;
};


TEST_CASE("mimo")
{
  MimoTestReceiver rx(NULL);	// is also parent
  mimo_stats stats(NULL);

  stats.setReceiver(&rx);

  const int numframes = 3, numcols = 3;
  float data[numframes * numcols] = { 1, 4, 7, 2, 5, 8, 3, 6, 9 };

  SECTION("setup")
  {
    WHEN("setup called")
    {
      PiPoStreamAttributes attr = PiPoStreamAttributes();
      const PiPoStreamAttributes *attrarr[1] = { &attr }; // put them in an array per track
      attr.dims[0] = numcols;
      attr.dims[1] = 1;
      const int sizes[1] = { numframes };

      int ret = stats.setup(1, 1, sizes, attrarr);

      THEN("check")
      {
	;
      }

      WHEN("data input")
      {
	mimo_buffer inbuf;
	inbuf.numframes = numframes;
	inbuf.data      = data;
	inbuf.varsize   = NULL;
	inbuf.has_timetags = false;
	inbuf.time.starttime = 0;
	
	stats.train(0, 0, 1, &inbuf);

	THEN("result is")
	{
	  stats_model_data res = *stats.getmodel();

	  REQUIRE(res.num.size() == numcols);
	  REQUIRE(res.mean.size() == numcols);

#define printl(NAME, F) printf("%s:\t" F " " F " " F "\n", #NAME, res.NAME[0], res.NAME[1], res.NAME[2]);
	  printl(num, "%ld");
	  printl(mean, "%f");
	  printl(std, "%f");
	  printl(min, "%f");
	  printl(max, "%f");

	  THEN("model as json is")
	  {
	    mimo_model_data *model = stats.getmodel();
	    int size = model->json_size();
	    char json[size];
	    model->to_json(json, size);
	    
	    printf("\nmodel to json:\n%s\n", json);
	  }
	}

	SECTION("decoding")
	{
	  WHEN ("stream setup")
	  {
	    const char *labels[] = { "col0", "col1", "col2" };
	    int ret = stats.streamAttributes(false, 1000, 0, numcols, 1, labels, false, 0, 1);

	    THEN ("setup is propagated") {
	      CHECK(ret == 0);
	      CHECK(rx.prx.count_streamAttributes == 1);
	      CHECK(rx.prx.sa.dims[0] == 3);	// width = #columns
	      CHECK(rx.prx.sa.dims[1] == 1);
	      CHECK(rx.prx.sa.domain  == 0);
	      CHECK(rx.prx.sa.maxFrames == 1);
	      REQUIRE(rx.prx.sa.labels != NULL);
	      CHECK(std::strcmp(rx.prx.sa.labels[0], "col0Norm") == 0);
	      rx.zero();
	    }

	    WHEN ("input is training data") {
	      int ret2 = stats.frames(0, 1, data, numcols, numframes);

	      THEN ("output is ...") {
		CHECK(ret2 == 0);
		CHECK(rx.prx.count_frames == numframes);
		REQUIRE(rx.prx.values != NULL);

		// last frame is mean + 1
		CHECK(rx.prx.values[0] == Approx(1 / 1.63299322));
		CHECK(rx.prx.values[1] == Approx(1 / 4.54606056));
		CHECK(rx.prx.values[2] == Approx(1 / 7.52772665));
		rx.zero();
	      }
	    }
	  }
	}
      }
    }
  }
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
