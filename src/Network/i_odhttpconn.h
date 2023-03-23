#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhttp.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEvent>
#include <ptrman.h>
#include <thread.h>

QT_BEGIN_NAMESPACE


class QNetworkReplyConn : public QObject
{
public:
    static void setReadEventType(int);

private:
Q_OBJECT
friend class Network::HttpRequestProcess;
static int readeventtype;

protected:

QNetworkReplyConn( QNetworkReply* sndr, Network::HttpRequestProcess* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sender_, &QNetworkReply::downloadProgress,
	     this, &QNetworkReplyConn::downloadProgress );
    connect( sender_, &QNetworkReply::sslErrors,
	     this, &QNetworkReplyConn::sslErrors );
    connect( sender_, &QNetworkReply::errorOccurred,
	     this, &QNetworkReplyConn::errorOccurred );
    connect( sender_, &QNetworkReply::finished,
	     this, &QNetworkReplyConn::finished );
    connect( sender_, &QNetworkReply::metaDataChanged,
	     this, &QNetworkReplyConn::metaDataChanged );
    connect( sender_, &QNetworkReply::uploadProgress,
	     this, &QNetworkReplyConn::uploadProgress );

    // From QIODevice
    connect( sender_, &QIODevice::aboutToClose,
	     this, &QNetworkReplyConn::aboutToClose );
    connect( sender_, &QIODevice::bytesWritten,
	     this, &QNetworkReplyConn::bytesWritten );
    connect( sender_, &QIODevice::readyRead,
	     this, &QNetworkReplyConn::readyRead );
    connect( sender_, &QIODevice::readChannelFinished,
	    this, &QNetworkReplyConn::readChannelFinished );
}


~QNetworkReplyConn()
{}


private slots:

void downloadProgress( qint64 bytes, qint64 totalbytes )
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportDownloadProgress( bytes, totalbytes );
}


void errorOccurred( QNetworkReply::NetworkError )
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportError();
}

void sslErrors( const QList<QSslError>& errors )
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportSSLErrors( errors );
}


void finished()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportFinished();
}

void metaDataChanged()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportMetaDataChanged();
}

void uploadProgress( qint64 bytes, qint64 totalbytes )
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportUploadProgress( bytes, totalbytes );
}

void aboutToClose()
{}

void bytesWritten( qint64 )
{}

void readChannelFinished()
{}

void readyRead()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver )
	receiver->reportReadyRead();
}


private:

    QNetworkReply*				sender_;
    WeakPtr<Network::HttpRequestProcess>	receiver_;

};

QT_END_NAMESPACE
