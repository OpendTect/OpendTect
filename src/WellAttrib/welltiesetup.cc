/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiesetup.cc,v 1.1 2009-04-21 13:56:00 cvsbruno Exp $";


#include "welltiesetup.h"

#include "keystrs.h"
#include "settings.h"
#include "ascstream.h"
#include <iostream>

static const char* sKeyAttrID = "ID of selected attribute";
static const char* sKeyVelLogName = "Velocity log name";
static const char* sKeyDensLogName = "Density log name";
static const char* sKeyWavltID = "ID of selected wavelet";
static const char* sKeyIsSonic = "Provided TWT log is sonic";


void WellTieSetup::usePar( const IOPar& iop )
{
    iop.get( IOPar::compKey("",sKeyAttrID), attrid_.asInt() );
    iop.get( IOPar::compKey("",sKeyVelLogName), denlognm_ );
    iop.get( IOPar::compKey("",sKeyDensLogName), vellognm_ );
    iop.get( IOPar::compKey("",sKeyWavltID), wvltid_ );
    iop.getYN( IOPar::compKey("",sKeyIsSonic), issonic_ );
}


void WellTieSetup::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey("",sKeyAttrID), attrid_.asInt() );
    iop.set( IOPar::compKey("",sKeyVelLogName), denlognm_ );
    iop.set( IOPar::compKey("",sKeyDensLogName), vellognm_ );
    iop.set( IOPar::compKey("",sKeyWavltID), wvltid_ );
    iop.setYN( IOPar::compKey("",sKeyIsSonic), issonic_ );
}


WellTieSetup& WellTieSetup::defaults()
{
    static WellTieSetup* ret = 0;

    if ( !ret )
    {
	Settings& setts = Settings::fetch( "welltie" );
	ret = new WellTieSetup;
	ret->usePar( setts );
    }

    return *ret;
}


void WellTieSetup::commitDefaults()
{
    Settings& setts = Settings::fetch( "welltie" );
    defaults().fillPar( setts );
    setts.write();
}



WellTieWriter::WellTieWriter( const char* f, const WellTieSetup& wts )
            : WellTieIO(f,false)
	    , wts_(wts)
{
}


bool WellTieWriter::wrHdr( std::ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(fileky) )
    {
	BufferString msg( "Cannot write to " );
	msg += fileky;
	msg += " file";
	ErrMsg( msg );
	return false;
    }
    return true;
}


bool WellTieWriter::putWellTieSetup() const
{
    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return false;

    const bool isok = putWellTieSetup( *sd.ostrm );
    sd.close();
    return isok;
}


bool WellTieWriter::putWellTieSetup( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWellTieSetup()) ) return false;

    ascostream astrm( strm );
    IOPar iop; wts_.fillPar( iop );
    iop.putTo( astrm );
    return strm.good();
}


static const char* rdHdr( std::istream& strm, const char* fileky )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( "Opened file has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return 0;
    }

    static BufferString hdrln; hdrln = astrm.headerStartLine();
    return hdrln.buf();
}


const char* WellTieIO::sKeyWellTieSetup()   { return "Well Tie Setup"; }
const char* WellTieIO::sExtWellTieSetup()   { return ".tie"; }

WellTieReader::WellTieReader( const char* f, WellTieSetup& wts )
            : WellTieIO(f,true)
	    , wts_(wts)
{
}


bool WellTieReader::getWellTieSetup() const
{
    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return false;

    const bool isok = getWellTieSetup( *sd.istrm );
    sd.close();
    return isok;
}


bool WellTieReader::getWellTieSetup( std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyWellTieSetup()) )
    return false;

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    wts_.usePar( iop );
    return true;
}
             
