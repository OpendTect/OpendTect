#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
 Contents:      Ranges
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionextender.h"
#include "trckey.h"

namespace EM { class Horizon3D; }

namespace MPE
{

/*!
\brief Sub class of SectionExtender. Use Horizon3DExtender instead.
*/

mExpClass(MPEEngine) BaseHorizon3DExtender : public SectionExtender
{ mODTextTranslationClass(BaseHorizon3DExtender)
public:
    void			setDirection(const TrcKeyValue&);
    const TrcKeyValue*		getDirection() const { return &direction_; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const TrcKeyZSampling&	getExtBoundary() const;

protected:
				BaseHorizon3DExtender(EM::Horizon3D&);

    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const;

    TrcKeyValue			direction_;
    EM::Horizon3D&		horizon_;
};


/*!
\brief Used to extend EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DExtender : public BaseHorizon3DExtender
{ mODTextTranslationClass(Horizon3DExtender)
public:
    static void			initClass();
    static SectionExtender*	create(EM::Object*);
				Horizon3DExtender(EM::Horizon3D&);
};

} // namespace MPE
