/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "netservice.h"

#include "commandlineparser.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "keystrs.h"
#include "netserver.h"
#include "netsocket.h"
#include "od_ostream.h"
#include "odjson.h"
#include "oscommand.h"
#include "sighndl.h"
#include "uistrings.h"


#define sProtocolStr "http"
#define sUrlSepStr "://"

const char* Network::Service::sKeySubID()
{ return OS::MachineCommand::sKeyJobID(); }

mDefineEnumUtils(Network::Service,ServType,"Service Type")
{
    "OpendTect interface",
    "OpendTect batch interface",
    "OpendTect batch program",
    "OpendTect test program",
    "Python application",
    "Other",
    0
};



Network::Service::Service( PortNr_Type portid, const char* hostnm  )
    : NamedObject()
    , auth_(hostnm ? hostnm : Network::Socket::sKeyLocalHost(),portid)
{
    init();
}


Network::Service::Service( const Network::Authority& auth )
    : NamedObject()
    , auth_(auth)
{
    init();
}


Network::Service::Service( const OD::JSON::Object& par )
    : NamedObject()
{
    init();
    msg_ = useJSON( par );
}


Network::Service::~Service()
{
    pid_ = 0; //TODO: remove later
    if ( !viewonly_ )
	stop();

    delete logfp_;
}


bool Network::Service::operator ==( const Service& oth ) const
{
    if ( &oth == this )
	return true;

    return pid_ == oth.pid_ && subid_ == oth.subid_ && auth_ == oth.auth_ &&
	   name_ == oth.name_;
}


bool Network::Service::operator !=( const Service& oth ) const
{
    return !(*this == oth);
}


void Network::Service::init()
{
    setPID( GetPID() );
    CommandLineParser clp;
    clp.setKeyHasValue( sKeySubID() );
    clp.getVal( sKeySubID(), subid_ );
}


bool Network::Service::isOK() const
{
   return msg_.isOK();
}


bool Network::Service::isEmpty() const
{
    return !auth_.isOK();
}


bool Network::Service::isPortValid() const
{
    uiString errmsg;
    const bool ret = !isEmpty() && auth_.portIsFree( &errmsg );
    if ( !ret && !errmsg.isEmpty() )
	msg_.set( errmsg );
     return ret;
}


bool Network::Service::isAlive() const
{
    return pid_ > 0 && isProcessAlive( pid_ ); //add ping/connection test ?
}


Network::Service::ID Network::Service::getID() const
{
    return PID();
}


bool Network::Service::isBatch() const
{
    return type_ == ODBatch;
}


BufferString Network::Service::url() const
{
    return BufferString( sProtocolStr, sUrlSepStr, auth_.toString() );
}


BufferString Network::Service::address() const
{
    return auth_.getHost();
}


PortNr_Type Network::Service::port() const
{
    return auth_.getPort();
}


BufferString Network::Service::logFnm() const
{
    BufferString ret;
    if ( logfp_ )
	ret = logfp_->fullPath();

    return ret;
}


BufferString Network::Service::lockFnm() const
{
    BufferString ret;
    if ( logfp_ )
    {
	BufferString logflpath = logfp_->fullPath();
	logflpath.remove( "_log" );
	FilePath lockfp( logflpath );
	lockfp.setExtension( "lock" );
	ret = lockfp.fullPath();
    }

    return ret;
}


void Network::Service::fillJSON( OD::JSON::Object& jsonobj ) const
{
    jsonobj.set( sKeyServiceType(), type_ );
    jsonobj.set( sKeyPID(), pid_ );
    jsonobj.set( sKeySubID(), subid_ );
    jsonobj.set( sKeyServiceName(), name() );
    const bool islocal = auth_.isLocal();
    jsonobj.set( Server::sKeyLocal(), islocal );
    if ( islocal )
	jsonobj.set( Server::sKeyHostName(), auth_.toString() );
    else
    {
	jsonobj.set( Server::sKeyHostName(), auth_.getHost() );
	jsonobj.set( Server::sKeyPort(), auth_.getPort() );
    }

    if ( logfp_ )
	jsonobj.set( sKeyLogFile(), *logfp_ );
}


BufferString Network::Service::getServiceName( const OD::JSON::Object& jsonobj )
{
    return jsonobj.isPresent( sKeyServiceName() )
		? jsonobj.getStringValue(Server::sKeyHostName())
		: BufferString::empty();
}


Network::Service::ID Network::Service::getID( const OD::JSON::Object& jsonobj )
{
    return jsonobj.isPresent( sKeyPID() ) ? jsonobj.getIntValue( sKeyPID() )
					  : 0;
}


uiRetVal Network::Service::useJSON( const OD::JSON::Object& jsonobj )
{
    uiRetVal uirv;
    if ( jsonobj.isPresent(sKeyServiceName()) )
	setName( jsonobj.getStringValue( sKeyServiceName() ) );
    else
	uirv.add(tr("missing key: %1").arg(sKeyServiceName()));

    if ( jsonobj.isPresent(sKeyPID()) )
	pid_ = jsonobj.getIntValue( sKeyPID() );
    if ( jsonobj.isPresent(sKeySubID()) )
	subid_ = jsonobj.getIntValue( sKeySubID() );

    const char* hostnmkey = Server::sKeyHostName();
    const char* portkey = Server::sKeyPort();

    BufferString hostnm;
    if ( jsonobj.isPresent(hostnmkey) )
    {
	hostnm.set( jsonobj.getStringValue(hostnmkey) );
	const bool hasportnr = jsonobj.isPresent( portkey );
	const bool haslocal = jsonobj.isPresent( Server::sKeyLocal() );
	if ( !hasportnr && !haslocal )
	    uirv.add( tr("missing key to set the authority. "
			 "Need either port number or --local flag") );
	else
	{
	    if ( haslocal )
		auth_.localFromString( hostnm );
	    else
	    {
		setHostName( hostnm );
		setPort( jsonobj.getIntValue(portkey) );
	    }
	}
    }
    else
	uirv.add(tr("missing key: %1").arg(hostnmkey));

    if ( jsonobj.isPresent(sKeyServiceType()) )
	type_ = sCast(ServType,jsonobj.getIntValue( sKeyServiceType() ));

    if ( jsonobj.isPresent(sKeyLogFile()) )
	setLogFile( jsonobj.getFilePath(sKeyLogFile()) );

    return uirv;
}


void Network::Service::setAuthority( const Authority& auth )
{
    auth_ = auth;
}


void Network::Service::setPort( PortNr_Type portid )
{
    auth_.setPort( portid );
}


void Network::Service::setHostName( const char* hostnm )
{
    auth_.setHost( hostnm );
}


void Network::Service::setLogFile( const char* fnm )
{
    if ( !logfp_ )
	logfp_ = new FilePath();

    logfp_->set( fnm );
}


void Network::Service::setLogFile( const FilePath& fp )
{
    auto* logfp = new FilePath( fp );
    if ( logfp_ )
	delete logfp_;

    logfp_ = logfp;
}


void Network::Service::setPID( ProcID pid )
{
    pid_ = pid;
    setName( getProcessNameForPID(pid_) );
}


void Network::Service::setPID( const OS::CommandLauncher& cl )
{
    pid_ = cl.processID();
    setName( getProcessNameForPID(pid_) );
}


void Network::Service::stop( bool removelog )
{
    if ( viewonly_ )
	return;

    if ( isAlive() )
    {
	SignalHandling::stopProcess( pid_ );
	pid_ = 0;
    }

    if ( removelog && logfp_ && logfp_->exists() )
	File::remove( logfp_->fullPath() );
}


void Network::Service::setEmpty()
{
    setPort( 0 );
}


void Network::Service::printInfo( const char* ky, od_ostream* ostrm ) const
{
    od_ostream& strm = ostrm ? *ostrm : od_cout();
    strm << "Service";
    if ( ky && *ky )
	strm << " [" << ky << "]";
    strm << " " << name()
	 << " - status: " << isOK()
	 << " - PID: " << PID()
	 << " - ID: " << getID()
	 << " - SubID: " << getSubID()
	 << od_endl;
}
