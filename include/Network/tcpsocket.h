#ifndef tcpsocket_h
#define tcpsocket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpsocket.h,v 1.4 2009-11-25 03:30:52 cvsnanne Exp $
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

    const char*			errorMsg() const;

    void			connectToHost(const char* host,int port);
    void			disconnectFromHost();

    Notifier<TcpSocket>		connected;
    Notifier<TcpSocket>		disconnected;
    Notifier<TcpSocket>		hostFound;
    Notifier<TcpSocket>		readyRead;
    Notifier<TcpSocket>		error;
    Notifier<TcpSocket>		stateChanged;

protected:

    QTcpSocket*			qtcpsocket_;
    QTcpSocketComm*		comm_;
    mutable BufferString	errmsg_;
};

#endif
