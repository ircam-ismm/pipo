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

#define private public /* brutally force access to private parts of graph class */
#include "PiPoGraph.h"
#undef private

TEST_CASE ("pipo-graph", "[seg]")
{
  PiPoCollection::init();

  WHEN ("Instantiate complex graph syntax causing parse failure <a><c>")
  {
    PiPo *graph = PiPoCollection::create("<thru><thru>"); // another simple graph that should be detected as sequence
    REQUIRE(graph != NULL);
    CHECK(static_cast<PiPoGraph *>(graph)->graphtype_ == PiPoGraph::sequence);
  }

  WHEN ("Instantiate complex graph syntax causing stack overflow on parsing with <a>b<c>")
  { // https://git.forum.ircam.fr/haddad/mubu/-/issues/286
    // PiPo *graph = PiPoCollection::create("<yin<select(self):scale(scf),select(selp)>,slice:fft:bands:delta:sum:scale(fsc)>thru<thru,segment:scale>"); // original graph causing stack overflow on parsing
    PiPo *graph = PiPoCollection::create("<thru>thru<thru>"); // a simple graph that caused stack overflow on parsing
    REQUIRE(graph != NULL);    // should parse and not crash
    CHECK(static_cast<PiPoGraph *>(graph)->graphtype_ == PiPoGraph::sequence);
  }

  WHEN ("Instantiate bad syntax graph descr:<chop,gate> that should return error")
  {
    PiPo *graph = PiPoCollection::create("descr:<chop,gate>"); // a graph with wrong syntax :<
    // REQUIRE(graph == NULL);    // does not parse!
    REQUIRE(graph != NULL);    // well, with the above 2 latest fixes, this does parse into the correct graph, so let's accept this syntax for now

    graph = PiPoCollection::create("<thru,thru>:thru"); // a graph with wrong syntax >:
    REQUIRE(graph == NULL);    // does not parse!
    // REQUIRE(graph != NULL);    // well, with the above 2 latest fixes, this does parse into the correct graph, so let's accept this syntax for now
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
