/**
 * @file PiPoGraph.h
 * @author Joseph Larralde
 * 
 * @brief PiPo dataflow graph class that parses a graph description string and instantiates
 * the corresponding graph of PiPos (made of PiPoSequence and PiPoParallel instances)
 *
 * @ingroup pipoapi
 *
 * @copyright
 * Copyright (C) 2017 by ISMM IRCAM - Centre Pompidou, Paris, France.
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

#ifndef _PIPOHOST_H_
#define _PIPOHOST_H_

#include <string>
#include <vector>
#include <iostream>

#include "PiPoSequence.h"
#include "PiPoParallel.h"
#include "PiPoHost.h"

// TODO: roll your own Module, Factory and Op ?
// --------------------------------------------

// class PiPoModule {

// };

// class PiPoModuleFactory {

// };

// class PiPoOp {

// };



// NB : this is a work in progress, only performs parsing at the moment
// TODO : implement actual graph instatiation.
// TODO: define error return codes for parsing

class PiPoGraph : public PiPo {

  typedef enum PiPoGraphTypeE {
    undefined = -1, leaf = 0, sequence, parallel
  } PiPoGraphType;

private:
  std::string representation;  
  PiPoGraphType graphType;

  // parallel graphs if isParallel
  // sequence of graphs otherwise (not sequence of PiPos)
  // empty if leaf
  std::vector<PiPoGraph> subGraphs;

  // use op if we are a leaf to parse instanceName and to hold attributes
  PiPoOp op;

  PiPo *pipo;
  std::vector<std::string *> attrNames;
  std::vector<std::string *> attrDescrs;
  PiPoModuleFactory *moduleFactory;

  // for intermediary level PiPoGraphs : these point to leaves' attrs so that
  // top level PiPoGraph can add them to itself with this->addAttr
  std::vector<PiPo::Attr *> attrs;

public:
  PiPoGraph(PiPo::Parent *parent, PiPoModuleFactory *moduleFactory = NULL) :
  PiPo(parent)
  {
    this->parent = parent;
    this->moduleFactory = moduleFactory;
  }

  ~PiPoGraph()
  {
    for (unsigned int i = 0; i < attrNames.size(); i++)
    {
      delete attrNames[i];
    }

    for (unsigned int i = 0; i < attrDescrs.size(); i++)
    {
      delete attrDescrs[i];
    }

    this->clear();    
  }

  void clear()
  {
    for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
    {
      this->subGraphs[i].clear();
    }

    if (this->op.getPiPo() == NULL)
    {
      delete this->pipo;
    }
    else
    {
      this->op.clear();
    }
  };

  //======================== PARSE GRAPH EXPRESSION ==========================//

  // or use const char * ?
  bool parse(std::string graphStr) {

    //=================== BASIC SYNTAX RULES CHECKING ========================//

    //========== check if we have the right number of '<' and '>' ============//

    int cnt = 0;
    for (unsigned int i = 0; i < graphStr.length(); ++i)
    {
      if (graphStr[i] == '<')
      {
        cnt++;
      }
      else if (graphStr[i] == '>')
      {
        cnt--;
      }
    }

    if (cnt != 0)
    {
      return false;
    }

    // TODO : add more syntax checking here ?

    //======= determine the type of graph (leaf, sequence or parallel) =======//

    unsigned int trims = 0;
    while (graphStr[0] == '<' && graphStr[graphStr.length() - 1] == '>')
    {
      graphStr = graphStr.substr(1, graphStr.length() - 2);
      trims++;
    }
    
    this->representation = graphStr;

    // by default we are a sequence
    this->graphType = sequence;

    // if we have surrounding "<...>" and first-level commas, we are a parallel
    if (trims > 0)
    {
      int subLevelsCnt = 0;
      for (unsigned int i = 0; i < graphStr.length(); ++i)
      {
        if (graphStr[i] == '<')
        {
          subLevelsCnt++;
        }
        else if (graphStr[i] == '>')
        {
          subLevelsCnt--;
        }
        else if (subLevelsCnt == 0 && graphStr[i] == ',')
        {
          this->graphType = parallel;
          break;
        }
      }
    }

    // if we don't have any sequential / parallelism symbol, then we are a leaf
    if (graphStr.find("<") >= std::string::npos &&
        graphStr.find(">") >= std::string::npos &&
        graphStr.find(",") >= std::string::npos &&
        graphStr.find(":") >= std::string::npos)
    {
      this->graphType = leaf;
    }

    // std::cout << representation << " " << this->graphType << std::endl;

    //====== now fill (or not) subGraphs vector according to graph type ======//

    switch (this->graphType)
    {

      //========================== leaf graph, we are a single PiPo, trim spaces
      case leaf:
      {
        // trim spaces
        // TODO: trim tabs and other spaces ?
        this->representation.erase(
          std::remove(this->representation.begin(), this->representation.end(), ' '),
          this->representation.end()
        );

        // this parses pipoName and instanceName
        // keep using PiPoOp class for this instead (as in PiPoChain) ?
        // ===> YES !!! DEFINITELY !!! PiPoOp manages PiPoVersion etc
        // (LEAVE THIS TO PiPoOp CLASS)

        /*
        size_t open = this->representation.find('(');
        size_t close = this->representation.find(')');

        if (open < std::string::npos || close < std::string::npos) {
          // ok, do nothing
          if (open > 0 && open < close && close == this->representation.length() - 1) {
            // ok, parse
          } else {
            // parentheses are not in the right place
          }
        }
        */

        this->op.parse(this->representation.c_str());

        // leaf string representation cannot be empty
        return (graphStr.length() > 0);
      }

      break;

      //================= sequence of graphs, parse according to ':', <' and '>'
      case sequence:
      {
        int subLevelsCnt = 0;
        unsigned int lastStartIndex = 0;
        std::vector< std::pair<unsigned int, unsigned int> > subStrings;

        if (graphStr[0] == '<')
        {
          subLevelsCnt++;
        }

        for (unsigned int i = 1; i < graphStr.length(); ++i)
        {
          if (graphStr[i] == ':' && subLevelsCnt == 0)
          {
            subStrings.push_back(std::pair<unsigned int, unsigned int>(
              lastStartIndex, i - lastStartIndex
            ));
            lastStartIndex = i + 1;
          }
          else if (graphStr[i] == '<')
          {
            if (subLevelsCnt == 0)
            {
              subStrings.push_back(std::pair<unsigned int, unsigned int>(
                lastStartIndex, i - lastStartIndex
              ));
              lastStartIndex = i;
            }
            subLevelsCnt++;
          }
          else if (graphStr[i] == '>')
          {
            subLevelsCnt--;
            if (subLevelsCnt == 0)
            {
              subStrings.push_back(std::pair<unsigned int, unsigned int>(
                lastStartIndex, i - lastStartIndex + 1
              ));
              lastStartIndex = i + 1;
            }
          }
          else if (graphStr[i] == ',' && subLevelsCnt == 0)
          {
            // cannot have first-level commas in a sequence
            return false;
          }
        }

        if (graphStr[graphStr.length() - 1] != '>')
        {
          subStrings.push_back(std::pair<unsigned int, unsigned int>(
            lastStartIndex, graphStr.length() - lastStartIndex
          ));
        }

        for (unsigned int i = 0; i < subStrings.size(); ++i)
        {
          this->subGraphs.push_back(PiPoGraph());
          PiPoGraph &g = this->subGraphs[subGraphs.size() - 1];

          if (!g.parse(graphStr.substr(subStrings[i].first, subStrings[i].second)))
          {
            return false;
          }
        }

        return (subLevelsCnt == 0);
      }

      break;

      //============================= parallel graphs, parse according to commas
      case parallel:
      {
        int subLevelsCnt = 0;
        std::vector<unsigned int> commaIndices;

        commaIndices.push_back(0);

        for (unsigned int i = 0; i < graphStr.length(); ++i)
        {
          if (graphStr[i] == '<')
          {
            subLevelsCnt++;
          }
          else if (graphStr[i] == '>')
          {
            subLevelsCnt--;
          }
          else if (graphStr[i] == ',' && i < graphStr.length() - 1 && subLevelsCnt == 0)
          {
            commaIndices.push_back(i + 1);
          }
        }

        commaIndices.push_back(graphStr.length() + 1);

        for (unsigned int i = 0; i < commaIndices.size() - 1; ++i)
        {
          this->subGraphs.push_back(PiPoGraph());
          PiPoGraph &g = this->subGraphs[this->subGraphs.size() - 1];
          unsigned int blockStart = commaIndices[i];
          unsigned int blockLength = commaIndices[i + 1] - blockStart - 1;

          if (!g.parse(graphStr.substr(blockStart, blockLength)))
          {
            return false;
          }          
        }

        return (subLevelsCnt == 0);
      }

      break;

      //===================================================== this never happens
      default:

      break;
    }

    return false;
  }

  //================ ONCE EXPRESSION PARSED, INSTANTIATE OPs =================//

  bool instantiate()
  {
    if (this->graphType == leaf)
    {
      if (!this->op.instantiate(this->parent, this->moduleFactory))
      {
        return false;
      }
      else
      {
        this->pipo = this->op.getPiPo();
      }
    }
    else if (this->graphType == sequence)
    {
      for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
      {
        if (!this->subGraphs[i].instantiate())
        {
          return false;
        }
      }
      
      this->pipo = new PiPoSequence(this->parent);
    }
    else if (this->graphType == parallel)
    {
      for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
      {
        if (!this->subGraphs[i].instantiate())
        {
          return false;
        }
      }

      this->pipo = new PiPoParallel(this->parent);      
    }

    return true;
  }

  void connect( bool topLevel = true)
  {
    // add from top level to bottom level (see PiPoBasic)
    if (this->graphType == sequence)
    {
      for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
      {
        this->pipo->add(this->subGraphs[i].getPiPo());
        this->subGraphs[i].connect(false);
      }
    }
    else if (this->graphType == parallel)
    {
      for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
      {
        this->pipo->add(this->subGraphs[i].getPiPo());
        this->subGraphs[i].connect(false);
      }      
    }

    if (topLevel)
    {
      this->setReceiver(this->pipo);
    }
  }

  void setReceiver(PiPo *receiver)
  {
    this->setReceiver(receiver);
  }

  PiPoGraphType getGraphType()
  {
    return this->graphType;
  }

  PiPo *getPiPo()
  {
    return this->pipo;
  }

  // TODO: add an option to get PiPoAttributes only from named modules ?
  void copyPiPoAttributes(bool topLevel = true)
  {
    for (unsigned int i = 0; i < this->subGraphs.size(); ++i)
    {
      if (this->subGraphs[i].getGraphType() == leaf)
      {

      }
    }
  }

  // void print() {
  //   std::cout << this->representation << " " << this->graphType << std::endl;
  //   for (unsigned int i = 0; i < this->subGraphs.size(); ++i) {
  //     std::cout << " ";
  //     this->subGraphs[i].print();
  //   }
  // }
};

//==================== NOW WE CAN WRITE A PIPOHOST CLASS =====================//

// class PiPoHost {

// };

#endif /* _PIPOHOST_H_ */
