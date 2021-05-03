#include "algorithmfactory.h"
#include "windowing.h"
#include "fftk.h"
#include "fftkcomplex.h"
#include "constantq.h"

namespace essentia {
namespace standard {

ESSENTIA_API void registerAlgorithm() {
    AlgorithmFactory::Registrar<Windowing> regWindowing;
    AlgorithmFactory::Registrar<FFTK> regFFT;
    AlgorithmFactory::Registrar<FFTKComplex> regFFTC;
    AlgorithmFactory::Registrar<ConstantQ> regConstantQ;
}}}


namespace essentia {
namespace streaming {

ESSENTIA_API void registerAlgorithm() {
#if 0 // we're not using streaming for pipo
    AlgorithmFactory::Registrar<FFTC, essentia::standard::FFTC> regFFTC;
    AlgorithmFactory::Registrar<ConstantQ, essentia::standard::ConstantQ> regConstantQ;
#endif
}}}

