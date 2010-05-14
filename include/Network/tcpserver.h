#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpserver.h,v 1.5 2010-05-14 07:21:53 cvsranojay Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpServer;
class QTcpServerComm;
class QTcpSocket;
class TcpSocket;


mClass TcpServer : public CallBacker
{
friend class QTcpServerComm;

public:
    			TcpServer();
			~TcpServer();

    bool		listen(const char* host,int port);
    			//!<If host is 0, server will listen to any host
    bool		isListening() const;
    void		close();
    bool		hasPendingConnections() const;
    int			write(const char*);
    			//!<Writes to next pending socket    
    void		read(BufferString&) const;
    const char*		errorMsg() const;

    Notifier<TcpServer>	newConnection;
    Notifier<TcpServer>	readyRead;

    QTcpSocket*		nextPendingConnection();
    			//!<Use when you want to access Qt object directly

protected:
    
    void		newConnectionCB(CallBacker*);
    void		readyReadCB(CallBacker*);
    void		disconnectCB(CallBacker*);
    
    QTcpServer*		 qtcpserver_;
    QTcpServerComm*	 comm_;
    TcpSocket*		 tcpsocket_;
    mutable BufferString errmsg_;
    ObjectSet<TcpSocket> sockets2bdeleted_;
};

#endif
