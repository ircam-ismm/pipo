#include <iostream>

#include "catch.hpp"
//#include "PiPoTestReceiver.h"
//#include "PiPoCollection.h"
//#include "PiPoGraph.h"
#include "PiPoHost.h"

// easily create your own host derived from PiPoHost :
class MyPiPoHost : public PiPoHost
{
    void onNewFrame(double time, double weight, PiPoValue *values, unsigned int size)
    {
        std::cout << "I am a MyPiPoHost instance and I have a new frame of size " << size << std::endl;
    }
};

TEST_CASE ("Test pipo host")
{
    PiPoStreamAttributes sa;

    WHEN ("Instantiating a PiPoHost with a graph description")
    {
        MyPiPoHost h;
        h.setGraph("slice:fft<sum:scale,moments>");
        //h.setGraph("slice");

        h.setAttr("slice.size", 10); // slice.size
        h.setAttr("slice.hop", 5);  // slice.hop

        std::vector<std::string> attrNames = h.getAttrNames();
        for (int i = 0; i < attrNames.size(); ++i)
        {
            std::cout << attrNames[i] << std::endl;
        }

        h.setInputStreamAttributes(sa);
        
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        for (int i = 0; i < outSa.dims[0]; ++i)
        {
            std::cout << std::string(outSa.labels[i]) << " ";
        }
        std::cout << std::endl;

        THEN ("It should spit out expected values from its onNewFrame method")
        {
            float vals[1024];
            std::srand(static_cast<unsigned int>(std::time(0)));

            for (unsigned int i = 0; i < 1024; ++i) {
                vals[i] = std::rand() / static_cast<float>(RAND_MAX);
            }

            for (unsigned int i = 0; i < 1024; ++i) {
                h.frames(0, 0, &vals[i], 1, 1);
            }

        }
    }
}
