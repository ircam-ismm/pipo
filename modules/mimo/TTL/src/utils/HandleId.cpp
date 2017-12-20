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


#include "../../include/ttl/utils/HandleId.h"


#include <iostream>
#include <cstdlib>


using namespace std;


static char local_HandleId_new = '0'; // for hack: op.new and constructor


//--------------------------------------------------------------------------------------------------
HandleId::HandleId () {
  refcount = 0; 
  dynamic_object = local_HandleId_new;
  local_HandleId_new = '0';
}


//--------------------------------------------------------------------------------------------------
void* HandleId::operator new (size_t t) {
	return HandleId::operator new(t,0,0,0);
}


//--------------------------------------------------------------------------------------------------
void* HandleId::operator new (size_t t,int n, const char * file, int line) {
   HandleId* tmp = (HandleId*) calloc(1,t);

  if (local_HandleId_new == '1')
    cout << "HandleId::operator new" <<
	      "nested calls of new are detected, the code may be typically\n" <<
	      "like this: some_obj.rebind (new B(new A())); where both class\n" <<
	      "A and class B are derived from HandleId. This construction is\n" <<
	      "not recommended. Modify your code (the size of the allocated\n" <<
	      "object in this operator new is %d, this may help you to\n"
	      "locate the critical statements)." << t << endl;

  local_HandleId_new = '1';

  return tmp;
}


//--------------------------------------------------------------------------------------------------
void HandleId::operator delete (void* v) {
  free(v);
}
