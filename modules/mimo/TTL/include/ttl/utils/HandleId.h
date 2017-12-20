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

#ifndef _HANDLEID_H_
#define _HANDLEID_H_


#include <stddef.h>


//--------------------------------------------------------------------------------------------------
// HandleId class
//--------------------------------------------------------------------------------------------------

/** \class HandleId 
*   \brief Base class with reference counting for smart pointers
*/

class HandleId {

protected:
  int     refcount;       // reference count
  char    dynamic_object; // '1': explicit call to new created the object

public:
  HandleId ();
  virtual ~HandleId () {};

  bool isReferenced () const { return refcount != 0; }
  int  getNoRefs    () const { return refcount; }
  bool dynamicObj   () const { return dynamic_object == '1'; }

  void  increment () { refcount++; }
  void  decrement () { refcount--; }

  void* operator new (size_t t);
  void* operator new (size_t t,int, const char * file, int line);

  void  operator delete (void* v);

};
 
#endif
