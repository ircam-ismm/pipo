//===========================================================================
// TTL - The Triangulation Template Library, version 1.1.0
//
// Copyright (C) 2000-2007, 2010 SINTEF ICT, Applied Mathematics, Norway.
//
// This program is free software; you can redistribute it and/or          
// modify it under the terms of the GNU General Public License            
// as published by the Free Software Foundation version 2 of the License. 
//
// This program is distributed in the hope that it will be useful,        
// but WITHOUT ANY WARRANTY; without even the implied warranty of         
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
// GNU General Public License for more details.                           
//
// You should have received a copy of the GNU General Public License      
// along with this program; if not, write to the Free Software            
// Foundation, Inc.,                                                      
// 59 Temple Place - Suite 330,                                           
// Boston, MA  02111-1307, USA.                                           
//
// Contact information: E-mail: tor.dokken@sintef.no                      
// SINTEF ICT, Department of Applied Mathematics,                         
// P.O. Box 124 Blindern,                                                 
// 0314 Oslo, Norway.                                                     
//
// Other licenses are also available for this software, notably licenses
// for:
// - Building commercial software.                                        
// - Building software whose source code you wish to keep private.        
//===========================================================================

#ifndef _HALF_EDGE_DART_
#define _HALF_EDGE_DART_


#include "../halfedge/HeTriang.h"


namespace hed {


  //------------------------------------------------------------------------------------------------
  // Dart class for the half-edge data structure
  //------------------------------------------------------------------------------------------------

  /** \class Dart
  *   \brief \b %Dart class for the half-edge data structure.
  *
  *   See \ref api for a detailed description of how the member functions
  *   should be implemented.
  */

  class Dart {

    Edge* edge_;
    bool dir_; // true if dart is counterclockwise in face

  public:
    /// Default constructor
    Dart() { edge_ = NULL; dir_ = true; }

    /// Constructor
    Dart(Edge* edge, bool dir = true) { edge_ = edge; dir_ = dir; }

    /// Copy constructor
    Dart(const Dart& dart) { edge_ = dart.edge_; dir_ = dart.dir_; }

    /// Destructor
    ~Dart() {}

    /// Assignment operator
    Dart& operator = (const Dart& dart) {
      if (this == &dart)
        return *this;
      edge_ = dart.edge_;
      dir_  = dart.dir_;
      return *this;
    }

    /// Comparing dart objects
    bool operator==(const Dart& dart) const {
      if (dart.edge_ == edge_ && dart.dir_ == dir_)
        return true;
      return false;
    }

    /// Comparing dart objects
    bool operator!=(const Dart& dart) const {
      return !(dart==*this);
    }

    /// Maps the dart to a different node
    Dart& alpha0() { dir_ = !dir_; return *this; }

    /// Maps the dart to a different edge
    Dart& alpha1() {
      if (dir_) {
        edge_ = edge_->getNextEdgeInFace()->getNextEdgeInFace();
        dir_ = false;
      }
      else {
        edge_ = edge_->getNextEdgeInFace();
        dir_ = true;
      }
      return *this;
    }

    /// Maps the dart to a different triangle. \b Note: the dart is not changed if it is at the boundary!
    Dart& alpha2() {
      if (edge_->getTwinEdge()) {
        edge_ = edge_->getTwinEdge();
        dir_ = !dir_;
      }
      // else, the dart is at the boundary and should not be changed
      return *this;
    }


    // Utilities not required by TTL
    // -----------------------------

    /** @name Utilities not required by TTL */
    //@{

    void init(Edge* edge, bool dir = true) { edge_ = edge; dir_ = dir; }

    double x() const { return getNode()->x(); } // x-coordinate of source node
    double y() const { return getNode()->y(); } // y-coordinate of source node

    bool isCounterClockWise() const { return dir_; }

    Node* getNode() const { return dir_ ? edge_->getSourceNode() : edge_->getTargetNode(); }
    Node* getOppositeNode() const { return dir_ ? edge_->getTargetNode() : edge_->getSourceNode(); }
    Edge* getEdge() const { return edge_; }

    //@} // End of Utilities not required by TTL

  };

}; // End of hed namespace

#endif
