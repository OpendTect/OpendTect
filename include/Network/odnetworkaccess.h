#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkmod.h"

#include "bufstringset.h"
#include "odhttp.h"
#include "uistring.h"

class DataBuffer;
class ODNetworkProcess;
class od_ostream;
class QEventLoop;
class QFile;
class QNetworkAccessManager;
class TaskRunner;

namespace Network
{
    class HttpRequestProcess;

/*!< Functions to download/upload one or more files/data using HTTP protocol*/

    mGlobal(Network) uiRetVal downloadFile(const char* url,const char* outpath,
					   TaskRunner* =nullptr);

    mGlobal(Network) uiRetVal downloadFiles(BufferStringSet& urls,
					    const char* outpath,
					    TaskRunner* =nullptr,
					    bool canfail =false);

    mGlobal(Network) uiRetVal downloadFiles(BufferStringSet& urls,
					    BufferStringSet& outpaths,
					    TaskRunner* =nullptr,
					    bool canfail =false);

    mGlobal(Network) uiRetVal downloadToBuffer(const char* url,DataBuffer&,
					       TaskRunner* =nullptr);

    mGlobal(Network) bool   uploadFile(const char* url,const char* localfname,
				       const char* remotefname,
				       const char* ftype,const IOPar& postvars,
				       uiString& errmsg,TaskRunner* =nullptr,
				       uiString* returnedmessage =nullptr);

    mGlobal(Network) bool   uploadQuery(const char* url,const IOPar& querypars,
					uiString& errmsg,TaskRunner* =nullptr,
					uiString* returnedmessage =nullptr);

    mGlobal(Network) bool   getRemoteFileSize(const char* url,od_int64& size,
					      uiString& errmsg);

    mGlobal(Network) bool   ping(const char* url, uiString& msg );

    mGlobal(Network) void   setHttpProxy(const char* hostname,int port,
					 bool auth=false,
					 const char* username=nullptr,
					 const char* password=nullptr);

    mGlobal(Network) void   setHttpProxyFromSettings();
    mGlobal(Network) void   setHttpProxyFromIOPar(const IOPar&);
    mGlobal(Network) bool   getProxySettingsFromUser();

    inline const char*	    sKeyUseProxy()  { return "Use Proxy"; }
    inline const char*	    sKeyUseAuthentication()
					    { return "Use Authentication";}
    inline const char*	    sKeyProxyHost()
					    { return "Http Proxy Host"; }
    inline const char*	    sKeyProxyPort()
					    { return "Http Proxy Port"; }
    inline const char*	    sKeyProxyUserName()
					    { return "Http Proxy User Name"; }
    inline const char*	    sKeyProxyPassword()
					    { return "Http Proxy Password"; }
    inline const char*	    sKeyCryptProxyPassword()
					{ return "Http Crypt Proxy Password"; }

				// File:: counterparts
    mGlobal(Network) bool	exists(const char*);
    mGlobal(Network) od_int64	getFileSize(const char*);
    mGlobal(Network) bool	getContent(const char*,BufferString&);

} // namespace Network


mExpClass(Network) NetworkUserQuery
{
public:
    virtual			~NetworkUserQuery();
    virtual bool		setFromUser()		= 0;

    static void			setNetworkUserQuery(NetworkUserQuery*);
    static NetworkUserQuery*	getNetworkUserQuery();

protected:
				NetworkUserQuery();

    static NetworkUserQuery*	inst_;
};
