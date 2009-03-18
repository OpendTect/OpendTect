#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpserver.h,v 1.1 2009-03-18 04:24:39 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpServer;
class QTcpServerComm;


class TcpServer : public CallBacker
{
friend class QTcpServerComm;

public:
    				TcpServer();
				~TcpServer();

    bool			listen(int port);
    bool			isListening() const;
    void			close();

    const char*			errorMsg() const;

    Notifier<TcpServer>		newConnection;

protected:

    QTcpServer*			qtcpserver_;
    QTcpServerComm*		comm_;
    mutable BufferString	errmsg_;
};

#endif
