/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "hostdata.h"

#include "ascstream.h"
#include "debug.h"
#include "envvars.h"
#include "filepath.h"
#include "keystrs.h"
#include "genc.h"
#include "iopar.h"
#include "msgh.h"
#include "oddirs.h"
#include "od_strstream.h"
#include "oscommand.h"
#include "safefileio.h"
#include "separstr.h"
#include "strmoper.h"
#include "strmprov.h"

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
    , sharedata_(0)
{ init( nm ); }


HostData::HostData( const char* nm, const OD::Platform& plf )
    : platform_(plf)
    , localhd_(0)
    , sharedata_(0)
{ init( nm ); }


HostData::HostData( const char* nm, const HostData& localhost,
		    const OD::Platform& plf )
    : platform_(plf)
    , localhd_(&localhost)
    , sharedata_(0)
{ init( nm ); }


HostData::HostData( const HostData& oth )
    : aliases_( oth.aliases_ )
    , platform_( oth.platform_ )
    , appl_pr_( oth.appl_pr_ )
    , data_pr_( oth.data_pr_ )
    , pass_( oth.pass_ )
    , localhd_( oth.localhd_ )
    , sharedata_( oth.sharedata_ )
{
    init( oth.hostname_ );
}


HostData::~HostData()
{}


const char* HostData::localHostName()
{
    return GetLocalHostName();
}


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

FilePath::Style HostData::pathStyle() const
{ return platform_.isWindows() ? FilePath::Windows : FilePath::Unix; }

const FilePath& HostData::prefixFilePath( PathType pt ) const
{ return pt == Appl ? appl_pr_ : data_pr_; }

void HostData::setDataRoot( const char* dataroot )
{ data_pr_ = dataroot; }

const char* HostData::getDataRoot() const
{ return data_pr_.fullPath(); }


void HostData::init( const char* nm )
{
    BufferString name = nm;
    if ( name.isEmpty() ) return;

    const char* ptr = name.buf();
    bool is_ip_adrr = true;

    while ( ptr++ && *ptr )
    {
	mSkipBlanks(ptr)
	if ( !isdigit(*ptr) && * ptr != '.' )
	    is_ip_adrr = false;
    }

    if ( !is_ip_adrr )
    {
	char* dot = name.find( '.' );
	if ( dot ) { *dot ='\0'; addAlias(nm); }
	hostname_ = name;
    }
    else
	ipaddress_ = name;
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

    const char* tail = dir.find( fromprefix );
    if ( !tail )
    {
	fromprefix.toLower();
	tail = dir.find( fromprefix );
    }

    if ( !tail )
	return FilePath(dir_);

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
static const char* sKeyNiceLevel()	{ return "Nice level"; }
static const char* sKeyFirstPort()	{ return "First port"; }

HostDataList::HostDataList( bool readhostfile, bool addlocalhost )
	: logincmd_("rsh")
	, nicelvl_(19)
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
    if ( readhostfile )
	readHostFile( fname );

    if ( addlocalhost )
	handleLocal();
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


void HostDataList::setNiceLevel( int lvl )		{ nicelvl_ = lvl; }
int HostDataList::niceLevel() const			{ return nicelvl_; }
void HostDataList::setFirstPort( int port )		{ firstport_ = port; }
int HostDataList::firstPort() const			{ return firstport_; }
void HostDataList::setLoginCmd( const char* cmd )	{ logincmd_ = cmd; }
const char* HostDataList::loginCmd() const		{ return logincmd_; }


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

    BufferString sharehost;

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

	if ( astrm.hasKeyword("Data host") )
	    sharehost = astrm.value();  // store in a temporary
	if ( astrm.hasKeyword("Data drive") )
	    sharedata_.drive_ = astrm.value();
	if ( astrm.hasKeyword("Data share") )
	    sharedata_.share_ = astrm.value();
	if ( astrm.hasKeyword("Data pass") )
	    sharedata_.pass_ = astrm.value();

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
		newhd->pass_ = vstr;
	    }
	    else if ( newhd->isWindows() )
		newhd->pass_ = sharedata_.pass_;

	    newhd->setShareData( &sharedata_ );
	}
	*this += newhd;
    }

    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];
	if ( hd->isKnownAs(sharehost) )
	{
	    sharedata_.host_ = hd;
	    break;
	}
    }

    static bool complain = !GetEnvVarYN( "OD_NO_DATAHOST_CHK" );
    if ( sharehost.size() && !sharedata_.host_ && complain )
    {
	BufferString msg("No host "); msg += sharehost;
	msg += "  found in "; msg += fname;
	msg += ". Multi machine batch processing may not work as expected.";
	ErrMsg( msg );
	complain = false;
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
    mPrMemb(this,win_appl_pr_.fullPath())
    mPrMemb(this,unx_appl_pr_.fullPath())
    mPrMemb(this,win_data_pr_.fullPath())
    mPrMemb(this,unx_data_pr_.fullPath())
    const ShareData* sd = &sharedata_;
    strm << "-- -- Global share data:\n";
    mPrMemb(sd,drive_)
    mPrMemb(sd,share_)
    mPrMemb(sd,pass_)
    if ( sd->host_ )
	mPrMemb(sd,host_->getHostName())
    else
	strm << "-- No sharedata_->host_\n";
    strm << "--\n-- -- Host data:\n--\n";
    for ( int idx=0; idx<size(); idx++ )
    {
	const HostData* hd = (*this)[idx];
	mPrMemb(hd,hostname_)
	mPrMemb(hd,isWindows())
	mPrMemb(hd,appl_pr_.fullPath())
	mPrMemb(hd,data_pr_.fullPath())
	mPrMemb(hd,pass_)
	if ( hd->localhd_ )
	    mPrMemb(hd,localhd_->getHostName())
	else
	    strm << "-- No localhd_\n";
	sd = hd->sharedata_;
	if ( sd )
	{
	    strm << "-- -- -- Share data:\n";
	    mPrMemb(sd,drive_)
	    mPrMemb(sd,share_)
	    mPrMemb(sd,pass_)
	    if ( sd->host_ )
		mPrMemb(sd,host_->getHostName())
	    else
		strm << "-- No sharedata_->host_\n";
	}
	strm << "--\n";
    }
    strm.flush();
}


void HostDataList::handleLocal()
{
    const int sz = size();
    const char* localhoststd = "localhost";
    HostData* localhd = 0;
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
	else if ( hd->isKnownAs(HostData::localHostName()) )
	{
	    hd->addAlias( localhoststd );

	    // Ensure this is the first entry
	    if ( idx != 0 )
		swap( idx, 0 );

	    localhd = hd;
	    break;
	}
    }

    BufferString hnm( HostData::localHostName() );
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
	    BufferString* al = lochd.aliases_[idx];
	    if ( *al == hnm )
		lochd.aliases_ -= al;
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
