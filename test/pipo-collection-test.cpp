#include <cstdlib>
#include <ctime>
#include <iostream>

extern "C" {
#include <unistd.h>
}

#include "catch.hpp"
#include "PiPoTestReceiver.h"
#include "PiPoCollection.h"
#include "PiPoGraph.h"

TEST_CASE ("Test pipo collection")
{
  PiPoCollection::init();

  WHEN ("Trying to instantiate a pipo chain")
  {
    /*
    PiPo *seg = PiPoCollection::create("slice:fft:sum:scale:onseg");
    PiPo *lpcf = PiPoCollection::create("lpcformants");
    //*/
      
    //*
    //PiPo *graph = PiPoCollection::create("slice:fft:sum:scale:onseg");
    PiPo *graph = PiPoCollection::create("slice:fft<_,sum,moments>");
    //PiPo *graph = PiPoCollection::create("<sum,moments,_>");
    PiPoTestReceiver *rx = new PiPoTestReceiver(NULL);
      
    static_cast<PiPoGraph *>(graph)->connect(rx);
    // graph->connect(rx);
      
    std::cout << graph->getNumAttrs() << std::endl;
    
    graph->setAttr(0, 10); // slice.size
    graph->setAttr(1, 5);  // slice.hop
    
    std::cout << graph->getAttr((unsigned int)0)->getInt(0) << std::endl;
    
    int ret = graph->streamAttributes(false, 1, 0, 1, 1, NULL, false, 0, 1);
        
    float vals[1024];
    std::srand(static_cast<unsigned int>(std::time(0)));

    for (unsigned int i = 0; i < 1024; ++i) {
        vals[i] = std::rand() / static_cast<float>(RAND_MAX);
    }
    
    for (unsigned int i = 0; i < 1024; ++i) {
        // printf("%f\n", vals[i]);
        graph->frames(0, 0, &vals[i], 1, 1);
    }
    //*/

    THEN ("Chains / modules should be instantiated")
    {
      // Chains / modules are not null
  
      //sleep(1);

      //*
      std::cout << ret << std::endl;
      std::cout << rx->count_error << std::endl;
      std::cout << rx->sa.rate << std::endl;
      std::cout << rx->sa.dims[0] << " " << rx->sa.dims[1] << std::endl;
      
      for (int i = 0; i < rx->sa.numLabels; i++)
      {
        std::cout << rx->sa.labels[i] << " ";
      }
      std::cout << std::endl;

      std::cout << rx->count_frames << std::endl;
       
      REQUIRE (graph != NULL);
      //*/
    
      /*
      REQUIRE (seg != NULL);
      REQUIRE (lpcf != NULL);
      //*/
    }
  }
}
