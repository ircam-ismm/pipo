/* -*- mode: c; c-basic-offset:2 -*- */

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

TEST_CASE ("collection")
{
  PiPoCollection::init();

  WHEN ("Trying to instantiate a pipo graph")
  {
    /*
    PiPo *seg = PiPoCollection::create("slice:fft:sum:scale:onseg");
    PiPo *lpcf = PiPoCollection::create("lpcformants");
    //*/

    //*
    //PiPo *graph = PiPoCollection::create("slice:fft:sum:scale:onseg");
    //PiPo *graph = PiPoCollection::create("slice:fft<_,sum,moments>");
    //PiPo *graph = PiPoCollection::create("slice<_,fft<sum:scale,moments>>"); // this graph makes no sense but works
    PiPo *graph = PiPoCollection::create("slice<yin,fft<sum:scale,moments>>"); // this graph makes sense 
    //PiPo *graph = PiPoCollection::create("slice:fft<sum:scale,moments>");
    //std::shared_ptr<PiPo> graph = std::make_shared<PiPo>(PiPoCollection::create("slice<_,fft<sum:scale,moments>>"));

    //PiPo *graph = PiPoCollection::create("<sum,moments,_>");

    // Chains / modules are not null
    REQUIRE (graph != NULL);
    PiPoTestReceiver rx(NULL);

    // static_cast<PiPoGraph *>(graph)->setReceiver(&rx);
    // graph->connect(&rx);
    graph->setReceiver(&rx);

    // std::cout << graph->getNumAttrs() << std::endl;

    const int winsize = 1000;
    const int hopsize = 100;
    graph->setAttr(0, winsize); // slice.size
    graph->setAttr(1, hopsize); // slice.hop

    //std::cout << graph->getAttr((unsigned int)0)->getInt(0) << std::endl;

    int ret = graph->streamAttributes(false, 10000, 0, 1, 1, NULL, false, 0, 1);

    THEN ("Graph is ok")
    {
      CHECK(rx.sa.rate == 100);
      CHECK(rx.sa.dims[0] == 9);	// 5 columns from yin and sum:scale, 4 moments
      CHECK(rx.sa.dims[1] == 1);	// slice is column vector, so output frame is nonsensical 1000 x 6 matrix

      for (int i = 0; i < rx.sa.numLabels; i++)
      {
        std::cout << rx.sa.labels[i] << ", ";
      }
      std::cout << std::endl;
    }

    WHEN ("Sending data")
    {
      const int numsamples = 10000;
      float vals[numsamples];
      std::srand(static_cast<unsigned int>(std::time(0)));

      for (unsigned int i = 0; i < numsamples; ++i)
      {
        vals[i] = std::rand() / static_cast<float>(RAND_MAX);
      }

      //for (unsigned int i = 0; i < numsamples; ++i)
      //  graph->frames(0, 0, &vals[i], 1, 1);
      graph->frames(0, 0, &vals[0], 1, numsamples);

      THEN ("Received data is ok")
      {
	CHECK(rx.count_error == 0);
        REQUIRE(rx.count_frames > 0);
        REQUIRE(rx.size >= rx.sa.dims[0]); // size of last received frame

	// check we get the signal back
	double absmean = 0;
	for (int i = 0; i < rx.sa.dims[0]; i++)
	  absmean += fabs(rx.values[i]);
	absmean /= winsize;
	
	CHECK(absmean > 0);
	CHECK(rx.values[5]  > 0); // centroid
      }
    }
  }
}
