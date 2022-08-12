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
#include "trckeyvalue.h"


namespace EM { class Horizon2D; }

namespace MPE
{

/*!
\brief SectionExtender to extend EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DExtender : public SectionExtender
{
public:
				Horizon2DExtender(EM::Horizon2D&);
    static SectionExtender*	create(EM::EMObject*);
    static void			initClass();

    void			setAngleThreshold(float radians);
    float			getAngleThreshold() const;

    void			setDirection(const TrcKeyValue&) override;
    const TrcKeyValue*		getDirection() const override;
    void			setGeomID(Pos::GeomID);
    Pos::GeomID			geomID() const;

    int				nextStep() override;

// Deprecated public functions
    mDeprecated("Use without SectionID")
				Horizon2DExtender(EM::Horizon2D& h2d,
						  EM::SectionID)
				    : Horizon2DExtender(h2d)		{}
    mDeprecated("Use without SectionID")
    static SectionExtender*	create(EM::EMObject* obj,EM::SectionID)
				{ return create(obj); }

protected:

    void			addNeighbor(bool upwards,
					    const TrcKey& sourcesid);
    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const override;
    virtual void		prepareDataIfRequired() override	{}

    float			anglethreshold_		= 0.5f;
    bool			alldirs_		= true;
    Coord			xydirection_;
    TrcKeyValue			direction_;
    EM::Horizon2D&		hor2d_;
    Pos::GeomID			geomid_;
};

} // namespace MPE
