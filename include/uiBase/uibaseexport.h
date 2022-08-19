#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

# ifdef do_import_export

#  include "geometry.h"

mExportTemplClassInst( uiBase ) Geom::Size2D<int>;
mExportTemplClassInst( uiBase ) Geom::PixRectangle<int>;

# endif
