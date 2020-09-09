/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		November 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "netservice.h"

#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "netserver.h"
#include "odjson.h"
#include "oscommand.h"
#include "sighndl.h"


#define sProtocolStr "http"
#define sUrlSepStr "://"


Network::Service::Service( PortNr_Type portid, const char* hostnm  )
    : NamedObject()
    , auth_(hostnm ? hostnm : GetLocalHostName(),portid)
{
    setPID( GetPID() );
}


Network::Service::Service( const OD::JSON::Object& par, const Authority& auth )
    : NamedObject()
    , auth_(auth)
{
    setPID( GetPID() );
    msg_ = useJSON( par );
}


Network::Service::~Service()
{
    pid_ = 0; //TODO: remove later
    stop();
    delete logfp_;
}


bool Network::Service::operator ==( const Service& oth ) const
{
    if ( &oth == this )
	return true;

    return NamedObject::operator==( oth ) &&
		auth_ == oth.auth_ &&
		pid_ == oth.pid_;
}


bool Network::Service::operator !=( const Service& oth ) const
{
    return !(*this == oth);
}


bool Network::Service::isOK() const
{
   return msg_.isOK();
}


bool Network::Service::isEmpty() const
{
    return !auth_.isUsable();
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


bool Network::Service::fillJSON( OD::JSON::Object& jsonobj ) const
{
    if ( !fillJSON( getAuthority(), jsonobj ) )
	return false;

    if ( logfp_ )
	jsonobj.set( sKeyLogFile(), logfp_->fullPath() );

    return true;
}



bool Network::Service::fillJSON( const Authority& auth,
				 OD::JSON::Object& jsonobj )
{
    const ProcID pid = GetPID();
    const BufferString servnm( getProcessNameForPID(pid) );
    jsonobj.set( sKeyServiceName(), servnm );
    jsonobj.set( Server::sKeyHostName(), auth.getHost() );
    jsonobj.set( Server::sKeyPort(), auth.getPort() );
    jsonobj.set( sKeyPID(), pid );

    return true;
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
    if ( jsonobj.isPresent( sKeyServiceName() ) )
	setName( jsonobj.getStringValue( sKeyServiceName() ) );
    else
	uirv.add(tr("missing key: %1").arg(sKeyServiceName()));

    if ( jsonobj.isPresent( Server::sKeyHostName() ) )
	setHostName( jsonobj.getStringValue(Server::sKeyHostName()) );
    else
	uirv.add(tr("missing key: %1").arg(Server::sKeyHostName()));

    if ( jsonobj.isPresent( Server::sKeyPort() ) )
	setPort( jsonobj.getIntValue( Server::sKeyPort() ) );
    else
	uirv.add(tr("missing key: %1").arg(Server::sKeyPort()));

    if ( jsonobj.isPresent( sKeyPID() ) )
	pid_ = jsonobj.getIntValue( sKeyPID() );

    if ( jsonobj.isPresent( sKeyLogFile() ) )
	setLogFile( jsonobj.getStringValue( sKeyLogFile() ) );

    return uirv;
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
