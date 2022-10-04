#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "namedobj.h"
#include "ptrman.h"
#include "uistringset.h"

namespace Network {
    class Authority;
    class RequestConnection;
    class RequestPacket;
    class RequestServer;
    class Service;
}
namespace OD { namespace JSON { class Object; } }

mExpClass(MMProc) MMPServer : public NamedCallBacker
{ mODTextTranslationClass(MMPServer)
public:
			MMPServer(PortNr_Type,int timeout=2000);
			~MMPServer();

    uiRetVal		errMsg() const		{ return errmsg_; }
    bool		isOK() const;
    void		setTimeout( int timeout ) { timeout_ = timeout; }
    void		setLogFile(const char*);

    uiRetVal		sendResponse(const char* key,const OD::JSON::Object&);

    CNotifier<MMPServer,const OD::JSON::Object&>	startJob;
    CNotifier<MMPServer,const uiRetVal&>		logMsg;
    Notifier<MMPServer>					dataRootChg;
    Notifier<MMPServer>					getLogFile;

protected:
			mOD_DisableCopy(MMPServer);

    Network::RequestServer&		server_;
    Network::Service&			thisservice_;
    int					timeout_;
    Network::RequestConnection*		tcpconn_ = nullptr;
    RefMan<Network::RequestPacket>	packet_;
    uiRetVal				errmsg_;

    void		handleStatusRequest(const OD::JSON::Object&);
    void		handleSetDataRootRequest(const OD::JSON::Object&);

    void		newConnectionCB(CallBacker*);
    void		packetArrivedCB(CallBacker*);
    void		connClosedCB(CallBacker*);

    uiRetVal		doHandleRequest(const OD::JSON::Object&);

};
