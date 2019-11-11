/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		November 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "netservice.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "filepath.h"
#include "genc.h"
#include "ptrman.h"
#include "oscommand.h"
#include "odjson.h"


#define sProtocolStr "http"
#define sUrlSepStr "://"

Network::Service::Service()
: hostnm_(GetLocalHostName())
, pid_(GetPID())
, portid_(0)
{
    setName( getProcessNameForPID(pid_) );
}


Network::Service::Service( port_nr_type portid, const char* hostnm  )
: hostnm_(hostnm ? hostnm : GetLocalHostName())
, pid_(GetPID())
, portid_(portid)
{
    setName( getProcessNameForPID(pid_) );
}


Network::Service::Service( const Network::Service& oth )
{
    hostnm_	= oth.hostnm_;
    portid_	= oth.portid_;
    pid_	= oth.pid_;
    logfp_	= oth.logfp_;
}

Network::Service::~Service()
{
}


bool Network::Service::operator ==( const Network::Service& oth ) const
{
    return	portid_ == oth.portid_ &&
		hostnm_ == oth.hostnm_ &&
		name() == oth.name();
}


bool Network::Service::operator !=( const Network::Service& oth ) const
{
    return !(*this == oth);
}


BufferString Network::Service::url() const
{
    BufferString urlstr;
    urlstr.set( sProtocolStr ).add( sUrlSepStr )
    .add( hostname() ).add( ":" ).add( port() );
    return urlstr;
}


BufferString Network::Service::logFnm() const
{
    BufferString ret;
    if ( logfp_ )
	ret = logfp_->fullPath();
    return ret;
}


bool Network::Service::fillJSON( OD::JSON::Object& jsonobj) const
{
    jsonobj.set( sKeyServiceName(), name() );
    jsonobj.set( Network::Server::sKeyHostName(), hostnm_ );
    jsonobj.set( Network::Server::sKeyPort(), portid_ );
    jsonobj.set( sKeyPID(), pid_ );
    if ( logfp_ )
	jsonobj.set( sKeyLogFile(), logfp_->fullPath() );

    return true;
}


uiRetVal Network::Service::useJSON( const OD::JSON::Object& jsonobj )
{
    uiRetVal uirv;
    if ( jsonobj.isPresent( sKeyServiceName() ) )
	setName( jsonobj.getStringValue( sKeyServiceName() ) );
    else
	uirv.add(tr("missing key: %1").arg(sKeyServiceName()));

    if ( jsonobj.isPresent( Server::sKeyHostName() ) )
	hostnm_ = jsonobj.getStringValue( Network::Server::sKeyHostName() );
    else
	uirv.add(tr("missing key: %1").arg(Network::Server::sKeyHostName()));

    if ( jsonobj.isPresent( Server::sKeyPort() ) )
	portid_ = jsonobj.getIntValue( Network::Server::sKeyPort() );
    else
	uirv.add(tr("missing key: %1").arg(Network::Server::sKeyPort()));

    if ( jsonobj.isPresent( sKeyPID() ) )
	pid_ = jsonobj.getIntValue( sKeyPID() );

    if ( jsonobj.isPresent( sKeyLogFile() ) )
	setLogFile( jsonobj.getStringValue( sKeyLogFile() ) );

    return uirv;
}


void Network::Service::setLogFile( const char* fnm )
{
    if ( !logfp_ )
	logfp_ = new FilePath();

    logfp_->set( fnm );
}


void Network::Service::setPID( const OS::CommandLauncher& cl )
{
    pid_ = cl.processID();
}


void Network::Service::setEmpty()
{
    setPort( 0 );
}
