#ifndef basicexport_h
#define basicexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

# ifdef do_import_export
#  include "posidxpairvalue.h"

mExportTemplClassInst( Basic ) ValueIdxPair<BinID,float>;
mExportTemplClassInst( Basic ) IdxPairValues<BinID,float>;

# endif
#endif
