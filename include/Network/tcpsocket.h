#ifndef tcpsocket_h
#define tcpsocket_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpsocket.h,v 1.1 2009-04-01 09:40:06 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpSocket;
class QTcpSocketComm;


class TcpSocket : public CallBacker
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
    Notifier<TcpSocket>		error;
    Notifier<TcpSocket>		stateChanged;

protected:

    QTcpSocket*			qtcpsocket_;
    QTcpSocketComm*		comm_;
    mutable BufferString	errmsg_;
};

#endif
