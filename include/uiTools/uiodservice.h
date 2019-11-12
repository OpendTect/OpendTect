#ifndef uiodservice_h
#define uiodservice_h
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
#include "ptrman.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */


mExpClass(uiTools) uiODServiceBase : public CallBacker
{ mODTextTranslationClass(uiODServiceBase)
public:
    typedef Network::port_nr_type port_nr_type;

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
    static const char*	sKeySurveyChangeEv()	{ return "surveychange"; }

    static const char*	sKeyODServer()		{ return "odserver"; }

protected:
			uiODServiceBase(bool assignport=true);
    virtual		~uiODServiceBase();

    virtual void	startServer(port_nr_type);
    void		sendOK(Network::RequestConnection*,
				PtrMan<Network::RequestPacket>);
    void		sendErr(Network::RequestConnection*,
				PtrMan<Network::RequestPacket>,
				uiRetVal&);
    void		sendErr(Network::RequestConnection*,
				PtrMan<Network::RequestPacket>,
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

protected:
			uiODService(bool assignport=false);
    virtual		~uiODService();

    BufferString	odhostname_;
    port_nr_type	odport_;

};

#endif
