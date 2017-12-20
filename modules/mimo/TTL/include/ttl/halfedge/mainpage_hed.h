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

#ifndef _MAINPAGE_HED_H
#define _MAINPAGE_HED_H

//===========================================================================
//
//  Main page for Half-Edge documentation
//
//===========================================================================


/** \page halfedge The Half-Edge Data Structure and Adaption to TTL

An implementation of the the well-known \e half-edge data structure for
triangulations is documented briefly on these pages together with
adaption of this data structure to
\ref index "TTL, The Triangulation Template Library".
Only the class hed::Dart and the
(static) struct hed::TTLtraits are documented thoroughly. These
compounds represent the interface channels between TTL and the application
data structure. See \ref api for a general documentation of this mechanism.
Follow the links to the source code for classes that are not
documented in detail - most member functions are self-explanatory.

\section heclassdiagram The Class Diagram
The classes hed::Node, hed::Edge and
hed::Triangulation represent the half-edge data structure
shown as a class diagram in the figure below. ("Edge" denotes a half-edge).
These classes, together with hed::Dart and hed::TTLtraits for adaption to TTL,
are encapsulated in the namespace \b hed.


\image html he_classdiagram.gif "Class Diagram for the Half-Edge Data Structure"


Each half-edge has a pointer to the hed::Node it starts from,
a pointer to the next half-edge belonging to the same triangle
(counterclockwise), and a pointer to its "twin-edge" belonging
to another triangle.

The class hed::Triangulation containts a list of (half-) edges, each
of which represents one triangle in the triangulation.
In addition, the class hed::Triangulation also serves as an interface
to the TTL as it implements hed::TTLtraits::swapEdge, hed::TTLtraits::splitTriangle etc.,
which are functions required on the application side by function templates
in the \ref ttl "TTL" (via the traits class).

\section example_he Example
Consult the file \ref hesimplest "main.cpp" for a program using TTL and
the half-edge data structure.

*/

#endif // _MAINPAGE_HED_H
