// -*- mode: c++; c-basic-offset:2 -*-
#ifndef _PIPO_TEST_HOST_
#define _PIPO_TEST_HOST_

#include <float.h>
#include "PiPoHost.h"

class PiPoTestHost : public PiPoHost
{
public:
  // capture output frames
  std::vector<double>			received_times_;
  std::vector<std::vector<PiPoValue>> receivedFrames;

  // capture arguments of last call of frames()
  // TODO: capture all
  double  last_time = DBL_MAX;
  int     last_size = -1;
  double  end_time  = DBL_MAX;

  // count messages from pipos
  unsigned int count_error = 0;
  unsigned int count_warning = 0;
  unsigned int count_finalize = 0;

  // capture errors
  std::string last_error;
  std::string last_warning;

private:
  // this works when streamAttributesChanged is protected in PiPoHost class
  // void streamAttributesChanged(PiPo *pipo, PiPo::Attr *attr)
  // {
  //     this->propagateInputStreamAttributes();
  //     std::cout << "just re-propagated stream attributes because an attribute value changed" << std::endl;
  // }

  void onNewFrame (double _time, double _weight, PiPoValue *_values, unsigned int _size) override
  {
    last_time   = _time;
    last_size   = _size;
    std::vector<PiPoValue> frame(_values, _values + _size); // copy values via input iterator
    receivedFrames.emplace_back(frame);
    received_times_.emplace_back(_time);
  }

  void onFinalize (double time) override
  {
    count_finalize++;
    end_time = time;
  }

  /** called by pipo to signal error in parameters */
  virtual void signalError(PiPo *pipo, std::string errorMsg) override
  {
    count_error++;
    last_error = errorMsg;
    printf("\n!!!!!!!!!! ERROR !!!!!!!!!! PiPoTestHost::signalError: %s\n", errorMsg.c_str());
  }

  /** called by pipo to signal warning in parameters */
  virtual void signalWarning(PiPo *pipo, std::string errorMsg) override
  {
    count_warning++;
    last_warning = errorMsg;
    printf("\n!!!!!!!!!! WARNING !!!!!!!!!! PiPoTestHost::signalWarning: %s\n", errorMsg.c_str());
  }

public:
  void reset()
  {
    receivedFrames.resize(0);
  }
};

#endif /* _PIPO_TEST_HOST_ */
