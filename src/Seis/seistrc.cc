/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.26 2004-08-25 12:27:06 bert Exp $";

#include "seistrc.h"
#include "simpnumer.h"
#include "interpol1d.h"
#include "socket.h"
#include "iopar.h"
#include "errh.h"
#include <math.h>
#include <float.h>

const float SeisTrc::snapdist = 1e-3;


SeisTrc::~SeisTrc()
{
    cleanUp();
}


void SeisTrc::cleanUp()
{
    delete soffs_; soffs_ = 0;
    if ( intpols_ )
	{ deepErase( *intpols_ ); delete intpols_; intpols_ = 0; }
}


SeisTrc& SeisTrc::operator =( const SeisTrc& t )
{
    if ( &t == this ) return *this;

    cleanUp();
    info_ = t.info_;
    copyDataFrom( t, -1, false );

    if ( t.soffs_ )
	soffs_ = new TypeSet<int>( *t.soffs_ );

    if ( t.intpols_ )
    {
	intpols_ = new ObjectSet<Interpolator1D>;
	intpols_->allowNull();
	for ( int idx=0; idx<t.intpols_->size(); idx++ )
	{
	    Interpolator1D* interpol =  (*t.intpols_)[idx] ?
					(*t.intpols_)[idx]->clone() : 0;

	    if ( interpol )
		interpol->setData( *data().getComponent(idx),
				   *data().getInterpreter(idx) );

	    (*intpols_) += interpol;
	}
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


void SeisTrc::setInterpolator( Interpolator1D* intpol, int icomp )
{
    if ( (!intpol && !intpols_) || icomp >= data().nrComponents() )
	{ delete intpol; return; }

    if ( !intpols_ )
    {
	intpols_ = new ObjectSet<Interpolator1D>;
	intpols_->allowNull();
    }

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


bool SeisTrc::isNull( int icomp ) const
{
    if ( icomp >= 0 )
	return data_.isZero(icomp);

    for ( icomp=0; icomp<data_.nrComponents(); icomp++ )
    {
	if ( !data_.isZero(icomp) )
	    return false;
    }
    return true;
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size( icomp );
    int sampidx = nearestSample( t, icomp );
    if ( sampidx < 0 || sampidx >= sz ) return 0;

    const float pos = ( t - startPos( icomp ) ) / info_.sampling.step;
    if ( sampidx-pos > -snapdist && sampidx-pos < snapdist )
	return get( sampidx, icomp );

    const Interpolator1D* intpol = interpolator( icomp );
    if ( !intpol )
    {
	PolyInterpolator1D polyintpol( snapdist, 0 );

	polyintpol.setData( *data().getComponent(icomp),
			    *data().getInterpreter(icomp) );
	return polyintpol.value(pos);
    }

    return intpol->value(pos);
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


void SeisTrc::copyDataFrom( const SeisTrc& trc, int tarcomp, bool forcefloats )
{
    int startcomp = tarcomp < 0 ? 0 : tarcomp;
    int stopcomp = tarcomp < 0 ? trc.data().nrComponents()-1 : tarcomp;
    for ( int icomp=startcomp; icomp<=stopcomp; icomp++ )
    {
	DataCharacteristics dc = forcefloats
	    			? DataCharacteristics()
				: data().getInterpreter(icomp)->dataChar();
	const int sz = trc.size(icomp);

	if ( data().nrComponents() <= icomp )
	    data().addComponent( sz, dc );
	else
	{
	    if ( data().getInterpreter(icomp)->dataChar() != dc )
		data().setComponent(dc,icomp);
	    if ( size(icomp) != sz )
		data().getComponent(icomp)->reSize( sz );
	}
	memcpy( data().getComponent(icomp)->data(),
		trc.data().getComponent(icomp)->data(),
		sz * (int)dc.nrBytes() );
    }
}


#define mErrRet(msg) \
    { \
	if ( errbuf )	{ *errbuf=msg; } \
	else		{ sock.writeErrorMsg(msg); } \
	return false; \
    } 

#define mSockErrRet( msg ) \
    { \
	if ( errbuf ) \
	{ \
	    sock.fetchMsg(*errbuf); \
	    if ( *errbuf == "" ) \
	    { \
		*errbuf = msg; \
		pErrMsg("Warning: error msg empty; fallback to default msg.");\
	    } \
	} \
	return false; \
    } 

bool SeisTrc::putTo(Socket& sock, bool withinfo, BufferString* errbuf) const
{
    if ( !sock.write( withinfo ) ) mSockErrRet( "Could not write to socket." );

    if ( withinfo )
    {
	IOPar infpar;
	info().fillPar(infpar);
	if ( !sock.write( infpar ) )
	    { mSockErrRet( "Could not write to socket." ); }
    }

    int nrcomps = data_.nrComponents();
    if ( !sock.write( nrcomps ) )
	    { mSockErrRet( "Could not write to socket." ); }

    for( int idx=0; idx< data_.nrComponents(); idx++ )
    {
	DataCharacteristics dc = data_.getInterpreter(idx)->dataChar();

	BufferString dcrep;
	dc.toString( dcrep.buf() );

	if ( !sock.write(dcrep) )
	    { mSockErrRet( "Could not write to socket." ); }

	const DataBuffer* buf = data_.getComponent( idx );
	int nrbytes = buf->size() * buf->bytesPerSample();
	if ( !sock.write( nrbytes ) )
	    { mSockErrRet( "Could not write to socket." ); }

	const unsigned char* rawdata = buf->data();

	if ( !sock.writedata( rawdata, nrbytes ) )
	    { mSockErrRet( "Could not write data to socket." ); }
    }

    return true;
}


bool SeisTrc::getFrom( Socket& sock, BufferString* errbuf )
{
    int totalcmps = data().nrComponents();
    for ( int idx=0; idx < totalcmps; idx++ )
	data().delComponent(0);

    if ( data().nrComponents() )
	{ mErrRet( "Could not clear trace buffers" ); }

    bool withinfo;
    if ( !sock.read( withinfo ) )
	    { mSockErrRet( "Could not read from socket." ); }
    if ( withinfo )
    {
	IOPar infpar;
	if ( !sock.read( infpar ) )
	    { mSockErrRet( "Could not read from socket." ); }
	info().usePar(infpar);
    }

    int nrcomps;
    if ( !sock.read( nrcomps ) )
	{ mSockErrRet( "Could not read from socket." ); }

    for ( int idx=0; idx<nrcomps; idx++ )
    {
	BufferString dcrep;
	if ( !sock.read(dcrep) )
	    { mSockErrRet( "Could not read from socket." ); }

	DataCharacteristics dc( dcrep );

	int nrbytes;
	if ( !sock.read( nrbytes ) )
	    { mSockErrRet( "Could not read from socket." ); }

	int nrsamples = nrbytes / (int)dc.nrBytes();
	data().addComponent(nrsamples,dc);

	unsigned char* dest = data().getComponent(idx)->data();
	if ( !dest ) { mErrRet("Could not create buffer for trace data."); }

	int bytesread = sock.readdata( (void*)dest, nrbytes );
	if ( bytesread != nrbytes )
	    { mSockErrRet( "Could not read data from socket." ); }
    }

    return true;
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
