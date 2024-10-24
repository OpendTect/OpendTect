#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emtracker.h"

namespace EM { class Horizon3D; }

namespace MPE
{

class Horizon3DSeedPicker;
class HorizonTrackerMgr;

/*!
\brief EMTracker to track EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DTracker : public EMTracker
{
public:

    static RefMan<Horizon3DTracker> create(EM::Horizon3D&);

    void			updateFlatCubesContainer(const TrcKeyZSampling&,
						     bool addremove) override;
				/*!< add = true, remove = false. */
private:
				Horizon3DTracker(EM::Horizon3D&);
				~Horizon3DTracker();

    bool			is2D() const override	       { return false; }

    bool			hasTrackingMgr() const override;
    bool			createMgr() override;
    void			startFromSeeds(const TypeSet<TrcKey>&) override;
    void			initTrackingMgr() override;
    bool			trackingInProgress() const override;
    void			stopTracking() override;

    SectionTracker*		createSectionTracker() override;
    EMSeedPicker*		getSeedPicker(
					bool createifnotpresent=true) override;

    Horizon3DSeedPicker*	seedpicker_	= nullptr;
    HorizonTrackerMgr*		htmgr_		= nullptr;

};

} // namespace MPE
