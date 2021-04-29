#include "algorithmfactory.h"
#include "constantq.h"

namespace essentia {
namespace standard {

ESSENTIA_API void registerAlgorithm() {
    AlgorithmFactory::Registrar<ConstantQ> regConstantQ;
}}}



namespace essentia {
namespace streaming {

ESSENTIA_API void registerAlgorithm() {
  //    AlgorithmFactory::Registrar<ConstantQ, essentia::standard::ConstantQ> regConstantQ;
}}}
