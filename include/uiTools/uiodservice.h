#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	Wayne Mogg
 * Date:		Oct 2019
 * RCS:		$Id$
 * ________________________________________________________________________
 *
 * -*/

#include "uitoolsmod.h"
#include "callback.h"
#include "uistring.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netservice.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */

typedef Network::RequestConnection::port_nr_type port_nr_type;

mExpClass(uiTools) uiODServiceBase : public CallBacker
{ mODTextTranslationClass(uiODServiceBase)
public:

    uiODServiceBase(uiODServiceBase const&) = delete;
    uiODServiceBase(uiODServiceBase&&) = delete;
    uiODServiceBase& operator=(uiODServiceBase const&) = delete;
    uiODServiceBase& operator=(uiODServiceBase &&) = delete;

    bool		isOK() const { return port()>0; }
    port_nr_type	port() const { return server_ ?
					    server_->server()->port() : 0; }
    virtual void	stopServer();

    static const char*	sKeyAction()		{ return "action"; }
    static const char*	sKeyError()		{ return "error"; }
    static const char*	sKeyOK()		{ return "ok"; }

    static const char*	sKeyEvent()		{ return "event"; }
    static const char*	sKeyRegister()		{ return "register"; }
    static const char*	sKeyDeregister()	{ return "deregister"; }

    static const char*	sKeyCloseEv()		{ return "close"; }
    static const char*	sKeyHideEv()		{ return "hide"; }
    static const char*	sKeyPyEnvChangeEv()	{ return "pyenvchange"; }
    static const char*	sKeyRaiseEv()		{ return "raise"; }
    static const char*	sKeyStatusEv()		{ return "status"; }
    static const char*	sKeySurveyChangeEv()	{ return "surveychange"; }

    static const char*	sKeyODServer()		{ return "odserver"; }

protected:
			uiODServiceBase(bool assignport=true);
			~uiODServiceBase();

    virtual void	startServer( port_nr_type );
    void		sendOK(Network::RequestConnection*,
				RefMan<Network::RequestPacket>);
    void		sendErr(Network::RequestConnection*,
				RefMan<Network::RequestPacket>,
				uiRetVal&);
    void		sendErr(Network::RequestConnection*,
				RefMan<Network::RequestPacket>,
				const char*);

    Network::RequestServer*	server_ = nullptr;
};


mExpClass(uiTools) uiODService : public uiODServiceBase
{ mODTextTranslationClass(uiODService)
public:
    uiODService(uiODService const&) = delete;
    uiODService(uiODService&&) = delete;
    uiODService& operator=(uiODService const&) = delete;
    uiODService& operator=(uiODService &&) = delete;

    void	setServiceName( const char* nm ) { serviceinfo_.setName(nm); }
protected:
    uiODService( bool assignport=false );
    ~uiODService();

    void		packetArrivedCB(CallBacker*);
    void		connClosedCB(CallBacker*);
    void		newConnectionCB(CallBacker*);

    uiRetVal		sendAction( OD::JSON::Object* );
    virtual uiRetVal	doAction( BufferString );
    uiRetVal		doRegister();
    uiRetVal		doDeRegister();

    void		statusMsg( uiString );

    Network::Service	serviceinfo_;
    BufferString	odhostname_;
    port_nr_type	odport_;

};
