/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.29 2005-03-09 12:22:17 cvsbert Exp $";

#include "seistrc.h"
#include "simpnumer.h"
#include "interpol1d.h"
#include "socket.h"
#include "iopar.h"
#include "errh.h"
#include "valseriesinterpol.h"
#include <math.h>
#include <float.h>

const float SeisTrc::snapdist = 1e-3;


SeisTrc::~SeisTrc()
{
    delete intpol_;
}


SeisTrc& SeisTrc::operator =( const SeisTrc& t )
{
    if ( &t == this ) return *this;

    info_ = t.info_;
    copyDataFrom( t, -1, false );

    delete intpol_; intpol_ = 0;
    if ( t.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *t.intpol_ );

    return *this;
}


bool SeisTrc::reSize( int sz, bool copydata )
{
    for ( int idx=0; idx<nrComponents(); idx++ )
	data_.reSize( sz, idx, copydata );
    return data_.allOk();
}


const ValueSeriesInterpolator<float>& SeisTrc::interpolator() const
{
    static ValueSeriesInterpolator<float>* defintpol = 0;
    if ( !defintpol )
    {
	defintpol = new ValueSeriesInterpolator<float>();
	defintpol->snapdist_ = snapdist;
	defintpol->smooth_ = true;
	defintpol->extrapol_ = false;
	defintpol->udfval_ = 0;
    }
    ValueSeriesInterpolator<float>& ret
	= const_cast<ValueSeriesInterpolator<float>&>(
	    				intpol_ ? *intpol_ : *defintpol );
    ret.maxidx_ = size() - 1;
    return ret;
}


void SeisTrc::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    delete intpol_; intpol_ = intpol;
}


bool SeisTrc::isNull( int icomp ) const
{
    if ( icomp >= 0 )
	return data_.isZero(icomp);

    for ( icomp=0; icomp<nrComponents(); icomp++ )
    {
	if ( !data_.isZero(icomp) )
	    return false;
    }
    return true;
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size();
    int sampidx = nearestSample( t );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float pos = ( t - startPos() ) / info_.sampling.step;
    if ( sampidx-pos > -snapdist && sampidx-pos < snapdist )
	return get( sampidx, icomp );

    return interpolator().value( SeisTrcValueSeries(*this,icomp), pos );
}


SampleGate SeisTrc::sampleGate( const Interval<float>& tg, bool check ) const
{
    SampleGate sg( info_.sampleGate(tg) );
    if ( !check ) return sg;

    const int maxsz = size() - 1;
    if ( sg.start > maxsz ) sg.start = maxsz;
    if ( sg.stop > maxsz ) sg.stop = maxsz;
    return sg;
}


void SeisTrc::copyDataFrom( const SeisTrc& trc, int tarcomp, bool forcefloats )
{
    int startcomp = tarcomp < 0 ? 0 : tarcomp;
    int stopcomp = tarcomp < 0 ? trc.nrComponents()-1 : tarcomp;
    const int sz = trc.size();
    for ( int icomp=startcomp; icomp<=stopcomp; icomp++ )
    {
	DataCharacteristics dc = forcefloats
	    			? DataCharacteristics()
				: trc.data().getInterpreter(icomp)->dataChar();

	if ( nrComponents() <= icomp )
	    data().addComponent( sz, dc );
	else
	{
	    if ( data().getInterpreter(icomp)->dataChar() != dc )
		data().setComponent(dc,icomp);
	    if ( data_.size(icomp) != sz )
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

    int nrcomps = nrComponents();
    if ( !sock.write( nrcomps ) )
	    { mSockErrRet( "Could not write to socket." ); }

    for( int idx=0; idx< nrComponents(); idx++ )
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
    int totalcmps = nrComponents();
    for ( int idx=0; idx < totalcmps; idx++ )
	data().delComponent(0);

    if ( nrComponents() )
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

inline float x() const
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
