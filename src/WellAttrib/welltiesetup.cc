/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltiesetup.h"

#include "ascstream.h"
#include "ioman.h"
#include "keystrs.h"
#include "linekey.h"
#include "settings.h"
#include "stratsynthgenparams.h"
#include "perthreadrepos.h"

namespace WellTie
{

const char* IO::sKeyWellTieSetup()	{ return "Well Tie Setup"; }

static const char* sKeySeisID = "ID of selected seismic";
static const char* sKeySeisLine = "Line name";
static const char* sKeyDensLogName = "Density log name";
static const char* sKeyVelLogName = "Velocity log name";
static const char* sKeySVelLogName = "Shear Velocity log name";
static const char* sKeyWavltID = "ID of selected wavelet";
static const char* sKeyIsSonic = "Provided TWT log is sonic";
static const char* sKeyIsShearSonic = "Provided SVel log is sonic";
static const char* sKeySetupPar = "Well Tie Setup";
static const char* sKeySeisLineID = "ID of selected Line"; //backward compat.

} // namespace WellTie


// WellTie::Setup

const char* WellTie::Setup::sKeyCSCorrType()
{ return "CheckShot Corrections"; }

const char* WellTie::Setup::sKeyUseExistingD2T()
{ return "Use Existing Depth/Time model"; }

const uiString WellTie::Setup::sCSCorrType()
{ return tr("CheckShot Corrections"); }

const uiString WellTie::Setup::sUseExistingD2T()
{ return tr("Use Existing Depth/Time model"); }

mDefineEnumUtils(WellTie::Setup,CorrType,"Check Shot Corrections")
{ "None", "Automatic", "Use editor", nullptr };

WellTie::Setup::Setup()
    : sgp_(*new SynthGenParams)
{
}


WellTie::Setup::Setup( const Setup& oth )
    : sgp_(*new SynthGenParams)
{
    *this = oth;
}


WellTie::Setup::~Setup()
{
    delete &sgp_;
}


WellTie::Setup& WellTie::Setup::operator =( const Setup& oth )
{
    if ( &oth == this )
	return *this;

    wellid_ = oth.wellid_;
    seisid_ = oth.seisid_;
    sgp_ = oth.sgp_;
    linenm_ = oth.linenm_;
    seisnm_ = oth.seisnm_;
    denlognm_ = oth.denlognm_;
    vellognm_ = oth.vellognm_;
    svellognm_ = oth.svellognm_;
    issonic_ = oth.issonic_;
    isshearsonic_ = oth.isshearsonic_;
    useexistingd2tm_ = oth.useexistingd2tm_;
    corrtype_ = oth.corrtype_;

    return *this;
}


void WellTie::Setup::supportOldPar( const IOPar& iop )
{
    const int majver = iop.majorVersion();
    const int minver = iop.minorVersion();
    if ( majver<=0 || majver>=5 || (majver==4 && minver>=4) )
	return;

    iop.get( sKeyDensLogName, vellognm_ );
    iop.get( sKeyVelLogName, denlognm_ );
}


void WellTie::Setup::usePar( const IOPar& iop )
{
    iop.get( sKeySeisID, seisid_ );
    iop.get( sKeySeisLine, linenm_ );
    iop.get( sKeyDensLogName, denlognm_ );
    iop.get( sKeyVelLogName, vellognm_ );
    iop.getYN( sKeyIsSonic, issonic_ );
    iop.get( sKeySVelLogName, svellognm_ );
    iop.getYN( sKeyIsShearSonic, isshearsonic_ );

    MultiID wvltid;
    if ( iop.get(sKeyWavltID,wvltid) && !wvltid.isUdf() )
    {
	PtrMan<IOObj> wvltobj = IOM().get( wvltid );
	if ( wvltobj )
	    sgp_.setWavelet( wvltobj->name() );
    }

    sgp_.usePar( iop );
    iop.getYN( sKeyUseExistingD2T(), useexistingd2tm_ );
    parseEnumCorrType( sKeyCSCorrType(), corrtype_ );

    LineKey lk;
    iop.get( sKeySeisLineID, lk );
    if ( linenm_.isEmpty() && !lk.lineName().isEmpty() ) //copy old key to new
	linenm_ = lk.lineName();

    supportOldPar( iop );
}


void WellTie::Setup::fillPar( IOPar& iop ) const
{
    iop.set( sKeySeisID, seisid_ );
    iop.set( sKeySeisLine, linenm_ );
    iop.set( sKeyDensLogName, denlognm_ );
    iop.set( sKeyVelLogName, vellognm_ );
    iop.setYN( sKeyIsSonic, issonic_ );
    if ( !svellognm_.isEmpty() )
    {
	iop.set( sKeySVelLogName, svellognm_ );
	iop.setYN( sKeyIsShearSonic, isshearsonic_ );
    }

    sgp_.fillPar( iop );
    iop.set( sKeyWavltID, sgp_.getWaveletID() ); //backward compatibility
    iop.setYN( sKeyUseExistingD2T(), useexistingd2tm_ );
    iop.set( sKeyCSCorrType(), getCorrTypeString( corrtype_ ) );
    const LineKey lk( linenm_, 0 );
    iop.set( sKeySeisLineID, lk ); //backward compatibility
}


WellTie::Setup& WellTie::Setup::defaults()
{
    mDefineStaticLocalObject( PtrMan<Setup>, ret, = nullptr );
    if ( !ret )
    {
	Settings& setts = Settings::fetch( "welltie" );
	auto* newret = new Setup;
	newret->usePar( setts );

	ret.setIfNull(newret,true);
    }

    return *ret;
}


void WellTie::Setup::commitDefaults()
{
    Settings& setts = Settings::fetch( "welltie" );
    defaults().fillPar( setts );
    setts.write();
}


// WellTie::IO

WellTie::IO::IO( const char* fnm, uiString& errmsg )
    : Well::odIO(fnm,errmsg)
{
}


WellTie::IO::~IO()
{
}


// WellTie::Writer

WellTie::Writer::Writer( const char* fnm )
    : IO(fnm,errmsg_)
{
}


WellTie::Writer::~Writer()
{
}


bool WellTie::Writer::wrHdr( od_ostream& strm, const char* fileky ) const
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


bool WellTie::Writer::putWellTieSetup( const WellTie::Setup& wst ) const
{
    IOPar par; wst.fillPar( par );
    return putIOPar( par, sKeySetupPar );
}


bool WellTie::Writer::putIOPar( const IOPar& iop, const char* subsel ) const
{
    BufferString fnm( getFileName(sExtWellTieSetup()) );
    Reader rdr( fnm );
    PtrMan<IOPar> filepar = rdr.getIOPar( 0 );
    if ( !filepar ) filepar = new IOPar;
    filepar->mergeComp( iop, subsel );

    od_ostream strm( getFileName(sExtWellTieSetup(),0) );
    return strm.isOK() && putIOPar( *filepar, subsel, strm );
}


bool WellTie::Writer::putIOPar( const IOPar& iop, const char* subs,
				od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWellTieSetup()) )
	return false;

    ascostream astrm( strm );
    iop.putTo( astrm );
    return strm.isOK();
}


namespace WellTie
{

static const char* rdHdr( od_istream& strm, const char* fileky )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( "Opened file has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return nullptr;
    }

    mDeclStaticString( hdrln ); hdrln = astrm.headerStartLine();
    return hdrln.buf();
}

} // namespace WellTie


// WellTie::Reader

WellTie::Reader::Reader( const char* fnm )
    : IO(fnm,errmsg_)
{
}


WellTie::Reader::~Reader()
{
}


void WellTie::Reader::getWellTieSetup( WellTie::Setup& wst ) const
{
    PtrMan<IOPar> iop = getIOPar( sKeySetupPar );
    if ( !iop )
	iop = getIOPar( "" );

    if ( iop )
	wst.usePar( *iop );
}

#define mGetOutStream(ext,nr,todo) \
    od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) { todo; }


IOPar* WellTie::Reader::getIOPar( const char* subsel ) const
{
    od_istream strm( getFileName(sExtWellTieSetup(),0) );
    return strm.isOK() ? getIOPar( subsel, strm ) : nullptr;
}


IOPar* WellTie::Reader::getIOPar( const char* subsel, od_istream& strm ) const
{
    if ( !rdHdr(strm,sKeyWellTieSetup()) )
	return nullptr;

    strm.setReadPosition( 0 );
    ascistream astrm( strm );
    IOPar iop; iop.getFrom( astrm );
    return subsel ? iop.subselect( subsel ) : new IOPar(iop);
}
