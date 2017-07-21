
#include <cmath>
#include "PiPo.h"

/** test instrument pipo that records the last calls to be checked against expected values
 */
class PiPoTestReceiver : public PiPo, public PiPo::Parent
{
public:
  // count calls to functions
  int		count_streamAttributes;
  int		count_reset;
  int		count_frames;
  int		count_finalize;
  int		count_error;
  int		count_warning;

  // capture errors
  const char	*last_error;
  const char	*last_warning;

  // capture last call of streamAttributes()
  PiPoStreamAttributes sa;
  std::vector<std::string> labelstore;	// temp storage for label strings

  // capture last call of frames()
  double	time;
  PiPoValue	*values;
  int		size;
  double	end_time;
  int		count_invalid;	// check for nan, inf, etc.
  
public:
  void zero ()
  {
    // reset all counters
    count_streamAttributes = 0;
    count_reset = 0;
    count_frames = 0;
    count_finalize = 0;
    count_error = 0;
    count_warning = 0;
    count_invalid = 0;
    
    //TODO: set all capture fields to guard values
    sa.numLabels = 0;
    if (sa.labels)
    {
      free(sa.labels);
      sa.labels = NULL;
    }

    if (values)
    {
      free(values);
      values = NULL;
    }
}
  
  PiPoTestReceiver (PiPo::Parent *parent)
  : PiPo(parent)
  {
    values = NULL;
    sa.labels = NULL;
    zero();
  }

  ~PiPoTestReceiver ()
  {  }


  virtual int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    count_streamAttributes++;
    sa.hasTimeTags = hasTimeTags;
    sa.rate = rate;
    sa.offset = offset;
    sa.dims[0] = width;
    sa.dims[1] = height;
    sa.hasVarSize = hasVarSize;
    sa.domain = domain;
    sa.maxFrames = maxFrames;

    // copy labels
    sa.numLabels = labels ? width : 0;
    sa.labels = (const char **) malloc(sa.numLabels * sizeof(char *));
    labelstore.resize(sa.numLabels);
    
       //printf("%i %i\n", sa.dims[0], sa.dims[1]);
       printf("%i %i\n", width, height);

      for (int i = 0; i < sa.numLabels; i++)
    {
      labelstore[i] = std::string(labels[i]);
      sa.labels[i] = labelstore[i].c_str();
    }
    
    return 0;
  }

  virtual int frames (double _time, double _weight, PiPoValue *_values, unsigned int _size, unsigned int _num)
  {
    count_frames++;
    time   = _time;
    size   = _size;
    if (values)
      values = (PiPoValue *) realloc(values, size * sizeof(PiPoValue));
    else
      values = (PiPoValue *) malloc(size * sizeof(PiPoValue));
    memcpy(values, _values, size * sizeof(PiPoValue));

    for (int i = 0; i < size; i++)
      count_invalid += !std::isfinite(_values[i]);
    
      // printf("new frame\n");
      
    return 0;
  }

  int finalize (double inputEnd)
  {
    count_finalize++;
    end_time = inputEnd;
    return 0;
  }

  // parent methods
  /** called by pipo when an attribute with "changesstream" is set */
  //virtual void streamAttributesChanged(PiPo *pipo, PiPo::Attr *attr) { };
    
  /** called by pipo to signal error in parameters */
  virtual void signalError(PiPo *pipo, std::string errorMsg)
  {
      count_error++;
      last_error = errorMsg.c_str();
      printf("error: PiPoTestReceiver::signalError: %s\n", errorMsg.c_str());
  }

  /** called by pipo to signal warning in parameters */
  virtual void signalWarning(PiPo *pipo, std::string errorMsg)
  {
      count_warning++;
      last_warning = errorMsg.c_str();
      printf("warning: PiPoTestReceiver::signalWarning: %s\n", errorMsg.c_str());
  }
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
