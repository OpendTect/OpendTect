#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"
#include "bufstring.h"

class QTcpServer;
class QTcpServerComm;
class QTcpSocket;
class TcpSocket;


mExpClass(Network) TcpServer : public CallBacker
{
friend class QTcpServerComm;

public:
			TcpServer();
			~TcpServer();

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

    TcpSocket*		getSocket(int id);
    const TcpSocket*	getSocket(int id) const;

    CNotifier<TcpServer,int>	newConnection;
    CNotifier<TcpServer,int>	readyRead;

    QTcpSocket*		nextPendingConnection();
			//!<Use when you want to access Qt object directly

    bool		waitForNewConnection(int msec);
			//!<Useful when no event loop available
			//!<If msec is -1, this function will not time out

protected:

    void		notifyNewConnection();
    void		readyReadCB(CallBacker*);
    void		disconnectCB(CallBacker*);

    QTcpServer*		 qtcpserver_;
    QTcpServerComm*	 comm_;
    mutable BufferString errmsg_;
    ObjectSet<TcpSocket> sockets_;
    TypeSet<int>	ids_;
    ObjectSet<TcpSocket> sockets2bdeleted_;
};

#endif

