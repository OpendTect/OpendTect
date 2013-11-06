#ifndef odnetworkaccess_h
#define odnetworkaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Oct 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"
#include "bufstringset.h"
#include "executor.h"

class QByteArray;
class QEventLoop;
class QFile;
class QNetworkAccessManager;
class ODNetworkReply;
class DataBuffer;
class od_ostream;




/*!< Functions to download/upload one or more files/data using HTTP protocol*/

mGlobal(Network) bool	downloadFile(const char* url,const char* outpath,
				     BufferString& errmsg,TaskRunner* tr=0);

mGlobal(Network) bool	downloadFiles(BufferStringSet& urls,const char* outpath,
				      BufferString& errmsg,TaskRunner* tr=0);

mGlobal(Network) bool	downloadToBuffer(const char* url,DataBuffer* databuffer,
					 BufferString& errmsg,TaskRunner* tr=0);

mGlobal(Network) bool	uploadFile(const char* url,const char* localfname,
				   const char* remotefname,const char* ftype,
				   const IOPar& postvars, BufferString& errmsg,
				   TaskRunner* tr=0);

mGlobal(Network) bool	uploadQuery(const char* url,const IOPar& querypars,
				    BufferString& errmsg,TaskRunner* tr=0);

mGlobal(Network) bool	getRemoteFileSize(const char* url,od_int64& size, 
					  BufferString& errmsg);

mGlobal(Network) bool	ping(const char* url, BufferString& msg );

mGlobal(Network) void	setHttpProxy(const char* hostname,int port,
				     bool auth=false,const char* username=0,
				     const char* password=0);


//!>Provides file download facility
mExpClass(Network) FileDownloader : public SequentialTask
{
public:
			FileDownloader(const char* url);
			FileDownloader(const char* url,DataBuffer* db);
			FileDownloader(const BufferStringSet& urls,
				       const char* outpath);
			~FileDownloader();

    od_int64		getDownloadSize();

    const char*		message() const;
    int			nextStep();
    od_int64		nrDone() const;
    const char*		nrDoneText() const;
    od_int64		totalNr() const;

protected:

    void		setSaveAsPaths(const BufferStringSet&,const char*);
    int			errorOccured();

    bool		writeData();
    bool		writeDataToFile(const char* buffer, int size);
    bool		writeDataToBuffer(const char* buffer, int size);

    bool		initneeded_;
    BufferStringSet	urls_;
    BufferStringSet	saveaspaths_;
    int			nrfilesdownloaded_;
    DataBuffer*		databuffer_;
    od_ostream*		osd_;

    QEventLoop*		qeventloop_;
    ODNetworkReply*	odnr_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    BufferString	msg_;
};


//!>Provides file or data upload facility
mExpClass(Network) DataUploader : public SequentialTask
{
public:
			DataUploader(const char* url,const DataBuffer& data,
				     BufferString& header);
			~DataUploader();

    const char*		message() const;
    int			nextStep();
    od_int64		nrDone() const;
    const char*		nrDoneText() const;
    od_int64		totalNr() const;

protected:

    int			errorOccured();

    bool		init_;
    BufferString	url_;
    BufferString	header_;

    QByteArray*		data_;
    QEventLoop*		qeventloop_;
    ODNetworkReply*	odnr_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    BufferString	msg_;
};


mGlobal(Network) QNetworkAccessManager&  ODNA();

#endif

