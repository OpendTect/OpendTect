#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionadjuster.h"

#include "datapack.h"
#include "ranges.h"
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
			HorizonAdjuster(EM::Horizon&,EM::SectionID);
			~HorizonAdjuster();

    void		reset();
    int			nextStep();

    void		getNeededAttribs(
				TypeSet<Attrib::SelSpec>&) const;
    TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;

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
    void 		setAllowedVariance(float v);
    void		setAllowedVariances(const TypeSet<float>& avs);
    TypeSet<float>&	getAllowedVariances();
    float 		allowedVariance() const;
    void		setUseAbsThreshold(bool abs);
    bool		useAbsThreshold() const;

    void		setSimilarityWindow(const Interval<float>& rg);
    Interval<float>	similarityWindow() const;
    void		setSimilarityThreshold(float th);
    float		similarityThreshold() const;
    void		setSnapToEvent(bool);
    bool		snapToEvent() const;

    int			getNrAttributes() const;
    const Attrib::SelSpec* getAttributeSel(int idx) const;
    void		setAttributeSel(int idx,const Attrib::SelSpec&);

    bool		hasInitializedSetup() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    Attrib::SelSpec*	attribsel_;
    EM::Horizon&	horizon_;
    EventTracker&	evtracker_;

private:

    DataPackMgr&	dpm_;
    DataPack::ID	datapackid_;

    bool		track(const TrcKey&,const TrcKey&,float&) const;
    void		setHorizonPick(const TrcKey&,float val);

    static const char*	sKeyTracker()		{ return "Tracker"; }
    static const char*	sKeyAttribID()		{ return "Attribute"; }
};

} // namespace MPE

