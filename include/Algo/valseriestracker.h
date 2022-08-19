#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "valseriesevent.h"

template <class T> class ValueSeries;


/*!
\brief Base class for a tracker that tracks something (e.g. min, max,
a certain value)++ from one ValueSeries<float> to another.
*/

mExpClass(Algo) ValSeriesTracker
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

    virtual float	targetValue() const		{ return targetvalue_; }
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
    float			targetvalue_;
};


/*!
\brief Tracker that tracks Min/Max & Zero crossings between valueseries.
*/

mExpClass(Algo) EventTracker : public ValSeriesTracker
{
public:
				EventTracker();

    const char*			type() override	{ return sType(); }
    static const char*		sType()		{ return "EventTracker"; }

    enum CompareMethod		{ None, SeedTrace, AdjacentParent };
				mDeclareEnumUtils(CompareMethod)

    void			setCompareMethod(CompareMethod);
    CompareMethod		getCompareMethod() const;

    static const char**		sEventNames();
    static const VSEvent::Type*	cEventTypes();
    static int			getEventTypeIdx(VSEvent::Type);

    bool			isOK() const override;

    void			setSeed(const ValueSeries<float>*,int sz,
					float depth);

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
    void			allowAmplitudeSignChange(bool);
				/*! If true, a peak can have a negative
				    amplitude value, and a trough can have a
				    positive amplitude value.*/
    bool			isAmplitudeSignChangeAllowed() const;
    bool			snap(const Interval<float>& amplrg);
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
				    compareamplitude. The compare amplitude is
				    either extracted from the seed valseries or
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

    void			setSnapToEvent(bool);
    bool			snapToEvent() const;

    bool			track() override;
    float			quality() const	override { return quality_; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:

    ValueSeriesEvent<float,float>
			findExtreme(const ValueSeriesEvFinder<float, float>&,
			const Interval<float>&,float,float&,bool&,float&) const;
    ValueSeriesEvent<float,float>
			findExtreme(const ValueSeriesEvFinder<float, float>&,
				const Interval<float>& zrg,
				const Interval<float>& amplrg,float& avgampl,
				bool& hasloopskips,float& troughampl) const;
    bool		findMaxSimilarity(int nrtests,int step,int nrgracetests,
					 float& res,float& maxsim,
					 bool& flatstart) const;
    bool			isTargetValueAllowed() const;

    VSEvent::Type		evtype_;
    Interval<float>		permrange_;
    CompareMethod		comparemethod_;
    float			ampthreshold_;
    TypeSet<float>		ampthresholds_;
    float			allowedvar_;
    TypeSet<float>		allowedvars_;
    bool			useabsthreshold_;
    Interval<float>		similaritywin_;
    float			rangestep_;
    float			similaritythreshold_;
    bool			usesimilarity_;
    bool			normalizesimi_;
    float			compareampl_;
    bool			dosnap_;
    float			quality_;
    bool			allowamplsignchg_;

    const ValueSeries<float>*	seedvs_;
    float			seeddepth_;
    int				seedsize_;

    static const char*		sKeyPermittedRange();
    static const char*		sKeyValueThreshold();
    static const char*		sKeyValueThresholds();
    static const char*		sKeyAllowedVariance();
    static const char*		sKeyAllowedVariances();
    static const char*		sKeyUseAbsThreshold();
    static const char*		sKeySimWindow();
    static const char*		sKeySimThreshold();
    static const char*		sKeyNormSimi();
    static const char*		sKeyTrackByValue();
    static const char*		sKeyTrackEvent();
    static const char*		sKeyCompareMethod();
    static const char*		sKeyAttribID();
    static const char*		sKeySnapToEvent();
    static const char*		sKeyAllowSignChg();

};
