/**
 * @file PiPoCollection.h
 * @author Norbert.Schnell@ircam.fr
 * @author joseph.larralde@ircam.fr
 * 
 * @brief PiPo Module Collection
 *
 * @ingroup pipoapi
 * 
 * @copyright
 * Copyright (C) 2013 - 2015 by ISMM IRCAM – Centre Pompidou, Paris, France.
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
 * 3. Neither the name of mosquitto nor the names of its
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

#ifndef _PIPO_COLLECTION_
#define _PIPO_COLLECTION_

#include <map>
#include "PiPo.h"

//============================================================================//
//============================== PiPo factory ================================//
//============================================================================//

//------------------------------ PiPoCreatorBase
class PiPoCreatorBase
{
public:
  //PiPoCreatorBase() {}
  virtual ~PiPoCreatorBase() {}
  virtual PiPo *create() = 0;
};

//------------------------------ PiPoCreator
template<class T>
class PiPoCreator : public PiPoCreatorBase
{
public:
    PiPoCreator() {}
    virtual ~PiPoCreator() {}
    virtual PiPo *create()
    {
        return new T(NULL);
    }
};

//============================================================================//
//============================== PiPo collection =============================//
//============================================================================//

// interface with the outside world (uses the factory internally)

/**
 * @brief PiPoCollection class : contains all base PiPo classes
 * (with ability to hide them from the outside world)
 * and methods to register new PiPos and get PiPo chains
 *
 * \code
 *
 * #include "PiPoCollection.h"
 *
 * // example : register a new non native PiPo :
 * PiPoCollection::init();
 * PiPoCollection::addToCollection("mypipo", new PiPoCreator<MyPiPo>);
 * PiPo *mychain = PiPoCollection::create("slice:mypipo");
 *
 * \endcode
 *
 */

class PiPoCollection
{
public:
  static void deinit();
  static void init(bool defaultPipos = true);
  //static void init(std::string pipoPath = "");
  static void addToCollection(std::string name, PiPoCreatorBase *creator);
  // static void addAlias(std::string alias, std::string name); ----> TODO
  static PiPo *create(std::string name);
private:
};

#endif
