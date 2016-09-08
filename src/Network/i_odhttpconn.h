#ifndef qnetworkaccessconn_h
#define qnetworkaccessconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
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
    : sender_(sndr), receiver_(rec)
{
    connect( sender_, SIGNAL(downloadProgress(qint64,qint64)),
	     this, SLOT(downloadProgress(qint64,qint64)) );
    connect( sender_, SIGNAL(sslErrors(const QList<QSslError> &)),
	     this, SLOT(sslErrors(const QList<QSslError> &)) );
    connect( sender_, SIGNAL(error(QNetworkReply::NetworkError)),
	     this, SLOT(error(QNetworkReply::NetworkError)) );
    connect( sender_, SIGNAL(finished()),
	     this, SLOT(finished()) );
    connect( sender_, SIGNAL(metaDataChanged()),
	     this, SLOT(metaDataChanged()) );
    connect( sender_, SIGNAL(uploadProgress(qint64,qint64)),
	     this, SLOT(uploadProgress(qint64,qint64)) );

    // From QIODevice
    connect( sender_, SIGNAL(aboutToClose()),
	     this, SLOT(aboutToClose()) );
    connect( sender_, SIGNAL(bytesWritten(qint64)),
	     this, SLOT(bytesWritten(qint64)) );
    connect( sender_, SIGNAL(readyRead()),
	     this, SLOT(readyRead()) );
    connect( sender_, SIGNAL(readChannelFinished()),
	    this, SLOT(readChannelFinished()) );
}


private slots:

void downloadProgress(qint64 bytes,qint64 totalbytes)
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportDownloadProgress( bytes, totalbytes );
}


void error(QNetworkReply::NetworkError)
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportError();
}

void sslErrors(const QList<QSslError>& errors)
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportSSLErrors( errors );
}


void finished()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportFinished();
}

void metaDataChanged()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportMetaDataChanged();
}

void uploadProgress(qint64 bytes,qint64 totalbytes)
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportUploadProgress( bytes, totalbytes );
}

void aboutToClose()
{}

void bytesWritten(qint64)
{}

void readChannelFinished()
{}

void readyRead()
{
    RefMan<Network::HttpRequestProcess> receiver = receiver_;
    if ( receiver ) receiver->reportReadyRead();
}


private:

    QNetworkReply*				sender_;
    WeakPtr<Network::HttpRequestProcess>	receiver_;

};

QT_END_NAMESPACE

#endif
