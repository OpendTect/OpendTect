#ifndef tcpsocket_h
#define tcpsocket_h

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

class QTcpSocket;
class OldQTcpSocketComm;


/* This class is DEPRECATED. Will disappear after 5.0.
   Use Network::Socket instead. */


mExpClass(Network) TcpSocket : public CallBacker
{
friend class OldQTcpSocketComm;

public:
				TcpSocket();
				TcpSocket(QTcpSocket*,int id);
				~TcpSocket();

    void			connectToHost(const char* host,int port);
    void			disconnectFromHost();
    void			abort();
    int				write(const char*);
    int				write(const IOPar&);
    int				write(const int&);
    int				write(bool);
    int				writedata(const char*, int nr);

    void			read(BufferString&) const;
    void			read(IOPar&) const;
    void			read(int&) const;
    void			read(bool&) const;
    int				readdata(char* data, int sz) const;
    int				getID() const { return id_; }

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
    bool			waitForReadyWrite(int msec);
				//!<Useful when no event loop available
    bool			waitForDisconnected(int msec);
				//!<Useful when no event loop available

protected:

    QTcpSocket*			qtcpsocket_;
    OldQTcpSocketComm*		comm_;
    mutable BufferString	errmsg_;
    const int			id_;
    bool			isownsocket_;
};

#endif

