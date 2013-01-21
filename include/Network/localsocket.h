#ifndef localsocket_h
#define localsocket_h

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
#include "sets.h"

class QLocalSocket;
class QLocalSocketComm;

mExpClass(Network) LocalSocket : public CallBacker
{
friend class QLocalSocketComm;

public:
    				LocalSocket();
				~LocalSocket();

    void			connectToServer(const char*);
    void			disconnectFromServer();
    void			abort();
    int				write(const char*);
    void			read(BufferString&);
    const char*			errorMsg() const;

    Notifier<LocalSocket>	connected;
    Notifier<LocalSocket>	disconnected;
    Notifier<LocalSocket>	error;
    Notifier<LocalSocket>	readyRead;

    bool			waitForConnected(int msec);
    				//!<Useful when no event loop available
    bool			waitForReadyRead(int msec);
    				//!<Useful when no event loop available

protected:

    QLocalSocket*		qlocalsocket_;
    QLocalSocketComm*		comm_;
    mutable BufferString	errmsg_;
};

#endif

