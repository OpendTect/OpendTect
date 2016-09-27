#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
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
mExportTemplClassInst( Basic ) Geom::Point2D<int>;
mExportTemplClassInst( Basic ) Geom::Point3D<double>;
mExportTemplClassInst( Basic ) Geom::Point3D<float>;
mExportTemplClassInst( Basic ) PerThreadObjectRepository<BufferString>;
mExportTemplClassInst( Basic ) PerThreadObjectRepository<BufferString>;

# endif
