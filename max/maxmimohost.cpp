
#include "maxmimohost.h"

Mimo *MaxMimoHost::set_module (const char *str, Mimo *receiver)
{
  this->chain.clear();
  
  if (this->chain.parse(str) > 0 && this->chain.instantiate() && this->chain.connect(receiver))
    return dynamic_cast<Mimo *>(this->chain.getHead());
  else
    return NULL;
}
