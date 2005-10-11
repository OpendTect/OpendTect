#ifndef horisonadjuster_h
#define horisonadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: horizonadjuster.h,v 1.9 2005-10-11 20:00:21 cvskris Exp $
________________________________________________________________________

-*/

#include "sectionadjuster.h"
#include "ranges.h"
#include "trackplane.h"
#include "valseriesevent.h"

class IOPar;
namespace EM { class Horizon; };


namespace MPE
{

class AttribPositionScoreComputer;
class PositionScoreComputerAttribData;
class SectionExtender;

class HorizonAdjuster : public SectionAdjuster
{
public:
			HorizonAdjuster(EM::Horizon&,
						const EM::SectionID&);

    virtual void	reset() {};
    int			nextStep();

    void		setPermittedZRange(const Interval<float>& rg) 
    			    { permzrange_ = rg; }
    Interval<float>	permittedZRange() const
    			    { return permzrange_; }
    void		setTrackByValue(bool yn)
    			    { trackbyvalue_ = yn; };
    bool		trackByValue()
    			    { return trackbyvalue_; };
    void		setTrackEvent( VSEvent::Type ev )
			    { trackevent_ = ev; }
    VSEvent::Type	trackEvent()
			    { return trackevent_; }

    void		setAmplitudeThreshold(float th)
    			    { ampthreshold_ = th; }
    float		amplitudeTreshold() const
			    { return ampthreshold_; }
    void 		setAllowedVariance(float v)
    			    { allowedvar_ = v; }
    float 		allowedVariance()
    			    { return allowedvar_; }
    void		setUseAbsThreshold(bool abs)
			    { useabsthreshold_ = abs; }
    bool		useAbsThreshold()
			    { return useabsthreshold_; }
                                                          
    void		setSimilarityWindow(const Interval<float>& rg)
    			    { similaritywin_ = rg; }
    Interval<float>	similarityWindow() const
    			    { return similaritywin_; }
    void		setSimiliarityThreshold(float th)
    			    { similaritythreshold_ = th; }
    float		similarityThreshold()
    			    { return similaritythreshold_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    EM::Horizon&	horizon_;
    VSEvent::Type	trackevent_;
    Interval<float>	permzrange_;
    float		ampthreshold_;
    float		allowedvar_;
    bool		useabsthreshold_;
    Interval<float>	similaritywin_;
    float		similaritythreshold_;
    bool		trackbyvalue_;

    bool		trackTrace( const BinID& refbid,
				const BinID& targetbid, float& targetz,
				float* refsamples = 0 );
    int			adjoiningExtremePos( VSEvent::Type, const float*,
				int startsample, int endsample, float refval,
				float& matchval, bool& eqstart );
    float		adjoiningEventPosByValue( VSEvent::Type,  const float*,
                                int nrsamples, int refpos, float refval);
    float 		adjoiningZeroEventPos( VSEvent::Type, const float*,
    				int nrsamples, int startpos );
    float 		firstZeroEventPos( VSEvent::Type, const float*,
    				int startsample, int endsample );
    float		exactExtremePos( VSEvent::Type, const float*,
    				int nrsamples, int pickpos );
    int			matchingSampleBySimilarity( const float*,
                                int startsample, int endsample,
				const float* refvals,
				float &matchratio, bool& eqfromstart );
private:
    void		initTrackParam();
    int			matchwinsamples_;
    int			simlaritymatchpt_;

    void		setHorizonPick(const BinID& bid, float val);
    bool		getCompSamples( const PositionScoreComputerAttribData*,
    				const BinID& bid, float z, float* buf);
    const AttribPositionScoreComputer*
    			getAttribComputer() const;

    static const char*	sKeyPermittedZRange()	{ return "Permitted Z range"; }
    static const char*	sKeyValueThreshold()	{ return "Value threshhold"; }
    static const char*	sKeyAllowedVariance()	{ return "Allowed variance"; }
    static const char*	sKeyUseAbsThreshold()	{ return "Use abs threshhold"; }
    static const char*	sKeySimWindow()		{ return "Similarity window"; }
    static const char*	sKeySimThreshold() { return "Similarity threshhold"; }
    static const char*	sKeyTrackByValue()	{ return "Track by value"; }
    static const char*	sKeyTrackEvent()	{ return "Track event"; }
};

}; // namespace MPE

#endif
