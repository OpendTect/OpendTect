#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "ranges.h"
#include "sectionadjuster.h"
#include "seisdatapack.h"
#include "valseriestracker.h"

class EventTracker;
namespace EM { class Horizon; }

namespace MPE
{

class SectionExtender;

/*!
\brief SectionAdjuster to adjust EM::Horizon.
*/

mExpClass(MPEEngine) HorizonAdjuster : public SectionAdjuster
{
public:
			HorizonAdjuster(EM::Horizon&);
			~HorizonAdjuster();

    void		reset() override;
    int			nextStep() override;

    void		getNeededAttribs(
				TypeSet<Attrib::SelSpec>&) const override;
    TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const override;

    void		setCompareMethod(EventTracker::CompareMethod);
    EventTracker::CompareMethod	getCompareMethod() const;

    void		setSearchWindow(const Interval<float>& rg);
    Interval<float>	searchWindow() const;
    void		setTrackByValue(bool yn);
    bool		trackByValue() const;
    void		setTrackEvent(VSEvent::Type ev);
    VSEvent::Type	trackEvent() const;
    void		allowAmplitudeSignChange(bool);
    bool		isAmplitudeSignChangeAllowed() const;

    void		setAmplitudeThreshold(float th);
    float		amplitudeThreshold() const;
    void		setAmplitudeThresholds(const TypeSet<float>& ats);
    TypeSet<float>&	getAmplitudeThresholds();
    void		setAllowedVariance(float v);
    void		setAllowedVariances(const TypeSet<float>& avs);
    TypeSet<float>&	getAllowedVariances();
    float		allowedVariance() const;
    void		setUseAbsThreshold(bool abs);
    bool		useAbsThreshold() const;

    void		setSimilarityWindow(const Interval<float>& rg);
    Interval<float>	similarityWindow() const;
    void		setSimilarityThreshold(float th);
    float		similarityThreshold() const;
    void		setSnapToEvent(bool);
    bool		snapToEvent() const;

    int			getNrAttributes() const override;
    const Attrib::SelSpec* getAttributeSel(int idx) const override;
    void		setAttributeSel(int idx,
					const Attrib::SelSpec&) override;

    bool		hasInitializedSetup() const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:

    Attrib::SelSpec*	attribsel_	= nullptr;
    EM::Horizon&	horizon_;
    EventTracker&	evtracker_;

private:

    WeakPtr<SeisDataPack> seisdp_;

    bool		track(const TrcKey&,const TrcKey&,float&) const;
    void		setHorizonPick(const TrcKey&,float val);

    static const char*	sKeyTracker()		{ return "Tracker"; }
    static const char*	sKeyAttribID()		{ return "Attribute"; }

public:

    mDeprecated("Use without SectionID")
			HorizonAdjuster(EM::Horizon& hor,EM::SectionID)
			    : HorizonAdjuster(hor)			{}
};

} // namespace MPE
