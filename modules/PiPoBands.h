/**
 * @file PiPoBands.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief RTA bands PiPo
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_BANDS_
#define _PIPO_BANDS_

#include <algorithm>
#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_complex.h"
#include "rta_bands.h"
#include "rta_mel.h"
#include <float.h>
#include <math.h>
}

#include <vector>
//#include <complex>

#ifdef WIN32
static float cabsf(floatcomplex value)
{
	_complex cval = { value.real, value.imag };
	return (float)_cabs(cval);
}
#endif

class PiPoBands : public PiPo
{
public:
  enum BandsModeE { UndefinedBands = -1, MelBands = 0, HtkMelBands = 1 }; //todo: bark, erb
  enum EqualLoudnessModeE { None = 0, Hynek = 1 };

private:
  std::vector<PiPoValue> bands;
  std::vector<float> weights;
  std::vector<unsigned int> bounds;
  std::vector<float> bandfreq;	// band centre frequency in Hz
  std::vector<float> eqlcurve;	// equal loudness curve
  std::vector<float> power_spectrum;

  enum BandsModeE bandsMode;
  enum EqualLoudnessModeE eqlMode;
  unsigned int specSize;
  bool complex_input;
  float sampleRate;

public:
  PiPoScalarAttr<PiPo::Enumerate> mode;
  PiPoScalarAttr<PiPo::Enumerate> eqlmode;
  PiPoScalarAttr<int> num;
  PiPoScalarAttr<bool> log;
  PiPoScalarAttr<float> power;

  PiPoBands(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  bands(), weights(), bounds(), bandfreq(),
  mode(this, "mode", "Bands Mode", true, MelBands),
  eqlmode(this, "eqlmode", "Equal Loudness Curve", true, None),
  num(this, "num", "Number Of Bands", true, 24),
  log(this, "log", "Logarithmic Bands", false, true),
  power(this, "power", "Power Scaling Exponent", false, 1.)
  {
    this->bandsMode = UndefinedBands;
    this->eqlMode = None;

    this->specSize = 0;
    this->complex_input = false;
    this->sampleRate = 1.0;

    this->mode.addEnumItem("mel", "MEL bands (normalized band energy)");
    this->mode.addEnumItem("htkmel", "HTK like MEL bands (preserved peak energy)");

    this->eqlmode.addEnumItem("none", "no equal loudness scaling");
    this->eqlmode.addEnumItem("hynek", "Hynek's equal loudness curve");
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int size, const char **labels,
                       bool hasVarSize, double domain, unsigned int maxFrames)
  {
    enum BandsModeE bandsMode = static_cast<BandsModeE>(this->mode.get());
    enum EqualLoudnessModeE eqlMode = static_cast<EqualLoudnessModeE>(this->eqlmode.get());
    int numBands = std::max(1, this->num.get());
    int specSize = size;
    float sampleRate = 2.0 * domain;

    if (width >= 2)
    {
      complex_input = true;
      power_spectrum.resize(specSize);
    }

    if (bandsMode < MelBands)
      bandsMode = MelBands;
    else if (bandsMode > HtkMelBands)
      bandsMode = HtkMelBands;

    if (numBands < 1)
      numBands = 1;

    if (specSize < 0)
      specSize = 0;

    if (bandsMode != this->bandsMode || eqlMode != this->eqlMode ||
        numBands != this->bands.size() || specSize != this->specSize ||
        sampleRate != this->sampleRate)
    {
      this->bands.resize(numBands);
      this->eqlcurve.resize(numBands);
      this->weights.resize(specSize * numBands);
      this->bounds.resize(2 * numBands);
      this->bandfreq.resize(numBands);

      this->bandsMode = bandsMode;
      this->eqlMode = eqlMode;
      this->specSize = specSize;
      this->sampleRate = sampleRate;

      switch (bandsMode)
      {
        default:
        case MelBands:
        {
          rta_spectrum_to_mel_bands_weights(&this->weights[0], &this->bounds[0], specSize,
                                            sampleRate, numBands, 0.0, domain, 1.0,
                                            rta_hz_to_mel_slaney, rta_mel_to_hz_slaney, rta_mel_slaney);

          // calculate band centre freqs (TODO: pass up from rta_spectrum_to_mel_bands_weights)
          for (int i = 0; i < numBands; i++)
          {
            double b = (bounds[2 * i] + bounds[2 * i + 1]) / 2.; // take mean as band centre freq
            bandfreq[i] = b / (double) specSize * sampleRate / 2. ; // in Hz
          }
          break;
        }

        case HtkMelBands:
        {
          rta_spectrum_to_mel_bands_weights(&this->weights[0], &this->bounds[0], specSize,
                                            sampleRate, numBands, 0.0, domain, 1.0,
                                            rta_hz_to_mel_htk, rta_mel_to_hz_htk, rta_mel_htk);

          // calculate band centre freqs (TODO: pass up from rta_spectrum_to_mel_bands_weights)
          for (int i = 0; i < numBands; i++)
          {
            double b = (bounds[2 * i] + bounds[2 * i + 1]) / 2.; // take mean as band centre freq
            bandfreq[i] = b / (double) specSize * sampleRate / 2. ; // in Hz
          }
          break;
        }
          /*
           case ERBBands:
           rta_spectrum_to_erb_bands_weights(&weights[0], &bounds[0], &bandfreq[0], specSize,
           sampleRate, numBands);
           break;
           */
      }

      switch (this->eqlmode.get())
      {
        case Hynek:

          for (int i = 0; i < numBands; i++)
          { // Hynek's equal-loudness-curve formula
            double fsq  = bandfreq[i] * bandfreq[i];
            double ftmp = fsq / (fsq + 1.6e5);
            eqlcurve[i] = (ftmp * ftmp) * ((fsq + 1.44e6) / (fsq + 9.61e6));
          }
          break;

        default: /* curve will not be applied */
          break;
      }
    }

#if (DEBUG * 0)
    printf("PiPoBands::streamAttributes  timetags %d  rate %.0f  offset %f  width %d  size %d  "
           "labels %s  varsize %d  domain %f  maxframes %d\nrta_real_t size = %d\n",
           hasTimeTags, rate, offset, (int) width, (int) size, labels ? labels[0] : "n/a",
           (int) hasVarSize, domain, (int) maxFrames, sizeof(rta_real_t));
    static FILE *filtout = fopen("/tmp/melfilter.raw", "w");
    fwrite(&weights[0], weights.size(), sizeof(float), filtout);
    static FILE *bout = fopen("/tmp/melbounds.raw", "w");
    fwrite(&bounds[0], bounds.size(), sizeof(int), bout);
#endif

    return this->propagateStreamAttributes(hasTimeTags, rate, offset, numBands, 1, NULL, 0, 0.0, 1);
  }

  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    unsigned int numBands = (unsigned int) this->bands.size();
    bool log = this->log.get();
    float p = this->power.get();
    float *spectrum;
    int specsize = size;

    for (unsigned int i = 0; i < num; i++)
    {
      float scale = 1.0;

      switch (this->bandsMode)
      {
        default:
        case MelBands:
        {
          scale *= 66519.0 / numBands;
          break;
        }
        case HtkMelBands:
        {
          break;
        }
      }

      if (complex_input)
      { // convert to power spectrum
        specsize = (unsigned int) power_spectrum.size();
        spectrum = &(power_spectrum[0]);

        for (int i = 0; i < specsize; i++)
          spectrum[i] = cabsf(((rta_complex_t *) values)[i]);

#if (DEBUG * 0)
        static FILE *specout = fopen("/tmp/powerspectrum.raw", "w");
        fwrite(spectrum, sizeof(float), specsize, specout);
#endif
      }
      else
        spectrum = values;

      /* calculate MEL bands */
      rta_spectrum_to_bands_abs(&this->bands[0], spectrum,
                                &this->weights[0], &this->bounds[0],
                                specsize, numBands);

      /* apply equal loudness curve*/
      if (this->eqlmode.get() != None)
        for (unsigned int j = 0; j < numBands; j++)
          this->bands[j] *= this->eqlcurve[j];

      if (log)
        scale *= numBands;

      if (scale != 1.0)
        for(unsigned int j = 0; j < numBands; j++)
          this->bands[j] *= scale;

      if (log)
      {
        const double minLogValue = 1e-48;
        const double minLog = -480.0;

        /* calculate log output values */
        for (unsigned int i = 0; i < numBands; i++)
        {
          float band = this->bands[i];

          if (band > minLogValue)
            this->bands[i] = 10.0 * log10f(band);
          else
            this->bands[i] = minLog;
        }
      }

      if (p != 1)
        for (unsigned int j = 0; j < numBands; j++)
          this->bands[j] = powf(this->bands[j], p);

      int ret = this->propagateFrames(time, weight, &this->bands[0], numBands, 1);

      if (ret != 0)
        return ret;

      values += size;
    }

    return 0;
  }
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-file-style: "stroustrup"
 * c-basic-offset:2
 * End:
 */

#endif
