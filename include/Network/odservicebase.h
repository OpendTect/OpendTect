#pragma once
/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	Prajjaval Singh
* Date:		April 2020
* RCS:		$Id$
* ________________________________________________________________________
*
* -*/

#include "networkmod.h"

#include "networkcommon.h"
#include "ptrman.h"

namespace Network {
    class RequestConnection;
    class RequestPacket;
    class RequestServer;
};

namespace OD { namespace JSON { class Object; }; };


/*!\brief Base class for OpendTect Service Manager and external services/apps */

mExpClass(Network) ODServiceBase : public CallBacker
{ mODTextTranslationClass(ODServiceBase)
public:

    virtual		~ODServiceBase();

    bool		isOK(bool islocal) const;

    Network::Authority	getAuthority(bool islocal) const;
    virtual void	stopServer();

    static const char*	sKeyAction()		{ return "action"; }
    static const char*	sKeyError()		{ return "error"; }
    static const char*	sKeyOK()		{ return "ok"; }

    static const char*	sKeyEvent()		{ return "event"; }
    static const char*	sKeyRegister()		{ return "register"; }
    static const char*	sKeyDeregister()	{ return "deregister"; }
    static const char*	sKeyStart()		{ return "start"; }

    static const char*	sKeyCloseEv()		{ return "close"; }
    static const char*	sKeyHideEv()		{ return "hide"; }
    static const char*	sKeyPyEnvChangeEv()	{ return "pyenvchange"; }
    static const char*	sKeyRaiseEv()		{ return "raise"; }
    static const char*	sKeyStatusEv()		{ return "status"; }
    static const char*	sKeySurveyChangeEv()	{ return "surveychange"; }
    static const char*	sKeyTransferCmplt()	{ return "transfer_complete"; }
    static const char*	sKeyProcessingDone()	{ return "processing_done"; }

    static const char*	sKeyODServer()		{ return "odserver"; }

    CNotifier<ODServiceBase,BufferString>		externalAction;
    CNotifier<ODServiceBase,const OD::JSON::Object*>	externalRequest;

    void		setRetVal( const uiRetVal& uirv )	{ uirv_ = uirv;}
			/* Must be called to set the result of intercepting
			   either externalAction or externalRequest */

protected:
			ODServiceBase(const char* hostname,
						bool assignport=true);
			explicit ODServiceBase(bool islocal,
			    const char* servernm=nullptr,bool assignport=true);

    bool		isMainService() const;
    virtual void	startServer(const Network::Authority&);

    static uiRetVal	sendAction(const Network::Authority&,
				   const char* servicenm,const char* action);
    static uiRetVal	sendRequest(const Network::Authority&,
				    const char* servicenm,const char* reqkey,
				    const OD::JSON::Object&);

			/*!< Reimplement these functions to intercept
			     packets intended for your service */
    virtual bool	doParseAction(const char*,uiRetVal&);
			//!< Return true if the action was handled
    virtual bool	doParseRequest(const OD::JSON::Object&,
				       uiRetVal&);
			//!< Return true if the request was handled

    static const OD::JSON::Object* getSubObj(const OD::JSON::Object&,
					     const char* key);
    static void		getPythEnvRequestInfo(OD::JSON::Object&);

    void		sendOK();
    void		sendErr(uiRetVal&);

    virtual void	doSurveyChanged(CallBacker*)		{}
    virtual void	doAppClosing(CallBacker*);
    virtual void	doPyEnvChange(CallBacker*)		{}
    virtual void	doConnClosed(CallBacker*)		{}

    virtual void	closeApp();

private:
			ODServiceBase(const ODServiceBase&) = delete;
			ODServiceBase(ODServiceBase&&) = delete;

    ODServiceBase&	operator=(const ODServiceBase&) = delete;
    ODServiceBase&	operator=(ODServiceBase &&) = delete;

    void		init(bool islocal,const char* hostname,
							    bool assignport);

    uiRetVal		doAction(const OD::JSON::Object&);
    uiRetVal		doRequest(const OD::JSON::Object&);
    uiRetVal		survChangedReq(const OD::JSON::Object&);
    uiRetVal		pythEnvChangedReq(const OD::JSON::Object&);
    uiRetVal		doPrepareForClose();

    void		newConnectionCB(CallBacker*);
    void		packetArrivedCB(CallBacker*);
    void		externalActionCB(CallBacker*);
    void		externalRequestCB(CallBacker*);
    void		connClosedCB(CallBacker*);
    void		appClosingCB(CallBacker*);

    void		surveyChangedCB(CallBacker*);
    void		pyenvChangeCB(CallBacker*);

    Network::RequestServer*	tcpserver_ = nullptr;
    Network::RequestServer*	localserver_ = nullptr;
    bool		serverismine_ = true;
    Network::RequestConnection*		tcpconn_ = nullptr;
    Network::RequestConnection*		localconn_ = nullptr;
    PtrMan<Network::RequestPacket>	packet_;
    bool		needclose_ = false;
    uiRetVal		uirv_;
    Threads::Lock	lock_;
    static ODServiceBase* theMain(ODServiceBase* newmain=nullptr);


};
