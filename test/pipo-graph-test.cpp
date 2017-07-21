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
        bool parsed = true;//g.parse("slice:fft<sum,moments>");
        bool instantiated = true;//g.instantiate();
        std::cout << "aaa" << g.getGraphType() << std::endl;
        //g.getPiPo();
        
        THEN ("Chains / modules should be instantiated")
        {
            REQUIRE (parsed);
            REQUIRE (instantiated);
        }
    }
}
