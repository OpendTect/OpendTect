/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.6 2001-02-13 17:21:08 bert Exp $";

#include "seistrc.h"
#include "simpnumer.h"
#include <math.h>
#include <float.h>


void SeisTrc::setSampleOffset( int icomp, int so )
{
    if ( icomp >= data_.nrComponents() ) return;
    while ( soffs_.size() <= icomp ) soffs_ += 0;
    soffs_[icomp] = so;
}


float SeisTrc::getValue( double t, int icomp ) const
{
    static const float trcundef = 0;
    static const float snapdist = 1e-4;

    register float x = (t - startPos(icomp)) / info_.sampling.step;
    const_cast<SeisTrc*>(this)->curcomp = icomp;
    interpolateSampled( *this, size(icomp), x, x, false, trcundef, snapdist );

    return x;
}


SampleGate SeisTrc::sampleGate( const Interval<float>& tg, bool check,
				int icomp ) const
{
    SampleGate sg( info_.sampleGate(tg,sampleOffset(icomp)) );
    if ( !check ) return sg;

    int maxsz = size(icomp) - 1;
    if ( sg.start > maxsz ) sg.start = maxsz;
    if ( sg.stop > maxsz ) sg.stop = maxsz;

    return sg;
}


void SeisTrc::setStartPos( float pos, int icomp )
{
    if ( icomp == 0 )
	info_.sampling.start = pos;
    else
    {
	float offs = (pos - info_.sampling.start) / info_.sampling.step;
	setSampleOffset( icomp, mNINT(offs) );
    }
}


class SeisDataTrcIter : public XFunctionIter
{
public:
SeisDataTrcIter( SeisDataTrc* xf, bool bw, bool isc ) : XFunctionIter(xf,bw,isc)
{
    reset();
}

inline void reset()
{
    idx_ = td().size();
    if ( idx_ && !bw_ ) idx_ = 1;
}

inline double x() const
{
    return td().start() + (idx_-1) * td().step();
}

inline float y() const
{
    return idx_ ? td()[idx_-1] : 0;
}

bool next()
{
    if ( !idx_ ) return false;

    if ( bw_ ) idx_--;
    else if ( ++idx_ > td().size() ) idx_ = 0;

    return idx_;
}

bool setValue( float v )
{
    if ( !idx_ || !canSet() ) return false;

    td().set( idx_-1, v );

    return idx_;
}

bool remove( float v )
{
    if ( !idx_ || !canSet() ) return false;

    const int sz = td().size();
    for ( int idx=idx_; idx<sz; idx++ )
	td().set( idx-1, td()[idx] );

    return true;
}

inline const	SeisDataTrc& td() const	{ return *((SeisDataTrc*)func_); }
inline		SeisDataTrc& td()	{ return *((SeisDataTrc*)func_); }

};


XFunctionIter* SeisDataTrc::mkIter( bool bw, bool isc ) const
{
    return new SeisDataTrcIter( const_cast<SeisDataTrc*>(this), bw, isc );
}
