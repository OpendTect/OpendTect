#ifndef valseriestracker_h
#define valseriestracker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Dec 2006
 RCS:		$Id: valseriestracker.h,v 1.4 2008-02-05 20:43:48 cvskris Exp $
________________________________________________________________________

-*/

#include "valseriesevent.h"

class IOPar;
template <class T> class ValueSeries;

/*!Base class for a tracker that tracks something (e.g. min, max, a certain
   value)++ from one ValueSeries<float> to another. */

class ValSeriesTracker
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


class EventTracker : public ValSeriesTracker
{
public:
    				EventTracker();
    const char*			type()		{ return sType(); }
    static const char*		sType()		{ return "EventTracker"; }

    static const char**		sEventNames();
    static const VSEvent::Type*	cEventTypes();
    static int			getEventTypeIdx(VSEvent::Type);

    virtual bool		isOK() const;

    void			setPermittedZRange(const Interval<int>& rg);
    				/*!<In samples*/
    const Interval<int>&	permittedZRange() const;
    				/*!<In samples*/
    void			setTrackEvent(VSEvent::Type ev);
    VSEvent::Type		trackEvent() const;
    bool			snap(float threshold);
				/*!Snaps at nearest event. Only
				   needs target data.*/

    void			useSimilarity(bool yn);
    bool			usesSimilarity() const;
    void			setAmplitudeThreshold(float th);
    float			amplitudeThreshold() const;
    void			setAllowedVariance(float v);
    float			allowedVariance() const;
    void			setUseAbsThreshold(bool abs);
    bool			useAbsThreshold() const; 

    void			setSimilarityWindow(const Interval<int>& rg);
    const Interval<int>&	similarityWindow() const;
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
    Interval<int>	permzrange_;
    float		ampthreshold_;
    float		allowedvar_;
    bool		useabsthreshold_;
    Interval<int>	similaritywin_;
    float		similaritythreshold_;
    bool		usesimilarity_;

    float		quality_;

    static const char*	sKeyPermittedZRange()   { return "Permitted Z range"; }
    static const char*	sKeyValueThreshold()    { return "Value threshhold"; }
    static const char*	sKeyAllowedVariance()   { return "Allowed variance"; }
    static const char*	sKeyUseAbsThreshold()   { return "Use abs threshhold"; }
    static const char*	sKeySimWindow()         { return "Similarity window"; }
    static const char*	sKeySimThreshold() { return "Similarity threshhold"; }
    static const char*	sKeyTrackByValue()      { return "Track by value"; }
    static const char*	sKeyTrackEvent()        { return "Track event"; }
    static const char*	sKeyAttribID()          { return "Attribute"; }

};


#endif
