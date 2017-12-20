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

#ifndef _HANDLE_H_
#define _HANDLE_H_


#include <assert.h>


//--------------------------------------------------------------------------
// Handle class
//--------------------------------------------------------------------------

/** \class Handle
*   \brief Template class for smart pointers. The actual class must
*   inherit from HandleId
*/

template <class T>
class Handle_TTL {

protected:
  T* classptr; 

 public:
  Handle_TTL() { classptr = 0; }

  Handle_TTL(const T& ref) {
    classptr = (T*) &ref;
    classptr->increment();
  }

  Handle_TTL(T* p) {
    classptr = p;
    if (classptr != 0) classptr->increment();
  }

  Handle_TTL(const Handle_TTL<T>& ref) {
    classptr = ref.classptr;
    if (classptr != 0)
      classptr->increment();
  }

  ~Handle_TTL() {
    if (classptr != 0) {
      classptr->decrement ();
      assert(classptr->getNoRefs() >= 0);
      if (!classptr->isReferenced()) {
        if (classptr->dynamicObj())
          delete classptr;
      }
    }
  }

  void rebind(const T * pc) {
    if (classptr != pc) {
      T* p = (T*) pc; // cast const away
      if (p != 0)
        p->increment();
      if (classptr != 0) {
        classptr->decrement ();
        assert(classptr->getNoRefs() >= 0);
        if (!classptr->isReferenced() && classptr->dynamicObj())
          delete classptr;
      }
      classptr = p;
    }
  }

  void rebind(const T& p) { rebind(&p); }
  
  const T* operator->() const { return  classptr; }
  T* operator->()             { return  classptr; }
  const T& operator()() const { return *classptr; }
  T& operator()()             { return *classptr; }
  const T& operator*() const  { return *classptr; }
  T& operator*()              { return *classptr; }
  const T* getPtr() const     { return  classptr; }
  T* getPtr()                 { return  classptr; }
  const T& getRef() const     { return *classptr; }
  T& getRef()                 { return *classptr; }

  void operator=(const Handle_TTL<T>& h) { rebind(h.getPtr()); }
  void operator=(const T* p)         { rebind(p); }
  void operator=(const T& p)         { rebind(p); }

  bool operator==(const Handle_TTL<T>& h) const { return classptr == h.classptr; }
  bool operator!=(const Handle_TTL<T>& h) const { return classptr != h.classptr; }
  bool operator< (const Handle_TTL<T>& h) const { return classptr < h.classptr; }
  bool operator> (const Handle_TTL<T>& h) const { return classptr > h.classptr; }

};

#endif
