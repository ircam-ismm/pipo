
#ifndef _MAX_MIMO_HOST_H_
#define _MAX_MIMO_HOST_H_

#include "mimo.h"
#include "MaxPiPoHost.h"

class MaxMimoHost : public MaxPiPoHost
{
public:
  MaxMimoHost (t_object *ext)
  : MaxPiPoHost(ext, "mimo")
  {
  }
    
  Mimo *set_module (const char* name, Mimo *receiver = NULL);
};

#endif // _MAX_MIMO_HOST_H_

