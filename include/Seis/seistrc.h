#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		10-5-1995
________________________________________________________________________

-*/

#include "seisinfo.h"
#include "tracedata.h"
#include "datachar.h"
#include "valseries.h"
#include "mathfunc.h"
#include "odmemory.h"
#include "valseriesinterpol.h"
#include "geomid.h"

template <class T> class ValueSeriesInterpolator;
template <class T> class Array1D;

/*!
\brief Seismic trace.

A seismic trace is composed of trace info and trace data. The trace data
consists of one or more components. These are represented by a set of buffers,
interpreted by DataInterpreters.
*/

mExpClass(Seis) SeisTrc
{
public:

    mUseType( Pos,	GeomID );

			SeisTrc( int ns=0, const DataCharacteristics& dc
					   = DataCharacteristics() )
			: intpol_(0)
						{ data_.addComponent(ns,dc); }
			SeisTrc( const SeisTrc& t )
			: intpol_(0)
						{ *this = t; }
			~SeisTrc();
    SeisTrc&		operator =(const SeisTrc& t);
    inline bool		isEmpty() const		{ return data_.isEmpty(); }

    SeisTrcInfo&	info()			{ return info_; }
    const SeisTrcInfo&	info() const		{ return info_; }
    TraceData&		data()			{ return data_; }
    const TraceData&	data() const		{ return data_; }
    int			nrComponents() const	{ return data_.nrComponents(); }
    template <class T>
    T*			arr( int icomp=0 )	{ return data_.arr<T>(icomp); }
    template <class T>
    const T*		arr( int icomp=0 ) const { return data_.arr<T>(icomp); }
    template <class T>
    void		setArr(const T* vals,int icomp=0);

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
    inline void		zero( int icomp=-1 )	    { data_.zero( icomp ); }
    void		setAll(float,int icomp=-1);
    bool		hasUndefs(int icomp=-1) const;
    void		ensureNoUndefs(float withval=mUdf(float));
			//!< default is inter- and extrapolate

    void		setNrComponents(int,OD::DataRepType dt=OD::AutoDataRep);
    void		addComponent( int ns=0,
				      const DataCharacteristics& dc
						= DataCharacteristics(),
				      bool cleardata=false )
			{ data_.addComponent( ns, dc, cleardata ); }
    void		removeComponent( int icomp )
			{ data_.delComponent( icomp ); }
    bool		reSize(int,bool copydata);
    bool		setSize( int sz )	{ return reSize( sz, false ); }
    void		erase()			{ reSize( 0, false); }
    void		copyDataFrom(const SeisTrc&,int icomp=-1,
				     bool forcefloats=false);
			//!< icomp -1 (default) is all components

    float*		arr(int icomp);
    const float*	arr(int icomp) const;
    bool		copytoArray(Array1D<float>&,int icomp) const;
    void		copyFromArray(const Array1D<float>&,int icomp);

    const ValueSeriesInterpolator<float>& interpolator() const;
    void		setInterpolator(ValueSeriesInterpolator<float>*);
			//!< becomes mine

			//!, sme convenient shortcuts into info()
    inline bool		is2D() const
			{ return info_.is2D(); }
    inline GeomID	geomID() const
			{ return info_.geomID(); }
    inline float	startPos() const
			{ return info_.sampling_.start; }
    inline float	endPos() const
			{ return info_.sampling_.atIndex( size()-1 ); }
    inline float	stepPos() const
			{ return info_.sampling_.step; }
    inline float	zStep() const
			{ return info_.sampling_.step; }
    inline ZSampling	zRange() const
			{ return ZSampling(startPos(),endPos(),stepPos()); }
    inline float	samplePos( int idx ) const
			{ return info_.samplePos(idx); }
    inline float	zPos( int idx ) const
			{ return info_.samplePos(idx); }
    inline int		nearestSample( float pos ) const
			{ return info_.nearestSample(pos); }
    void		setStartPos( float p )
			{ info_.sampling_.start = p; }
    void		setZStart( float p )
			{ info_.sampling_.start = p; }
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

    static const char*	sKeyExtTrcToSI()
			{ return "Extend Traces To Survey Z Range"; }

protected:

    TraceData						data_;
    SeisTrcInfo						info_;
    mutable PtrMan<ValueSeriesInterpolator<float> >	intpol_;

    bool		chkForSpecVal(int icomp,bool isnull) const;

private:

    void		cleanUp();

};


template <class T>
inline void SeisTrc::setArr( const T* vals, int icomp )
{
    const auto sz = size();
    auto* myarr = arr<float>(icomp);
    if ( vals && myarr && sz>0 )
	OD::sysMemCopy( myarr, vals, sz*sizeof(T) );
}


/*!> Seismic traces conforming the ValueSeries<float> interface.

One of the components of a SeisTrc can be selected to form a ValueSeries.

*/

mExpClass(Seis) SeisTrcValueSeries : public ValueSeries<float>
{
public:

		SeisTrcValueSeries( const SeisTrc& t, int c )
		    : trc_(const_cast<SeisTrc&>(t))
		    , icomp_(c)			{}

    void	setComponent( int idx )		{ icomp_ = idx; }
    float	value( od_int64 idx ) const;
    bool	writable() const		{ return true; }
    void	setValue( od_int64 idx,float v);
    float*	arr();
    const float* arr() const;

    od_int64	size() const override	{ return trc_.size(); }

    bool	copytoArray(Array1D<float>&);

    inline ValueSeries<float>*	clone() const;

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

		SeisTrcFunction(const SeisTrc& trc, int icomp)
		    : trc_(trc), icomp_(icomp)			{}

    float	getValue( float z ) const { return trc_.getValue(z,icomp_); }
    float	getValue( const float* p ) const { return getValue(*p); }

protected:

    const SeisTrc&	trc_;
    const int		icomp_;
};

inline ValueSeries<float>* SeisTrcValueSeries::clone() const
{ return new SeisTrcValueSeries( trc_, icomp_ ); }
