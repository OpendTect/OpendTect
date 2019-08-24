#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionextender.h"

#include "coord.h"
#include "posinfo2dsurv.h"
#include "trckey.h"


namespace EM { class Horizon2D; }

namespace MPE
{

/*!
\brief SectionExtender to extend EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DExtender : public SectionExtender
{ mODTextTranslationClass(Horizon2DExtender)
public:
				Horizon2DExtender(EM::Horizon2D&);
    static SectionExtender*	create(EM::Object*);
    static void			initClass();

    void			setAngleThreshold(float radians);
    float			getAngleThreshold() const;

    void			setDirection(const TrcKeyValue&);
    const TrcKeyValue*		getDirection() const;
    void			setGeomID(Pos::GeomID);
    Pos::GeomID			geomID() const;

    int				nextStep();

protected:

    void			addNeighbor(bool upwards,
					    const TrcKey& sourcesid);
    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const;
    virtual void		prepareDataIfRequired()		{}

    float			anglethreshold_;
    bool			alldirs_;
    Coord			xydirection_;
    TrcKeyValue			direction_;
    EM::Horizon2D&		hor2d_;
    Pos::GeomID			geomid_;
};

} // namespace MPE
