#ifndef horisonadjuster_h
#define horisonadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: horizonadjuster.h,v 1.6 2005-08-25 08:38:57 cvsduntao Exp $
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
    Interval<float>	similaritywin_;
    float		similaritythreshold_;
    bool		trackbyvalue_;

    bool		trackTrace( const BinID& refbid,
				const BinID& targetbid, float& targetz,
				float* refsamples = 0 );
    int			adjoiningExtremePos( const float* srctrc,
				int startsample, int endsample, float refval,
				float& matchval, bool& eqstart );
    float		adjoiningEventPosByValue( const float* srctrc,
                                int nrsamples, int refpos, float refval);
    int			matchingSampleBySimilarity( const float* srctrc,
                                int startsample, int endsample,
				const float* refval,
				float &matchratio, bool& eqfromstart );
    float 		adjoiningZeroEventPos( const float* srctrc,
    				int nrsamples, int startpos, VSEvent::Type );
    float 		firstZeroEventPos( const float* srctrc, int startsample,
    				int endsample, VSEvent::Type );
    float		exactExtremePos(const float *smplbuf,
    				int nrsamples, int pickpos, VSEvent::Type );
private:
    void		initTrackParam();
    int			matchwinsamples_;
    int			simlaritymatchpt_;

    void		setHorizonPick(const BinID& bid, float val);
    bool		getCompSamples( const PositionScoreComputerAttribData*,
    				const BinID& bid, float z, float* buf);
    const AttribPositionScoreComputer*
    			getAttribComputer() const;

    static const char*	permzrgstr_;
    static const char*	ampthresholdstr_;
    static const char*	similaritywinstr_;
    static const char*	similaritythresholdstr_;
    static const char*	trackbyvalstr_;
    static const char*	trackeventstr_;
};

}; // namespace MPE

#endif
