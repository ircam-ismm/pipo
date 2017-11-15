#ifndef _PIPO_TEST_HOST_
#define _PIPO_TEST_HOST_

#include "PiPoHost.h"

class PiPoTestHost : public PiPoHost
{
public:
    std::vector<std::vector<PiPoValue>> receivedFrames;

private:
    // this works when streamAttributesChanged is protected in PiPoHost class
    // void streamAttributesChanged(PiPo *pipo, PiPo::Attr *attr)
    // {
    //     this->propagateInputStreamAttributes();
    //     std::cout << "just re-propagated stream attributes because an attribute value changed" << std::endl;
    // }

    void onNewFrame(double time, double weight, PiPoValue *values, unsigned int size)
    {
        std::vector<PiPoValue> frame(size);

        for (unsigned int i = 0; i < size; ++i)
        {
            frame[i] = values[i];
        }

        receivedFrames.push_back(frame);
    }

public:
    void reset()
    {
        receivedFrames.resize(0);
    }
};

#endif /* _PIPO_TEST_HOST_ */
