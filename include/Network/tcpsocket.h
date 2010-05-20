#ifndef tcpsocket_h
#define tcpsocket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpsocket.h,v 1.7 2010-05-20 09:46:53 cvsranojay Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpSocket;
class QTcpSocketComm;
class IOPar;


mClass TcpSocket : public CallBacker
{
friend class QTcpSocketComm;

public:
    				TcpSocket();
				TcpSocket(QTcpSocket*);
				~TcpSocket();

    void			connectToHost(const char* host,int port);
    void			disconnectFromHost();
    void			abort();
    int				write(const char*) const;  
    int				write(const IOPar&) const;

    void			read(BufferString&) const;
    void			read(IOPar&) const;

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
