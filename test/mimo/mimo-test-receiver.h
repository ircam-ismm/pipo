

#include "mimo.h"
#include "PiPoTestReceiver.h"

class MimoTestReceiver : public Mimo
{
public:
  std::vector<mimo_buffer>	      output_buffers; // capture training output buffers
  std::vector<std::vector<PiPoValue>> output_data;    // capture training output data arrays for output_buffers
  size_t			      framesize = 0;

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
    framesize = at->dims[0] * at->dims[1];

    char str[1000];
    printf("%s: received mimo setup output stream attributes\n%s", __PRETTY_FUNCTION__, at->to_string(str, 1000));

    // store stream attributes in outputTrackDescr via base PiPoProcReceiver's method
    return 0; // propagateStreamAttributes(at->hasTimeTags, at->rate, at->offset, at->dims[0], at->dims[1], at->labels, at->hasVarSize, at->domain, at->maxFrames);
  }

  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[]) override
  {
    printf("%s: count %d trackindex %d, received %d mimo training output buffers\n%s", __PRETTY_FUNCTION__, itercount, trackindex, numbuffers);    

    output_buffers.resize(numbuffers);
    output_data.resize(numbuffers);

    for (int i = 0; i < numbuffers; i++)
    {
	output_data[i].assign(buffers[i].data, buffers[i].data + buffers[i].numframes * framesize);	// copy buffer data array
	output_buffers[i] = buffers[i];							// shallow copy of mimo_buffer struct
	output_buffers[i].data = &(output_data[i][0]);					// copy buffer data pointer
    }
    return 0;
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
