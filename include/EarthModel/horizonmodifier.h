#ifndef horizonmodifier_h
#define horizonmodifier_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"
#include "surv2dgeom.h"

namespace EM { class Horizon; }
class BinID;
class BufferStringSet;
class HorSamplingIterator;

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
    TypeSet<PosInfo::GeomID>	geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;
    HorSamplingIterator*	iter_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};


#endif

