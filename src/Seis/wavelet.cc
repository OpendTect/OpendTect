/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: wavelet.cc,v 1.9 2002-12-27 16:15:17 bert Exp $";

#include "wavelet.h"
#include "ascstream.h"
#include "streamconn.h"
#include "ioobj.h"
#include "errh.h"

DefineConcreteClassDef(Wavelet,"Wavelet");


Wavelet::Wavelet( const char* nm, int idxfsamp, float sr )
	: UserIDObject(nm)
	, iw(idxfsamp)
	, dpos(sr)
	, sz(0)
	, samps(0)
{
}


Wavelet::Wavelet( istream& stream )
	: iw(0)
{
    dgbWaveletTranslator tr;
    StreamConn strm( stream );
    tr.read( this, strm );
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


int Wavelet::write( ostream& stream ) const
{
    dgbWaveletTranslator tr;
    StreamConn strm( stream );
    return tr.write( this, strm );
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


const IOObjContext& WaveletTranslator::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( Translator::groups()[listid] );
	ctxt->crlink = NO;
	ctxt->newonlevel = 1;
	ctxt->needparent = NO;
	ctxt->maychdir = NO;
	ctxt->stdseltype = IOObjContext::Seis;
    }

    return *ctxt;
}


int WaveletTranslator::selector( const char* key )
{
    int retval = defaultSelector( classdef.name(), key );
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
    if ( !conn.forRead() )				return NO;
    else if ( !conn.hasClass(StreamConn::classid) )	return NO;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(WaveletTranslator::classDef().name()) )
	return NO;

    int iw = 0; float sr = 0.004;
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    wv->reSize( astream.getVal() );
        else if ( astream.hasKeyword( sIndex ) )
	    iw = astream.getVal();
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getValue();
        else if ( astream.hasKeyword(sNameKey) )
	    wv->setName( astream.value() );
    }
    wv->set( -iw, sr/1000 );

    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() >> wv->samples()[idx];

    return astream.stream().good();
}


int dgbWaveletTranslator::write( const Wavelet* wv, Conn& conn )
{
    if ( !conn.forWrite() )				return NO;
    else if ( !conn.hasClass(StreamConn::classid) )	return NO;

    ascostream astream( ((StreamConn&)conn).oStream() );
    UserIDString head( WaveletTranslator::classDef().name() );
    head += " file";
    if ( !astream.putHeader( head ) ) return NO;

    if ( *(const char*)wv->name() ) astream.put( sNameKey, wv->name() );
    astream.put( sLength, wv->size() );
    astream.put( sIndex, -wv->centerSample() );
    astream.put( sSampRate, wv->sampleRate()*1000 );
    astream.newParagraph();
    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() << wv->samples()[idx] << '\n';
    astream.newParagraph();

    return astream.stream().good();
}
