#ifndef tcpserver_h
#define tcpserver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpserver.h,v 1.3 2009-07-22 16:01:17 cvsbert Exp $
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
