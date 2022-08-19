#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "gendefs.h"

class UnitOfMeasure;
namespace Well { class Log; }

namespace WellTie
{

mExpClass(WellAttrib) UnitFactors
{
public:

    double		getVelFactor(const Well::Log&,bool issonic) const;
    double		getDenFactor(const Well::Log&) const;

    static const char*	getStdVelLabel();
    static const char*	getStdTimeLabel();
    static const char*	getStdSonLabel();

protected:

    const UnitOfMeasure* getUOM(const Well::Log&) const;
    double		calcVelFactor(const char*,bool) const;
    double		calcVelFactor(const char*) const;
    double		calcSonicVelFactor(const char*) const;
    double		calcDensFactor(const char*) const;
};

} // namespace WellTie
