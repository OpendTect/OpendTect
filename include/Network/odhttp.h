#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:		Salil Agarwal
Date:		Oct 2012
________________________________________________________________________

-*/


#include "networkmod.h"

#include "notify.h"
#include "iopar.h"
#include "limits.h"
#include "manobjectset.h"
#include "ptrman.h"
#include "uistring.h"

class QEventLoop;
class QNetworkReply;
class QNetworkReplyConn;
class QNetworkRequest;
class QNetworkAccessManager;
class DataBuffer;
class QByteArray;
class QSslError;
template <class T> class QList;


namespace Network
{
class HttpRequestProcess;
class HttpRequestManager;

/*!Description of an HTTP request, including headers and post-data */

mExpClass(Network) HttpRequest : public RefCount::Referenced
{
public:
    enum AccessType		{ Get, Put, Delete, Post, Head };
				HttpRequest(const char* url,
					    AccessType);
				HttpRequest(const HttpRequest&);

    HttpRequest&		payloadData(const DataBuffer&);
    HttpRequest&		contentType(const BufferString&);
    HttpRequest&		rawHeader(const char* key,
					  const char* val);

    void			setPayloadData(const DataBuffer&);
				//!<For post/put requests Data is copied
    void			setContentType(const BufferString&);
    void			setRawHeader(const char* key,
					     const char* val);
protected:
				~HttpRequest();
private:

    friend			class HttpRequestManager;

    void			fillRequest(QNetworkRequest&) const;
    QByteArray*			payload_ = nullptr;
    BufferString		url_;
    BufferString		contenttype_;
    IOPar			rawheaders_;
    const AccessType		accesstype_;
};


/*! Main entrypoint of http(s) connections. A new request is
 created by calling either head(), get() or request().

 The call will return a HttpRequestProcess, which can be queried
 about the results.
 */

mExpClass(Network) HttpRequestManager : public CallBacker
{
public:
    static HttpRequestManager&	instance();

				HttpRequestManager();
				~HttpRequestManager();

    RefMan<HttpRequestProcess>	request(const HttpRequest*);
    RefMan<HttpRequestProcess>	get(const char* url);
    RefMan<HttpRequestProcess>	head(const char* url);

private:
    static void			CloseInstance();
    void			shutDownThreading();

    void			threadFuncCB(CallBacker*);

    Threads::Thread*		thread_ = nullptr;
    QNetworkAccessManager*	qnam_ = nullptr;
    QEventLoop*			eventloop_ = nullptr;
    Threads::ConditionVar*	eventlooplock_;
				//Temporary, only used in constructor

    void			doRequestCB(CallBacker*);
				//Only called in thread_.
				//CB must be HttpRequestProcess

    RefObjectSet<HttpRequestProcess>	activeevents_;
    Threads::SpinLock			activeeventslock_;
};


/*!The upload or download process. Can be queried for progress, data and
  errors*/
mExpClass(Network) HttpRequestProcess : public RefCount::Referenced,
					public CallBacker
{
public:
				 //General purpose callbacks
    Notifier<HttpRequestProcess> finished;
    Notifier<HttpRequestProcess> error;

    bool			isRunning() const;
    bool			isFinished() const;
    bool			isError() const;

    void			waitForFinish(int timeout_in_ms=-1);
				//<Waits for error or Finish

				//Download access
    Notifier<HttpRequestProcess> downloadDataAvailable;
    bool			waitForDownloadData(int timeout_ms=-1);
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

    uiString			errMsg() const;

private:

				//Interface from QNetworkReplyConn
    friend			class ::QNetworkReplyConn;

    void			reportDownloadProgress(od_int64 nrdone,
						       od_int64 totalnr);
    void			reportError();
    void			reportSSLErrors(const QList<QSslError>&);
    void			reportFinished();
    void			reportUploadProgress(od_int64 bytes,
						     od_int64 totalbytes);
    void			reportMetaDataChanged();
    void			reportReadyRead();

				//Interface from NetworkAccessManager
    friend			class HttpRequestManager;
    void			setQNetworkReply(QNetworkReply*);
				/*!<Becomes mine. Also signals start of
				    the request.  */

    bool			waitForRequestStart();
				/*!<Wait for the request to start in the
				    networking thread.
				    \returns if request was created */

				HttpRequestProcess(const HttpRequest*);
				~HttpRequestProcess();

    enum Status			{ NotStarted, //Before setQNetworkReply
				  Running, Error, Finished };

    Status					status_ = NotStarted;
    mutable Threads::ConditionVar		statuslock_;

    ConstRefMan<HttpRequest>			request_;

    Threads::Atomic<od_int64>			bytesuploaded_;
    Threads::Atomic<od_int64>			totalbytestoupload_;
    Threads::Atomic<od_int64>			bytesdownloaded_;
    Threads::Atomic<od_int64>			totalbytestodownload_;
    Threads::Atomic<od_int64>			contentlengthheader_;

    QNetworkReplyConn*				qnetworkreplyconn_ = nullptr;
    QNetworkReply*				qnetworkreply_ = nullptr;

    QByteArray*					receiveddata_ = nullptr;
    Threads::ConditionVar			receiveddatalock_;

    uiString					errmsg_;
};

} // namespace Network
