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

class Timer;
namespace Network {
class RequestConnection;
class RequestPacket;
class RequestServer;
};

namespace OD {
namespace JSON {
class Object;
};
};

namespace sKey
{
inline FixedString NN3D()	{ return "NeuralNetwork3D"; }
inline FixedString NN2D()	{ return "NeuralNetwork2D"; }
inline FixedString UVQ3D()	{ return "NNQuickUVQ3D"; }
inline FixedString UVQ2D()	{ return "NNQuickUVQ2D"; }
};


/*!\brief Base class for OpendTect Service Manager and external services/apps */

mExpClass(Network) ODServiceBase : public CallBacker
{ mODTextTranslationClass(ODServiceBase)
public:

    virtual		~ODServiceBase();

    bool		isOK() const;

    Network::Authority	getAuthority() const;
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

    static const char*	sKeyODServer()		{ return "odserver"; }

    CNotifier<ODServiceBase,BufferString>	externalAction;
    CNotifier<ODServiceBase,OD::JSON::Object>	externalRequest;

    void		setRetVal( const uiRetVal& uirv )	{ uirv_ = uirv;}
			/* Must be called to set the result of intercepting
			   either externalAction or externalRequest */

protected:
			ODServiceBase(bool assignport=true);

    static uiRetVal	sendAction(const Network::Authority&,
				   const char* servicenm,const char* action);
    static uiRetVal	sendRequest(const Network::Authority&,
				    const char* servicenm,const char* reqkey,
				    const OD::JSON::Object&);
    virtual uiRetVal	doAction(const OD::JSON::Object&);
    virtual uiRetVal	doRequest(const OD::JSON::Object&);
    uiRetVal		doCloseAct();

    static const OD::JSON::Object* getSubObj(const OD::JSON::Object&,
					     const char* key);
    uiRetVal		survChangedReq(const OD::JSON::Object&);
    uiRetVal		pythEnvChangedReq(const OD::JSON::Object&);
    static void		getPythEnvRequestInfo(OD::JSON::Object&);

    void		sendOK();
    void		sendErr(uiRetVal&);

    virtual void	doSurveyChanged(CallBacker*)		{}
    virtual void	doAppClosing(CallBacker*)		{}
    virtual void	doPyEnvChange(CallBacker*)		{}
    virtual void	connClosedCB(CallBacker*);

    bool		needclose_ = false;

private:
			ODServiceBase(const ODServiceBase&) = delete;
			ODServiceBase(ODServiceBase&&) = delete;

    ODServiceBase&	operator=(const ODServiceBase&) = delete;
    ODServiceBase&	operator=(ODServiceBase &&) = delete;

    virtual void	startServer(PortNr_Type);

    void		newConnectionCB(CallBacker*);
    void		packetArrivedCB(CallBacker*);

    void		surveyChangedCB(CallBacker*);
    void		appClosingCB(CallBacker*);
    void		pyenvChangeCB(CallBacker*);

    Network::RequestServer*	server_ = nullptr;
    Network::RequestConnection*		conn_ = nullptr;
    PtrMan<Network::RequestPacket>	packet_;

    uiRetVal		uirv_;

};
