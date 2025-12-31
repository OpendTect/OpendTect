#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkmod.h"

#include "bufstringset.h"
#include "executor.h"
#include "odhttp.h"
#include "ptrman.h"
#include "uistring.h"

class QByteArray;
class QEventLoop;
class QFile;
class QNetworkAccessManager;
class ODNetworkProcess;
class DataBuffer;
class od_ostream;

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


//!>Provides file download facility
mExpClass(Network) FileDownloader : public SequentialTask
{ mODTextTranslationClass(FileDownloader);
public:
			FileDownloader(const char* url);
			FileDownloader(const char* url,DataBuffer&);
			FileDownloader(const BufferStringSet& urls,
				       const BufferStringSet& outputpaths);
			~FileDownloader();

    od_int64		getDownloadSize();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    uiRetVal		allMessages() const;
    bool		hasFails() const;
    void		setContinueOnFail( bool yn )  { continueonfail_ = yn; }

private:
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    double		progressFactor() const override;
    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream* =nullptr) override;

    void		setSaveAsPaths(const BufferStringSet&,const char*);
    int			errorOccured();

    bool		writeData();
    bool		writeDataToFile(const char* buffer,int size);
    bool		writeDataToBuffer(const char* buffer,int size);

    bool		initneeded_ = true;
    bool		continueonfail_ = false;
    BufferStringSet	urls_;
    BufferStringSet	saveaspaths_;
    int			currurlidx_	    = 0;
    int			nrfilesdownloaded_  = 0;
    DataBuffer*		databuffer_ = nullptr;
    od_ostream*		osd_ = nullptr;

    RefMan<Network::HttpRequestProcess> odnr_;

    od_int64		nrdone_ = 0;
    od_int64		totalnr_ = 0;
    uiString		msg_;
    uiRetVal		uirv_;
};


//!>Provides file or data upload facility
mExpClass(Network) DataUploader : public SequentialTask
{ mODTextTranslationClass(DataUploader);
public:
			DataUploader(const char* url,const DataBuffer& data,
				     BufferString& header);
			~DataUploader();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

private:
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    double		progressFactor() const override;

    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    int			errorOccured();
    bool		doFinish(bool,od_ostream* =nullptr) override;

    BufferString	url_;
    BufferString	header_;

    const DataBuffer&			data_;
    RefMan<Network::HttpRequestProcess> odnr_;

    od_int64		nrdone_ = 0;
    od_int64		totalnr_ = 1;
    uiString		msg_;
};


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
