#ifndef localserver_h
#define localserver_h

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

class QLocalServer;
class QLocalServerComm;
class QLocalSocket;

mExpClass(Network) LocalServer : public CallBacker
{
public:
			LocalServer();
			~LocalServer();

    bool		listen(const char*);
    bool		isListening() const;
    void		close();
    bool		hasPendingConnections() const;
    int			write(const char*);
    			//!<Writes to next pending socket
    const char*		errorMsg() const;

    Notifier<LocalServer> newConnection;

    bool		waitForNewConnection(int msec,bool& timedout);
    			//!<Mostly useful when there is no event loop available
    QLocalSocket*	nextPendingConnection();
			//!<Use when you want to access Qt object directly

protected:

    QLocalServer*		qlocalserver_;
    QLocalServerComm*		comm_;
    mutable BufferString	errmsg_;

};

#endif

