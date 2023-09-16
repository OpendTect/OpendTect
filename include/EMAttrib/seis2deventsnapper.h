#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "seiseventsnapper.h"
#include "seistrc.h"

namespace EM { class Horizon2D; }
class SeisTrcReader;
class IOObj;

/*!
\brief SeisEventSnapper for 2D.
*/

mExpClass(EMAttrib) SeisEventSnapper2D : public SeisEventSnapper
{
public:

				SeisEventSnapper2D(const IOObj&,Pos::GeomID,
						   const EM::Horizon2D&,
						   EM::Horizon2D&,
						   const Interval<float>& gate,
						   bool eraseundef=true);
				~SeisEventSnapper2D();

protected:

    int				nextStep() override;

    Pos::GeomID			geomid_;
    SeisTrc			trc_;
    SeisTrcReader*		seisrdr_;
    ConstRefMan<EM::Horizon2D>	inhorizon_;
    RefMan<EM::Horizon2D>	outhorizon_;
};

