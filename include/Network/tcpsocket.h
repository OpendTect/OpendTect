#ifndef tcpsocket_h
#define tcpsocket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpsocket.h,v 1.5 2010-02-08 11:35:53 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpSocket;
class QTcpSocketComm;


mClass TcpSocket : public CallBacker
{
friend class QTcpSocketComm;

public:
    				TcpSocket();
				~TcpSocket();

    void			connectToHost(const char* host,int port);
    void			disconnectFromHost();
    void			abort();
    int				write(const char*);
    void			read(BufferString&);

    const char*			errorMsg() const;

    Notifier<TcpSocket>		connected;
    Notifier<TcpSocket>		disconnected;
    Notifier<TcpSocket>		hostFound;
    Notifier<TcpSocket>		readyRead;
    Notifier<TcpSocket>		error;
    Notifier<TcpSocket>		stateChanged;

    bool			waitForConnected(int msec);
    				//!<Useful when no event loop available
    bool			waitForReadyRead(int msec);
    				//!<Useful when no event loop available

protected:

    QTcpSocket*			qtcpsocket_;
    QTcpSocketComm*		comm_;
    mutable BufferString	errmsg_;
};

#endif
