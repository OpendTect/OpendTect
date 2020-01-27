#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "sharedobject.h"
#include "mathfunc.h"
#include "ranges.h"
#include "valseries.h"
template <class T> class TypeSet;
template <class T> class ValueSeriesInterpolator;


mExpClass(Seis) Wavelet : public SharedObject
{
public:

    typedef int		size_type;
    typedef size_type	idx_type;
    typedef float	ValueType;
    typedef float	ZType;

			Wavelet(const char* nm=0);
			Wavelet(bool ricker_else_sinc,ValueType fpeak,
				ValueType sample_intv=mUdf(ValueType),
				ValueType scale=1);
			mDeclInstanceCreatedNotifierAccess(Wavelet);
			mDeclMonitorableAssignment(Wavelet);

    mImplSimpleMonitoredGetSet(inline,sampleRate,setSampleRate,
				    ZType,dpos_,cParChange())
    mImplSimpleMonitoredGetSet(inline,centerSample,setCenterSample,
				    idx_type,cidx_,cParChange())
    size_type		size() const;
    inline bool		isEmpty() const		    { return size() < 1; }
    bool		validIdx(idx_type) const;
    ValueType		get(idx_type) const;
    ValueType		getValue(ZType) const;
    void		set(idx_type,ValueType);
    ValueType*		getSamples() const;	//!< needs delete []
    void		getSamples(ValueType*) const;
    void		getSamples(TypeSet<ValueType>&) const;
    void		setSamples(const ValueType*,size_type);
    void		setSamples(const TypeSet<ValueType>&);
    const ValueSeriesInterpolator<ValueType>& interpolator() const;
    void		setInterpolator(ValueSeriesInterpolator<ValueType>*);
			//!< becomes mine
    void		reSize(size_type,ValueType val=0.f);

    StepInterval<ZType>	samplePositions() const;
    idx_type		nearestSample(ZType) const;
    bool		hasSymmetricalSamples() const;

    bool		reSample(ZType);
    bool		reSampleTime(ZType);
    void		ensureSymmetricalSamples();
			//!< pads with zeros - use with and before reSample
			//  for better results
    void		transform(ValueType b,ValueType a);
			//!< a*X+b transformation
    void		normalize();
    bool		trimPaddedZeros(); //!< returns whether any change
    ValueType		getExtrValue(bool ismax = true) const;
    void		getExtrValues(Interval<ZType>&) const;
    int			getPos(ZType val,bool closetocenteronly=false) const;

    static ChangeType	cParChange()	    { return 2; }
    static ChangeType	cSampleChange()	    { return 3; }

protected:

    virtual		~Wavelet();

    ValueType*		samps_;
    size_type		sz_;
    ZType		dpos_;		//!< delta Z, sample interval
    idx_type		cidx_;		//!< The index at pos == 0

    ValueSeriesInterpolator<ValueType>* intpol_;

    void		doReSize(int);
    StepInterval<ZType>	gtSamplePositions() const;

    friend class	WaveletValueSeries;

};


/*!> Wavelet conforming the ValueSeries interface. */

mExpClass(Seis) WaveletValueSeries : public ValueSeries<Wavelet::ValueType>
{
public:

			WaveletValueSeries(const Wavelet&);

    virtual ValueType	value(od_int64) const;
    virtual bool	writable() const		{ return true; }
    virtual void	setValue(od_int64 idx,ValueType);
    virtual ValueType*	arr();
    virtual const ValueType* arr() const;
    od_int64		size() const override	{ return wv_->size(); }

    inline ValueSeries<ValueType>*  clone() const
			{ return new WaveletValueSeries( *wv_ ); }

protected:

    RefMan<Wavelet>	wv_;
    MonitorLock		ml_;

};

/*!> Wavelet conforming the MathFunction interface. */


mExpClass(Seis) WaveletFunction : public FloatMathFunction
{
public:
		WaveletFunction(const Wavelet&);

    float	getValue( float z ) const	 { return wv_->getValue(z); }
    float	getValue( const float* p ) const { return getValue(*p); }

protected:

    ConstRefMan<Wavelet>	wv_;
    MonitorLock			ml_;

};
