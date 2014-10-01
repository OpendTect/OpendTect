#ifndef netserver_h
#define netserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"
#include "bufstring.h"

mFDQtclass(QTcpSocket)
mFDQtclass(QTcpServer)
mFDQtclass(QTcpServerComm)


namespace Network
{

class Socket;


mExpClass(Network) Server : public CallBacker
{
public:
			Server();
			~Server();

    bool		listen(const char* host,int port=0);
			//!<If host is 0, server will listen to any host
    bool		isListening() const;
    int			port() const;

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

    bool		waitForNewConnection(int msec);
			//!<Useful when no event loop available
			//!<If msec is -1, this function will not time out

protected:

    void		notifyNewConnection();
    void		readyReadCB(CallBacker*);
    void		disconnectCB(CallBacker*);

    mQtclass(QTcpServer)* qtcpserver_;
    mQtclass(QTcpServerComm)* comm_;
    mutable BufferString errmsg_;
    ObjectSet<Socket>	sockets_;
    TypeSet<int>	ids_;
    ObjectSet<Socket>	sockets2bdeleted_;

    friend class	mQtclass(QTcpServerComm);

};

} // namespace Network


#endif
