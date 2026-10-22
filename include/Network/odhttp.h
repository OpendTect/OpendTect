#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkmod.h"

#include "iopar.h"
#include "limits.h"
#include "ptrman.h"
#include "sharedobject.h"
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
class Authority;
class HttpRequestProcess;
class HttpRequestManager;

/* One route for an HTTP request. Unset lets the caller pick a route. */

mExpClass(Network) HttpProxyConfig
{
public:
			HttpProxyConfig();
			HttpProxyConfig(const HttpProxyConfig&);
			~HttpProxyConfig();

    HttpProxyConfig&	operator=(const HttpProxyConfig&);

    enum class Type	{ Unset, Direct, Http, Socks5 };

    Type		type_		= Type::Unset;
    Authority&		authority_;
    BufferString	username_;
    BufferString	password_;
};

/* Parses http://, https://, socks5:// and host:port proxy specs. */
mGlobal(Network) bool proxySpecToConfig(const char* spec,HttpProxyConfig&);

/* Host is listed in a no_proxy/NO_PROXY style list. */
mGlobal(Network) bool proxyExcludesHost(const char* host,const char* noproxy);

/* Proxy from https_proxy, http_proxy or all_proxy for this URL.
   A null URL checks those variables in that order. */
mGlobal(Network) bool environmentProxyFor(const char* url,HttpProxyConfig&);

/* First route to try: a route that already worked, then the environment,
   the system proxy, a direct connection, and a saved manual proxy. */
mGlobal(Network) HttpProxyConfig preferredDownloadProxy(const char* url);


/*!Description of an HTTP request, including headers and post-data */

mExpClass(Network) HttpRequest : public ReferencedObject
{
public:
    enum class AccessType	{ Get, Put, Delete, Post, Head };
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
    void			setEncodedData(const char* key,
					       const char* val);
    void			setProxyConfig(const HttpProxyConfig&);
    const HttpProxyConfig&	proxyConfig() const	{ return proxy_; }
    void			setTransferTimeoutMs(int timeoutms);
				/*!<Aborts the transfer when nothing is
				    received for this many milliseconds.
				    Zero keeps the Qt default. */
    const char*			url() const		{ return url_.buf(); }

protected:
				~HttpRequest();

private:

    friend			class HttpRequestManager;

    void			fillRequest(QNetworkRequest&) const;
    QByteArray*			payload_ = nullptr;
    BufferString		url_;
    BufferString		contenttype_;
    IOPar			rawheaders_;
    IOPar			encodeddata_;
    const AccessType		accesstype_;
    HttpProxyConfig		proxy_;
    int				transfertimeoutms_	= 0;
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
mExpClass(Network) HttpRequestProcess : public SharedObject
{ mODTextTranslationClass(HttpRequestProcess);
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
    int				httpStatusCode() const;

    enum class FailureKind	{ NoFailure, ConnectFailure,
				  ProxyAuthFailure, SslFailure, HttpFailure };
    FailureKind			failureKind() const;

				//Upload access
    Notifier<HttpRequestProcess> uploadProgress;
    od_int64			getBytesUploaded() const;
    od_int64			getTotalBytesToUpload() const;

    uiRetVal			errMsgs() const;

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

    enum class Status		{ NotStarted, //Before setQNetworkReply
				  Running, Error, Finished };

    Status			status_ = Status::NotStarted;
    FailureKind			failurekind_ = FailureKind::NoFailure;
    mutable Threads::ConditionVar statuslock_;

    ConstRefMan<HttpRequest>	request_;

    Threads::Atomic<od_int64>	bytesuploaded_;
    Threads::Atomic<od_int64>	totalbytestoupload_;
    Threads::Atomic<od_int64>	bytesdownloaded_;
    Threads::Atomic<od_int64>	totalbytestodownload_;
    Threads::Atomic<od_int64>	contentlengthheader_;

    QNetworkReplyConn*		qnetworkreplyconn_ = nullptr;
    QNetworkReply*		qnetworkreply_ = nullptr;

    QByteArray*			receiveddata_ = nullptr;
    mutable Threads::ConditionVar receiveddatalock_;

    uiRetVal			errmsgs_;
};

} // namespace Network
