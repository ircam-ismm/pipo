/**
 * @file PiPoWavelet.h
 * @author jules.francoise@ircam.fr
 *
 * @brief PiPo calculating wavelets
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 * 
 * License : GPL-v3
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef maxpipo_PiPoWavelet_h
#define maxpipo_PiPoWavelet_h

#include "PiPo.h"
#include "wavelet_all.hpp"
#include <cmath>

using namespace std;

class PiPoWavelet : public PiPo
{
protected:
  std::vector<wavelet::Filterbank> filterbank;
  enum OutputMode { Power, Complex };
  enum OutputMode outputMode;
  enum RescaleMode { RescaleDisabled, RescaleEnabled };
  enum RescaleMode rescaleMode;
  
public:
  PiPoScalarAttr<float> bandsperoctave;
  PiPoScalarAttr<float> minfreq;
  PiPoScalarAttr<float> maxfreq;
  PiPoScalarAttr<float> omega0;
  PiPoScalarAttr<float> delay;
  PiPoScalarAttr<PiPo::Enumerate> optimisation;
  PiPoScalarAttr<PiPo::Enumerate> mode;
  PiPoScalarAttr<PiPo::Enumerate> rescale;

  PiPoWavelet(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  bandsperoctave(this, "bandsperoctave", "number of bands per octave", true, 4.),
  minfreq(this, "minfreq", "minimum frequency (Hz)", true, 0.1),
  maxfreq(this, "maxfreq", "maximum frequency (Hz)", true, 50.),
  omega0(this, "omega0", "[Morlet] carrier frequency (z)", true, 5.),
  delay(this, "delay", "Delay (proportional to the wavelet's critical time)", true, 1.5),
  optimisation(this, "optimisation", "Optimisation of the transform", true, wavelet::Filterbank::STANDARD),
  mode(this, "mode", "Output mode", true, Power),
  rescale(this, "rescale", "Rescale Scalogram", true, RescaleEnabled)
  {
    this->optimisation.addEnumItem("none", "No optimisation");
    this->optimisation.addEnumItem("standard", "Standard optimisation (wavelet decimation)");
    this->optimisation.addEnumItem("agressive", "Agressive optimisation (wavelet + signal decimation)");
    this->mode.addEnumItem("power", "Power Spectrum");
    this->mode.addEnumItem("complex", "Complex Spectrum");
    this->rescale.addEnumItem("no", "No rescaling");
    this->rescale.addEnumItem("yes", "Rescaling with by scale length");
    
    this->outputMode = static_cast<OutputMode>(this->mode.get());
    this->rescaleMode = static_cast<RescaleMode>(this->rescale.get());
  }

  ~PiPoWavelet(void) {}

  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    filterbank.resize(width, wavelet::Filterbank(rate, this->minfreq.get(), this->maxfreq.get(), this->bandsperoctave.get()));
    
    if (filterbank[0].getAttribute<float>("samplerate") != rate)
    {
      for (auto &bank : filterbank)
        bank.setAttribute<float>("samplerate", rate);
    }

    try {
      for (auto &bank : filterbank)
      {
        bank.setAttribute<float>("bands_per_octave", this->bandsperoctave.get());
        bank.setAttribute<float>("frequency_min", this->minfreq.get());
        bank.setAttribute<float>("frequency_max", this->maxfreq.get());
        bank.setAttribute<float>("omega0", this->omega0.get());
        bank.setAttribute<float>("delay", this->delay.get());
        bank.setAttribute<bool>("rescale", (this->rescale.get() == RescaleDisabled) ? false : true);
        bank.setAttribute<wavelet::Filterbank::Optimisation>("optimisation", static_cast<wavelet::Filterbank::Optimisation>(this->optimisation.get()));
      }
    } catch (exception &e){
      std::cout << "Error: " << e.what() << std::endl;
      signalError(e.what());
    }

    this->bandsperoctave.set(0, filterbank[0].getAttribute<float>("bands_per_octave"), true);
    this->minfreq.set(0, filterbank[0].getAttribute<float>("frequency_min"), true);
    this->maxfreq.set(0, filterbank[0].getAttribute<float>("frequency_max"), true);
    this->omega0.set(0, filterbank[0].getAttribute<float>("omega0"), true);
    this->delay.set(0, filterbank[0].getAttribute<float>("delay"), true);
    this->optimisation.set(0, (int)filterbank[0].getAttribute<wavelet::Filterbank::Optimisation>("optimisation"), true);

    this->outputMode = static_cast<OutputMode>(this->mode.get());
    this->rescaleMode = static_cast<RescaleMode>(this->rescale.get());

    if (this->outputMode == Power) {
      return this->propagateStreamAttributes(hasTimeTags, rate, offset, filterbank[0].size(), height, NULL, 0, 0.0, 1);
    } else {
      return this->propagateStreamAttributes(hasTimeTags, rate, offset, filterbank[0].size() * 2, height, NULL, 0, 0.0, 1);
    }
  }

  int reset(void) {
    for (auto &bank : filterbank) {
      bank.reset();
    }
    return this->propagateReset();
  };

  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    unsigned int numbands = filterbank[0].size();
    for (unsigned int i = 0; i < num; i++)
    {
      std::vector<PiPoValue> result(numbands * 2, 0.0);
      for (unsigned int dimension = 0; dimension < size; dimension++)
      {
        filterbank[dimension].update(values[dimension]);
        for (unsigned int t = 0; t < numbands; t++)
        {
          switch (outputMode)
          {
            case Power:
              result[t] += filterbank[dimension].result_power[t] / float(size);
              break;

            case Complex:
              result[2 * t] += filterbank[dimension].result_complex[t].real() / float(size);
              result[2 * t + 1] += filterbank[dimension].result_complex[t].imag() / float(size);
              break;

            default:
              break;
          }
        }
      }
      int ret;
      if (this->outputMode == Power)
        ret = this->propagateFrames(time, weight, &result[0], filterbank[0].size(), 1);
      else
        ret = this->propagateFrames(time, weight, &result[0], filterbank[0].size() * 2, 1);
      
      if (ret != 0) return ret;

      values += size;
    }

    return 0;
  }
};

#endif
