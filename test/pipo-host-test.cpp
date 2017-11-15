#include <iostream>

#include "catch.hpp"
#include "PiPoTestHost.h"

static void fillArrayWithRandomValues(PiPoValue *array, unsigned long size)
{
    for (unsigned int i = 0; i < size; ++i)
    {
        std::srand(static_cast<unsigned int>(std::time(0)));

        for (unsigned int i = 0; i < size; ++i) {
            array[i] = std::rand() / static_cast<float>(RAND_MAX);
        }
    }
}

TEST_CASE ("Test pipo host")
{
    PiPoStreamAttributes sa;

    WHEN ("Instantiating a class derived from PiPoHost with a simple \"slice\" graph")
    {
        PiPoTestHost h;

        //h.setGraph("slice:fft<sum:scale,moments>");
        h.setGraph("slice");

        /*
        std::vector<std::string> attrNames = h.getAttrNames();
        for (int i = 0; i < attrNames.size(); ++i)
        {
            std::cout << attrNames[i] << std::endl;
        }
        //*/

        unsigned int windSize = 10;
        unsigned int hopSize = 5;

        h.setAttr("slice.size", windSize); // slice.size
        h.setAttr("slice.hop", hopSize);  // slice.hop

        h.setInputStreamAttributes(sa);

        THEN ("output frame rate should be input frame rate divided by hop size")
        {
            REQUIRE (h.getOutputStreamAttributes().rate == sa.rate / hopSize);
        }

        /*
        PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
        for (int i = 0; i < outSa.numLabels; ++i)
        {
            std::cout << std::string(outSa.labels[i]) << " ";
        }
        std::cout << std::endl;
        //*/

        unsigned long nRandValues = 30;

        /*
        THEN ("size of output slices should be ")
        {
            PiPoValue vals[nRandValues];

            fillArrayWithRandomValues(vals, nRandValues);

            for (unsigned int i = 0; i < nRandValues; ++i) {
                h.frames(0, 0, &vals[i], 1, 1);
            }

            std::cout << h.receivedFrames.size() << std::endl;
        }
        //*/
    }
}
