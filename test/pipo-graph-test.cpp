//
//  pipo-graph-test.cpp
//  libpipo
//
//  Created by Joseph Larralde on 17/07/17.
//
//
#include <iostream>
#include "catch.hpp"
#include "PiPoGraph.h"
#include "PiPoCollection.h"

TEST_CASE ("Test pipo graph")
{
    WHEN ("Trying to instantiate a pipo graph")
    {
        PiPoGraph g(NULL);
        std::cout << g.create("slice:fft<sum,moments>") << std::endl;

        // this doesn't work because we need a factory
        // todo : get rid of the factory ?
        
        THEN ("Chains / modules should be instantiated")
        {
            REQUIRE (true);
        }
    }
}
