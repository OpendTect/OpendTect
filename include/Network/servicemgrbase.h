#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "netservice.h"

#include "ptrman.h"

namespace Network
{
    class RequestConnection;
    class RequestPacket;
    class RequestServer;
}

namespace OD { namespace JSON { class Object; } }


/*!\brief Base class for OpendTect Service Manager and external services/apps */

mExpClass(Network) ServiceMgrBase : public NamedCallBacker
{ mODTextTranslationClass(ServiceMgrBase)
public:

    virtual		~ServiceMgrBase();

    bool		isOK(bool islocal) const;

    Network::Authority	getAuthority(bool islocal) const;
    virtual void	stopServer(bool islocal);

    static const char*	sKeyStatusEv()		{ return "status"; }
    static const char*	sKeyCloseEv()		{ return "close"; }
    static const char*	sKeySurveyChangeEv()	{ return "surveychange"; }
    static const char*	sKeyPyEnvChangeEv()	{ return "pyenvchange"; }

    static const char*	sKeyEvent()		{ return "event"; }
    static const char*	sKeyRegister()		{ return "register"; }
    static const char*	sKeyDeregister()	{ return "deregister"; }

    static const char*	sKeyODServer()		{ return "odserver"; }

protected:
			ServiceMgrBase(const char* servicenm);
			/* Main constructor returns a local service */
			ServiceMgrBase(const char* servicenm,bool assignport,
				      Network::SpecAddr=Network::Any);
			/* Constructor for a TCP-based server */

    bool		isMainService() const;
    bool		addLocalServer();
    bool		addTCPServer(bool assignport=true,
				     Network::SpecAddr=Network::Any);

    static uiRetVal	sendAction(const Network::Authority&,
				   const char* servicenm,const char* action);
    static uiRetVal	sendRequest(const Network::Authority&,
				    const char* servicenm,const char* reqkey,
				    const OD::JSON::Object&);

			/*!< Reimplement these functions to intercept
			     packets intended for your service
			     Return true if the action/request can be handled */
    virtual bool	canParseAction(const char*,uiRetVal&);
    virtual bool	canParseRequest(const OD::JSON::Object&,
					uiRetVal&);

    CNotifier<ServiceMgrBase,BufferString>	startHandleAction;
    CNotifier<ServiceMgrBase,const OD::JSON::Object*>	startHandleRequest;

    virtual uiRetVal	doHandleAction(const char* action);
    virtual uiRetVal	doHandleRequest(const OD::JSON::Object&);

    static void		getPythEnvRequestInfo(OD::JSON::Object&);

    void		sendOK();
    void		sendErr(uiRetVal&);

    virtual void	doPyEnvChange(CallBacker*)		{}
    virtual void	doSurveyChanged(CallBacker*)		{}

    virtual void	doAppClosing(CallBacker*);
				/*<! Must be re-implemented to delete all
				     important objects before the application
				     is closed.
				     Must end by calling the base class
				     implementation			  */

    virtual void	closeApp();

    static bool		addApplicationAuthority(bool local,OS::MachineCommand&);
    mDeprecated("Use bool argument")
    static const ServiceMgrBase* theMain();
    static const ServiceMgrBase* theMain(bool local);

    static void		debugMsg(const char*); // Not implemented

    static const char* sKeyClientAppCloseEv() { return "clientapp-closing"; }

private:

    mStruct(Network) packetData
    {
			packetData(const Network::RequestConnection*);
			~packetData();

	const Network::RequestConnection*	conn_;
	ServiceMgrBase*		servicemgr_ = nullptr;
	BufferString		action_;
	OD::JSON::Object*	request_ = nullptr;
	mutable uiRetVal	msg_;
    };
			mOD_DisableCopy(ServiceMgrBase);
			ServiceMgrBase(ServiceMgrBase&&) = delete;
    ServiceMgrBase&	operator=(ServiceMgrBase &&) = delete;

    void		init(bool islocal,bool assignport=true,
			     Network::SpecAddr=Network::Any);
    bool		useServer(Network::RequestServer*,bool islocal);
    static uiRetVal	sendRequest_(const Network::Authority&,
				     const char* servicenm,
				     const OD::JSON::Object&);

    bool		canDoAction(const OD::JSON::Object&,packetData&);
    bool		canDoRequest(const OD::JSON::Object&,packetData&);
    uiRetVal		applyInOtherThread(const packetData&);
    uiRetVal		survChangedReq(const OD::JSON::Object&);
    uiRetVal		pythEnvChangedReq(const OD::JSON::Object&);

    bool		isServerOK(bool local) const;
    bool&		serverIsMine(bool islocal);

    void		newConnectionCB(CallBacker*);
    void		packetArrivedCB(CallBacker*);
    void		connClosedCB(CallBacker*);
    void		doHandleActionRequest(CallBacker*);
    uiRetVal		handleActionRequestInThread(const packetData&);

    void		pyenvChangeCB(CallBacker*);
    void		surveyChangedCB(CallBacker*);
    void		appClosingCB(CallBacker*);

    Network::RequestServer*	    tcpserver_ = nullptr;
    Network::RequestServer*	    localserver_ = nullptr;
    bool			    tcpserverismine_ = true;
    bool			    localserverismine_ = true;
    Network::RequestConnection*     tcpconn_ = nullptr;
    Network::RequestConnection*     localconn_ = nullptr;
    RefMan<Network::RequestPacket>  packet_;
    ObjectSet<packetData>	    applydata_;

    static const char*	sKeyAction()		{ return "action"; }
    static const char*	sKeyError()		{ return "error"; }
    static const char*	sKeyOK()		{ return "ok"; }

    mDeprecated("Use bool argument")
    static ServiceMgrBase* theNewMain(ServiceMgrBase*);
    static ServiceMgrBase* theNewMain(bool local,ServiceMgrBase*);

};
