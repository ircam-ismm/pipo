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

#ifndef _MAINPAGE_TTL_H
#define _MAINPAGE_TTL_H

//===========================================================================
//
//  Main page for TTL documentation
//
//===========================================================================


/** \mainpage TTL, The Triangulation Template Library

\image html anim_opt.gif "Constrained Delaunay Triangulation (CDT)"
<hr>

\section aboutdoc About TTL
TTL is a \e generic triangulation library developed at <A
HREF="http://www.sintef.no/math">SINTEF Applied Mathematics</A>.  TTL
is generic in the sense that it does not rely on a special data
structure.  Thus, you can operate with your own application data
structure and benefit from a variety of generic algorithms in TTL that
can work directly on any data structure for triangulations.

If you do not want to bother with making your own data structure and
adapt it to TTL, you can use one of the data structures that comes
with TTL and use TTL as you would use any other triangulation library.

TTL is written in C++ and makes extensive use of function templates as
a generic tool for the 
\ref api "application programming interface". 
Moreover, it offers a new programming philosophy for
triangulations based on a few clear and robust algebraic concepts
which makes it easy to build applications and extend them with new
functionality.  The programming philosophy is somewhat like STL, The
Standard Template Library, with iterator concepts. The code is limited
to a few include files with the main interface in 
\ref ttl "namespace ttl".

Examples of generic tools currently available are:\n
- Incremental Delaunay triangulation
- Constrained Delaunay triangulation
- Insert and remove nodes in a triangulation
- Searching and traversal operations
- Misc. queries for extracting information for visualisation systems etc.

\subsection datastructures Data structures distributed with TTL
A data structure represents the \em topology of the triangulation,
i.e., how nodes, edges and triangles are glued together to form a
triangulation.  The well-known
\ref halfedge "half-edge data structure"
 is distributed with TTL and can be used if you do not have
your own data structure.  In the future we will also make other common
data structures available.

\subsection startdoc Getting started
- If you just want to make a Delaunay triangulation from a set of points
  in the plane (with associated height values), compile and run
  the sample program in the file \ref hesimplest "main.cpp".
  The program also demonstrates insertion of nodes and constraints, removal of nodes
  and other useful functionality in TTL.
  Modify the program for your needs.
  This program uses the \ref halfedge "half-edge data structure".

- If you want to adapt your own data structure and utilize the generic
  concepts of TTL, read the section \ref api. Implement your own \ref hed::Dart "DartType"
  and \ref hed::TTLtraits "TraitsType" classes (or structs) interfacing your data structure
  similar to that of the \ref halfedge "half-edge data structure".
  Make a sample program similar to that of \ref hesimplest "main.cpp"
  and run Delaunay triangulation, remove/insert nodes etc., using
  function templates in \ref ttl "namespace ttl".
  
\subsection publications Literature

The generic programming philosophy of TTL is published as one of the chapters the book:\n
<a href="http://www.springer.com/east/home/math/cse?SGWID=5-10045-22-173660199-0">
�yvind Hjelle and Morten D�hlen.
<em>Triangulations and Applications.</em>
Springer-Verlag, 2006. (ISBN: 3-540-33260-X) 
</a> 

See also:<br>
<EM>�. Hjelle. A Triangulation Template Library (TTL): Generic Design of Triangulation Software.
Technical Report STF42 A00015, SINTEF 2000.</EM>\n
<a href="http://www.sintef.no/upload/IKT/9011/geometri/TTL/TTLreport.pdf">Full report (344 K, pdf)</a>

\subsection funding Funding
The development of TTL has been supported by the
<a href="http://www.forskningsradet.no">Research Council of Norway</a>
under the research program 117644/223,
<a href="http://www.sintef.no/static/AM/dynamap/index.html">
"DYNAMAP II, Management and Use of Geodata"</a>.

\subsection platform Platform and compiler
TTL runs on a number of platforms, including Unix, Linux and Windows.
If you have any problems it is straightforward to modify the source code.
The C++ compiler must support the syntax: \c function<     \c >(..,..) for calling
function templates.

\subsection download Download
TTL with the the half-edge data structure and its documentation, together with various
examples and demos, can be downloaded from the <a href="http://www.sintef.no/Projectweb/Geometry-Toolkits/Downloads">downloads page</a>.

*/

#endif // _MAINPAGE_TTL_H
