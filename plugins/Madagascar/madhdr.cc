/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2012
________________________________________________________________________

-*/


#include "madhdr.h"
#include "od_iostream.h"


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyIn = "in";
static const char* sKeyStdIn = "\"stdin\"";
static const char* sKeyDataFormat = "data_format";
static const char* sKeyNativeInt = "\"native_int\"";
//static const char* sKeyAsciiInt = "\"ascii_int\"";
static const char* sKeyNativeFloat = "\"native_float\"";
//static const char* sKeyAsciiFloat = "\"ascii_float\"";
static const char* sKeyTrcHeader = "head";
static const char* sKeyODVer = "OD Version";


const char* TrcHdrDef::sKeySize = "Size";
const char* TrcHdrDef::sKeyTrcNr = "Trace Number";
const char* TrcHdrDef::sKeyOffset = "Offset";
const char* TrcHdrDef::sKeyScalco = "Scale Factor";
const char* TrcHdrDef::sKeyDelRt = "Delay Recording Time";
const char* TrcHdrDef::sKeyNs = "No. of Samples";
const char* TrcHdrDef::sKeyDt = "Sample Interval";
const char* TrcHdrDef::sKeyXcdp = "X coordinate of CDP";
const char* TrcHdrDef::sKeyYcdp = "Y coordinate of CDP";
const char* TrcHdrDef::sKeyInline = "Inline Number";
const char* TrcHdrDef::sKeyCrossline = "Crossline Number";
const char* TrcHdrDef::sKeySP = "Shotpoint Number";
const char* TrcHdrDef::sKeySPScale = "Scale Factor of Shotpoint";


int TrcHdrDef::StdSize()		{ return 91; }
int TrcHdrDef::StdIdxTrcNr()		{ return 5; }
int TrcHdrDef::StdIdxOffset()		{ return 11; }
int TrcHdrDef::StdIdxScalco()		{ return 20; }
int TrcHdrDef::StdIdxDelRt()		{ return 35; }
int TrcHdrDef::StdIdxNs()		{ return 38; }
int TrcHdrDef::StdIdxDt()		{ return 39; }
int TrcHdrDef::StdIdxXcdp()		{ return 71; }
int TrcHdrDef::StdIdxYcdp()		{ return 72; }
int TrcHdrDef::StdIdxInline()		{ return 73; }
int TrcHdrDef::StdIdxCrossline()	{ return 74; }
int TrcHdrDef::StdIdxSP()		{ return 75; }
int TrcHdrDef::StdIdxSPScale()		{ return 76; }


DefineEnumNames( RSFHeader, Format, 0, "data_format" )
{ "native_float", "native_int", "ascii_float", "ascii_int", "other" };


bool RSFHeader::read( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )	return false;
    return read( *(sd.istrm) );
}


bool RSFHeader::read( od_istream& istrm )
{
    char linebuf[256];
    if ( !istrm ) return false;

    while ( istrm )
    {
	int idx = 0, nullcount = 0;
	while ( istrm && nullcount < 3 )
	{
	    if ( idx >= 255 ) return false;

	    char c;
	    istrm.get( c );
	    if ( c == '\n' ) { linebuf[idx] = '\0'; break; }
	    else if ( !isspace(c) )
		linebuf[idx++] = c;

	    if ( c == sKeyRSFEndOfHeader[nullcount] )
		nullcount++;
	}

	if ( nullcount > 2 ) break;

	char* valstr = firstOcc( linebuf, '=' );
	if ( !valstr ) continue;

	*valstr = '\0'; valstr++;
	set( linebuf, valstr );
    }

    if ( !size() ) return false;

    return true;
}


bool RSFHeader::write( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() ) return false;
    return write( *(sd.ostrm) );
}


bool RSFHeader::write( od_ostream& ostrm ) const
{
    if ( !size() ) return false;

    for ( int idx = 0; idx < size(); idx++ )
    {
	ostrm << "\t" << getKey(idx);
	ostrm << "=" << getValue(idx) << od_endl;
    }

    ostrm << sKeyRSFEndOfHeader;
    return true;
}


int RSFHeader::nrDims() const
{
    bool found = true;
    int dim = 0;
    while ( found )
    {
	BufferString dimnm( "n" );
	dimnm += ++dim;
	found = find( dimnm );
    }
    return dim - 1;
}


int RSFHeader::nrVals( int dim ) const
{
    BufferString dimnm( "n" );
    dimnm += dim;
    int nrvals = 0;
    return get(dimnm,nrvals) ? nrvals : 0;
}


SamplingData<int> RSFHeader::getSampling( int dim ) const
{
    SamplingData<int> sampdata( 0, 1 );
    BufferString origin( "o" ), step( "d" );
    origin += dim; step += dim;
    get( origin, sampdata.start );
    get( step, sampdata.step );
    return sampdata;
}


SamplingData<float> RSFHeader::getFSampling( int dim ) const
{
    SamplingData<float> sampdata( 0.0f, 1.0f );
    BufferString origin( "o" ), step( "d" );
    origin += dim; step += dim;
    get( origin, sampdata.start );
    get( step, sampdata.step );
    return sampdata;
}


void RSFHeader::setNrVals( int dim, int nr )
{
    BufferString dimnm( "n" );
    dimnm += dim;
    set( dimnm, nr );
}


void RSFHeader::setSampling( int dim, const SamplingData<int>& sampdata )
{
    BufferString origin( "o" ), step( "d" );
    origin += dim; step += dim;
    set( origin, sampdata.start );
    set( step, sampdata.step );
}


void RSFHeader::setFSampling( int dim, const SamplingData<float>& sampdata )
{
    BufferString origin( "o" ), step( "d" );
    origin += dim; step += dim;
    set( origin, sampdata.start );
    set( step, sampdata.step );
}


const char* RSFHeader::getDataSource() const
{
    FixedString fs = find( sKeyIn );
    return fs.str();
}


void RSFHeader::setDataSource( const char* datasrc )
{
    set( sKeyIn, datasrc );
}


RSFHeader::Format RSFHeader::getDataFormat() const
{
    RSFHeader::Format datafmt;
    if ( !parseEnumFormat( find(sKeyDataFormat), datafmt ) )
	datafmt = Other;
    return datafmt;
}


void RSFHeader::setDataFormat( Format fmt )
{
    set( sKeyDataFormat, RSFHeader::getFormatString(fmt) );
}


const char* RSFHeader::getTrcHeaderFile() const
{
    FixedString fs = find( sKeyTrcHeader );
    return fs.str();
}


void RSFHeader::setTrcHeaderFile( const char* trchdrfle )
{
    set( sKeyTrcHeader, trchdrfle );
}


const char* RSFHeader::getODVersion() const
{
    FixedString fs = find( sKeyODVer );
    return fs.str();
}


void RSFHeader::setODVersion( const char* odver )
{
    set( sKeyODVer, odver );
}


TrcHdrDef::TrcHdrDef()
{
    set( sKeySize, StdSize() );
    set( sKeyTrcNr, StdIdxTrcNr() );
    set( sKeyOffset, StdIdxOffset() );
    set( sKeyScalco, StdIdxScalco());
    set( sKeyDelRt, StdIdxDelRt() );
    set( sKeyNs, StdIdxNs() );
    set( sKeyDt, StdIdxDt() );
    set( sKeyXcdp, StdIdxXcdp() );
    set( sKeyYcdp, StdIdxYcdp() );
    set( sKeyInline, StdIdxInline() );
    set( sKeyCrossline, StdIdxCrossline() );
    set( sKeySP, StdIdxSP() );
    set( sKeySPScale, StdIdxSPScale() );
}


TrcHeader::TrcHeader( bool is2d, const TrcHdrDef& def )
    : TypeSet<int>(trchdrdef_.StdSize(),0)
    , is2d_(is2d)
    , trchdrdef_(def)
{
}

#define mGetIntFld( fld, val ) \
    if ( (*this)[trchdrdef_.StdIdx##fld()] ) val = \
		(*this)[trchdrdef_.StdIdx##fld()]
bool TrcHeader::fillTrcInfo( SeisTrcInfo& ti ) const
{
    mGetIntFld( Inline, ti.binid.inl );
    mGetIntFld( Crossline, ti.binid.crl );
    mGetIntFld( TrcNr, ti.nr );

    getFld( trchdrdef_.StdIdxXcdp(), ti.coord.x );
    getFld( trchdrdef_.StdIdxYcdp(), ti.coord.y );
    getFld( trchdrdef_.StdIdxOffset(), ti.offset );
    getFld( trchdrdef_.StdIdxSP(), ti.refnr );
    getFld( trchdrdef_.StdIdxDelRt(), ti.sampling.start );
    //mPIEPAdj(Z,ti.sampling.step,false);
    getFld( trchdrdef_.StdIdxDt(), ti.sampling.step );

    double xyscale = 0.0;
    getFld( trchdrdef_.StdIdxScalco(), xyscale );
    float spscale = 0.0f;
    getFld( trchdrdef_.StdIdxSPScale(), spscale );

    if ( xyscale )
    {
	ti.coord.x = ti.coord.x * (xyscale > 0 ? xyscale : -1.0/xyscale);
	ti.coord.y = ti.coord.y * (xyscale > 0 ? xyscale : -1.0/xyscale);
    }
    if ( spscale )
	ti.refnr = ti.refnr * (spscale > 0 ? spscale : -1.0f/spscale);

    return true;
}


#define mPutFld( val, fld ) \
        if ( val ) (*this)[trchdrdef_.StdIdx##fld()] = (int)val
bool TrcHeader::useTrcInfo( const SeisTrcInfo& ti )
{
    mPutFld( ti.coord.x, Xcdp );
    mPutFld( ti.coord.y, Ycdp );
    mPutFld( ti.binid.inl, Inline );
    mPutFld( ti.binid.crl, Crossline );
    mPutFld( ti.nr, TrcNr );
    mPutFld( ti.offset, Offset );
    mPutFld( ti.refnr, SP );
    mPutFld( ti.sampling.start, DelRt );
    mPutFld( ti.sampling.step, Dt);

    int xyscale = 0, spscale = 0;
    mPutFld( xyscale, Scalco );
    mPutFld( spscale, SPScale );

    if ( xyscale )
    {
	(*this)[trchdrdef_.StdIdxXcdp()] = (*this)[trchdrdef_.StdIdxXcdp()]
	    * (xyscale > 0 ? (int)1/xyscale : -(int)xyscale);

	(*this)[trchdrdef_.StdIdxYcdp()] = (*this)[trchdrdef_.StdIdxYcdp()]
	    * (xyscale > 0 ? (int)1/xyscale : -(int)xyscale);
    }
    if ( spscale )
	(*this)[trchdrdef_.StdIdxSP()] = (*this)[trchdrdef_.StdIdxSP()]
	    * (spscale > 0 ? (int)1/spscale : -(int)spscale);

    return true;
}


#define mbuflen 2048

bool TrcHeader::read( od_istream& istrm )
{
    for ( int idx = 0; idx < trchdrdef_.size_; idx++ )
    {
	if ( !istrm ) return false;

	static char buf[mbuflen];
	trchdrdef_.isbinary_ ? istrm.read( buf, mbuflen ) : istrm >> buf;
	(*this)[idx] = toInt( buf );
    }
    return true;
}


void TrcHeader::write( od_ostream& ostrm ) const
{
    for ( int idx = 0; idx < trchdrdef_.size_; idx++ )
    {
	//static char buf[mbuflen];
	const char* buf = toString( (*this)[idx] );
	//buf = toString( (*this)[idx] );

	trchdrdef_.isbinary_ ? ostrm.write( buf, mbuflen ) : ostrm << buf;
    }
}


TrcHdrStrm::TrcHdrStrm( bool is2d, bool read, const char* fnm, TrcHdrDef& def )
	      : is2d_(is2d)
	      , trchdrdef_(def)
	      , stream_(*(od_stream::create(fnm,read,errmsg_)))
{
}


bool TrcHdrStrm::initRead()
{
    if ( !rsfheader_->read(stream_) ) return false;
    trchdrdef_.size_ = rsfheader_->nrVals(1);

    trchdrdef_.isbinary_ =
	rsfheader_->getDataFormat() == (sKeyNativeFloat || sKeyNativeInt);

    const char* datasrc = rsfheader_->getDataSource();
    if ( datasrc != sKeyStdIn )
	stream_.setFileName( datasrc );
    return true;
}


bool TrcHdrStrm::initWrite() const
{
    return rsfheader_->write(stream_);
}


TrcHeader* TrcHdrStrm::readNextTrc()
{
    TrcHeader* trchdr = new TrcHeader( is2d_, trchdrdef_ );
    trchdr->read( stream_ );
    return trchdr;
}


bool TrcHdrStrm::writeNextTrc( const TrcHeader& trchdr ) const
{
    trchdr.write( stream_ );
    return true;
}


#undef mbuflen
#undef mPutFld
#undef mGetFld
