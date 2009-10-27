#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpserver.h,v 1.4 2009-10-27 03:22:20 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpServer;
class QTcpServerComm;
class QTcpSocket;


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
    const char*		errorMsg() const;

    Notifier<TcpServer>	newConnection;

    QTcpSocket*		nextPendingConnection();
    			//!<Use when you want to access Qt object directly

protected:

    QTcpServer*		qtcpserver_;
    QTcpServerComm*	comm_;
    mutable BufferString errmsg_;
};

#endif
