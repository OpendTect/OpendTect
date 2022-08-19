#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionextender.h"
#include "trckeyvalue.h"

namespace EM { class Horizon3D; }

namespace MPE
{

/*!
\brief Sub class of SectionExtender. Use Horizon3DExtender instead.
*/

mExpClass(MPEEngine) BaseHorizon3DExtender : public SectionExtender
{
public:
    void			setDirection(const TrcKeyValue&) override;
    const TrcKeyValue*		getDirection() const override
				{ return &direction_; }

    int				nextStep() override;

    int				maxNrPosInExtArea() const override;
    void			preallocExtArea() override;

    const TrcKeyZSampling&	getExtBoundary() const override;

protected:
				BaseHorizon3DExtender(EM::Horizon3D&);

    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const override;

    TrcKeyValue			direction_;
    EM::Horizon3D&		horizon_;
};


/*!
\brief Used to extend EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DExtender : public BaseHorizon3DExtender
{
public:
    static void			initClass();
    static SectionExtender*	create(EM::EMObject*);

protected:
				Horizon3DExtender(EM::Horizon3D&);
};

} // namespace MPE
