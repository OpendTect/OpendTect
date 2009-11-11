/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiesetup.cc,v 1.5 2009-11-11 15:14:10 cvsbruno Exp $";


#include "welltiesetup.h"

#include "keystrs.h"
#include "settings.h"
#include "ascstream.h"
#include <iostream>

namespace WellTie
{

static const char* sKeySeisID = "ID of selected seismic";
static const char* sKeySeisLine = "ID of selected Line";
static const char* sKeyVelLogName = "Velocity log name";
static const char* sKeyDensLogName = "Density log name";
static const char* sKeyWavltID = "ID of selected wavelet";
static const char* sKeyIsSonic = "Provided TWT log is sonic";


void Setup::usePar( const IOPar& iop )
{
    iop.get( IOPar::compKey("",sKeySeisID), seisid_ );
    iop.get( IOPar::compKey("",sKeySeisLine), linekey_ );
    iop.get( IOPar::compKey("",sKeyVelLogName), denlognm_ );
    iop.get( IOPar::compKey("",sKeyDensLogName), vellognm_ );
    iop.get( IOPar::compKey("",sKeyWavltID), wvltid_ );
    iop.getYN( IOPar::compKey("",sKeyIsSonic), issonic_ );
}


void Setup::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey("",sKeySeisID), seisid_ );
    iop.set( IOPar::compKey("",sKeySeisLine), linekey_ );
    iop.set( IOPar::compKey("",sKeyVelLogName), denlognm_ );
    iop.set( IOPar::compKey("",sKeyDensLogName), vellognm_ );
    iop.set( IOPar::compKey("",sKeyWavltID), wvltid_ );
    iop.setYN( IOPar::compKey("",sKeyIsSonic), issonic_ );
}


Setup& Setup::defaults()
{
    static Setup* ret = 0;

    if ( !ret )
    {
	Settings& setts = Settings::fetch( "welltie" );
	ret = new Setup;
	ret->usePar( setts );
    }

    return *ret;
}


void Setup::commitDefaults()
{
    Settings& setts = Settings::fetch( "welltie" );
    defaults().fillPar( setts );
    setts.write();
}


bool Writer::wrHdr( std::ostream& strm, const char* fileky ) const
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


bool Writer::putWellTieSetup() const
{
    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return false;

    const bool isok = putWellTieSetup( *sd.ostrm );
    sd.close();
    return isok;
}


bool Writer::putWellTieSetup( std::ostream& strm ) const
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


const char* IO::sKeyWellTieSetup()   { return "Well Tie Setup"; }
const char* IO::sExtWellTieSetup()   { return ".tie"; }


bool Reader::getWellTieSetup() const
{
    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return false;

    const bool isok = getWellTieSetup( *sd.istrm );
    sd.close();
    return isok;
}


bool Reader::getWellTieSetup( std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyWellTieSetup()) )
    return false;

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    wts_.usePar( iop );
    return true;
}

}; // namespace WellTie
