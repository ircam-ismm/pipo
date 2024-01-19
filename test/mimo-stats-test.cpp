/* run this test only with
../../maxpipo//build//osx-macho//build/Debug/pipo-test mimo-stats
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
  };

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


TEST_CASE("mimo-stats")
{
  MimoTestReceiver rx(NULL);	// is also parent
  mimo_stats stats(NULL);

  stats.setReceiver(&rx);

  const int numframes = 2, numcols = 3, numrows = 1;
  float data1[numframes * numcols * numrows] = { 1, 2, 3, 4, 5, 6};
  float data2[numframes * numcols * numrows] = { 10, 20, 30, 40, 50, 60};

  // result for one buffer
  float mean1[numcols * numrows] = { 2.5, 3.5, 4.5 }; 
  float std1[numcols * numrows]  = { 1.5, 1.5, 1.5 }; // unbiased: { 2.121320343559642, 2.121320343559642, 2.121320343559642 };

  // result for two buffers
  float mean2[numcols * numrows] = { 13.75, 19.25, 24.75 };
  float std2[numcols * numrows]  = { 15.497983739828868, 19.018083499658950, 22.884219453588535 };
// unbiased: { 17.895530168173281, 21.960191255997749, 26.424420523447623 };

  PiPoStreamAttributes attr = PiPoStreamAttributes();
  const PiPoStreamAttributes *attrarr[1] = { &attr }; // put them in an array per track
  attr.dims[0] = numcols;
  attr.dims[1] = numrows;

  WHEN("setup called for 1 buffer")
  {
    const int sizes[1] = { numframes };
    int ret = stats.setup(1, 1, sizes, attrarr);
    REQUIRE(ret >= 0);

    THEN("check")
    {
      ;
    }

    WHEN("data input")
    {
      mimo_buffer inbuf(numframes, data1, NULL, false, NULL, 0);
      stats.train(0, 0, 1, &inbuf);

      THEN("result is")
      {
	stats_model_data res = *stats.getmodel();

	REQUIRE(res.num.size() == numcols * numrows);
	REQUIRE(res.min.size() == numcols * numrows);
	REQUIRE(res.max.size() == numcols * numrows);
	REQUIRE(res.mean.size() == numcols * numrows);
	REQUIRE(res.std.size() == numcols * numrows);

	// compare with expected results
	for (int i = 0; i < numcols; i++)
	{
	  CHECK(i == i);	// display index with -s
	  CHECK(res.num[i] == numframes);
	  CHECK(res.min[i] == data1[i]);
	  CHECK(res.max[i] == data1[i + numcols]);
	  CHECK(res.mean[i] == mean1[i]);
	  CHECK(res.std[i] == std1[i]);
	}

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
	  int ret = stats.streamAttributes(false, 1000, 0, numcols, numrows, labels, false, 0, 1);

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
	    int ret2 = stats.frames(0, 1, data1, numcols * numrows, numframes);

	    THEN ("output is ...") {
	      REQUIRE(ret2 == 0);
	      CHECK(rx.prx.count_frames == numframes);
	      REQUIRE(rx.prx.values != NULL);

	      // last frame is mean + 1
	      CHECK(rx.prx.values[0] == 1);
	      CHECK(rx.prx.values[1] == 1);
	      CHECK(rx.prx.values[2] == 1);
	      rx.zero();
	    }
	  }
	}
      }
    }
  }

  WHEN("setup called for 2 buffers")
  {
    const int sizes[2] = { numframes, numframes };
    int ret = stats.setup(2, 1, sizes, attrarr);
    REQUIRE(ret >= 0);

    WHEN("data input 2")
    {
      mimo_buffer inbuf[] = { mimo_buffer(numframes, data1, NULL, false, NULL, 0),
			      mimo_buffer(numframes, data2, NULL, false, NULL, 0) };
      stats.train(0, 0, 2, inbuf);

      THEN("result is")
      {
	stats_model_data res = *stats.getmodel();

	REQUIRE(res.num.size() == numcols * numrows);
	REQUIRE(res.min.size() == numcols * numrows);
	REQUIRE(res.max.size() == numcols * numrows);
	REQUIRE(res.mean.size() == numcols * numrows);
	REQUIRE(res.std.size() == numcols * numrows);

	// compare with expected results
	for (int i = 0; i < numcols; i++)
	{
	  CHECK(res.num[i] == numframes * 2);
	  CHECK(res.min[i] == data1[i]);
	  CHECK(res.max[i] == data2[i + numcols]);
	  CHECK(res.mean[i] == mean2[i]);
	  CHECK(res.std[i] == Approx(std2[i]));
	}

	THEN("model as json is")
	{
	  mimo_model_data *model = stats.getmodel();
	  int size = model->json_size();
	  char json[size];
	  model->to_json(json, size);
	    
	  printf("\nmodel to json:\n%s\n", json);
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
