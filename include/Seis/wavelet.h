#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "mathfunc.h"
#include "namedobj.h"
#include "ranges.h"
#include "valseries.h"
#include "uistring.h"
class IOObj;
template <class T> class ValueSeriesInterpolator;


mExpClass(Seis) Wavelet : public NamedCallBacker
{ mODTextTranslationClass(Wavelet);
public:
			Wavelet(const char* nm=0);
			Wavelet(bool ricker_else_sinc,float fpeak,
				float sample_intv=mUdf(float),float scale=1);
			Wavelet(const Wavelet&);
    Wavelet&		operator=(const Wavelet&);
    virtual		~Wavelet();

    static Wavelet*	get(const IOObj*);
    static IOObj*	getIOObj(const char* wvltnm);
    bool		put(const IOObj*) const;

    float*		samples()		{ return samps_; }
    const float*	samples() const		{ return samps_; }
    inline void		set( int idx, float v )
			{ if ( isValidSample(idx) ) samps_[idx] = v; }
    inline float	get( int idx ) const
			{ return isValidSample(idx) ? samps_[idx] : 0.f; }
    inline bool		isValidSample( int idx ) const
			{ return idx>=0 && idx<sz_; }

    float		sampleRate() const	{ return dpos_; }
    int			centerSample() const	{ return cidx_; }
    StepInterval<float>	samplePositions() const
			{ return StepInterval<float>(
				-cidx_*dpos_, (sz_-cidx_-1)*dpos_, dpos_ ); }
    int			nearestSample(float z) const;
    bool		hasSymmetricalSamples()	{ return cidx_ * 2 + 1 == sz_; }

    int			size() const		{ return sz_; }
    float		getValue(float) const;

    void		setSampleRate(float sr)	{ dpos_ = sr; }
    void		setCenterSample(int cidx)	{ cidx_ = cidx; }
			//!< positive for starttwt < 0
    void		reSize(int); // destroys current sample data!

    bool		reSample(float newsr);
    bool		reSampleTime(float newsr);
    void		ensureSymmetricalSamples();
			//!< pads with zeros - use with and before reSample
			//  for better results
    void		transform(float b,float a);
			//!< a*X+b transformation
    void		normalize();
    bool		trimPaddedZeros(); //!< returns whether any change
    float		getExtrValue(bool ismax = true) const;
    void		getExtrValues(Interval<float>&) const;
    int			getPos(float val,bool closetocenteronly=false) const;

    const ValueSeriesInterpolator<float>& interpolator() const;
    void		setInterpolator(ValueSeriesInterpolator<float>*);
			//!< becomes mine

    static void		markScaled(const MultiID& id); //!< "External"
    static void		markScaled(const MultiID& id,const MultiID& orgid,
				   const MultiID& horid,const MultiID& seisid,
				   const char* lvlnm);
    static bool		isScaled(const MultiID&);
    static bool		isScaled(const MultiID& id,MultiID& orgid,
					MultiID& horid,MultiID& seisid,
					BufferString& lvlnm);
					//!< if external, orgid will be "0"

protected:

    float		dpos_;
    float*		samps_ = nullptr;
    int			sz_ = 0;
    int			cidx_;		//!< The index of the center sample
    ValueSeriesInterpolator<float>*	intpol_ = nullptr;

};


/*!> Wavelet conforming the ValueSeries<float> interface.

  The Wavelet can form a ValueSeries.

*/

mExpClass(Seis) WaveletValueSeries : public ValueSeries<float>
{ mODTextTranslationClass(WaveletValueSeries);
public:

			WaveletValueSeries( const Wavelet& wv )
			    :wv_(const_cast<Wavelet&>(wv)) {}

    float		value( od_int64 idx ) const override;
    bool		writable() const override		{ return true; }
    void		setValue( od_int64 idx,float v) override;
    float*		arr() override;
    const float*	arr() const override;
    od_int64		size() const override	{ return wv_.size(); }

    inline ValueSeries<float>*	clone() const override;

protected:

    Wavelet&	wv_;
};

/*!> Wavelet conforming the MathFunction interface.

  The Wavelet can form a MathFunction

*/


mExpClass(Seis) WaveletFunction : public FloatMathFunction
{ mODTextTranslationClass(WaveletFunction);
public:
		WaveletFunction(const Wavelet& wv)
		    : wv_(wv)
		{}

    float	getValue( float z ) const override { return wv_.getValue(z); }
    float	getValue( const float* p ) const { return getValue(*p); }

protected:

    const Wavelet& wv_;
};

inline ValueSeries<float>* WaveletValueSeries::clone() const
{ return new WaveletValueSeries( wv_ ); }
