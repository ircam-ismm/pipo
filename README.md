
# PiPo — Plugin Interface for Processing Objects #

PiPo is an extremely simple plugin API for modules processing streams of multi-dimensional data such as audio, audio descriptors, or gesture and motion data. The current version of the interface is limited to unary operations. Each PiPo module receives and produces a single stream. The elements of a stream are time-tagged or regularly sampled scalars, vectors, or two-dimensional matrices.


### More Information

http://ismm.ircam.fr/pipo/

### Authors

This code has been initially authored by Norbert Schnell in the <a href="http://ismm.ircam.fr">Sound Music Movement Interaction</a> team of the <a href="https://www.ircam.fr/recherche/lunite-mixte-de-recherche-stms/">STMS Lab</a> - IRCAM - CNRS - UPMC.

### Copyright

Copyright (c) 2012–2017 by IRCAM – Centre Pompidou, Paris, France.
All rights reserved.

### Licence: BSD 3-clause

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

- Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Features

PiPo has been developed to make things simple for users and developers.

#### User Level Features

- Easy integration and customization of stream processing modules
- Suited for filtering, transformation, extraction, and segmentation algorithms
- Real-time and offline processing
- Applying to audio and control streams (for whom this distinction still makes sense)

#### Developer Level Features

- Fast and easy implementation of modules and reuse of code for different contexts
- C/C++ API defined by a single header file without additional dependencies
- Core API as single abstract class defining a small set of virtual methods

#### PiPo Stream Attributes

PiPo streams are a sequences of frames characterized by a set of attributes. A PiPo module defines the attributes of its output stream when receiving the attributes of the input stream.

Each module can configure its internal state depending on the attributes of the input stream (e.g. memory allocation and pre-calculated state variables) before propagating its output stream attributes to the next module.

This way, the attributes of the input stream are propagated through a series of PiPo modules before starting the actual stream processing.

In summary, a PiPo stream is described by the following attributes:

- a boolean representing whether the elements of the stream are time-tagged
- frame rate (highest average rate for time-tagged streams)
- lag of the output stream relative to the input
- frame width (also number of channels or matrix columns)
- frame height (or number of matrix rows)
- labels (for the frame channels or columns)
- a boolean representing whether the frames have a variable size (respecting the given frame size as maximum)
- extent of a frame in the given domain (e.g. duration or frequency range)
- maximum number of frames in a block exchanged between two modules

#### PiPo Module Parameters

The PiPo SDK comes with a template class [PiPo::Attr](http://recherche.ircam.fr/equipes/temps-reel/mubu/pipo/sdk-doc-v0.1/class_pi_po_1_1_attr.html) permits to define scalar, enum, or variable or fixed size vector attributes of a pipo module that are exposed to the host environment.

Since certain parameter changes may also change the attributes of a module’s output stream, PiPo provides a mechanism for signaling these changes through the following modules to the processing environment (i.e. the PiPo host).

For MAX/MSP PiPo includes a binding to extend a PiPo class to a Max/MSP external that then declares the module's parameters as Max/MSP attributes and implement the required setters and getters.

## PiPo API

### Module API

The PiPo Module API consists of the abstract class [PiPo](http://recherche.ircam.fr/equipes/temps-reel/mubu/pipo/sdk-doc-v0.1/class_pi_po.html) in [PiPo.h](src/include/PiPo.h) of a few virtual methods for propagating stream attributes (see above), frames, and additional processing control through a series of modules:

- Propagating stream attributes
- Propagating frames
- Reset stream processing
- Finalize stream processing
- Propagate the change of a parameter requiring redefining the output stream attributes

See the doc extraced from [PiPo.h](include/PiPo.h) at: http://recherche.ircam.fr/equipes/temps-reel/mubu/pipo/sdk-doc-v0.1/

### Host API

Classes supporting implementation of PiPo hosts are found in directory [host](src/host).

PiPo comes with a collection of native modules that can be combined into graphs to form more complex modules.
Their source code is located into directory [modules](src/modules).

The host-level `PiPoCollection` class is a module factory, able to instantiate complex PiPo modules from a simple graph syntax.
It provides all the native PiPo modules and can be extended with third-party PiPo modules, thus allowing to create a wide number
of combinations.

## Dependencies

Pipo depends on the following sources in submodules:
- sdk from https://github.com/Ircam-RnD/pipo-sdk.git, branch develop
- modules/rta from https://github.com/Ircam-RnD/rta-lib.git, branch master
- modules/bayesfilter from https://github.com/Ircam-RnD/bayesfilter.git
- modules/wavelet from https://github.com/Ircam-RnD/wavelet.git

After first git clone, initialise the submodules with:

``git submodule update --remote --merge``
