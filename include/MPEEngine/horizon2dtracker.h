#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emtracker.h"

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

    static RefMan<Horizon2DTracker> create(EM::Horizon2D&);

private:
				Horizon2DTracker(EM::Horizon2D&);
				~Horizon2DTracker();

    bool			hasTrackingMgr() const override;
    bool			createMgr() override;
    void			startFromSeeds(const TypeSet<TrcKey>&) override;
    void			initTrackingMgr() override;
    bool			trackingInProgress() const override;
    void			updateFlatCubesContainer(const TrcKeyZSampling&,
						     bool addremove) override;
				/*!< add = true, remove = false. */
    void			stopTracking() override;

    bool			is2D() const override		{ return true; }

    SectionTracker*		createSectionTracker() override;
    EMSeedPicker*		getSeedPicker(
					bool createifnotpresent=true) override;

    Horizon2DSeedPicker*	seedpicker_	= nullptr;

};

} // namespace MPE
