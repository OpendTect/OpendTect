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
#  include "binid.h"
#  include "geometry.h"
#  include "perthreadrepos.h"
#  include "bufstring.h"

mExportTemplClassInst( Basic ) Pos::ValueIdxPair<BinID,float>;
mExportTemplClassInst( Basic ) Pos::IdxPairValues<BinID,float>;
mExportTemplClassInst( Basic ) Pos::IdxPairValues<Pos::IdxPair,float>;
mExportTemplClassInst( Basic ) Geom::Point2D<double>;
mExportTemplClassInst( Basic ) PerThreadObjectRepository<BufferString>;

# endif
#endif
