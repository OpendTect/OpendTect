/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: wavelet.cc,v 1.15 2004-07-21 12:07:10 nanne Exp $";

#include "wavelet.h"
#include "seisinfo.h"
#include "wvltfact.h"
#include "ascstream.h"
#include "streamconn.h"
#include "survinfo.h"
#include "ioobj.h"
#include "errh.h"
#include "ptrman.h"

#include <math.h>


Wavelet::Wavelet( const char* nm, int idxfsamp, float sr )
	: UserIDObject(nm)
	, iw(idxfsamp)
	, dpos(mIsUndefined(sr)?SeisTrcInfo::defaultSampleInterval(true):sr)
	, sz(0)
	, samps(0)
{
}


Wavelet::Wavelet( bool isricker, float fpeak, float sr, float scale )
    	: dpos(sr)
    	, sz(0)
	, samps(0)
{
    if ( mIsUndefined(dpos) )
	dpos = SeisTrcInfo::defaultSampleInterval(true);
    if ( mIsUndefined(scale) )
	scale = 1;
    if ( mIsUndefined(fpeak) || fpeak <= 0 )
	fpeak = 25;
    iw = (int)( -( 1 + 1. / (fpeak*dpos) ) );

    BufferString nm( isricker ? "Ricker " : "Sinc " );
    nm += fpeak; nm += " Hz";
    setName( nm );

    int lw = 1 - 2*iw;
    reSize( lw );
    float pos = iw * dpos;
    for ( int idx=0; idx<lw; idx++ )
    {
	float x = M_PI * fpeak * pos;
	float x2 = x * x;
	if ( idx == -iw )
	    samps[idx] = scale;
	else if ( isricker )
	    samps[idx] = scale * exp(-x2) * (1-2*x2);
	else
	    samps[idx] = scale * exp(-x2) * sin(x)/x;
	pos += dpos;
    }
}


Wavelet::Wavelet( const Wavelet& wv )
	: sz(0)
	, samps(0)
{
    *this = wv;
}


Wavelet& Wavelet::operator =( const Wavelet& wv )
{
    iw = wv.iw;
    dpos = wv.dpos;
    reSize( wv.size() );
    if ( sz ) memcpy( samps, wv.samps, sz*sizeof(float) );
    return *this;
}


Wavelet::~Wavelet()
{
    delete [] samps;
}


Wavelet* Wavelet::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    WaveletTranslator* tr = (WaveletTranslator*)ioobj->getTranslator();
    if ( !tr ) return 0;
    Wavelet* newwv = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( connptr && !connptr->bad() )
    {
	newwv = new Wavelet;
	if ( !tr->read( newwv, *connptr ) )
	{
	    ErrMsg( "Format error for Wavelet" );
	    delete newwv;
	    newwv = 0;
	}
    }
    else
	ErrMsg( "Cannot create data stream for Wavelet" );

    delete connptr; delete tr;
    return newwv;
}


int Wavelet::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return NO;
    WaveletTranslator* tr = (WaveletTranslator*)ioobj->getTranslator();
    if ( !tr ) return NO;
    int retval = NO;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->bad() )
    {
	if ( tr->write( this, *connptr ) )
	    retval = YES;
	else ErrMsg( "Format error for Dictionary" );
    }
    else
	ErrMsg( "Cannot create data stream for Dictionary" );

    delete connptr; delete tr;
    return retval;
}


void Wavelet::reSize( int newsz )
{
    if ( newsz < 1 ) { delete [] samps; samps = 0; sz = 0; return; }

    float* newsamps = new float [newsz];
    if ( !newsamps ) return;

    delete [] samps;
    samps = newsamps;
    sz = newsz;
}


void Wavelet::set( int center, float samplerate )
{
    iw = -center;
    dpos = samplerate;
}


void Wavelet::transform( float constant, float factor )
{
    for ( int idx=0; idx<sz; idx++ )
	samps[idx] = constant + samps[idx] * factor;
}


const IOObjContext& WaveletTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = NO;
	ctxt->newonlevel = 1;
	ctxt->needparent = NO;
	ctxt->maychdir = NO;
	ctxt->stdseltype = IOObjContext::Seis;
    }

    return *ctxt;
}


int WaveletTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Wavelet directory",key)
      || defaultSelector("Seismic directory",key) ) return 1;
    return 0;
}


static char* sLength	= "Length";
static char* sIndex	= "Index First Sample";
static char* sSampRate	= "Sample Rate";


int dgbWaveletTranslator::read( Wavelet* wv, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )	return NO;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(Wavelet)) )
	return NO;

    const float scfac = 1000;
    int iw = 0; float sr = scfac * SI().zRange().step;
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    wv->reSize( astream.getVal() );
        else if ( astream.hasKeyword( sIndex ) )
	    iw = astream.getVal();
        else if ( astream.hasKeyword(sNameKey) )
	    wv->setName( astream.value() );
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getValue();
    }
    wv->set( -iw, sr / scfac );

    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() >> wv->samples()[idx];

    return astream.stream().good();
}


int dgbWaveletTranslator::write( const Wavelet* wv, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )	return NO;

    ascostream astream( ((StreamConn&)conn).oStream() );
    UserIDString head( mTranslGroupName(Wavelet) );
    head += " file";
    if ( !astream.putHeader( head ) ) return NO;

    if ( *(const char*)wv->name() ) astream.put( sNameKey, wv->name() );
    astream.put( sLength, wv->size() );
    astream.put( sIndex, -wv->centerSample() );
    astream.put( sSampRate, wv->sampleRate() * 1000 );
    astream.newParagraph();
    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() << wv->samples()[idx] << '\n';
    astream.newParagraph();

    return astream.stream().good();
}
