#ifndef seistrc_h
#define seistrc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrc.h,v 1.20 2004-02-17 10:58:52 bert Exp $
________________________________________________________________________

-*/

#include <seisinfo.h>
#include <tracedata.h>
#include <datachar.h>
#include <datatrc.h>
class Interpolator1D;
class Socket;

/*!\brief Seismic traces

A seismic trace is composed of trace info and trace data. The trace data
consists of one or more components. These are represented by a set of buffers,
interpreted by DataInterpreters.

The number of samples of the components may vary, therfore the index of the
sample at the info().sampling.start can be set to non-zero. The first component
(icomp==0) has a sampleoffset of zero which cannot be set.

Interpolation between samples is automatic if you use the getValue() method.
The interpolation method can be customised.

*/

class SeisTrc
{
public:

			SeisTrc( int ns=0, const DataCharacteristics& dc
					    = DataCharacteristics() )
			: soffs_(0), intpols_(0)
						{ data_.addComponent(ns,dc); }
			SeisTrc( const SeisTrc& t )
			: soffs_(0), intpols_(0)
						{ *this = t; }
			~SeisTrc();
    SeisTrc&		operator =(const SeisTrc& t);

    SeisTrcInfo&	info()			{ return info_; }
    const SeisTrcInfo&	info() const		{ return info_; }
    TraceData&		data()			{ return data_; }
    const TraceData&	data() const		{ return data_; }

    inline int		sampleOffset( int icomp ) const
			{ return soffs_ && icomp && icomp < soffs_->size()
				? (*soffs_)[icomp] : 0; }
    void		setSampleOffset(int icomp,int);
    inline float	posOffset( int icomp ) const
			{ return sampleOffset(icomp) * info_.sampling.step; }


    inline void		set( int idx, float v, int icomp )
			{ data_.setValue( idx, v, icomp ); }
    inline float	get( int idx, int icomp ) const
			{ return data_.getValue(idx,icomp); }

    inline int		size( int icomp ) const
			{ return data_.size( icomp ); }
    inline double	getX( int idx, int icomp ) const
			{ return startPos(icomp) + idx * info_.sampling.step; }
    float		getValue(float,int icomp) const;

    const Interpolator1D* interpolator( int icomp=0 ) const;
			//!< May return null!
    Interpolator1D*	interpolator( int icomp=0 );
			//!< May return null!
    void		setInterpolator(Interpolator1D*,int icomp=0);
			//!< Passed Interpolator1D becomes mine
			//!< setData() will be called with appropriate args.

    inline bool		isNull( int icomp ) const
			{ return data_.isZero(icomp); }
    inline void		zero( int icomp=-1 )
			{ data_.zero( icomp ); }
    inline bool		dataPresent( float t, int icomp ) const
			{ return info_.dataPresent( t, size(icomp),
						    sampleOffset(icomp)); }
    inline SamplingData<float>	samplingData( int icomp ) const
			{ return SamplingData<float>( startPos(icomp),
						      info_.sampling.step); }
    inline float	startPos( int icomp ) const
			{ return info_.sampling.start + posOffset(icomp); }
    inline float	samplePos( int idx, int icomp ) const
			{ return info_.samplePos( idx, sampleOffset(icomp) ); }
    inline int		nearestSample( float pos, int icomp ) const
			{ return info_.nearestSample(pos,sampleOffset(icomp)); }
    void		setStartPos(float,int icomp=0);

    bool		reSize( int sz, int icomp, bool copydata=false )
			{ data_.reSize( sz, icomp, copydata );
			  return data_.allOk(); }
    SampleGate		sampleGate(const Interval<float>&,bool check,
				   int icomp) const;

    //! If !err, errors are handled trough the socket. withinfo : send info too.
    bool		putTo(Socket&,bool withinfo, BufferString* err=0) const;
    //! If !err, errors are handled trough the socket.
    bool		getFrom(Socket&, BufferString* err=0);


    static const float	snapdist;
    			//!< The relative distance from a sample below
    			//!< which no interpolation is done.

protected:

    TraceData		data_;
    SeisTrcInfo		info_;
    TypeSet<int>*	soffs_;
    ObjectSet<Interpolator1D>* intpols_;

private:

    void		cleanUp();

};


/*!> Seismic traces conforming the DataTrace interface.

One of the components of a SeisTrc can be selected to form a DataTrace.

*/

#ifndef mXFunctionIterTp
# define mXFunctionIterTp mPolyRet(FunctionIter,XFunctionIter)
#endif

class SeisDataTrc : public DataTrace
{
public:

			SeisDataTrc( SeisTrc& t, int comp=0 )
			: trc(t), ismutable(true), curcomp(comp)
							{}
			SeisDataTrc( const SeisTrc& t, int comp=0 )
			: trc(const_cast<SeisTrc&>(t)), ismutable(false)
			, curcomp(comp)			{}
    void		setComponent( int c )		{ curcomp = c; }
    int			component() const		{ return curcomp; }

    inline bool		set( int idx, float v )
			{ if ( ismutable ) trc.data().setValue( idx, v, curcomp );
			  return ismutable; }
    inline float	operator[]( int i ) const
			{ return trc.get( i, curcomp ); }
    int			getIndex( double val ) const
			{ return trc.info().nearestSample( val ); }
    double		getX( int idx ) const
			{ return trc.getX( idx, curcomp ); }
    float		getValue( double v ) const
			{ return trc.getValue( v, curcomp ); }

    inline int		size() const	{ return trc.size( curcomp ); }
    inline double	step() const	{ return trc.info().sampling.step; }
    double		start() const	{ return trc.startPos(curcomp); }

    bool		isMutable() const	{ return ismutable; }

protected:

    SeisTrc&		trc;
    int			curcomp;
    bool		ismutable;

    mXFunctionIterTp*	mkIter(bool, bool) const;

};


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
