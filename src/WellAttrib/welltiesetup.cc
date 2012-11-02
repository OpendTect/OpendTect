/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "welltiesetup.h"

#include "keystrs.h"
#include "settings.h"
#include "ascstream.h"
#include <iostream>

namespace WellTie
{

const char* IO::sKeyWellTieSetup()   	{ return "Well Tie Setup"; }

static const char* sKeySeisID = "ID of selected seismic";
static const char* sKeySeisLine = "ID of selected Line";
static const char* sKeyVelLogName = "Velocity log name";
static const char* sKeyDensLogName = "Density log name";
static const char* sKeyWavltID = "ID of selected wavelet";
static const char* sKeyIsSonic = "Provided TWT log is sonic";
static const char* sKeySetupPar = "Well Tie Setup";

DefineEnumNames(Setup,CorrType,0,"Check Shot Corrections")
{ "None", "Automatic", "Use editor", 0 };


void Setup::usePar( const IOPar& iop )
{
    iop.get( sKeySeisID, seisid_ );
    iop.get( sKeySeisLine,linekey_ );
    iop.get( sKeyVelLogName, denlognm_ );
    iop.get( sKeyDensLogName, vellognm_ );
    iop.get( sKeyWavltID, wvltid_ );
    iop.getYN( sKeyIsSonic, issonic_ );
    iop.getYN( sKeyUseExistingD2T(), useexistingd2tm_ );
    parseEnumCorrType( sKeyCSCorrType(), corrtype_ );
}


void Setup::fillPar( IOPar& iop ) const
{
    iop.set( sKeySeisID, seisid_ );
    iop.set( sKeySeisLine, linekey_ );
    iop.set( sKeyVelLogName, denlognm_ );
    iop.set( sKeyDensLogName, vellognm_ );
    iop.set( sKeyWavltID, wvltid_ );
    iop.setYN( sKeyIsSonic, issonic_ );
    iop.setYN( sKeyUseExistingD2T(), useexistingd2tm_ );
    iop.set( sKeyCSCorrType(), getCorrTypeString( corrtype_ ) );
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


bool Writer::putWellTieSetup( const WellTie::Setup& wst ) const
{
    IOPar par; wst.fillPar( par );
    return putIOPar( par, sKeySetupPar );
}


bool Writer::putIOPar( const IOPar& iop, const char* subsel ) const
{
    BufferString fnm( getFileName(sExtWellTieSetup()) );
    Reader wtr( fnm );
    IOPar* filepar = wtr.getIOPar( 0 );
    if ( !filepar ) filepar = new IOPar();
    filepar->mergeComp( iop, subsel );

    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return false;

    const bool isok = ptIOPar( *filepar, subsel, *sd.ostrm );
    sd.close();
    delete filepar;
    return isok;
}


bool Writer::ptIOPar(const IOPar& iop,const char* subs,std::ostream& strm) const
{
    if ( !wrHdr(strm,sKeyWellTieSetup()) ) return false;

    ascostream astrm( strm );
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


void Reader::getWellTieSetup( WellTie::Setup& wst ) const
{
    IOPar* iop = getIOPar( sKeySetupPar );
    if ( !iop ) iop = getIOPar( "" );
    if ( iop )
	wst.usePar( *iop );

    delete iop;
}


IOPar* Reader::getIOPar( const char* subsel ) const
{
    StreamData sd = mkSD( sExtWellTieSetup() );
    if ( !sd.usable() ) return 0;

    IOPar* iop = gtIOPar( subsel, *sd.istrm );
    sd.close();
    return iop;
}


IOPar* Reader::gtIOPar( const char* subsel, std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyWellTieSetup()) )
	return 0;

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    return subsel ? iop.subselect( subsel ) : new IOPar(iop);
}


}; // namespace WellTie
