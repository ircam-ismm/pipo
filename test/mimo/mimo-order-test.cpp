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
  float data1[numframes * numcols * numrows] = {  2,  5,
						  3,  6,
						  1,  4 };
  float data2[numframes * numcols * numrows] = { 10, -3,
						 33, -2,
						 22, -1 };
  // result for one buffer
  float order0[numframes * numcols * numrows] = { 1,  1,
						  2,  2,
						  0,  0 }; 
  // result for two buffers
  float order1[numframes * numcols * numrows] = { 1,  4,
						  2,  5,
						  0,  3 }; 
  float order2[numframes * numcols * numrows] = { 3,  0,
						  5,  1,
						  4,  2 }; 

  PiPoStreamAttributes attr = PiPoStreamAttributes();
  const PiPoStreamAttributes *attrarr[1] = { &attr }; // put them in an array per track
  attr.dims[0] = numcols;
  attr.dims[1] = numrows;

  WHEN("called for 1 buffer")
  {
    const int sizes[1] = { numframes };
    int ret = order.setup(1, 1, sizes, attrarr);
    REQUIRE(ret >= 0);

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
#	define printrow(str, arr, ind) printf("%s\t%2d %2d\n", str, (int) arr[ind * 2], (int) arr[ind * 2 + 1])
#	define printarr(str, arr) for(int i = 0; i < numframes; i++) { printrow(i == 0 ? str : "\t", arr, i); }
	
	printarr("\ninput",  in);
	printarr("output", out);

	// compare with expected results
	for (int i = 0; i < numframes * numcols * numrows; i++)
	{
	  CHECK(i == i);	// display index with -s
	  CHECK(out[i] == order0[i]);
	}
      }
    }
  }

  WHEN("called for 2 buffers")
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
	REQUIRE(rx.output_buffers.size() == 2);
	REQUIRE(rx.output_buffers[1].numframes == numframes);

	PiPoValue *out1 = rx.output_buffers[0].data;
	PiPoValue *out2 = rx.output_buffers[1].data;

	printarr("\ninput1", data1);
	printarr("input2",   data2);
	printarr("output1",  out1);
	printarr("output2",  out2);

	// compare with expected results
	for (int i = 0; i < numframes * numcols * numrows; i++)
	{
	  CHECK(i == i);	// display index with -s
	  CHECK(out1[i] == order1[i]);
	  CHECK(out2[i] == order2[i]);
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
