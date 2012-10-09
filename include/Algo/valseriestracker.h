#ifndef valseriestracker_h
#define valseriestracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Dec 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "valseriesevent.h"

class IOPar;
template <class T> class ValueSeries;

/*!Base class for a tracker that tracks something (e.g. min, max, a certain
   value)++ from one ValueSeries<float> to another. */

mClass ValSeriesTracker
{
public:
    			ValSeriesTracker();
    virtual		~ValSeriesTracker()		{}
    virtual const char* type()				= 0;

    virtual bool	isOK() const;
    			/*!<\returns whether the settings are OK, and it is
			     possible to track.*/
    virtual void	setSource(const ValueSeries<float>*,int sz,float depth);
    virtual void	setTarget(const ValueSeries<float>*,int sz,
	    			  float initialdepth);

    virtual bool	track()				= 0;
    			/*!<Calculates a new value for targetdepth_. */

    virtual float	targetDepth() const		{ return targetdepth_; }
    virtual float	quality() const			{ return 1; }

    virtual void	fillPar(IOPar& par) const	{}
    virtual bool	usePar(const IOPar& par)	{ return true; }

protected:

    const ValueSeries<float>*	sourcevs_;
    float			sourcedepth_;
    int				sourcesize_;
    const ValueSeries<float>*	targetvs_;
    float			targetdepth_;
    int				targetsize_;
};


/*!Tracker that tracks Min/Max & Zero crossings between valueseries. */


mClass EventTracker : public ValSeriesTracker
{
public:
    				EventTracker();
    const char*			type()		{ return sType(); }
    static const char*		sType()		{ return "EventTracker"; }

    static const char**		sEventNames();
    static const VSEvent::Type*	cEventTypes();
    static int			getEventTypeIdx(VSEvent::Type);

    virtual bool		isOK() const;

    void			setSource(const ValueSeries<float>*,int sz,
	    				  float depth);
    				//!<Will set sourceampl to udf
    void			setSourceAmpl(float v)	{ sourceampl_ = v; }
    				//!<If udf, sourceampl will be extracted from
				//!<source vs.
				
    float			getSourceAmpl() const	{ return sourceampl_; }

    void			setRangeStep(float step) { rangestep_ = step; }
    float			getRangeStep() const { return rangestep_; }
    void			setPermittedRange(const Interval<float>& rg);
    				//<!Is divided by rangestep to get nrof samples
    const Interval<float>&	permittedRange() const;
    void			setTrackEvent(VSEvent::Type ev);
    				/*!<
				    - VSEvent::Max
				       Will find max event within the permitted
				       range where the amplitude is higher than
				       the threshold.
    				    - VSEvent::Min
				       Will find minimum event within the
				       permitted range where the amplitude is
				       lower than the threshold.
    				    - VSEvent::ZCNegPos
				       Will find zerocrossing (Neg to Pos )
				       event within the permitted range. No
				       amplitude threshold is used.
    				    - VSEvent::ZCPosNeg
				       Will find zerocrossing (Pos to Neg)
				       event within the permitted range. No
				       amplitude threshold is used. */
    VSEvent::Type		trackEvent() const;
    bool			snap(float threshold);
				/*!Snaps at nearest event that is in permitted
				   range and where the amplitude meets the
				   threshold criterion. Only needs target
				   data.*/

    void			useSimilarity(bool yn);
    bool			usesSimilarity() const;
    void			normalizeSimilarityValues(bool yn);
    bool			normalizesSimilarityValues() const;

    void			setUseAbsThreshold(bool abs);
    				/*!<If on, the amplitude threshold
				    is set by setAmplitudeThreshold().
				    If off, the amplitude threshold
				    is set by (1-allowedVariance()) *
				    sourceamplitude. The source amplitude is
				    either set by setSourceAmpl or extracted
				    from source valseries. */
    bool			useAbsThreshold() const; 

    void			setAmplitudeThreshold(float th);
    				//!<Must be set if using absolute threshold.
    float			amplitudeThreshold() const;

    void			setAmplitudeThresholds(const TypeSet<float>&);
    TypeSet<float>&		getAmplitudeThresholds();

    void			setAllowedVariance(float v);
    				//!<Only used if not using absolute threshold
    float			allowedVariance() const;

    void			setAllowedVariances(const TypeSet<float>& avs);
    TypeSet<float>&		getAllowedVariances();

    void			setSimilarityWindow(const Interval<float>& rg);
    const Interval<float>&	similarityWindow() const;
    void			setSimilarityThreshold(float th);
    float			similarityThreshold() const;

    bool			track();
    float			quality() const		{ return quality_; }

    void			fillPar(IOPar& par) const;
    bool			usePar(const IOPar& par);
protected:

    ValueSeriesEvent<float,float>
			findExtreme(const ValueSeriesEvFinder<float, float>&,
			const Interval<float>&,float,float&,bool&,float&) const;
    bool		findMaxSimilarity(int nrtests,int step,int nrgracetests,
					 float& res,float& maxsim,
					 bool& flatstart) const;

    VSEvent::Type	evtype_;
    Interval<float>	permrange_;
    float		ampthreshold_;
    TypeSet<float>	ampthresholds_;
    float		allowedvar_;
    TypeSet<float>	allowedvars_;
    bool		useabsthreshold_;
    Interval<float>	similaritywin_;
    float		rangestep_;
    float		similaritythreshold_;
    bool		usesimilarity_;
    bool		normalizesimi_;
    bool		sourceampl_;

    float		quality_;

    static const char*	sKeyPermittedRange()	{ return "Permitted range"; }
    static const char*	sKeyValueThreshold()	{ return "Value threshhold"; }
    static const char*	sKeyValueThresholds()	{ return "Value threshholds"; }
    static const char*	sKeyAllowedVariance()	{ return "Allowed variance"; }
    static const char*	sKeyAllowedVariances()	{ return "Allowed variances"; }
    static const char*	sKeyUseAbsThreshold()	{ return "Use abs threshhold"; }
    static const char*	sKeySimWindow()		{ return "Similarity window"; }
    static const char*	sKeySimThreshold() { return "Similarity threshhold"; }
    static const char*	sKeyNormSimi() { return "Normalize similarity"; }
    static const char*	sKeyTrackByValue()	{ return "Track by value"; }
    static const char*	sKeyTrackEvent()	{ return "Track event"; }
    static const char*	sKeyAttribID()		{ return "Attribute"; }

};


#endif
