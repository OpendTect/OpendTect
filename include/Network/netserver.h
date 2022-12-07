#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkcommon.h"

mFDQtclass(QTcpSocket)
mFDQtclass(QTcpServer)
mFDQtclass(QLocalSocket)
mFDQtclass(QLocalServer)
mFDQtclass(QTcpServerComm)


namespace Network
{

class Socket;

mExpClass(Network) Server : public CallBacker
{ mODTextTranslationClass(Server)
public:
			Server(bool islocal);
			~Server();

    bool		listen(SpecAddr=Any,PortNr_Type port=0);
			//!<If Any, server will listen to all network interfaces
    bool		listen(const char* servernm,uiRetVal&);
    bool		isListening() const;
    PortNr_Type		port() const;
    Authority		authority() const;
    bool		isLocal() const { return qlocalserver_; }

    void		close();
    bool		hasPendingConnections() const;
    int			write(int id,const char*);
			//!<Writes to next pending socket
    int			write(int id,const IOPar&);
    void		read(int id,BufferString&) const;
    void		read(int id,IOPar&) const;
    const char*		errorMsg() const;

    Socket*		getSocket(int id);
    const Socket*	getSocket(int id) const;

    CNotifier<Server,int> newConnection;
    CNotifier<Server,int> readyRead;

    mQtclass(QTcpSocket)* nextPendingConnection();
			//!<Use when you want to access Qt object directly
    mQtclass(QLocalSocket)* nextPendingLocalConnection();

    bool		waitForNewConnection(int msec);
			//!<Useful when no event loop available
			//!<If msec is -1, this function will not time out

    static const char*	sKeyHostName()		{ return "hostname"; }
    static const char*	sKeyNoListen()		{ return "no-listen"; }
    static const char*	sKeyPort()		{ return "port"; }
    static const char*	sKeyTimeout()		{ return "timeout"; }
    static const char*	sKeyKillword()		{ return "kill"; }
    static const char*	sKeyLocal()		{ return "local"; }

protected:

    void		notifyNewConnection();
    void		readyReadCB(CallBacker*);
    void		disconnectCB(CallBacker*);

    mQtclass(QTcpServer)*	qtcpserver_		= nullptr;
    mQtclass(QLocalServer)*	qlocalserver_		= nullptr;
    mQtclass(QTcpServerComm)*	comm_;
    mutable BufferString	errmsg_;
    ObjectSet<Socket>		sockets_;
    TypeSet<int>		ids_;
    ObjectSet<Socket>		sockets2bdeleted_;

    friend class		mQtclass(QTcpServerComm);

};

} // namespace Network
