#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"
#include "posinfo2dsurv.h"

namespace EM { class Horizon; }
class TrcKeySamplingIterator;

/*!
\brief Modifies horizons.
*/

mExpClass(EarthModel) HorizonModifier
{
public:

				HorizonModifier(bool is2d=false);
				~HorizonModifier();

    enum ModifyMode		{ Shift, Remove };

    bool			setHorizons(const MultiID&,const MultiID&);
    void			setStaticHorizon(bool tophor);
    void			setMode(ModifyMode);

    void			doWork();

protected:

    bool			getNextNode(BinID&);
    bool			getNextNode3D(BinID&);
    bool			getNextNode2D(BinID&);
    void			getLines(const EM::Horizon*);
    float			getDepth2D(const EM::Horizon*,const BinID&);
    void			shiftNode(const BinID&);
    void			removeNode(const BinID&);

    EM::Horizon*		tophor_;
    EM::Horizon*		bothor_;

    bool			is2d_;
    TypeSet<Pos::GeomID>	geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;
    TrcKeySamplingIterator*	iter_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};
