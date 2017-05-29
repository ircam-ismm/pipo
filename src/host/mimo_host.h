
#include "../PiPoHost.h"

/*

  every mimo module inherits the basic stream description and data
  passing methods (streamAttributes() and frames()) from PiPo, but
  ignores real-time oriented methods (segment()), and adds iteration
  for training.
 */

class mimo_host : public PiPoChain
{
    /* for mimo, the streamAttributes() method should give the size of the input data as maxframes */
    
    /* push data through mimo modules 
     */
    int run_training (PiPoValue *indata, int numframes)
    {
	int itercount = 0;
	size = width_ * height_;

	// first iteration on imput data, modules output passed to our mimo_receiver
	int status = iterate(0, indata, numframes);
	
	while ((this->maxiter() == 0  ||  itercount < this->maxiter())  &&  status == 0)
	{
	    itercount++;
	    status = iterate(itercount, indata, numframes);
	}
    }
	
    int iterate (int itercount, PiPoValue *data, numframes)
    {
	int status = 0;

	for (int buf = 0; buf < numbuffers; buf++)
	    for (int track = 0; track < numtracks; track++)
		if ((status = getHead()->train(itercount, buf, track, numframes, data, timetags, varsize)) != 0)
		    return status;
		    
	return status;
    }
}


/*
  reveives data iterated upon by training mimo modules
 */
class mimo_receiver : mimo
{
    PiPoValue *outputdata_;

    int frames (PiPoValue *values)
    {
	outputdata_ = values;
    }

    // store or merge input data transformed by one iteration of training 
    int train ()
    {
    }
}
