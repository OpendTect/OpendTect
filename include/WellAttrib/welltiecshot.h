#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
