#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
