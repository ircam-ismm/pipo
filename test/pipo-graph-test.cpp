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

TEST_CASE ("pipo-graph", "[seg]")
{
  PiPoCollection::init();

  WHEN ("Instantiate bad syntax graph descr:<chop,gate>")
  {
    PiPo *graph = PiPoCollection::create("descr:<chop,gate>"); // a graph with wrong syntax :<
    REQUIRE(graph == NULL);    // does not parse!
  }

  const int numsamples = 10000;
  float vals[numsamples];
  std::srand(static_cast<unsigned int>(std::time(0)));
  
  for (unsigned int i = 0; i < numsamples; ++i)
  {
    vals[i] = std::rand() / static_cast<float>(RAND_MAX);
  }

  WHEN ("Instantiate simple pipo graph slice:fft")
  {
    PiPo *graph = PiPoCollection::create("slice:fft"); // a graph with simple modules
    REQUIRE(graph != NULL);    // chains / modules are not null
    CHECK(graph->getNumAttrs() > 0);

    graph->getAttr("slice", "hop")->set(0, 100);
    graph->getAttr("slice", "size")->set(0, 1024);
      
    PiPoTestReceiver rx(NULL);
    graph->setReceiver(&rx);

    // like audio input
    int ret = graph->streamAttributes(false, 10000, 0, 1, 1, NULL, false, 0, 1);

    THEN ("Graph is ok")
    {
      CHECK(rx.sa.rate == 100);
      CHECK(rx.sa.dims[0] == 1);
      CHECK(rx.sa.dims[1] == 513); // expect column vector of fftsize / 2 + 1

      for (int i = 0; i < rx.sa.numLabels; i++)
      {
        std::cout << rx.sa.labels[i] << ", ";
      }
      std::cout << std::endl;
    }

    WHEN ("Sending data")
    {
      graph->frames(0, 0, vals, 1, numsamples);

      THEN ("Received data is ok")
      {
	CHECK(rx.count_error == 0);
        REQUIRE(rx.count_frames > 0);

	// check we get the signal back
	double absmean = 0;
	for (int i = 0; i < rx.sa.dims[0]; i++)
	  absmean += fabs(rx.values[i]);
	absmean /= rx.sa.dims[0];
	
	CHECK(absmean > 0);
      }
    }
  }
  
  WHEN ("Instantiate complex pipo graph descr:chop")
  {
    PiPo *graph = PiPoCollection::create("descr:chop"); // a graph with complex module descr
    REQUIRE(graph != NULL);    // chains / modules are not null
    CHECK(graph->getNumAttrs() > 0);

    graph->getAttr("descr", "hopsize")->set(0, 100);
    graph->getAttr("chop",  "mean")->set(0, 1);
      
    PiPoTestReceiver rx(NULL);
    graph->setReceiver(&rx);

    // like audio input
    int ret = graph->streamAttributes(false, 10000, 0, 1, 1, NULL, false, 0, 1);

    THEN ("Graph is ok")
    {
      CHECK(rx.sa.rate == 100);
      CHECK(rx.sa.dims[0] == 9);
      CHECK(rx.sa.dims[1] == 1); 

      for (int i = 0; i < rx.sa.numLabels; i++)
      {
        std::cout << rx.sa.labels[i] << ", ";
      }
      std::cout << std::endl;
    }

    WHEN ("Sending data")
    {
      //for (unsigned int i = 0; i < numsamples; ++i) 
      //  graph->frames(0, 0, &vals[i], 1, 1);
      graph->frames(0, 0, vals, 1, numsamples);

      THEN ("Received data is ok")
      {
	CHECK(rx.count_error == 0);
        REQUIRE(rx.count_frames > 0);

	// check we get the signal back
	double absmean = 0;
	for (int i = 0; i < rx.sa.dims[0]; i++)
	  absmean += fabs(rx.values[i]);
	absmean /= rx.sa.dims[0];
	
	CHECK(absmean > 0);
	CHECK(rx.values[0] > 0); // pitch
	CHECK(rx.values[1] > 0); // periodicity
	CHECK(rx.values[2] > 0); // energy
	CHECK(rx.values[3] > 0); // ac1
	CHECK(rx.values[4] < 0); // Loudness
	CHECK(rx.values[5] > 0); // centroid
      }
    }
  }
}
