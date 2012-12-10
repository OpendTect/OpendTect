#ifndef uibaseexport_h
#define uibaseexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id$
________________________________________________________________________

-*/


#include "geometry.h"

mExportTemplClassInst( uiBase, Geom::Size2D<int> );
mExportTemplClassInst( uiBase, Geom::PixRectangle<int> );

#endif

