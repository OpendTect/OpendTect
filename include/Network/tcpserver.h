#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpserver.h,v 1.2 2009-03-20 06:15:03 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpServer;
class QTcpServerComm;


mClass TcpServer : public CallBacker
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
