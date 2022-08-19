#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon2D; }

namespace MPE
{

class Horizon2DSeedPicker;

/*!
\brief EMTracker to track EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DTracker : public EMTracker
{
public:
				Horizon2DTracker(EM::Horizon2D* =0);

    static EMTracker*		create(EM::EMObject* =0);
    static void			initClass();

    bool			is2D() const override		{ return true; }
    EMSeedPicker*		getSeedPicker(
					bool createifnotpresent=true) override;

    static const char*		keyword();

protected:

				~Horizon2DTracker();

    EM::Horizon2D*		getHorizon2D();
    const EM::Horizon2D*	getHorizon2D() const;

    SectionTracker*		createSectionTracker() override;
    Horizon2DSeedPicker*	seedpicker_;
};

} // namespace MPE
