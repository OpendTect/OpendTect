/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hostdata.h"

#include "ascstream.h"
#include "debug.h"
#include "envvars.h"
#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "keystrs.h"
#include "msgh.h"
#include "netsocket.h"
#include "oddirs.h"
#include "od_strstream.h"
#include "oscommand.h"
#include "perthreadrepos.h"
#include "safefileio.h"
#include "separstr.h"
#include "strmoper.h"
#include "strmprov.h"
#include "systeminfo.h"

#ifdef __win__
# include <windows.h>
#else
# include <unistd.h>
# include <netdb.h>
#endif

#define mDebugOn        (DBG::isOn(DBG_FILEPATH))

static const char* sKeyDispName()	{ return "Display Name"; }
static const char* sKeyHostName()	{ return "Hostname"; }
static const char* sKeyIPAddress()	{ return "IP Address"; }

HostData::HostData( const char* nm )
    : staticip_(false)
{ init( nm ); }


HostData::HostData( const char* nm, const OD::Platform& plf )
    : platform_(plf)
    , staticip_(false)
{ init( nm ); }


HostData::HostData( const char* nm, const HostData& localhost,
		    const OD::Platform& plf )
    : platform_(plf)
    , localhd_(&localhost)
    , staticip_(false)
{ init( nm ); }


HostData::HostData( const HostData& oth )
    : aliases_(oth.aliases_)
    , platform_(oth.platform_)
    , appl_pr_(oth.appl_pr_)
    , data_pr_(oth.data_pr_)
    , localhd_(oth.localhd_)
    , staticip_(oth.staticip_)
{
    if ( oth.isStaticIP() )
	init( oth.ipaddress_ );
    else
	init( oth.hostname_ );
}


HostData::~HostData()
{}


const char* HostData::localHostName()
{
    return System::localFullHostName();
}


bool HostData::isStaticIP() const
{
    return staticip_;
}


void HostData::setHostName( const char* nm )
{
    hostname_ = nm;
    ipaddress_.setEmpty();
    staticip_ = false;
}


const char* HostData::getHostName( bool full ) const
{
    mDeclStaticString( str );
    if ( staticip_ )
	str.set( System::hostName( ipaddress_ ) );
    else
	str.set( hostname_ );

    if ( !full )
	str.replace( '.', '\0' );
    return str.buf();
}


void HostData::setIPAddress( const char* ip )
{
    ipaddress_ = ip;
    hostname_.setEmpty();
    staticip_ = true;
}


const char* HostData::getIPAddress() const
{
    if ( staticip_ )
	return ipaddress_;
    else
	return System::hostAddress( hostname_ );
}


BufferString HostData::connAddress() const
{
    return staticip_ ? getIPAddress() : getHostName();
}


void HostData::setAlias( const char* nm )
{
    aliases_.erase();
    aliases_.add( nm );
}


bool HostData::isKnownAs( const char* nm ) const
{
    BufferString machnm( nm );
    if ( machnm.isEmpty() )
	return false;

    if ( System::isValidIPAddress(machnm) )
    {
	if ( machnm == getIPAddress() )
	    return true;
    }
    else
    {
	const BufferString hostnm( getHostName(false) );
	machnm.replace( '.', '\0' ); //Remove any domain name
	if ( hostnm == machnm )
	    return true;
    }

    for ( int idx=0; idx<aliases_.size(); idx++ )
    {
	if ( *aliases_[idx] == nm )
	    return true;
    }

    return false;
}


void HostData::addAlias( const char* nm )
{
    if ( !nm || !*nm || BufferString(getHostName()) == nm )
	return;

    for ( const auto* alias : aliases_ )
    {
	if ( *alias == nm )
	    return;
    }

    aliases_.add( nm );
}


BufferString HostData::getFullDispString() const
{
    const BufferString ipaddr( getIPAddress() );
    const BufferString hostnm( getHostName(false) );
    BufferString ret;
    if ( isStaticIP() )
	ret.set( ipaddr ).add( " (" ).add( hostnm );
    else
	ret.set( hostnm ).add( " (" ).add( ipaddr );
    ret.add( ")" );

    for ( int idx=0; idx<nrAliases(); idx++ )
	ret.add( " / " ).add( alias(idx) );
    return ret;
}


void HostData::setPlatform( const OD::Platform& plf )
{ platform_ = plf; }

const OD::Platform& HostData::getPlatform() const
{ return platform_; }

bool HostData::isWindows() const
{ return platform_.isWindows(); }

FilePath::Style HostData::pathStyle() const
{ return platform_.isWindows() ? FilePath::Windows : FilePath::Unix; }

const FilePath& HostData::prefixFilePath( PathType pt ) const
{ return pt == Appl ? appl_pr_ : data_pr_; }

void HostData::setDataRoot( const FilePath& dataroot )
{ data_pr_ = dataroot; }

const FilePath& HostData::getDataRoot() const
{ return data_pr_; }


void HostData::init( const char* nm )
{
    BufferString name = nm;
    if ( name.isEmpty() )
	return;

    const char* ptr = name.buf();
    bool is_ip_adrr = true;

    while ( ptr++ && *ptr )
    {
	mSkipBlanks(ptr)
	if ( !iswdigit(*ptr) && * ptr != '.' )
	    is_ip_adrr = false;
    }

    if ( !is_ip_adrr )
    {
	char* dot = name.find( '.' );
	if ( dot ) { *dot ='\0'; addAlias(nm); }
	setHostName( name );
    }
    else
    {
	setIPAddress( name );
	name = getHostName();
	char* dot = name.find( '.' );
	if ( dot ) { *dot ='\0'; addAlias(name); }
    }
}


static FilePath getReplacePrefix( const FilePath& dir_,
		const FilePath& fromprefix_, const FilePath& toprefix_ )
{
    if ( !fromprefix_.nrLevels() || !toprefix_.nrLevels() )
	return FilePath(dir_);

    // convert paths to Unix style
    BufferString dir = dir_.fullPath( FilePath::Unix );
    BufferString fromprefix = fromprefix_.fullPath( FilePath::Unix );
    BufferString toprefix = toprefix_.fullPath( FilePath::Unix );
    BufferString tail = dir.find( fromprefix.buf() );
    if ( tail.isEmpty() )
    {
	fromprefix.toLower();
	tail = dir.find( fromprefix.buf() );
    }

    if ( tail.isEmpty() )
	return FilePath(toprefix_);

    tail += fromprefix.size();

    BufferString ret = toprefix;
    ret += tail;

    if ( mDebugOn )
    {
	BufferString msg("getReplacePrefix:\n in: '");
	msg += dir;
	msg += "' \n replace '";
	msg += fromprefix;
	msg += "' with '";
	msg += toprefix;
	msg += "'\n to: '";
	msg += ret;
	msg += "'";

	DBG::message(msg);
    }

    return FilePath(ret);
}


FilePath HostData::convPath( PathType pt, const FilePath& fp,
			     const HostData* from ) const
{
    if ( !from ) from = &localHost();
    return getReplacePrefix( fp, from->prefixFilePath(pt), prefixFilePath(pt) );
}


bool HostData::isOK( uiString& errmsg ) const
{
    if ( !staticip_ && BufferString(getIPAddress()).isEmpty() )
	errmsg.append( "Hostname lookup failed; " );
    if ( staticip_ && BufferString(getHostName()).isEmpty() )
	errmsg.append( "IP address lookup failed; " );

    return errmsg.isEmpty();
}


void HostData::fillPar( IOPar& par ) const
{
    if ( staticip_ )
    {
	par.set( sKeyIPAddress(), ipaddress_ );
	par.removeWithKey( sKeyHostName() );
    }
    else
    {
	par.set( sKeyHostName(), hostname_ );
	par.removeWithKey( sKeyIPAddress() );
    }
    par.set( sKeyDispName(), nrAliases() ? alias(0) : "" );
    par.set( OD::Platform::sPlatform(), platform_.shortName() );
    BufferString dataroot = data_pr_.fullPath();
    dataroot.replace( ":", ";" );
    par.set( sKey::DataRoot(), dataroot );
}


void HostData::usePar( const IOPar& par )
{
    if ( par.get(sKeyHostName(), hostname_) &&
					par.get(sKeyIPAddress(), ipaddress_) )
    {
	staticip_ = false;
	if ( ipaddress_==getIPAddress() )
	{
	    staticip_ = true;
	    hostname_.setEmpty();
	}
	else
	    ipaddress_.setEmpty();
    }
    else if( par.get(sKeyIPAddress(), ipaddress_) )
    {
	staticip_ = true;
	hostname_.setEmpty();
    }
    else if( par.get(sKeyHostName(), hostname_) )
    {
	staticip_ = false;
	ipaddress_.setEmpty();
    }

    const BufferString hostname( getHostName() );
    BufferString res( hostname );
    par.get( sKeyDispName(), res );
    if ( hostname != res ) addAlias( res );

    res.setEmpty();
    par.get( OD::Platform::sPlatform(), res );
    if ( !res.isEmpty() )
	platform_.set( res.buf(), true );

    res.setEmpty();
    par.get( sKey::DataRoot(), res );
    res.replace( ";", ":" );
    if ( !res.isEmpty() ) data_pr_ = res;
}



static const char* sKeyLoginCmd()	{ return "Remote login command"; }
static const char* sKeyNiceLevel()	{ return "Nice level"; }
static const char* sKeyFirstPort()	{ return "First port"; }
static const char* sKeyUnixDataRoot()	{ return "Default Unix Data Root"; }
static const char* sKeyWinDataRoot()	{ return "Default Windows Data Root"; }

// HostDataList

HostDataList::HostDataList( bool foredit )
{
    BufferString bhfnm = "BatchHosts";
    if ( GetEnvVar("DTECT_BATCH_HOSTS_FILENAME") )
	bhfnm = GetEnvVar("DTECT_BATCH_HOSTS_FILENAME");

    BufferString fname( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,
				bhfnm.buf(),true) );
    if ( GetEnvVar("DTECT_BATCH_HOSTS_FILEPATH") )
	fname = GetEnvVar("DTECT_BATCH_HOSTS_FILEPATH");

    batchhostsfnm_ = fname;
    refresh( foredit );
}


HostDataList::~HostDataList()
{
}


bool HostDataList::refresh( bool foredit )
{
    readHostFile( batchhostsfnm_ );

    if ( !foredit )
    {
	handleLocal();
	initDataRoot();
    }

    return true;
}


void HostDataList::fillFromNetwork()
{
#ifdef __unix__
    sethostent(0);
    struct hostent* he;
    while ( (he = gethostent()) )
    {
	auto* newhd = new HostData( he->h_name );
	char** al = he->h_aliases;
	while ( *al )
	{
	    if ( !newhd->isKnownAs(*al) )
		newhd->aliases_.add( *al );
	    al++;
	}
	*this += newhd;
    }

    endhostent();
#endif
}


void HostDataList::initDataRoot()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	HostData* hd = (*this)[idx];
	BufferString dr = hd->getDataRoot().fullPath();
	if ( !dr.isEmpty() ) continue;

	if ( !__iswin__ && unx_data_pr_.isEmpty() )
	    unx_data_pr_ = GetBaseDataDir();
	if ( __iswin__ && win_data_pr_.isEmpty() )
	    win_data_pr_ = GetBaseDataDir();

	const FilePath fp( hd->isWindows() ? win_data_pr_ : unx_data_pr_ );
	hd->setDataRoot( fp );
    }
}


void HostDataList::setNiceLevel( int lvl )		{ nicelvl_ = lvl; }
int HostDataList::niceLevel() const			{ return nicelvl_; }
void HostDataList::setFirstPort( PortNr_Type port )	{ firstport_ = port; }
PortNr_Type HostDataList::firstPort() const		{ return firstport_; }
void HostDataList::setLoginCmd( const char* cmd )	{ logincmd_ = cmd; }
const char* HostDataList::loginCmd() const		{ return logincmd_; }

void HostDataList::setUnixDataRoot( const char* dr )
{ unx_data_pr_.set( dr ); }

const char* HostDataList::unixDataRoot() const
{ return unx_data_pr_.buf(); }

void HostDataList::setWinDataRoot( const char* dr )
{ win_data_pr_.set( dr ); }

const char* HostDataList::winDataRoot() const
{ return win_data_pr_.buf(); }


bool HostDataList::readHostFile( const char* fnm )
{
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(true) )
	return false;

    IOPar par;
    ascistream astrm( sfio.istrm() );
    par.getFrom( astrm );

    if ( par.odVersion() < 470 )
    {
	sfio.closeSuccess();
	return readOldHostFile( fnm );
    }

    par.get( sKeyLoginCmd(), logincmd_ );
    par.get( sKeyNiceLevel(), nicelvl_ );
    par.get( sKeyFirstPort(), firstport_ );
    OS::MachineCommand::setDefaultRemExec( logincmd_ );

    BufferString dataroot;
    par.get( sKeyUnixDataRoot(), dataroot );
    unx_data_pr_ = dataroot;
    dataroot.setEmpty();
    par.get( sKeyWinDataRoot(), dataroot );
    dataroot.replace( ";", ":" );
    win_data_pr_ = dataroot;

    deepErase( *this );
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> hdpar = par.subselect(IOPar::compKey("Host",idx) );
	if ( !hdpar ) break;

	HostData* hd = new HostData(0);
	hd->usePar( *hdpar );
	(*this) += hd;
    }

    sfio.closeSuccess();
    return true;
}


bool HostDataList::readOldHostFile( const char* fname )
{
    od_istream strm( fname );
    if ( !strm.isOK() )
	return false;

    ascistream astrm( strm );
    if ( !astrm.isOfFileType("Batch Processing Hosts") )
    {
	const BufferString msg( fname, ": hosts file has invalid file header" );
	ErrMsg( msg ); return false;
    }

    if ( atEndOfSection(astrm) ) astrm.next();

    bool foundrsh = false;
    while ( !atEndOfSection(astrm) )
    {
	if ( astrm.hasKeyword("Remote shell") )
	    { logincmd_ = astrm.value(); foundrsh = !logincmd_.isEmpty(); }
	if ( astrm.hasKeyword("Default nice level") )
	    nicelvl_ = astrm.getIValue();
	if ( astrm.hasKeyword("First port") )
	    firstport_ = astrm.getIValue();
	if ( astrm.hasKeyword("Win appl prefix") )
	    win_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx appl prefix") )
	    unx_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Win data prefix") )
	    win_data_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx data prefix") )
	    unx_data_pr_.set( (char*) astrm.value() );

	astrm.next();
    }

    if ( foundrsh )
	OS::MachineCommand::setDefaultRemExec( logincmd_ );

    while ( !atEndOfSection(astrm.next()) )
    {
	HostData* newhd = new HostData( astrm.keyWord() );
	if ( *astrm.value() )
	{
	    SeparString val( astrm.value(), ':' );
	    BufferString vstr;

#define mGetVStr(valnr) \
	    vstr = val[valnr]; vstr.trimBlanks()

	    mGetVStr(0);
	    if ( !vstr.isEmpty() )
		newhd->aliases_.add( vstr );

	    mGetVStr(1);
	    newhd->platform_.set( vstr == "win", false );

	    mGetVStr(2);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->isWindows() )
		    vstr.replace( ';', ':' );
		newhd->data_pr_ = vstr;
	    }
	    else
		newhd->data_pr_ =
			newhd->isWindows() ? win_data_pr_ : unx_data_pr_;

	    mGetVStr(3);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->isWindows() )
		    vstr.replace( ';', ':' );
		newhd->appl_pr_ = vstr;
	    }
	    else
		newhd->appl_pr_ =
			newhd->isWindows() ? win_appl_pr_ : unx_appl_pr_;

	    mGetVStr(4);
	    if ( !vstr.isEmpty() )
	    {
		if ( newhd->isWindows() )
		    vstr.replace( ';', ':' );
	    }

	}
	*this += newhd;
    }

    if ( mDebugOn )
    {
	od_ostrstream strstrm; dump( strstrm );
	DBG::message( strstrm.result() );
    }
    return true;
}


bool HostDataList::writeHostFile( const char* fnm )
{
    IOPar par;
    par.set( sKeyLoginCmd(), logincmd_ );
    par.set( sKeyNiceLevel(), nicelvl_ );
    par.set( sKeyFirstPort(), firstport_ );
    par.set( sKeyUnixDataRoot(), unx_data_pr_ );
    win_data_pr_.replace( ":", ";" );
    par.set( sKeyWinDataRoot(), win_data_pr_ );

    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar hostpar;
	(*this)[idx]->fillPar( hostpar );
	par.mergeComp( hostpar, IOPar::compKey("Host",idx) );
    }

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	return false;

    ascostream astrm( sfio.ostrm() );
    astrm.putHeader( "Batch Processing Hosts" );
    par.putTo( astrm );
    if ( astrm.isOK() )
	sfio.closeSuccess();
    else
	sfio.closeFail();

    return true;
}


#define mPrMemb(obj,x) { strm << "-- " << #x << "='" << obj->x << "'\n"; }

void HostDataList::dump( od_ostream& strm ) const
{
    strm << "\n\n-- Host data list:\n--\n";
    mPrMemb(this,logincmd_)
    mPrMemb(this,nicelvl_)
    mPrMemb(this,firstport_)
    mPrMemb(this,win_appl_pr_)
    mPrMemb(this,unx_appl_pr_)
    mPrMemb(this,win_data_pr_)
    mPrMemb(this,unx_data_pr_)

    strm << "--\n-- -- Host data:\n--\n";
    for ( int idx=0; idx<size(); idx++ )
    {
	const HostData* hd = (*this)[idx];
	mPrMemb(hd,getHostName())
	mPrMemb(hd,isWindows())
	mPrMemb(hd,appl_pr_.fullPath())
	mPrMemb(hd,data_pr_.fullPath())
	if ( hd->localhd_ )
	    mPrMemb(hd,localhd_->getHostName())
	else
	    strm << "-- No localhd_\n";
	strm << "--\n";
    }
    strm.flush();
}


bool HostDataList::isMostlyStaticIP() const
{
    int nrstatic = 0, nrdns = 0;
    for ( const auto* hd : *this )
    {
	if ( hd->isStaticIP() )
	    nrstatic++;
	else
	    nrdns++;
    }

    return nrstatic > nrdns;
}


void HostDataList::handleLocal()
{
    const int sz = size();
    const char* localhoststd = Network::Socket::sKeyLocalHost();
    HostData* localhd = nullptr;
    const BufferString hnm( System::localHostName() );
    const BufferString fqhnm( System::localFullHostName() );
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = get( idx );

	if ( hd->isKnownAs(localhoststd) ||
	     hd->isKnownAs("localhost.localdomain") )
	{
	    // Ensure this is the first entry
	    if ( idx != 0 )
	    {
		hd = removeAndTake( idx );
		insertAt( hd, 0 );
	    }

	    localhd = hd;
	    break;
	}
	else if ( hd->isKnownAs(hnm) || hd->isKnownAs(fqhnm) )
	{
	    hd->addAlias( localhoststd );
	    // Ensure this is the first entry
	    if ( idx != 0 )
	    {
		hd = removeAndTake( idx );
		insertAt( hd, 0 );
	    }

	    localhd = hd;
	    break;
	}
    }

    if ( hnm.isEmpty() )
	return;

    if ( !localhd )
    {
	const bool useipaddr = isMostlyStaticIP();
	localhd = useipaddr ? new HostData( System::hostAddress(fqhnm) )
			    : new HostData( fqhnm );
	localhd->addAlias( localhoststd );
	insertAt( localhd, 0 );
    }

    HostData& lochd = *first();
    if ( fqhnm != lochd.getHostName() )
    {
	BufferString oldnm = lochd.getHostName();
	lochd.setHostName( fqhnm );
	lochd.addAlias( oldnm );
	for ( int idx=lochd.aliases_.size()-1; idx>=0; idx-- )
	{
	    if ( lochd.aliases_.get(idx) == hnm ||
		 lochd.aliases_.get(idx) == fqhnm )
		lochd.aliases_.removeSingle( idx );
	}
    }

    for ( int idx=1; idx<size(); idx++ )
    {
	HostData* hd = get( idx );
	hd->setLocalHost( *localhd );

	if ( hd->isKnownAs(hnm) || hd->isKnownAs(fqhnm) )
	{
	    lochd.addAlias( hd->getHostName() );
	    for ( int idy=0; idy<hd->aliases_.size(); idy++ )
		lochd.addAlias( *hd->aliases_[idy] );
	    *this -= hd;
	    idx--;
	}
    }
}


HostData* HostDataList::findHost( const char* nm )
{
    if ( !nm || !*nm )
	return nullptr;

    for ( const auto* hd : *this )
    {
	if ( hd->isKnownAs(nm) )
	    return const_cast<HostData*>( hd );
    }

    for ( const auto* hd : *this )
    {
	if ( hd->getFullDispString() == nm )
	    return const_cast<HostData*>( hd );
    }

    return nullptr;
}


const HostData* HostDataList::findHost( const char* nm ) const
{
    return mSelf().findHost( nm );
}


void HostDataList::fill( BufferStringSet& bss, bool inclocalhost ) const
{
    for ( int idx=(inclocalhost?0:1); idx<size(); idx++ )
	bss.add( (*this)[idx]->getFullDispString() );
}


const char* HostDataList::getBatchHostsFilename() const
{ return batchhostsfnm_.buf(); }


bool HostDataList::isOK( uiStringSet& errors ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	uiString msg;
	if ( !(*this)[idx]->isOK(msg) )
	{
	    uiString fullmsg = tr("Host %1: %2").arg( idx+1 ).arg( msg );
	    errors.add( fullmsg );
	}
    }

    if ( errors.isEmpty() ) return true;

    errors.insert( 0, tr("Errors in Host information") );
    return false;
}
