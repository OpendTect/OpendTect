/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2002
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
#include "safefileio.h"
#include "separstr.h"
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
static const char* sKeyPlatform()	{ return "Platform"; }

HostData::HostData( const char* nm )
    : localhd_(0)
{ init( nm ); }


HostData::HostData( const char* nm, const OD::Platform& plf )
    : platform_(plf)
    , localhd_(0)
{ init( nm ); }


HostData::HostData( const char* nm, const HostData& localhost,
		    const OD::Platform& plf )
    : platform_(plf)
    , localhd_(&localhost)
{ init( nm ); }


HostData::HostData( const HostData& oth )
    : aliases_( oth.aliases_ )
    , platform_( oth.platform_ )
    , appl_pr_( oth.appl_pr_ )
    , data_pr_( oth.data_pr_ )
    , localhd_( oth.localhd_ )
{
    init( oth.hostname_ );
}


HostData::~HostData()
{}


void HostData::setHostName( const char* nm )
{ hostname_ = nm; }

const char* HostData::getHostName() const
{ return hostname_; }

void HostData::setIPAddress( const char* ip )
{ ipaddress_ = ip; }

const char* HostData::getIPAddress() const
{ return ipaddress_; }


void HostData::setAlias( const char* nm )
{
    aliases_.erase();
    aliases_.add( nm );
}


bool HostData::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm ) return false;
    if ( hostname_ == nm ) return true;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return true;
    return false;
}


void HostData::addAlias( const char* nm )
{
    if ( !nm || !*nm || hostname_ == nm ) return;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return;
    aliases_.add( nm );
}


BufferString HostData::getFullDispString() const
{
    BufferString ret( hostname_ );
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

File::Path::Style HostData::pathStyle() const
{ return platform_.isWindows() ? File::Path::Windows : File::Path::Unix; }

const File::Path& HostData::prefixFilePath( PathType pt ) const
{ return pt == Appl ? appl_pr_ : data_pr_; }

void HostData::setDataRoot( const File::Path& dataroot )
{ data_pr_ = dataroot; }

const File::Path& HostData::getDataRoot() const
{ return data_pr_; }


void HostData::init( const char* nm )
{
    BufferString name = nm;
    if ( name.isEmpty() ) return;

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
	hostname_ = name;
	ipaddress_ = System::hostAddress( hostname_ );
    }
    else
    {
	ipaddress_ = name;
	hostname_ = System::hostName( ipaddress_ );
    }
}


static File::Path getReplacePrefix( const File::Path& dir_,
		const File::Path& fromprefix_, const File::Path& toprefix_ )
{
    if ( !fromprefix_.nrLevels() || !toprefix_.nrLevels() )
	return File::Path(dir_);

    // convert paths to Unix style
    BufferString dir = dir_.fullPath( File::Path::Unix );
    BufferString fromprefix = fromprefix_.fullPath( File::Path::Unix );
    BufferString toprefix = toprefix_.fullPath( File::Path::Unix );

    const char* tail = dir.find( fromprefix );
    if ( !tail )
    {
	fromprefix.toLower();
	tail = dir.find( fromprefix );
    }

    if ( !tail )
	return File::Path(toprefix_);

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

    return File::Path(ret);
}


File::Path HostData::convPath( PathType pt, const File::Path& fp,
			     const HostData* from ) const
{
    if ( !from ) from = &localHost();
    return getReplacePrefix( fp, from->prefixFilePath(pt), prefixFilePath(pt) );
}


uiRetVal HostData::check() const
{
    uiRetVal uirv;
    if ( hostname_.isEmpty() )
	uirv.add( tr("Empty hostname") );
    else if ( ipaddress_.isEmpty() )
	uirv.add( tr("%1: IP address is empty").arg( hostname_ ) );
    return uirv;
}


void HostData::fillPar( IOPar& par ) const
{
    par.set( sKeyIPAddress(), ipaddress_ );
    par.set( sKeyHostName(), hostname_ );
    par.set( sKeyDispName(), nrAliases() ? alias(0) : "" );
    par.set( sKeyPlatform(), platform_.shortName() );
    BufferString dataroot = data_pr_.fullPath();
    dataroot.replace( ":", ";" );
    par.set( sKey::DataRoot(), dataroot );
}


void HostData::usePar( const IOPar& par )
{
    par.get( sKeyIPAddress(), ipaddress_ );
    par.get( sKeyHostName(), hostname_ );
    BufferString res = hostname_;
    par.get( sKeyDispName(), res );
    if ( hostname_ != res ) addAlias( res );

    res.setEmpty();
    par.get( sKeyPlatform(), res );
    if ( !res.isEmpty() ) platform_.set( res, true );

    res.setEmpty();
    par.get( sKey::DataRoot(), res );
    res.replace( ";", ":" );
    if ( !res.isEmpty() ) data_pr_ = res;
}



static const char* sKeyLoginCmd()	{ return "Remote login command"; }
static const char* sKeyPriorityLevel()	{ return "Priority level"; }
static const char* sKeyNiceLevel()	{ return "Nice level"; }
static const char* sKeyFirstPort()	{ return "First port"; }
static const char* sKeyUnixDataRoot()	{ return "Default Unix Data Root"; }
static const char* sKeyWinDataRoot()	{ return "Default Windows Data Root"; }

HostDataList::HostDataList( bool foredit )
    : logincmd_("ssh")
    , prioritylevel_(-1.f)
    , firstport_(37500)
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
#ifndef __win__
    sethostent(0);
    struct hostent* he;
    while ( (he = gethostent()) )
    {
	HostData* newhd = new HostData( he->h_name );
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

	const File::Path fp( hd->isWindows() ? win_data_pr_ : unx_data_pr_ );
	hd->setDataRoot( fp );
    }
}


void HostDataList::setNiceLevel( int nicelevel )
{
   if ( mIsUdf(nicelevel) )
       return;

   const StepInterval<int> machpriorg(
	   OS::CommandExecPars::cMachineUserPriorityRange(false) );

   prioritylevel_ = -1.f * mCast(float,nicelevel) /
		    mCast(float,machpriorg.stop);
}


int HostDataList::niceLevel() const
{ return OS::CommandExecPars::getMachinePriority( prioritylevel_, false ); }

float HostDataList::priorityLevel() const	{ return prioritylevel_; }
void HostDataList::setFirstPort( int port )		{ firstport_ = port; }
int HostDataList::firstPort() const			{ return firstport_; }
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
    if ( !par.get(sKeyPriorityLevel(),prioritylevel_) )
    {
	int nicelvl;
	if ( par.get(sKeyNiceLevel(),nicelvl) )
	    setNiceLevel( nicelvl );
    }

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
	{
	    const int nicelvl = astrm.getIValue();
	    if ( !mIsUdf(nicelvl) )
		setNiceLevel( nicelvl );
	}
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
    par.set( sKeyPriorityLevel(), prioritylevel_ );
    par.set( sKeyNiceLevel(),
	     OS::CommandExecPars::getMachinePriority(prioritylevel_,false) );
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
    mPrMemb(this,prioritylevel_)
    mPrMemb(this,firstport_)
    mPrMemb(this,win_appl_pr_)
    mPrMemb(this,unx_appl_pr_)
    mPrMemb(this,win_data_pr_)
    mPrMemb(this,unx_data_pr_)

    strm << "--\n-- -- Host data:\n--\n";
    for ( int idx=0; idx<size(); idx++ )
    {
	const HostData* hd = (*this)[idx];
	mPrMemb(hd,hostname_)
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


void HostDataList::handleLocal()
{
    const int sz = size();
    const char* localhoststd = Network::Socket::sKeyLocalHost();
    HostData* localhd = 0;
    const BufferString hnm( GetLocalHostName() );
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];

	if ( hd->isKnownAs(localhoststd)
	  || hd->isKnownAs("localhost.localdomain") )
	{
	    // Ensure this is the first entry
	    if ( idx != 0 )
		swap( idx, 0 );

	    localhd = hd;
	    break;
	}
	else if ( hd->isKnownAs(hnm) )
	{
	    hd->addAlias( localhoststd );

	    // Ensure this is the first entry
	    if ( idx != 0 )
		swap( idx, 0 );

	    localhd = hd;
	    break;
	}
    }

    if ( hnm.isEmpty() ) return;
    if ( !localhd )
    {
	localhd = new HostData( hnm );
	localhd->addAlias( localhoststd );
	insertAt( localhd, 0 );
    }

    HostData& lochd = *(*this)[0];
    if ( hnm != lochd.getHostName() )
    {
	BufferString oldnm = lochd.getHostName();
	lochd.hostname_ = hnm;
	lochd.addAlias( oldnm );
	for ( int idx=0; idx<lochd.aliases_.size(); idx++ )
	{
	    if ( lochd.aliases_.get(idx) == hnm )
		{ lochd.aliases_.removeSingle( idx ); idx--; }
	}
    }

    for ( int idx=1; idx<size(); idx++ )
    {
	HostData* hd = (*this)[idx];
	hd->setLocalHost( *localhd );

	if ( hd->isKnownAs(hnm) )
	{
	    lochd.addAlias( hd->getHostName() );
	    for ( int idy=0; idy<hd->aliases_.size(); idy++ )
		lochd.addAlias( *hd->aliases_[idy] );
	    *this -= hd;
	    idx--;
	}
    }
}


HostData* HostDataList::findHost( const char* nm ) const
{
    const HostData* ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->isKnownAs(nm) )
	    { ret = (*this)[idx]; break; }
    }
    if ( !ret )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    if ( (*this)[idx]->getFullDispString() == nm )
		{ ret = (*this)[idx]; break; }
	}
    }
    return const_cast<HostData*>( ret );
}


void HostDataList::fill( BufferStringSet& bss, bool inclocalhost ) const
{
    for ( int idx=(inclocalhost?0:1); idx<size(); idx++ )
	bss.add( (*this)[idx]->getFullDispString() );
}


const char* HostDataList::getBatchHostsFilename() const
{ return batchhostsfnm_.buf(); }


uiRetVal HostDataList::check() const
{
    uiRetVal uirv;
    for ( int idx=0; idx<size(); idx++ )
	uirv.add( (*this)[idx]->check() );
    return uirv;
}
