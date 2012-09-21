#ifndef seistrc_h
#define seistrc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisinfo.h"
#include "tracedata.h"
#include "datachar.h"
#include "valseries.h"
#include "mathfunc.h"

class TcpSocket;
template <class T> class ValueSeriesInterpolator;

/*!\brief Seismic traces

A seismic trace is composed of trace info and trace data. The trace data
consists of one or more components. These are represented by a set of buffers,
interpreted by DataInterpreters.

*/

mClass(Seis) SeisTrc
{
public:

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
    inline void		zero( int icomp=-1 )
			{ data_.zero( icomp ); }
    bool		reSize(int,bool copydata);
    void		copyDataFrom(const SeisTrc&,int icomp=-1,
	    			     bool forcefloats=false);
			//!< icomp -1 (default) is all components

    static const float	snapdist; //!< Default 1e-4
    			//!< relative distance from a sample below which no
    			//!< interpolation is done. 99.9% chance default is OK.

    const ValueSeriesInterpolator<float>& interpolator() const;
    void		setInterpolator(ValueSeriesInterpolator<float>*);
    			//!< becomes mine

    inline float	startPos() const
			{ return info_.sampling.start; }
    inline float	endPos() const
			{ return info_.sampling.atIndex( size()-1 ); }
    inline StepInterval<float> zRange() const
			{ return StepInterval<float>(startPos(),endPos(),
					info_.sampling.step ); }
    inline float	samplePos( int idx ) const
			{ return info_.samplePos(idx); }
    inline int		nearestSample( float pos ) const
			{ return info_.nearestSample(pos); }
    void		setStartPos( float p )
			{ info_.sampling.start = p; }
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

    bool		isWriteReady(const SamplingData<float>&,int ns) const;
    void		getWriteReady(SeisTrc&,SamplingData<float>&,int&) const;

    static const char*	sKeyExtTrcToSI()
    			{ return "Extend Traces To Survey Z Range"; }

protected:

    TraceData				data_;
    SeisTrcInfo				info_;
    ValueSeriesInterpolator<float>*	intpol_;

private:

    void		cleanUp();

};


/*!> Seismic traces conforming the ValueSeries<float> interface.

One of the components of a SeisTrc can be selected to form a ValueSeries.

*/

mClass(Seis) SeisTrcValueSeries : public ValueSeries<float>
{
public:

    		SeisTrcValueSeries( const SeisTrc& t, int c )
		    : trc_(const_cast<SeisTrc&>(t))
		    , icomp_(c)			{}

    void	setComponent( int idx )		{ icomp_ = idx; }
    float	value( od_int64 idx ) const	{ return trc_.get(idx,icomp_); }
    bool	writable() const		{ return true; }
    void	setValue( od_int64 idx,float v)	{ trc_.set(idx,v,icomp_); }
    float*	arr();
    const float* arr() const;

    inline ValueSeries<float>*	clone() const;

protected:

    SeisTrc&	trc_;
    int		icomp_;
};

/*!> Seismic traces conforming the MathFunction interface.

One of the components of a SeisTrc can be selected to form a ValueSeries.

*/


mClass(Seis) SeisTrcFunction : public FloatMathFunction
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


/*!\mainpage Seismics

Seismic data is sampled data along a vertical axis. Many 'traces' will usually
occupy a volume (3D seismics) or separate lines (2D data).

There's always lots of data, so it has to be stored efficiently. A consequence
is that storage on disk versus usage in memory are - contrary to most other data
types - closely linked. Instead of just loading the data in one go, we always
need to prepare a subcube of data before the work starts.

Although this model may have its flaws and may be outdated in the light of ever
increaing computer memory, it will probalbly satisfy our needs for some time
at the start of the 21st century.

The SeisTrc class is designed to be able to even have 1, 2 or 4-byte data in
the data() - the access functions get() and set() will know how to unpack and
pack this from/to float. SeisTrc objects can also hold more than one component.

To keep the SeisTrc object small, a lot of operations and processing options
have been moved to specialised objects - see seistrcprop.h and
seissingtrcproc.h .

*/

#endif

