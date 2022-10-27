#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "gendefs.h"
#include "ptrman.h"

class TaskRunner;
namespace Pick	{ class Set; }
namespace EM	{ class Horizon3D; }

/*!
\brief Calculate volume between horizon and polygon.
*/

mExpClass(EarthModel) Poly2HorVol
{
public:
			Poly2HorVol(const Pick::Set* =nullptr,
				    EM::Horizon3D* =nullptr);
    virtual		~Poly2HorVol();

    ConstRefMan<Pick::Set>	pickSet() const		{ return ps_; }
    void			setPickSet( const Pick::Set* ps )  { ps_ = ps; }

    EM::Horizon3D*		horizon()		{ return hor_; }
    void			setHorizon(EM::Horizon3D*);
    bool			setHorizon(const MultiID&,TaskRunner* =nullptr);

    float			getM3(float,bool upward,bool usenegvals);
    mDeprecatedDef
    const char*			dispText(float m3,bool zinft);

protected:

    ConstRefMan<Pick::Set>	ps_;
    RefMan<EM::Horizon3D>	hor_;
};
