/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "madhdr.h"
#include "strmprov.h"

#include <iostream>


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyIn = "in";
//static const char* sKeyStdIn = "\"stdin\"";
static const char* sKeyDataFormat = "data_format";
static const char* sKeyNativeInt = "\"native_int\"";
static const char* sKeyAsciiInt = "\"ascii_int\"";
static const char* sKeyNativeFloat = "\"native_float\"";
static const char* sKeyAsciiFloat = "\"ascii_float\"";
static const char* sKeyTrcHeader = "head";
static const char* sKeyODVer = "OD Version";


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


bool RSFHeader::read( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )	return false;
    return read( *(sd.istrm) );
}


bool RSFHeader::read( std::istream& istrm )
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
	
	char* valstr = strchr( linebuf, '=' );
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


bool RSFHeader::write( std::ostream& ostrm ) const
{
    if ( !size() ) return false;

    for ( int idx = 0; idx < size(); idx++ )
    {
	ostrm << "\t" << getKey(idx);
	ostrm << "=" << getValue(idx) << std::endl;
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
    BufferString datafmt;
    get( sKeyDataFormat, datafmt );

    if ( datafmt == sKeyNativeFloat )
	return NativeFloat;
    else if ( datafmt == sKeyNativeInt )
	return NativeInt;
    else if ( datafmt == sKeyAsciiFloat )
	return AsciiFloat;
    else if ( datafmt == sKeyAsciiInt )
	return AsciiInt;
    else
	return Other;
}


void RSFHeader::setDataFormat( Format fmt )
{
    switch ( fmt )
    {
	case 0:
	    set( sKeyDataFormat, sKeyNativeFloat );
	    break;
	case 1:
	    set( sKeyDataFormat, sKeyNativeInt );
	    break;
	case 2:
	    set( sKeyDataFormat, sKeyAsciiFloat );
	    break;
	case 3:
	    set( sKeyDataFormat, sKeyAsciiInt );
	    break;
	case 4:
	    break;
    }
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


TrcHeader::TrcHeader( bool is2d, const TrcHdrDef& def )
    : TypeSet<int>(trchdrdef_.StdSize(),0)
    , is2d_(is2d)
    , trchdrdef_(def)
{
}


#define mGetFld( fld, val ) \
    if ( (*this)[trchdrdef_.StdIdx##fld()] ) val = \
		(*this)[trchdrdef_.StdIdx##fld()]
bool TrcHeader::fillTrcInfo( SeisTrcInfo& ti ) const
    // Fills the rsf data in SeisTrcInfo so as to display in OD
{
    mGetFld( Xcdp, ti.coord.x );
    /*if ( (*this)[StdIdxXcdp()] )
	ti.coord.x = (*this)[StdIdxXcdp()];*/

    mGetFld( Ycdp, ti.coord.y );
    mGetFld( Inline, ti.binid.inl );
    mGetFld( Crossline, ti.binid.crl );
    mGetFld( TrcNr, ti.nr );
    mGetFld( Offset, ti.offset );
    mGetFld( SP, ti.refnr );

    mGetFld( DelRt, ti.sampling.start );
    //mPIEPAdj(Z,ti.sampling.step,false);
    mGetFld( Dt, ti.sampling.step );

    int xyscale = 0, spscale = 0;
    mGetFld( Scalco, xyscale );
    mGetFld( SPScale, spscale );
    
    if ( xyscale )
    {
	ti.coord.x = ti.coord.x * (xyscale > 0 ? xyscale : -1./xyscale);
	ti.coord.y = ti.coord.y * (xyscale > 0 ? xyscale : -1./xyscale);
    }
    if ( spscale )
	ti.refnr = ti.refnr * (spscale > 0 ? spscale : -1./spscale);

    return true;
}


#define mPutFld( val, fld ) \
        if ( val ) (*this)[trchdrdef_.StdIdx##fld()] = val
bool TrcHeader::useTrcInfo( const SeisTrcInfo& ti )
    // Uses the data in SeisTrcInfo so that it can be written in rsf format
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
	    * (xyscale > 0 ? 1./xyscale:-xyscale);

	(*this)[trchdrdef_.StdIdxYcdp()] = (*this)[trchdrdef_.StdIdxYcdp()]
	    * (xyscale > 0 ? 1./xyscale:-xyscale);
    }
    if( spscale )
	(*this)[trchdrdef_.StdIdxSP()] = (*this)[trchdrdef_.StdIdxSP()]
	    * (spscale > 0 ? 1./spscale : -spscale);

    return true;
}


#define mbuflen 2048

bool TrcHeader::read( const char* fnm ) // Reads from rsf file
{
    for ( int idx = 0; idx < trchdrdef_.size_; idx++ ) // 91
    {
    	StreamData sd( StreamProvider(fnm).makeIStream() );
    	if ( !sd.usable() ) return false;
    	
	static char buf[mbuflen];

	if ( trchdrdef_.isbinary_ )
	{
	    sd.istrm->read( buf, mbuflen ); // for binary ( unformatted input )	
    	    (*this)[idx] = toInt( buf );
	}
	else
	{
    	    *sd.istrm >> buf;
	    (*this)[idx] = toInt( buf ); // for ascii ( formatted input )
	}

	sd.close();
    }
    return true;
}


void TrcHeader::write( char* fnm ) const // writes to rsf file
{
    for ( int idx = 0; idx < trchdrdef_.size_; idx++ ) // 91
    {
	StreamData sd( StreamProvider(fnm).makeOStream() );
    	if ( !sd.usable() ) return;
	
    	//static char buf[mbuflen];
	const char* buf = toString( (*this)[idx] );
    	//buf = toString( (*this)[idx] );

	if ( trchdrdef_.isbinary_ )
    	    sd.ostrm->write( buf, mbuflen ); // for binary (unformatted output)
	else
    	    *sd.ostrm << buf; //for ascii (formatted output)
	
    	sd.close();
    }
}
#undef mbuflen
#undef mPutFld
#undef mGetFld


TrcHeaderSet::TrcHeaderSet( bool is2d, TrcHdrDef& def,
				const RSFHeader* rsfheader )
	      : is2d_(is2d)
	      , trchdrdef_(def)
    	      , rsfheader_(rsfheader)
{
    int nrsamps;
    if( rsfheader_->get( "n1", nrsamps ) )
    	trchdrdef_.size_ = nrsamps;

    BufferString dataformat;
    if ( rsfheader_->get(sKeyDataFormat,dataformat)
	    && dataformat != (sKeyAsciiFloat || sKeyAsciiInt) )
    	trchdrdef_.isbinary_ = true;
    else
	trchdrdef_.isbinary_ = false;
}
