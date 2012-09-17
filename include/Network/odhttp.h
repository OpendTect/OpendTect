#ifndef odhttp_h
#define odhttp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: odhttp.h,v 1.18 2012/06/22 05:07:51 cvsranojay Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstringset.h"

class MyHttp;
class QHttpConnector;

mClass ODHttp : public CallBacker
{
friend class QHttpConnector;

public:
    			ODHttp();
			~ODHttp();

    enum State		{ Unconnected, HostLookup, Connecting, Sending,
			  Reading, Connected, Closing };

    void		setASynchronous(bool);
    int			setHost(const char* host,int port=80);
    int			setHttpsHost(const char* host,int port=443);
    int			setProxy(const char* host,int port,
				 const char* usrnm=0,const char* pwd=0);
    int			close();
    void		forceAbort();
    void		resetForceAbort() { forcedabort_ = false; }
    bool		isForcedAbort() const { return forcedabort_; }
    void		abort();
    State		state() const;

    bool		hasPendingRequests() const;
    void		clearPendingRequests();

    int			currentRequestID() const	{ return requestid_; }
    int			get(const char* cmd,const char* dest=0);
    			//!<When dest=0, read from buffer
			//!<Returns -1 on error
    int			post( const char* cmd,
			      const IOPar& postvars );
    			//!<Returns -1 on error
    BufferString	readBuffer() const;
    wchar_t*		readWCharBuffer() const; //!< Buffer becomes yours
    const char*		readCharBuffer() const;

    od_int64		bytesAvailable() const;

    int			nrDone() const          { return nrdone_; }
    int			totalNr() const         { return totalnr_; }

    const bool		isOK() const		{ return !error_; }
    void		setMessage(const char*);
    const char*		message() const		{ return message_.buf(); }

    static const char*	sKeyUseProxy();
    static const char*	sKeyUseAuthentication();
    static const char*	sKeyProxyHost();
    static const char*	sKeyProxyPort();
    static const char*	sKeyProxyUserName();
    static const char*	sKeyProxyPassword();

    Notifier<ODHttp>	requestStarted;
    Notifier<ODHttp>	requestFinished;
    Notifier<ODHttp>	messageReady;
    Notifier<ODHttp>	readyRead;
    Notifier<ODHttp>	dataReadProgress;
    Notifier<ODHttp>	done;
    Notifier<ODHttp>	connected;
    Notifier<ODHttp>	disconnected;

protected:

    MyHttp*		qhttp_;
    QHttpConnector*	qhttpconn_;

    bool		error_;
    BufferString	message_;
    int			nrdone_;
    int			totalnr_;
    int			requestid_;
    bool		forcedabort_;
    void		useProxySettings();
    void		reqFinishedCB(CallBacker*);
};

#endif
