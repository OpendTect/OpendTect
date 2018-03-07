#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		27-1-98
________________________________________________________________________

-*/

# ifdef do_import_export
#  include <arraynd.h>
#  include <monitoriter.h>

mExportTemplClassInst( General ) Array2D<float>;
mExportTemplClassInst( General ) MonitorableIterBase<int>;
mExportTemplClassInst( General ) ValueSeries<float>;

# endif
