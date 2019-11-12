/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodservice.h"

#include "uimain.h"
#include "envvars.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "settings.h"
#include "commandlineparser.h"
#include "odjson.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */

uiODServiceBase::uiODServiceBase( bool assignport)
{
    typedef Network::RequestConnection NRC;
    const char* skeyport = Network::Server::sKeyPort();
    port_nr_type portid = 0;

    int defport = 0;
    Settings::common().get( skeyport, defport );

    int clport = 0;
    const CommandLineParser* clp = new CommandLineParser;
    if ( clp->hasKey( skeyport ) )
	clp->getVal( skeyport, clport );
    delete clp;

    if ( clport > 0 && NRC::isPortFree( (port_nr_type) clport ) )
	portid = (port_nr_type) clport;
    else if ( assignport && defport>0
			 && NRC::isPortFree((port_nr_type)defport) )
	portid = (port_nr_type) defport;
    else if ( assignport )
    {
	uiRetVal uirv;
	portid = NRC::getUsablePort( uirv );
	if ( !uirv.isOK() )
	{
	    pErrMsg( "unable to find usable port" );
	    portid = 0;
	}
    }

    if ( portid>0 )
	startServer( portid );
}


uiODServiceBase::~uiODServiceBase()
{
    stopServer();
}


void uiODServiceBase::startServer( port_nr_type portid )
{
    server_ = new Network::RequestServer( portid );
    if ( !server_ || !server_->isOK() )
    {
	pErrMsg( "startServer - failed" );
	stopServer();
	return;
    }
}


void uiODServiceBase::stopServer()
{
    deleteAndZeroPtr( server_ );
}


void uiODServiceBase::sendOK( Network::RequestConnection* conn,
				 PtrMan<Network::RequestPacket> packet )
{
    OD::JSON::Object response;
    response.set( sKeyOK(), BufferString::empty() );
    if ( packet )
	packet->setPayload( response );
    if ( packet && conn && !conn->sendPacket(*packet) )
	{ pErrMsg("sendOK - failed"); }
}


void uiODServiceBase::sendErr( Network::RequestConnection* conn,
				  PtrMan<Network::RequestPacket> packet,
				  uiRetVal& uirv )
{
    sendErr( conn, packet, uirv.getText() );
}


void uiODServiceBase::sendErr( Network::RequestConnection* conn,
				  PtrMan<Network::RequestPacket> packet,
				  const char* msg )
{
    OD::JSON::Object response;
    response.set( sKeyError(), msg );
    if ( packet )
	packet->setPayload( response );
    if ( packet && conn && !conn->sendPacket(*packet) )
	{ pErrMsg("sendErr - failed"); }
}



uiODService::uiODService( bool assignport )
    : uiODServiceBase(assignport)
    , odport_(0)
{
    const CommandLineParser* clp = new CommandLineParser;
    if ( clp->hasKey( sKeyODServer() ) )
    {
	BufferString odserverstr;
	clp->getVal( sKeyODServer(), odserverstr );
	BufferStringSet tmp;
	tmp.unCat( odserverstr, ":" );
	if ( tmp.size()==2 && tmp.get(1).isNumber(true) )
	{
	    odhostname_ = tmp.get(0);
	    odport_ = tmp.get(1).toInt();
	}
    }
    delete clp;
}


uiODService::~uiODService()
{
}
