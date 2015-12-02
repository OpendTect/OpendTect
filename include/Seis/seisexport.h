#ifndef seisexport_h
#define seisexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id$
________________________________________________________________________

-*/

# ifdef do_import_export

#  include <valseries.h>

mExportTemplClassInst( Seis ) ValueSeries<float>;

# endif
#endif
