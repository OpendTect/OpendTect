#ifndef horizonadjuster_h
#define horizonadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: horizonadjuster.h,v 1.25 2009-07-20 11:48:33 cvsumesh Exp $
________________________________________________________________________

-*/

#include "sectionadjuster.h"
#include "ranges.h"
#include "trackplane.h"
#include "valseriesevent.h"

class IOPar;
namespace EM { class Horizon; };
namespace Attrib { class DataCubes; };
class EventTracker;


namespace MPE
{

class SectionExtender;

mClass HorizonAdjuster : public SectionAdjuster
{
public:
			HorizonAdjuster(EM::Horizon&,const EM::SectionID&);
			~HorizonAdjuster();

    void		reset();
    int			nextStep();

    void		getNeededAttribs(
	    			ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling	getAttribCube(const Attrib::SelSpec&) const;
    bool		is2D() const;

    void		setPermittedZRange(const Interval<float>& rg);
    Interval<float>	permittedZRange() const;
    void		setTrackByValue(bool yn);
    bool		trackByValue() const;
    void		setTrackEvent(VSEvent::Type ev);
    VSEvent::Type	trackEvent() const;

    void		setAmplitudeThreshold(float th);
    float		amplitudeThreshold() const;
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

    int			getNrAttributes() const;
    const Attrib::SelSpec* getAttributeSel(int idx) const;
    void		setAttributeSel(int idx,const Attrib::SelSpec&);

    bool		hasInitializedSetup() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    Attrib::SelSpec*		attribsel_;
    const Attrib::DataCubes*	attrdata_;
    EM::Horizon&		horizon_;
    EventTracker*		tracker_;

private:

    bool		track(const BinID&,const BinID&,float&) const;

    const BinID		attrDataBinId( const BinID& bid ) const;

    void		setHorizonPick(const BinID& bid, float val);

    static const char*	sKeyTracker()		{ return "Tracker"; }
    static const char*	sKeyAttribID()		{ return "Attribute"; }
};

}; // namespace MPE

#endif
