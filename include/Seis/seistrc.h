#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datachar.h"
#include "mathfunc.h"
#include "seisinfo.h"
#include "tracedata.h"
#include "valseries.h"
#include "valseriesinterpol.h"

class UnitOfMeasure;
class VelocityDesc;
namespace ZDomain { class Info; }
template <class T> class Array1D;
template <class T> class ValueSeriesInterpolator;

/*!
\brief Seismic trace.

A seismic trace is composed of trace info and trace data. The trace data
consists of one or more components. These are represented by a set of buffers,
interpreted by DataInterpreters.
*/

mExpClass(Seis) SeisTrc
{
public:

			SeisTrc(int ns=0, const DataCharacteristics& dc
					   = DataCharacteristics());
			SeisTrc(const SeisTrc&);
			~SeisTrc();

    SeisTrc&		operator =(const SeisTrc&);
    inline bool		isEmpty() const		{ return data_.isEmpty(); }

    SeisTrcInfo&	info()			{ return info_; }
    const SeisTrcInfo&	info() const		{ return info_; }
    TraceData&		data()			{ return data_; }
    const TraceData&	data() const		{ return data_; }
    int			nrComponents() const	{ return data_.nrComponents(); }

    inline void		set( int idx, float v, int icomp )
			{ data_.setValue( idx, v, icomp ); }
    inline float	get( int idx, int icomp ) const
			{ return data_.getValue(idx,icomp); }
    inline float	getFirst( int icomp=0 ) const
			{ return isEmpty() ? 0 : get(0,icomp); }
    inline float	getLast( int icomp=0 ) const
			{ return isEmpty() ? 0 : get(size()-1,icomp); }

    inline int		size() const
			{ return data_.size(0); }
    float		getValue(float,int icomp) const;

    bool		isNull(int icomp=-1) const;
    bool		isUdf(int icomp=-1) const;
    bool		hasUndef(int icomp=-1) const;
    void		setAll(float,int icomp=-1);
    inline void		zero( int icomp=-1 )
			{ data_.zero( icomp ); }
    void		ensureNoUndefs(float withval=mUdf(float));
			//!< default is inter- and extrapolate
    void		reverse( int icomp=-1 );

    void		setNrComponents(int,DataCharacteristics::UserType dt
					=DataCharacteristics::Auto);
    bool		reSize(int,bool copydata);
    void		copyDataFrom(const SeisTrc&,int icomp=-1,
				     bool forcefloats=false);
			//!< icomp -1 (default) is all components

    static const float	snapdist; //!< Default 1e-4 /* mDeprecated */
			//!< relative distance from a sample below which no
			//!< interpolation is done. 99.9% chance default is OK.

    const ValueSeriesInterpolator<float>& interpolator() const;
    void		setInterpolator(ValueSeriesInterpolator<float>*);
			//!< becomes mine

    inline float	startPos() const
    { return info_.sampling_.start_; }
    inline float	endPos() const
    { return info_.sampling_.atIndex( size()-1 ); }
    inline StepInterval<float> zRange() const
			{ return StepInterval<float>(startPos(),endPos(),
						     info_.sampling_.step_ ); }
    inline float	samplePos( int idx ) const
			{ return info_.samplePos(idx); }
    inline int		nearestSample( float pos ) const
			{ return info_.nearestSample(pos); }
    void		setStartPos( float p )
    { info_.sampling_.start_ = p; }
    inline bool		dataPresent( float t ) const
			{ return info_.dataPresent(t,size()); }
    SampleGate		sampleGate(const Interval<float>&,bool check) const;

    SeisTrc*		getRelTrc(const ZGate&,float sr=mUdf(float)) const;
			//!< Resample around pick. No pick: returns null.
			//!< ZGate is relative to pick
    SeisTrc*		getExtendedTo(const ZGate&,bool usetrcvals=true) const;
			//!< Extends (or shrinks) trace to ZGate
			//!< Added values can be first/last value of input,
			//!< or zeros

    void		convertToFPs(bool preserve_data=true);
    bool		updateVelocities(const VelocityDesc& inpdesc,
				  const VelocityDesc& outdesc,
				  const ZDomain::Info&,double srd,
				  const UnitOfMeasure* srduom,
				  int icomp=-1,double t0=0.);
			/*!< Updates the velocity type and unit according
			     to provided VelocityDesc objects */

    static const char*	sKeyExtTrcToSI()
			{ return "Extend Traces To Survey Z Range"; }

protected:

    TraceData						data_;
    SeisTrcInfo						info_;
    mutable PtrMan<ValueSeriesInterpolator<float> >	intpol_;

private:

    void		cleanUp();

};


/*!> Seismic traces conforming the ValueSeries<float> interface.

One of the components of a SeisTrc can be selected to form a ValueSeries.

*/

mExpClass(Seis) SeisTrcValueSeries : public ValueSeries<float>
{
public:

		SeisTrcValueSeries( const SeisTrc& t, int c );
		~SeisTrcValueSeries();

    void	setComponent( int idx )			{ icomp_ = idx; }
    float	value( od_int64 idx ) const override;
    bool	writable() const override		{ return true; }
    void	setValue(od_int64 idx,float v) override;
    float*	arr() override;
    const float* arr() const override;

    od_int64	size() const override			{ return trc_.size(); }

    bool	copytoArray(Array1D<float>&);

    inline ValueSeries<float>*	clone() const override;

protected:

    SeisTrc&	trc_;
    int		icomp_;
};

/*!> Seismic traces conforming the MathFunction interface.

One of the components of a SeisTrc can be selected to form a ValueSeries.

*/


mExpClass(Seis) SeisTrcFunction : public FloatMathFunction
{
public:

		SeisTrcFunction(const SeisTrc& trc, int icomp);
		~SeisTrcFunction();

    float	getValue( float z ) const override
		{ return trc_.getValue(z,icomp_); }

    float	getValue( const float* p ) const { return getValue(*p); }

protected:

    const SeisTrc&	trc_;
    const int		icomp_;
};

inline ValueSeries<float>* SeisTrcValueSeries::clone() const
{ return new SeisTrcValueSeries( trc_, icomp_ ); }
