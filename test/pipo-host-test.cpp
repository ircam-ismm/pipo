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
    PiPoTestHost h;
    PiPoStreamAttributes sa;

    int sliceWindSize = 10;
    int sliceHopSize = 5;

    unsigned long nRandValues = 100;

    WHEN ("Instantiating a PiPoHost with a simple \"slice\" graph")
    {
        h.setGraph("slice");
        h.setAttr("slice.size", sliceWindSize);
        h.setAttr("slice.hop", sliceHopSize);
        h.setInputStreamAttributes(sa);

        THEN ("Output frame rate should equal input frame rate divided by hop size")
        {
            REQUIRE (h.getOutputStreamAttributes().rate == sa.rate / sliceHopSize);
        }
    }

    WHEN ("Instantiating a PiPoHost with a more complex graph")
    {
        h.setGraph("slice:fft<sum:scale,moments>");
        h.setAttr("slice.size", sliceWindSize);
        h.setAttr("slice.hop", sliceHopSize);
        h.setInputStreamAttributes(sa);

        THEN ("Labels in host's outputStreamAttributes should show expected values")
        {
            PiPoStreamAttributes &outSa = h.getOutputStreamAttributes();
            REQUIRE (std::strcmp(outSa.labels[0], "") == 0);
            REQUIRE (std::strcmp(outSa.labels[1], "Centroid") == 0);
            REQUIRE (std::strcmp(outSa.labels[2], "Spread") == 0);
            REQUIRE (std::strcmp(outSa.labels[3], "Skewness") == 0);
            REQUIRE (std::strcmp(outSa.labels[4], "Kurtosis") == 0);
        }
    }

    WHEN ("Using parallel slices with different attributes")
    {
        h.setGraph("<slice(s1):moments, slice(s2):fft:moments>");
        h.setAttr("s1.size", 20);
        h.setAttr("s1.hop", 10);
        h.setAttr("s2.size", 10);
        h.setAttr("s2.hop", 5);
        h.setInputStreamAttributes(sa);

        THEN ("Heterogeneous parallel frame rates have undefined behaviour")
        {
            // PiPoValue vals[nRandValues];

            // h.reset(); // clear received frames
            // fillArrayWithRandomValues(vals, nRandValues);

            // for (unsigned int i = 0; i < nRandValues; ++i) {
            //     h.frames(0, 0, &vals[i], 1, 1);
            // }

            // std::cout << h.receivedFrames.size() << std::endl;
            // std::cout << h.getOutputStreamAttributes().rate << std::endl;

            REQUIRE (true);
        }
    }
}
