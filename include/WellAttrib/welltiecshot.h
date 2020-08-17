#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: welltiecshot.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "gendefs.h"

namespace Well { class D2TModel; }

namespace WellTie
{

mExpClass(WellAttrib) CheckShotCorr
{
public:
    static void		calibrate(const Well::D2TModel& cs,Well::D2TModel& d2t);
};

} // namespace WellTie

