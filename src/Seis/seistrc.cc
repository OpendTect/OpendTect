/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.10 2001-11-08 09:58:12 kristofer Exp $";

#include "seistrc.h"
#include "simpnumer.h"
#include "interpol1d.h"
#include <math.h>
#include <float.h>


SeisTrc::~SeisTrc()
{
    cleanUp();
}


void SeisTrc::cleanUp()
{
    delete soffs_; soffs_ = 0;
    delete scalers_; scalers_ = 0;
    if ( intpols_ )
	{ deepErase( *intpols_ ); delete intpols_; intpols_ = 0; }
}


SeisTrc& SeisTrc::operator =( const SeisTrc& t )
{
    if ( &t == this ) return *this;

    cleanUp();
    info_ = t.info_;
    data_ = t.data_;

    if ( t.soffs_ )
	soffs_ = new TypeSet<int>( *t.soffs_ );

    if ( t.intpols_ )
    {
	intpols_ = new ObjectSet<Interpolator1D>;
	intpols_->allowNull();
	for ( int idx=0; idx<t.intpols_->size(); idx++ )
	    *intpols_ += (*t.intpols_)[idx] ? (*t.intpols_)[idx]->clone() : 0;
    }

    if ( t.scalers_ )
    {
	scalers_ = new ObjectSet<const Scaler>;
	scalers_->allowNull();
	for ( int idx=0; idx<t.scalers_->size(); idx++ )
	    *scalers_ += (*t.scalers_)[idx]
			? (*t.scalers_)[idx]->duplicate() : 0;
    }

    return *this;
}


void SeisTrc::setSampleOffset( int icomp, int so )
{
    if ( (!so && !soffs_) || icomp >= data_.nrComponents() ) return;

    if ( !soffs_ ) soffs_ = new TypeSet<int>;

    while ( soffs_->size() <= icomp ) (*soffs_) += 0;
    (*soffs_)[icomp] = so;
}


const Interpolator1D* SeisTrc::interpolator( int icomp ) const
{
    return !intpols_ || icomp>=intpols_->size() ? 0 : (*intpols_)[icomp];
}


Interpolator1D* SeisTrc::interpolator( int icomp )
{
    const Interpolator1D* i = ((const SeisTrc*) this)->interpolator( icomp );
    return const_cast<Interpolator1D*>(i);
}


const Scaler* SeisTrc::scaler( int icomp ) const
{
    return !scalers_ || icomp>=scalers_->size() ? 0 : (*scalers_)[icomp];
}


void SeisTrc::setInterpolator( Interpolator1D* intpol, int icomp )
{
    if ( (!intpol && !intpols_) || icomp >= data().nrComponents() )
	{ delete intpol; return; }

    if ( !intpols_ ) intpols_ = new ObjectSet<Interpolator1D>;
    while ( intpols_->size() <= icomp )
	(*intpols_) += 0;

    delete intpols_->replace( intpol, icomp );
    if ( intpol )
	intpol->setData( *data().getComponent(icomp),
			 *data().getInterpreter(icomp) );
    else
    {
	for ( int idx=0; idx<intpols_->size(); idx++ )
	    if ( (*intpols_)[idx] ) return;
	deepErase( *intpols_ ); delete intpols_; intpols_ = 0;
    }
}


void SeisTrc::setScaler( const Scaler* sc, int icomp )
{
    if ( (!sc && !scalers_) || icomp >= data().nrComponents() )
	return;

    if ( !scalers_ ) scalers_ = new ObjectSet<const Scaler>;
    while ( scalers_->size() <= icomp )
	(*scalers_) += 0;

    scalers_->replace( sc, icomp );

    if ( !sc )
    {
	for ( int idx=0; idx<scalers_->size(); idx++ )
	    if ( (*scalers_)[idx] ) return;
	delete scalers_; scalers_ = 0;
    }
}


float SeisTrc::getValue( float t, int icomp ) const
{
    static const float snapdist = 1e-4;
    static PolyInterpolator1D polyintpol( snapdist, 0 );

    const int sz = size( icomp );
    int sampidx = nearestSample( t, icomp );
    if ( sampidx < 0 || sampidx >= sz ) return 0;

    const float pos = ( t - startPos( icomp ) ) / info_.sampling.step;
    if ( sampidx-pos > -snapdist && sampidx-pos < snapdist )
	return get( sampidx, icomp );

    const Interpolator1D* intpol = interpolator( icomp );
    if ( !intpol )
    {
	polyintpol.setData( *data().getComponent(icomp),
			    *data().getInterpreter(icomp) );
	intpol = &polyintpol;
    }

    return scaled( intpol->value(pos), icomp );
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


mXFunctionIterTp* SeisDataTrc::mkIter( bool bw, bool isc ) const
{
    return new SeisDataTrcIter( const_cast<SeisDataTrc*>(this), bw, isc );
}
