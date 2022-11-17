/**
 * @file PiPoCollection.cpp
 * @author Norbert.Schnell@ircam.fr
 * @author joseph.larralde@ircam.fr
 *
 * @brief PiPo Module Collection
 *
 * @copyright
 * Copyright (C) 2013 - 2017 by ISMM IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 *
 * License (BSD 3-clause)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "PiPoCollection.h"

#include "PiPoOp.h"
#include "PiPoGraph.h"

#include "PiPoIdentity.h"
#include "PiPoBands.h"
#include "PiPoBayesFilter.h"
#include "PiPoBiquad.h"
#include "PiPoChop.h"
#include "PiPoConst.h"
#include "PiPoDct.h"
#include "PiPoDelta.h"
#include "PiPoDescr.h" // new name of PiPoBasic
#include "PiPoFft.h"
#include "PiPoFiniteDif.h"
#include "PiPoGate.h"
#include "PiPoLpc.h"
#include "PiPoLoudness.h"
// #include "PiPoMaximChroma.h" // << Maximilian is required to compile this
// #include "PiPoMeanStddev.h"
#include "PiPoMedian.h"
#include "PiPoMel.h"
#include "PiPoMfcc.h"
// #include "PiPoMinMax.h"
#include "PiPoMoments.h"
#include "PiPoMvavrg.h"
#include "PiPoOnseg.h"
#include "PiPoPeaks.h"
#include "PiPoPsy.h"
// #include "PiPoRms.h"
#include "PiPoScale.h"
#include "PiPoSelect.h"
#include "PiPoSegment.h"
#include "PiPoTemporalModeling.h"
#include "PiPoSlice.h"
#include "PiPoSum.h"
// #include "PiPoWavelet.h" // << boost is required to compile this
#include "PiPoYin.h"

class PiPoPool : public PiPoModuleFactory
{
  class PiPoPoolModule : public PiPoModule
  {
  private:
    PiPo *pipo;
    std::string instanceName;

  public:
    PiPoPoolModule(PiPo *pipo) {
      this->pipo = pipo;
    }

    PiPoPoolModule() {
      if(this->pipo != NULL) {
        delete this->pipo;
        this->pipo = NULL;
      }
    }
  };

public:
  PiPoPool(bool defaultPipos = true)
  {
    if(defaultPipos)
    {
      includeDefaultPiPos();
    }
  }

  ~PiPoPool()
  {
    /*for(pipoMap::iterator it = map.begin(); it != map.end(); ++it)
    {
      if(it->second != NULL) {
        delete it->second;
        it->second = NULL;
      }
    }*/
  }

  void includeDefaultPiPos()
  {
    include("_", new PiPoCreator<PiPoIdentity>);
    include("bands", new PiPoCreator<PiPoBands>);
    include("bayesfilter", new PiPoCreator<PiPoBayesFilter>);
    include("biquad", new PiPoCreator<PiPoBiquad>);
    include("chop", new PiPoCreator<PiPoChop>);
    include("const", new PiPoCreator<PiPoConst>);
    include("dct", new PiPoCreator<PiPoDct>);
    include("delta", new PiPoCreator<PiPoDelta>);
    include("descr", new PiPoCreator<PiPoDescr>); // << new PiPoBasic
    include("fft", new PiPoCreator<PiPoFft>);
    include("finitedif", new PiPoCreator<PiPoFiniteDif>);
    include("gate", new PiPoCreator<PiPoGate>);
    include("loudness", new PiPoCreator<PiPoLoudness>);
    include("lpc", new PiPoCreator<PiPoLpc>);
    // include("chroma", new PiPoCreator<PiPoMaximChroma>); // << needs Maximilian
    // include("meanstddev", new PiPoCreator<PiPoMeanStddev>);
    include("median", new PiPoCreator<PiPoMedian>);
    include("mel", new PiPoCreator<PiPoMel>);
    include("mfcc", new PiPoCreator<PiPoMfcc>);
    // include("minmax", new PiPoCreator<PiPoMinMax>);
    include("moments", new PiPoCreator<PiPoMoments>);
    include("mvavrg", new PiPoCreator<PiPoMvavrg>);
    include("onseg", new PiPoCreator<PiPoOnseg>);
    include("peaks", new PiPoCreator<PiPoPeaks>);
    include("psy", new PiPoCreator<PiPoPsy>);
    // include("rms", new PiPoCreator<PiPoRms>);
    include("scale", new PiPoCreator<PiPoScale>);
    include("select", new PiPoCreator<PiPoSelect>);
    include("segment", new PiPoCreator<PiPoSegment>);
    include("segduration", new PiPoCreator<PiPoSegDuration>);
    include("segmarker", new PiPoCreator<PiPoSegMarker>);
    include("segmean", new PiPoCreator<PiPoSegMean>);
    include("segstddev", new PiPoCreator<PiPoSegStd>);
    include("segmeanstd", new PiPoCreator<PiPoSegMeanStd>);
    include("segmin", new PiPoCreator<PiPoSegMin>);
    include("segmax", new PiPoCreator<PiPoSegMax>);
    include("segminmax", new PiPoCreator<PiPoSegMinMax>);
    include("segstats", new PiPoCreator<PiPoSegStats>);
    include("slice", new PiPoCreator<PiPoSlice>);
    include("sum", new PiPoCreator<PiPoSum>);
    include("thru", new PiPoCreator<PiPoThru>);
    // include("wavelet", new PiPoCreator<PiPoWavelet>); // << needs boost
    include("yin", new PiPoCreator<PiPoYin>);
  }

  void include(std::string name, PiPoCreatorBase *creator)
  {
      map[name] = creator;
  }

  PiPo *create(unsigned int index, const std::string &pipoName, const std::string &instanceName, PiPoModule *&module, PiPo::Parent *parent)
  {
      pipoMap::iterator it = map.find(pipoName);
      if (it == map.end())
	  return NULL;
      else
      {
	  PiPo *ret = it->second->create();
	  module = new PiPoPoolModule(ret);
	  return ret;
      }
  }

private:
  typedef std::map<std::string, PiPoCreatorBase *> pipoMap;
  pipoMap map;
};

//==========================================================//

static PiPoPool *factory;

void
PiPoCollection::init(bool defaultPipos)
{
  factory = new PiPoPool(defaultPipos);
}

void
PiPoCollection::deinit()
{
  if (factory != NULL) delete factory;
}

void
PiPoCollection::addToCollection(std::string name, PiPoCreatorBase *creator)
{
  factory->include(name, creator);
}

PiPo *
PiPoCollection::create(std::string name, PiPo::Parent *parent)
{
  PiPoGraph *graph = new PiPoGraph(parent, factory);

  if (graph->create(name))
  {
    return static_cast<PiPo *>(graph);
  }

  return NULL;
}
