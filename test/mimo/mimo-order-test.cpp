/* run this test only with
../../maxpipo//build//osx-macho//build/Debug/pipo-test mimo-order
*/

#include "../catch.hpp"
#include <stdlib.h>
#include "mimo-test-receiver.h"
#include "mimo_order.h"



TEST_CASE("mimo-order")
{
  MimoTestReceiver rx(NULL);	// is also parent
  MimoOrder order(NULL);

  order.setReceiver(&rx);

  const int numframes = 3, numcols = 2, numrows = 1;
  float data1[numframes * numcols * numrows] = { 1, 6, 2, 5, 3, 4};
  float data2[numframes * numcols * numrows] = { 10, -3, 20, -2, 30, -1};

  // result for one buffer
  float order0[numframes * numcols * numrows] = { 0, 2, 1, 1, 2, 0 }; 

  // result for two buffers
  float order1[numframes * numcols * numrows] = { 0, 5, 1, 4, 2, 3 }; 
  float order2[numframes * numcols * numrows] = { 3, 0, 4, 1, 5, 2}; 


  PiPoStreamAttributes attr = PiPoStreamAttributes();
  const PiPoStreamAttributes *attrarr[1] = { &attr }; // put them in an array per track
  attr.dims[0] = numcols;
  attr.dims[1] = numrows;

  WHEN("setup called for 1 buffer")
  {
    const int sizes[1] = { numframes };
    int ret = order.setup(1, 1, sizes, attrarr);
    REQUIRE(ret >= 0);

    THEN("check")
    {
      ;
    }

    WHEN("data input")
    {
      mimo_buffer inbuf(numframes, data1, NULL, false, NULL, 0);
      order.train(0, 0, 1, &inbuf);
      
      THEN("result is")
      {
	REQUIRE(rx.output_buffers.size() == 1);
	REQUIRE(rx.output_buffers[0].numframes == numframes);

	PiPoValue *in  = data1;
	PiPoValue *out = rx.output_buffers[0].data;
	printf("\ninput\t%d %d\n\t\t%d %d\n\t\t%d %d\n", (int)  in[0], (int) in[1], (int) in[2], (int) in[3], (int) in[4], (int) in[5]);
	printf("output\t%d %d\n\t\t%d %d\n\t\t%d %d\n", (int) out[0], (int) out[1], (int) out[2], (int) out[3], (int) out[4], (int) out[5]);

	// compare with expected results
	for (int i = 0; i < numframes * numcols * numrows; i++)
	{
	  CHECK(i == i);	// display index with -s
	  CHECK(data1[i] == order0[i]);
	}
      }
    }
  }
#if 0
  WHEN("setup called for 2 buffers")
  {
    const int sizes[2] = { numframes, numframes };
    int ret = order.setup(2, 1, sizes, attrarr);
    REQUIRE(ret >= 0);

    WHEN("data input 2")
    {
      mimo_buffer inbuf[] = { mimo_buffer(numframes, data1, NULL, false, NULL, 0),
			      mimo_buffer(numframes, data2, NULL, false, NULL, 0) };
      order.train(0, 0, 2, inbuf);

      THEN("result is")
      {
	order_model_data res = *order.getmodel();

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
	  mimo_model_data *model = order.getmodel();
	  int size = model->json_size();
	  char json[size];
	  model->to_json(json, size);
	    
	  printf("\nmodel to json:\n%s\n", json);
	}
      }
    }
  }
#endif
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
