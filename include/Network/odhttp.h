#ifndef odhttp_h
#define odhttp_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:		Salil Agarwal
Date:		Oct 2012
________________________________________________________________________

-*/


#include "networkmod.h"

#include "callback.h"
#include "ptrman.h"
#include "uistring.h"
#include "iopar.h"
#include "limits.h"

class QEventLoop;
class QNetworkReply;
class QNetworkReplyConn;
class QNetworkRequest;
class QNetworkAccessManager;
class DataBuffer;
class QByteArray;


namespace Network
{
class HttpRequestProcess;
class HttpRequestManager;
class HttpRequestManagerObj;
class RequestEvent;

/*!Description of an HTTP request, including headers and post-data */

mExpClass(Network) HttpRequest
{
public:
				HttpRequest(const char* url);
				HttpRequest(const HttpRequest&);
				~HttpRequest();

    HttpRequest&		postData(const DataBuffer&);
    HttpRequest&		contentType(const BufferString&);
    HttpRequest&		rawHeader(const char* key,
					  const char* val);

    void			setPostData(const DataBuffer&);
				//!<Data is copied
    void			setContentType(const BufferString&);
    void			setRawHeader(const char* key,
					     const char* val);
private:

    friend			class HttpRequestManager;
    friend			class HttpRequestManagerObj;

    void			fillRequest(QNetworkRequest&) const;
    QByteArray*			postdata_;
    BufferString		url_;
    BufferString		contenttype_;
    IOPar			rawheaders_;
};


/*! Main entrypoint of http(s) connections. A new request is
 created by calling either head(), get() or post().

 The call will return a RequestProcess, which can be queried
 about the results.
 */

mExpClass(Network) HttpRequestManager : public CallBacker
{
public:
    static HttpRequestManager&	instance();

				HttpRequestManager();
				~HttpRequestManager();

    void			shutDownThreading();

    RefMan<HttpRequestProcess>	head(const HttpRequest&);
    RefMan<HttpRequestProcess>	get(const HttpRequest&);
    RefMan<HttpRequestProcess>	post(const HttpRequest&);

private:
    enum AccessType		{ Get, Post, Head };
    RefMan<HttpRequestProcess>	request(const HttpRequest&,AccessType);

    void			threadFuncCB(CallBacker*);

    Threads::Thread*		thread_;
    QEventLoop*			eventloop_;
    QNetworkAccessManager*	qnam_;
    Threads::ConditionVar	lock_;

    struct RequestData : public CallBacker
    {
	RequestData(HttpRequestManager* hrm, const HttpRequest& req, AccessType at )
	    : req_(req)
	    , at_( at )
	    , reply_( 0 )
	    , isrun_( false )
	{}

	void wait()
	{
	    isrunlock_.lock();
	    while (!isrun_)
		isrunlock_.wait();
	    isrunlock_.unLock();
	}

	const AccessType		at_;
	const HttpRequest&		req_;
	RefMan<HttpRequestProcess>	reply_;
	bool				isrun_;
	Threads::ConditionVar		isrunlock_;
    };

    void			doRequestCB(CallBacker*);
				//Only called in thread_. CB must be RequestData
};


/*The upload or download process. Can be queried for progress, data and errors*/
mExpClass(Network) HttpRequestProcess : public RefCount::Referenced,
					public CallBacker
{
public:
				 //General purpose callbacks
    Notifier<HttpRequestProcess> finished;
    Notifier<HttpRequestProcess> error;


    bool			isRunning() const { return status_==Running; }
    bool			isFinished() const { return status_==Finished; }
    bool			isError() const { return status_==Error; }

    void			waitForFinish();
				//<Waits for error or Finish

				//Download access
    Notifier<HttpRequestProcess> downloadDataAvailable;
    bool			waitForDownloadData(int timeout_ms);
				//Returns false if timeout was hit
    od_int64			downloadBytesAvailable() const;
    od_int64			read(char*,od_int64 bufsize);
				//!<Returns nr bytes read
    BufferString		readAll();
    od_int64			getContentLengthHeader() const;

				//Upload access
    Notifier<HttpRequestProcess> uploadProgress;
    od_int64			getBytesUploaded() const;
    od_int64			getTotalBytesToUpload() const;

    uiString			errMsg() const { return errmsg_; }

private:

				//Interface from QNetworkReplyConn
    friend			class ::QNetworkReplyConn;

    void			setBytesUploaded(const od_int64 bytes);
    void			setTotalBytesToUpload(const od_int64 bytes);

				//Interface from NetworkAccessManager
    friend			class HttpRequestManager;
    friend			class HttpRequestManagerObj;
    void			setQNetowrkReply(QNetworkReply*);
				//!<Becomes mine

				HttpRequestProcess();
				~HttpRequestProcess();

    Threads::ConditionVar	statuslock_;
    enum Status			{ Running, Error, Finished } status_;

    void			errorOccurred(CallBacker*);
    void			finish(CallBacker*);
    void			dataAvailable(CallBacker*);
    void			uploadStatus(CallBacker*);

    Threads::Atomic<od_int64>	bytesuploaded_;
    Threads::Atomic<od_int64>	totalbytestoupload_;

    QNetworkReplyConn*		qnetworkreplyconn_;
    QNetworkReply*		qnetworkreply_;

    QByteArray*			receiveddata_;
    Threads::Lock		receiveddatalock_;

    uiString			errmsg_;
};

}; //namespace Network


#endif
