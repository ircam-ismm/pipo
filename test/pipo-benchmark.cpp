/*
test pipos rms, fft, mfcc
on window sizes 256, 1024, 4096 without overlap, dropping last incomplete frame 
on 1s of sound "drumloop"

result: num. complete analyses per second

hayai bench rig
http://www.bfilipek.com/2016/01/micro-benchmarking-libraries-for-c.html#hayai-library

js reference code:

function getFftSuites(buffer, bufferLength, sampleRate, log) {

  const suites = [256, 1024, 4096].map((frameSize) => {

    return function(next) {
      const numFrames = Math.floor(bufferLength / frameSize);
      const suite = new Benchmark.Suite();

      const fft = new lfo.operator.Fft({
        window: 'hamming',
        mode: 'magnitude',
        size: frameSize,
      });

      fft.initStream({ frameSize: frameSize, frameType: 'signal' });

      suite.add(`lfo:fft - frameSize: ${frameSize}`, {
        defer: true,
        fn: function(deferred) {
          for (let i = 0; i < numFrames; i++) {
            const start = i * frameSize;
            const end = start + frameSize;
            const frame = buffer.subarray(start, end);
            const res = fft.inputSignal(frame);
          }

          deferred.resolve();
        },
      });

      suite.run({ async: true });
    }

  });

  return suites;
}

*/

#include <math.h>
#include "hayai.hpp"
#include "pipo.h"
#include "PiPoFft.h"
#include "PiPoMfcc.h"

class PiPoRMS : public PiPo
{
public:  
  PiPoRMS (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver)
  { }
  
    int streamAttributes (bool hasTimeTags, double rate, double offset,
			  unsigned int width, unsigned int height,
			  const char **labels, bool hasVarSize,
			  double domain, unsigned int maxFrames)
	{
	    return propagateStreamAttributes(hasTimeTags, rate, offset, 1, 1,
					     labels, false, domain, 1);
	}
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
	{
	    for (int frame = 0; frame < num; frame++)
	    {
		double sum = 0;
	    
		for (int i = 0; i < size; i++)
		    sum += values[i] * values[i];
		
		sum /= size;
	    
		float rms = sqrt(sum);
		propagateFrames(time, weight, &rms, 1, num);

		values += size;
	    }

          return 0;
	}

};



    
class pipo_bench : public ::hayai::Fixture
{
public:
  
  virtual void SetUp ()
  {
    buffer_ = NULL;
    numsamps_ = 0;
  
#if 0
    memset(&sfinfo_, 0, sizeof(SF_INFO));
    sfhandle = NULL;

    sfhandle_ = sf_open(filename, SFM_READ, &sfinfo_);
    
    numframes    = sfinfo_.frames;
    numchannels  = sfinfo_.channels;
    samplerate_  = sfinfo_.samplerate;

    if (sfhandle_ != NULL)
      {
	numsamps_ = samplerate_; // 1 second of audio
	buffer_   = (float *) malloc(numsamps_ * sizeof(float));
      
	sf_read_float(sfhandle_, buffer_, numsamps_);

	sf_close(sfhandle_);
	sfhandle_ = NULL;
      }
#else
    samplerate_ = 44100;
    numsamps_ = samplerate_; // 1 second of audio
    buffer_   = (float *) malloc(numsamps_ * sizeof(float));

    for (int i = 0; i < numsamps_; i++)
      buffer_[i] = random() / (1 << 30) - 1.0;	// returns successive pseudo-random numbers in the range from 0 to (2**31)-1. 

#endif
  }

  virtual void TearDown ()
  {
    if (buffer_)
      free(buffer_);
  }
    
  void run (int winsize, PiPo *pipo)
  {
    int numframes = numsamps_ / winsize;	// rounded down
    double framerate = (float) samplerate_ / (float) winsize;	// not rounded
    double domain = winsize / samplerate_;
    
#if 0
    pipo->streamAttributes(false, framerate, 0, 1, 1, NULL, 0, domain, numframes);
    pipo->frames(0, 0, buffer_ , winsize, numframes);
#else
    pipo->streamAttributes(false, framerate, 0, 1, 1, NULL, 0, winsize / samplerate_, numframes);

    for (int i = 0; i < numframes; i++)
      pipo->frames(i * domain, 0, buffer_ + i * winsize, winsize, 1);
#endif
  }
  
private:
#if 0
  SF_INFO sfinfo;
  SNDFILE *sfhandle;
#endif
  int numsamps_;
  float *buffer_;
  int samplerate_;
  PiPo *pipo_;
};

#define NUMITER 500


PiPo     *piporms  = new PiPoRMS(NULL);
PiPoFft  *pipofft  = new PiPoFft(NULL);
PiPoMfcc *pipomfcc = new PiPoMfcc(NULL);

BENCHMARK_F (pipo_bench, FramesRMS1, 10, NUMITER) {  run(256,  piporms);  }
BENCHMARK_F (pipo_bench, FramesRMS2, 10, NUMITER) {  run(1024, piporms);  }
BENCHMARK_F (pipo_bench, FramesRMS3, 10, NUMITER) {  run(4096, piporms);  }

BENCHMARK_F (pipo_bench, FramesFFT1, 10, NUMITER) { pipofft->size.set(256);  run(256,  pipofft);  }
BENCHMARK_F (pipo_bench, FramesFFT2, 10, NUMITER) { pipofft->size.set(1024);  run(1024, pipofft);  }
BENCHMARK_F (pipo_bench, FramesFFT3, 10, NUMITER) { pipofft->size.set(4096);  run(4096, pipofft);  }

BENCHMARK_F (pipo_bench, setupmfcc, 1, 1) { pipomfcc->dct.order.set(13); pipomfcc->hop.set(4100); }
BENCHMARK_F (pipo_bench, FramesMFCC1, 10, NUMITER) { pipomfcc->size.set(256);  run(256,  pipomfcc);  }
BENCHMARK_F (pipo_bench, FramesMFCC2, 10, NUMITER) { pipomfcc->size.set(1024);  run(1024, pipomfcc);  }
BENCHMARK_F (pipo_bench, FramesMFCC3, 10, NUMITER) { pipomfcc->size.set(4096);  run(4096, pipomfcc);  }


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
