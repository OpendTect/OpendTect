#ifndef horisonadjuster_h
#define horisonadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: horizonadjuster.h,v 1.12 2005-11-29 21:37:26 cvskris Exp $
________________________________________________________________________

-*/

#include "sectionadjuster.h"
#include "ranges.h"
#include "trackplane.h"
#include "valseriesevent.h"

class IOPar;
namespace EM { class Horizon; };
namespace Attrib { class DataCubes; };


namespace MPE
{

class SectionExtender;

class HorizonAdjuster : public SectionAdjuster
{
public:
			HorizonAdjuster(EM::Horizon&, const EM::SectionID&);
			~HorizonAdjuster();

    void		reset();
    int			nextStep();

    void		getNeededAttribs(
	    			ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling	getAttribCube(const Attrib::SelSpec&) const;

    void		setPermittedZRange(const Interval<float>& rg);
    Interval<float>	permittedZRange() const;
    void		setTrackByValue(bool yn);
    bool		trackByValue() const;
    void		setTrackEvent( VSEvent::Type ev );
    VSEvent::Type	trackEvent() const;

    void		setAmplitudeThreshold(float th);
    float		amplitudeTreshold() const;
    void 		setAllowedVariance(float v);
    float 		allowedVariance() const;
    void		setUseAbsThreshold(bool abs);
    bool		useAbsThreshold() const;
                                                          
    void		setSimilarityWindow(const Interval<float>& rg);
    Interval<float>	similarityWindow() const;
    void		setSimiliarityThreshold(float th);
    float		similarityThreshold();

    int				getNrAttributes() const;
    const Attrib::SelSpec*	getAttributeSel( int idx ) const;
    void			setAttributeSel( int, const Attrib::SelSpec& );

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    Attrib::SelSpec&		attribsel;
    const Attrib::DataCubes*	attrdata;
    EM::Horizon&		horizon_;
    VSEvent::Type		evtype;
    Interval<float>		permzrange_;
    float			ampthreshold_;
    float			allowedvar_;
    bool			useabsthreshold_;
    Interval<float>		similaritywin_;
    float			similaritythreshold_;
    bool			trackbyvalue_;

private:

    bool		trackByAmplitude( const BinID& refbid,
					  const BinID& targetbid,
					  float& targetz ) const;

    bool		trackBySimilarity( const BinID& refbid,
					  const BinID& targetbid,
					  float& targetz ) const;

    bool		findMaxSimilarity( const float* fixedarr,
					   const float* slidingarr,
					   int nrsamples, int nrtests,
					   int step, int nrgracesamples,
					   float& res, float& maxsim,
					   bool& flatstart ) const;
    			/*!<Slides slidingarr as long as similarity increases.
			    \param nrgracesamples Number of decreasing
			    	   similarities that are allowed in a row
				   without stopping the search.
			    \param step Specifies how the slidingarr should
			    	   slide. Normally 1 or -1.
			    \param res Position of maximum similarity
			    \param maxsim Highest similarity found.
			    \param flatstart Is true if the similarity didn't
			    	   change from start.
			    \returns true if a max similarity was found that
			    	     is above similaritythreshold_ */

     ValueSeriesEvent<float, float>
	 		findExtreme( const ValueSeriesEvFinder<float, float>&,
				 const Interval<float>& rg, float threshold,
				 float& avgampl ) const;
     			/*!<\param avgamp	The average value from the
						rg.start to the found extreme.
     			    \param threshold	The min/max amplitude that is
			    			allowed.*/

    bool		snap( const BinID& bid, float threshold,
	    		      float& targetz ) const;

    void		setHorizonPick(const BinID& bid, float val);

    static const char*	sKeyPermittedZRange()	{ return "Permitted Z range"; }
    static const char*	sKeyValueThreshold()	{ return "Value threshhold"; }
    static const char*	sKeyAllowedVariance()	{ return "Allowed variance"; }
    static const char*	sKeyUseAbsThreshold()	{ return "Use abs threshhold"; }
    static const char*	sKeySimWindow()		{ return "Similarity window"; }
    static const char*	sKeySimThreshold() { return "Similarity threshhold"; }
    static const char*	sKeyTrackByValue()	{ return "Track by value"; }
    static const char*	sKeyTrackEvent()	{ return "Track event"; }
    static const char*	sKeyAttribID()		{ return "Attribute"; }
};

}; // namespace MPE

#endif
