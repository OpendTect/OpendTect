#ifndef odnetworkreply_h
#define odnetworkreply_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:		Salil Agarwal
Date:		Oct 2012
RCS:		$Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"

class QEventLoop;
class QNetworkReply;
class QNetworkReplyConn;


mExpClass(Network) ODNetworkReply : public CallBacker
{
public:

				ODNetworkReply(QNetworkReply*,QEventLoop*);
				//!<QNetworkReply becomes mine.
				~ODNetworkReply();

    QNetworkReply*		qNetworkReply() { return qnetworkreply_; }

    void			setBytesUploaded(const od_int64 bytes)
						{ bytesuploaded_ = bytes; }
    void			setTotalBytesToUpload(const od_int64 bytes)
						{ totalbytestoupload_ = bytes; }

    od_int64			getBytesUploaded(){return bytesuploaded_;}
    od_int64			getTotalBytesToUpload()
						{return totalbytestoupload_;}


    Notifier<ODNetworkReply>	downloadProgress;
    Notifier<ODNetworkReply>	finished;
    Notifier<ODNetworkReply>	metaDataChanged;
    Notifier<ODNetworkReply>	error;
    Notifier<ODNetworkReply>	uploadProgress;
    Notifier<ODNetworkReply>	aboutToClose;
    Notifier<ODNetworkReply>	bytesWritten;
    Notifier<ODNetworkReply>	readyRead;

protected:

    friend class QNetworkReplyConn;

    bool			errorOccurred(CallBacker*);
    bool			finish(CallBacker*);
    bool			dataAvailable(CallBacker*);
    bool			uploadStatus(CallBacker*);

    od_int64			bytesuploaded_;
    od_int64			totalbytestoupload_;

    QEventLoop*			qeventloop_;
    QNetworkReplyConn*		qnetworkreplyconn_;
    QNetworkReply*		qnetworkreply_;

};

#endif
